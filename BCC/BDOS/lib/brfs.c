// Bart's RAM File System (BRFS)

/*
Implementing in BDOS on PCB v3:
- BDOS only uses ~340 pages of 256 bytes -> 90000 bytes
- SPI flash has 65536 pages of 256 bytes -> 16777216 bytes
- With current verilog implementation, only 32MiB of RAM is addressable, so BRFS should be used somewhere in the first 32MiB
- With current BDOS memory map, BRFS should be placed in the first 8MiB available as BDOS Program Code
- Lets use the last 4MiB of this space for BRFS (0x100000 - 0x200000)
*/

#define BRFS_SUPPORTED_VERSION 1

#define BRFS_RAM_STORAGE_ADDR 0x100000 // From 4th MiB

// Addresses in SPI Flash
// Note that each section should be in a different 4KiB sector in SPI Flash
#define BRFS_SPIFLASH_SUPERBLOCK_ADDR 0xDF000 // One sector before FAT
#define BRFS_SPIFLASH_FAT_ADDR 0xE0000 // Can be 32768 words (128KiB) for 32MiB of 256word blocks
#define BRFS_SPIFLASH_BLOCK_ADDR 0x100000 // From first MiB

//#define MAX_PATH_LENGTH 127 // Set by BDOS
#define MAX_OPEN_FILES 16 // Can be set higher, but 4 is good for testing

// Length of structs, should not be changed
#define SUPERBLOCK_SIZE 16
#define DIR_ENTRY_SIZE 8

#define BRFS_MAX_BLOCKS 65536 // 64KiB
word brfs_changed_blocks[BRFS_MAX_BLOCKS >> 5]; // Bitmap of changed blocks, each block has 1 bit

// 16 words long
struct brfs_superblock
{
  word total_blocks;
  word words_per_block;
  word label[10];       // 1 char per word
  word brfs_version;
  word reserved[3];
};

// 8 words long
struct brfs_dir_entry
{
  word filename[4];       // 4 chars per word
  word modify_date;       // TBD when RTC added to FPGC
  word flags;             // 32 flags, from right to left: directory, hidden 
  word fat_idx;           // idx of first FAT block
  word filesize;          // file size in words, not bytes
};

word *brfs_ram_storage = (word*) BRFS_RAM_STORAGE_ADDR; // RAM storage of file system

// Variables for open files
word brfs_cursors[MAX_OPEN_FILES]; // Cursor position offset from start of file
word brfs_file_pointers[MAX_OPEN_FILES]; // FAT idx of open file
struct brfs_dir_entry* brfs_dir_entry_pointers[MAX_OPEN_FILES]; // Pointer to dir entry of open file

/**
 * Create a hexdump like dump of a section of memory
 * addr: address of the section
 * len: length of the section in words
 * linesize: number of words per line to print
*/
void brfs_dump_section(word* addr, word len, word linesize)
{
  char buf[16];
  word i;
  for (i = 0; i < len; i++)
  {
    itoah(addr[i], buf);
    if (strlen(buf+2) == 1)
      uprintc('0');
    uprint(buf+2);

    uprintc(' ');

    // newline every linesize words
    // also print last linesize words as chars if alphanum
    if (i != 0 && MATH_modU(i+1, linesize) == 0)
    {
      uprint("  ");
      word j;
      for (j = i - (linesize-1); j < i+1; j++)
      {
        if (isalnum(addr[j]) || addr[j] == ' ')
          uprintc(addr[j]);
        else
          uprintc('.');
      }
      uprintc('\n');
    }
  }
}


/**
 * Create a raw filesystem dump over UART
 * fatsize: size of the FAT table in words
 * datasize: size of the data section in words
*/
void brfs_dump(word fatsize, word datasize)
{
  // Superblock dump
  uprintln("Superblock:");
  brfs_dump_section(brfs_ram_storage, SUPERBLOCK_SIZE, 16);

  // FAT dump
  uprintln("\nFAT:");
  brfs_dump_section(brfs_ram_storage+SUPERBLOCK_SIZE, fatsize, 16);

  // Datablock dump
  uprintln("\nData:");
  brfs_dump_section(brfs_ram_storage+SUPERBLOCK_SIZE+fatsize, datasize, 32);

  uprintln("\nOpen files:");
  word i;
  for (i = 0; i < MAX_OPEN_FILES; i++)
  {
    uprint("FP");
    uprintDec(i+1);
    uprint(":");
    uprint(" FAT idx: ");
    uprintDec(brfs_file_pointers[i]);
    uprint(" Cursor: ");
    uprintDec(brfs_cursors[i]);
    uprint(" Size: ");
    uprintDec(brfs_dir_entry_pointers[i] ? brfs_dir_entry_pointers[i]->filesize : 0);
    uprintc('\n');
  }
}


/**
 * Return the FAT index of a directory, or -1 if not found
 * dir_path: full path of the directory
*/
word brfs_get_fat_idx_of_dir(char* dir_path)
{
  // Check length of path
  if (strlen(dir_path) > MAX_PATH_LENGTH)
  {
    uprintln("Path too long!");
    return -1;
  }

  // Start with root directory
  word current_dir_fat_idx = 0;

  // Check if root directory is requested
  if (strcmp(dir_path, "/") == 0)
  {
    return current_dir_fat_idx;
  }

  // Copy dir_path, size + 1 for null terminator
  // Since strtok modifies the string
  char dir_path_copy[MAX_PATH_LENGTH+1];
  strcpy(dir_path_copy, dir_path);

  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;
  word* brfs_data_block_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks;
  word dir_entries_max = superblock->words_per_block / sizeof(struct brfs_dir_entry);

  // Split path by '/' and traverse directories
  char* token = strtok(dir_path_copy, "/");
  while (token != (word*)-1)
  {
    // Find token in current directory
    word* dir_addr = brfs_data_block_addr + (current_dir_fat_idx * superblock->words_per_block);
    word found_dir = 0; // Keep track if token is found in current directory
    word i;
    for (i = 0; i < dir_entries_max; i++)
    {
      struct brfs_dir_entry* dir_entry = (struct brfs_dir_entry*) (dir_addr + (i * sizeof(struct brfs_dir_entry)));
      if (dir_entry->filename[0] != 0)
      {
        char decompressed_filename[17];
        strdecompress(decompressed_filename, (char*)&(dir_entry->filename));
        // Also check for directory flag
        if (strcmp(decompressed_filename, token) == 0 && dir_entry->flags == 1)
        {
          // Found token in current directory
          // Set current directory to token's FAT index
          current_dir_fat_idx = dir_entry->fat_idx;
          found_dir = 1;
          break;
        }
      }
    }

    // If token not found in current directory, return -1
    if (!found_dir)
    {
      uprint("Directory ");
      uprint(dir_path);
      uprintln(" not found!");
      return -1;
    }

    token = strtok((word*)-1, "/");
  }
  return current_dir_fat_idx;
}

/**
 * Given the address of the FAT table and the number of blocks, find the next free block
 * Returns -1 if no free block is found
 * fat_addr: address of the FAT table
 * blocks: number of blocks in the FAT table
*/
word brfs_find_next_free_block(word* fat_addr, word blocks)
{
  word i = 0;
  word* fat_ptr = fat_addr;

  while (i < blocks)
  {
    if (*fat_ptr == 0)
    {
      return i;
    }

    fat_ptr++;
    i++;
  }

  return -1;
}

/**
 * Given the address of a directory data block and the maximum number of entries, find the next free directory entry
 * Returns -1 if no free entry is found
 * dir_addr: address of the directory data block (not the FAT idx)
 * dir_entries_max: maximum number of entries in the directory
*/
word brfs_find_next_free_dir_entry(word* dir_addr, word dir_entries_max)
{
  word i = 0;
  word* dir_ptr = dir_addr;

  while (i < dir_entries_max)
  {
    if (*dir_ptr == 0)
    {
      return i;
    }

    dir_ptr += sizeof(struct brfs_dir_entry);
    i++;
  }

  return -1;
}

/**
 * Create a single directory entry
 * dir_entry: pointer to the directory entry to be created
 * filename: name of the file, max 16 chars and uncompressed
 * fat_idx: index of the first FAT block of the file/directory
 * filesize: size of the file in words
 * flags: flags of the file/directory
*/
void brfs_create_single_dir_entry(struct brfs_dir_entry* dir_entry, char* filename, word fat_idx, word filesize, word flags)
{
  // Initialize to 0
  memset((char*)dir_entry, 0, sizeof(*dir_entry));

  // Set filename
  char compressed_filename[4] = {0,0,0,0};
  strcompress(compressed_filename, filename);
  memcpy((char*)&(dir_entry->filename), compressed_filename, sizeof(compressed_filename));

  // Set other fields
  dir_entry->fat_idx = fat_idx;
  dir_entry->flags = flags;
  dir_entry->filesize = filesize;
}

/**
 * Initialize a directory with . and .. entries
 * dir_addr: address of the directory data block
 * dir_entries_max: maximum number of entries in the directory
 * dir_fat_idx: index of the FAT block of the directory
 * parent_fat_idx: index of the FAT block of the parent directory
*/
void brfs_init_directory(word* dir_addr, word dir_entries_max, word dir_fat_idx, word parent_fat_idx)
{
  // Get block size from superblock
  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;
  word block_size = superblock->words_per_block;

  // Set data block of dir_fat_idx to 0
  memset(dir_addr, 0, block_size);

  // Create . entry
  struct brfs_dir_entry dir_entry;
  brfs_create_single_dir_entry(&dir_entry, ".", dir_fat_idx, dir_entries_max*sizeof(struct brfs_dir_entry), 1);
  // Copy to first data entry
  memcpy(dir_addr, (char*)&dir_entry, sizeof(dir_entry));

  // Create .. entry
  brfs_create_single_dir_entry(&dir_entry, "..", parent_fat_idx, dir_entries_max*sizeof(struct brfs_dir_entry), 1);
  // Copy to second data entry
  memcpy(dir_addr+sizeof(dir_entry), (char*)&dir_entry, sizeof(dir_entry));

  // Set FAT table
  brfs_ram_storage[SUPERBLOCK_SIZE + dir_fat_idx] = -1;

  // Set changed block
  brfs_changed_blocks[dir_fat_idx >> 5] |= (1 << (dir_fat_idx & 31));
}

/**
 * Format the ram storage as a BRFS filesystem
 * Also writes the superblock to SPI Flash
 * blocks: number of blocks in the filesystem
 * words_per_block: number of bytes per block
 * label: label of the filesystem
 * full_format: if 1, initialize data section to 0
*/
void brfs_format(word blocks, word words_per_block, char* label, word full_format)
{
  // Create a superblock
  struct brfs_superblock superblock;

  // Initialize to 0
  memset((char*)&superblock, 0, sizeof(superblock));

  // Set values of superblock
  superblock.total_blocks = blocks;
  superblock.words_per_block = words_per_block;
  strcpy((char*)&superblock.label, label);
  superblock.brfs_version = BRFS_SUPPORTED_VERSION;

  // Copy superblock to head of ram addr
  memcpy(brfs_ram_storage, (char*)&superblock, sizeof(superblock));


  // Create FAT
  memset(brfs_ram_storage + SUPERBLOCK_SIZE, 0, blocks);

  // Create Data section
  if (full_format)
  {
    memset(brfs_ram_storage + SUPERBLOCK_SIZE + blocks, 0, blocks * words_per_block);
  }
  
  // Initialize root dir
  word dir_entries_max = words_per_block / sizeof(struct brfs_dir_entry);
  brfs_init_directory(brfs_ram_storage + SUPERBLOCK_SIZE + blocks, dir_entries_max, 0, 0);

  // Clear open files and cursors
  memset(brfs_file_pointers, 0, sizeof(brfs_file_pointers));
  memset(brfs_cursors, 0, sizeof(brfs_cursors));
  // Set all dir entry pointers to 0
  word i;
  for (i = 0; i < MAX_OPEN_FILES; i++)
  {
    brfs_dir_entry_pointers[i] = 0;
  }

  // For all blocks that have just been formatted, set changed block
  word j;
  for (j = 0; j < blocks; j++)
  {
    brfs_changed_blocks[j >> 5] |= (1 << (j & 31));
  }

  // Write superblock to SPI Flash
  spiflash_sector_erase(BRFS_SPIFLASH_SUPERBLOCK_ADDR);
  spiflash_write_page_in_words((char*)&superblock, BRFS_SPIFLASH_SUPERBLOCK_ADDR, sizeof(superblock));
}

/**
 * Create a new directory in the directory of parent_dir_path
 * Returns 1 on success, 0 on error
 * parent_dir_path: full path of the parent directory
 * dirname: name of the new directory
*/
word brfs_create_directory(char* parent_dir_path, char* dirname)
{
  // Check length of dirname
  if (strlen(dirname) >= 16)
  {
    uprintln("Directory name too long!");
    return 0;
  }
  
  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;

  word* brfs_data_block_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks;

  // Find first free FAT block
  word next_free_block = brfs_find_next_free_block(brfs_ram_storage + SUPERBLOCK_SIZE, superblock->total_blocks);
  if (next_free_block == -1)
  {
    uprintln("No free blocks left!");
    return 0;
  }

  // Find data block address of parent directory path
  word parent_dir_fat_idx = brfs_get_fat_idx_of_dir(parent_dir_path);
  if (parent_dir_fat_idx == -1)
  {
    uprint("Parent directory ");
    uprint(parent_dir_path);
    uprintln(" not found!");
    return 0;
  }

  // Check if file or folder already exists
  word* parent_dir_addr = brfs_data_block_addr + (parent_dir_fat_idx * superblock->words_per_block);
  word dir_entries_max = superblock->words_per_block / sizeof(struct brfs_dir_entry);
  word i;
  for (i = 0; i < dir_entries_max; i++)
  {
    struct brfs_dir_entry* dir_entry = (struct brfs_dir_entry*) (parent_dir_addr + (i * sizeof(struct brfs_dir_entry)));
    if (dir_entry->filename[0] != 0)
    {
      char decompressed_filename[17];
      strdecompress(decompressed_filename, (char*)&(dir_entry->filename));
      if (strcmp(decompressed_filename, dirname) == 0)
      {
        uprint(dirname);
        uprintln(" already exists!");
        return 0;
      }
    }
  }

  // Find first free dir entry
  word next_free_dir_entry = brfs_find_next_free_dir_entry(
    brfs_data_block_addr + (parent_dir_fat_idx * superblock->words_per_block), 
    superblock->words_per_block / sizeof(struct brfs_dir_entry)
  );
  if (next_free_dir_entry == -1)
  {
    uprintln("No free dir entries left!");
    return 0;
  }

  // Create dir entry
  struct brfs_dir_entry new_entry;
  brfs_create_single_dir_entry(&new_entry, dirname, next_free_block, dir_entries_max*sizeof(struct brfs_dir_entry), 1);

  // Copy dir entry to first free dir entry
  memcpy(
    brfs_data_block_addr + (parent_dir_fat_idx * superblock->words_per_block) + (next_free_dir_entry * sizeof(struct brfs_dir_entry)),
    (char*)&new_entry,
    sizeof(new_entry)
  );

  // Initialize directory
  brfs_init_directory(
    brfs_data_block_addr + (next_free_block * superblock->words_per_block),
    dir_entries_max,
    next_free_block,
    parent_dir_fat_idx
  );

  // Update changed block
  brfs_changed_blocks[next_free_block >> 5] |= (1 << (next_free_block & 31));
  brfs_changed_blocks[parent_dir_fat_idx >> 5] |= (1 << (parent_dir_fat_idx & 31));

  return 1;
}

/**
 * Create a new file in the directory of parent_dir_path
 * Returns 1 on success, 0 on error
 * parent_dir_path: full path of the parent directory
 * filename: name of the new file
*/
word brfs_create_file(char* parent_dir_path, char* filename)
{
  // Check length of filename
  if (strlen(filename) >= 16)
  {
    uprintln("Filename too long!");
    return 0;
  }

  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;

  word* brfs_data_block_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks;

  // Find first free FAT block
  word next_free_block = brfs_find_next_free_block(brfs_ram_storage + SUPERBLOCK_SIZE, superblock->total_blocks);
  if (next_free_block == -1)
  {
    uprintln("No free blocks left!");
    return 0;
  }

  // Find data block address of parent directory path
  word parent_dir_fat_idx = brfs_get_fat_idx_of_dir(parent_dir_path);
  if (parent_dir_fat_idx == -1)
  {
    uprint("Parent directory ");
    uprint(parent_dir_path);
    uprintln(" not found!");
    return 0;
  }

  // Check if file or folder already exists
  word* parent_dir_addr = brfs_data_block_addr + (parent_dir_fat_idx * superblock->words_per_block);
  word dir_entries_max = superblock->words_per_block / sizeof(struct brfs_dir_entry);
  word i;
  for (i = 0; i < dir_entries_max; i++)
  {
    struct brfs_dir_entry* dir_entry = (struct brfs_dir_entry*) (parent_dir_addr + (i * sizeof(struct brfs_dir_entry)));
    if (dir_entry->filename[0] != 0)
    {
      char decompressed_filename[17];
      strdecompress(decompressed_filename, (char*)&(dir_entry->filename));
      if (strcmp(decompressed_filename, filename) == 0)
      {
        uprint(filename);
        uprintln(" already exists!");
        return 0;
      }
    }
  }

  // Find first free dir entry
  word next_free_dir_entry = brfs_find_next_free_dir_entry(
    brfs_data_block_addr + (parent_dir_fat_idx * superblock->words_per_block), 
    superblock->words_per_block / sizeof(struct brfs_dir_entry)
  );
  if (next_free_dir_entry == -1)
  {
    uprintln("No free dir entries left!");
    return 0;
  }

  // Create file entry
  struct brfs_dir_entry new_entry;
  brfs_create_single_dir_entry(&new_entry, filename, next_free_block, 0, 0);

  // Copy dir entry to first free dir entry
  memcpy(
    brfs_data_block_addr + (parent_dir_fat_idx * superblock->words_per_block) + (next_free_dir_entry * sizeof(struct brfs_dir_entry)),
    (char*)&new_entry,
    sizeof(new_entry)
  );

  // Initialize file by setting data to 0
  memset(
    brfs_data_block_addr + (next_free_block * superblock->words_per_block),
    0,
    superblock->words_per_block
  );

  // Update FAT
  brfs_ram_storage[SUPERBLOCK_SIZE + next_free_block] = -1;

  // Update changed block
  brfs_changed_blocks[next_free_block >> 5] |= (1 << (next_free_block & 31));

  return 1;
}

/**
 * Reads all directory entries of a directory into a buffer
 * dir_path: full path of the directory
 * buffer: buffer to store the directory entries
 * Returns the number of entries read, or -1 on error
*/
word brfs_read_directory(char* dir_path, struct brfs_dir_entry* buffer)
{
  // Find data block address of parent directory path
  word dir_fat_idx = brfs_get_fat_idx_of_dir(dir_path);
  if (dir_fat_idx == -1)
  {
    uprint("Parent directory ");
    uprint(dir_path);
    uprintln(" not found!");
    return -1;
  }

  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;
  word* dir_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks + (dir_fat_idx * superblock->words_per_block);
  word dir_entries_max = superblock->words_per_block / sizeof(struct brfs_dir_entry);

  word entries_read = 0;

  word i;
  for (i = 0; i < dir_entries_max; i++)
  {
    struct brfs_dir_entry* dir_entry = (struct brfs_dir_entry*) (dir_addr + (i * sizeof(struct brfs_dir_entry)));
    if (dir_entry->filename[0] != 0)
    {
      memcpy(buffer, dir_entry, sizeof(struct brfs_dir_entry));
      buffer++;
      entries_read++;
    }
  }

  return entries_read;
}

/**
 * Open a file for reading and writing
 * Returns the file pointer (FAT idx of file), or -1 on error
 * file_path: full path of the file
*/
word brfs_open_file(char* file_path)
{

  // Split filename from path using basename and dirname
  char dirname_output[MAX_PATH_LENGTH];
  char* file_path_basename = basename(file_path);
  char* file_path_dirname = dirname(dirname_output, file_path);

  // Find data block address of parent directory path
  word dir_fat_idx = brfs_get_fat_idx_of_dir(file_path_dirname);
  if (dir_fat_idx == -1)
  {
    uprint("Parent directory ");
    uprint(file_path_dirname);
    uprintln(" not found!");
    return -1;
  }

  // Find file in directory
  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;
  word* dir_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks + (dir_fat_idx * superblock->words_per_block);
  word dir_entries_max = superblock->words_per_block / sizeof(struct brfs_dir_entry);

  word i;
  for (i = 0; i < dir_entries_max; i++)
  {
    struct brfs_dir_entry* dir_entry = (struct brfs_dir_entry*) (dir_addr + (i * sizeof(struct brfs_dir_entry)));
    if (dir_entry->filename[0] != 0)
    {
      char decompressed_filename[17];
      strdecompress(decompressed_filename, (char*)&(dir_entry->filename));
      // Also check for directory flag to be 0
      if (strcmp(decompressed_filename, file_path_basename) == 0 && dir_entry->flags == 0)
      {
        // Found file
        // Check if file is already open
        word j;
        for (j = 0; j < MAX_OPEN_FILES; j++)
        {
          if (brfs_file_pointers[j] == dir_entry->fat_idx)
          {
            uprint("File ");
            uprint(file_path_basename);
            uprintln(" already open!");
            return -1;
          }
        }

        // Find first free file pointer
        word next_free_file_pointer = -1;
        for (j = 0; j < MAX_OPEN_FILES; j++)
        {
          if (brfs_file_pointers[j] == 0)
          {
            next_free_file_pointer = j;
            break;
          }
        }

        if (next_free_file_pointer == -1)
        {
          uprintln("All files already opened!");
          return -1;
        }

        // Open file
        brfs_file_pointers[next_free_file_pointer] = dir_entry->fat_idx;
        brfs_cursors[next_free_file_pointer] = 0;
        brfs_dir_entry_pointers[next_free_file_pointer] = dir_entry;
        return brfs_file_pointers[next_free_file_pointer];
      }
    }
  }
  uprint("File ");
  uprint(file_path_basename);
  uprintln(" not found!");
  return -1;
}

/**
 * Close an opened file
 * Returns 1 on success, 0 on error
 * file_pointer: file pointer returned by brfs_open_file
*/
word brfs_close_file(word file_pointer)
{
  // Find file pointer
  word i;
  for (i = 0; i < MAX_OPEN_FILES; i++)
  {
    if (brfs_file_pointers[i] == file_pointer)
    {
      // Close file
      brfs_file_pointers[i] = 0;
      brfs_cursors[i] = 0;
      brfs_dir_entry_pointers[i] = 0;
      return 1;
    }
  }
  uprintln("File not found!");
  return 0;
}

/**
 * Delete a file by removing all FAT blocks and the directory entry
 * Deletes a directory only if it is empty
 * Returns 1 on success, 0 on error
 * file_path: full path of the file
*/
word brfs_delete(char* file_path)
{
  // Split filename from path using basename and dirname
  char dirname_output[MAX_PATH_LENGTH];
  char* file_path_basename = basename(file_path);
  char* file_path_dirname = dirname(dirname_output, file_path);

  // Find data block address of parent directory path
  word dir_fat_idx = brfs_get_fat_idx_of_dir(file_path_dirname);
  if (dir_fat_idx == -1)
  {
    uprint("Parent directory ");
    uprint(file_path_dirname);
    uprintln(" not found!");
    return 0;
  }

  // Find file in directory
  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;
  word* dir_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks + (dir_fat_idx * superblock->words_per_block);
  word dir_entries_max = superblock->words_per_block / sizeof(struct brfs_dir_entry);

  word i;
  for (i = 0; i < dir_entries_max; i++)
  {
    struct brfs_dir_entry* dir_entry = (struct brfs_dir_entry*) (dir_addr + (i * sizeof(struct brfs_dir_entry)));
    if (dir_entry->filename[0] != 0)
    {
      char decompressed_filename[17];
      strdecompress(decompressed_filename, (char*)&(dir_entry->filename));
      if (strcmp(decompressed_filename, file_path_basename) == 0)
      {
        if ((dir_entry->flags & 0x01) == 1)
        {
          // Check if directory is empty
          struct brfs_dir_entry buffer[128]; // 128 to be safe
          word num_entries = brfs_read_directory(file_path, buffer);
          if (num_entries > 2)
          {
            uprint("Directory ");
            uprint(file_path_basename);
            uprintln(" is not empty!");
            return 0;
          }
        }
        // Check if file is already open
        word j;
        for (j = 0; j < MAX_OPEN_FILES; j++)
        {
          if (brfs_file_pointers[j] == dir_entry->fat_idx)
          {
            uprint("File ");
            uprint(file_path_basename);
            uprintln(" is open!");
            return 0;
          }
        }

        // Delete fat blocks
        word current_fat_idx = dir_entry->fat_idx;
        word next_fat_idx;
        while (current_fat_idx != -1)
        {
          next_fat_idx = brfs_ram_storage[SUPERBLOCK_SIZE + current_fat_idx];
          brfs_ram_storage[SUPERBLOCK_SIZE + current_fat_idx] = 0;
          brfs_changed_blocks[current_fat_idx >> 5] |= (1 << (current_fat_idx & 31));
          current_fat_idx = next_fat_idx;
        }

        // Delete entry
        memset((char*)dir_entry, 0, sizeof(struct brfs_dir_entry));

        // Update changed block
        brfs_changed_blocks[dir_fat_idx >> 5] |= (1 << (dir_fat_idx & 31));
        return 1;
      }
    }
  }
  uprint("File ");
  uprint(file_path_basename);
  uprintln(" not found!");
  return 0;
}

/**
 * Set the cursor of an opened file
 * Returns 1 on success, 0 on error
 * file_pointer: file pointer returned by brfs_open_file
 * cursor: new cursor position in words
*/
word brfs_set_cursor(word file_pointer, word cursor)
{
  if (file_pointer == 0)
  {
    uprintln("File not open!");
    return 0;
  }

  // Find file pointer
  word i;
  for (i = 0; i < MAX_OPEN_FILES; i++)
  {
    if (brfs_file_pointers[i] == file_pointer)
    {
      // Set cursor
      if (cursor < 0 || cursor > brfs_dir_entry_pointers[i]->filesize)
      {
        cursor = brfs_dir_entry_pointers[i]->filesize;
      }

      brfs_cursors[i] = cursor;
      return 1;
    }
  }
  uprintln("File not found!");
  return 0;
}

/**
 * Get the cursor of an opened file
 * Returns the cursor position in words, or -1 on error
 * file_pointer: file pointer returned by brfs_open_file
*/
word brfs_get_cursor(word file_pointer)
{
  if (file_pointer == 0)
  {
    uprintln("File not open!");
    return -1;
  }

  // Find file pointer
  word i;
  for (i = 0; i < MAX_OPEN_FILES; i++)
  {
    if (brfs_file_pointers[i] == file_pointer)
    {
      // Get cursor
      return brfs_cursors[i];
    }
  }
  uprintln("File not found!");
  return -1;
}

/**
 * Get the FAT index of a file at the cursor
 * Returns the FAT index, or 0 on error
 * file_pointer: file pointer returned by brfs_open_file
 * cursor: cursor position of opened file
*/
word brfs_get_fat_idx_at_cursor(word file_pointer, word cursor)
{
  if (file_pointer == 0)
  {
    uprintln("File not open!");
    return 0;
  }
  // Get FAT index of file at cursor
  word current_fat_idx = file_pointer;
  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;

  // Loop through FAT until cursor is reached
  while (cursor > superblock->words_per_block)
  {
    current_fat_idx = brfs_ram_storage[SUPERBLOCK_SIZE + current_fat_idx];
    if (current_fat_idx == -1)
    {
      uprintln("Cursor is out of bounds!");
      return 0;
    }
    cursor -= superblock->words_per_block;
  }

  return current_fat_idx;
}

/**
 * Read a file from the cursor position
 * Returns 1 on success, or 0 on error
 * file_pointer: file pointer returned by brfs_open_file
 * buffer: buffer to read the file into
 * length: number of words to read
*/
word brfs_read(word file_pointer, word* buffer, word length)
{
  if (file_pointer == 0)
  {
    uprintln("File not open!");
    return 0;
  }

  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;
  word* data_block_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks;

  // Find file pointer
  word i;
  for (i = 0; i < MAX_OPEN_FILES; i++)
  {
    if (brfs_file_pointers[i] == file_pointer)
    {
      if (length < 0)
      {
        uprintln("Length cannot be negative!");
        return 0;
      }
      // Trunctate length to file size - cursor
      if (length > brfs_dir_entry_pointers[i]->filesize - brfs_cursors[i])
      {
        length = brfs_dir_entry_pointers[i]->filesize - brfs_cursors[i];
      }

      // Get FAT index of file at cursor
      word current_fat_idx = brfs_get_fat_idx_at_cursor(file_pointer, brfs_cursors[i]);
      if (current_fat_idx == 0)
      {
        uprintln("Error getting FAT index at cursor!");
        return 0;
      }

      // Loop:
      // - calculate words until end of block (or up to length)
      // - read words until end of block (or up to length)
      // - decrease length by words read
      // - get next block from FAT
      // - repeat until length is 0
      while (length > 0)
      {
        word words_until_end_of_block = superblock->words_per_block - (MATH_modU(brfs_cursors[i], superblock->words_per_block));
        word words_to_read = words_until_end_of_block > length ? length : words_until_end_of_block;

        // Copy words to buffer
        memcpy(buffer, data_block_addr + (current_fat_idx * superblock->words_per_block) + MATH_modU(brfs_cursors[i], superblock->words_per_block), words_to_read);

        // Update cursor and length
        brfs_cursors[i] += words_to_read;
        length -= words_to_read;
        buffer += words_to_read;

        // Get next block from FAT
        current_fat_idx = brfs_ram_storage[SUPERBLOCK_SIZE + current_fat_idx];
        if (current_fat_idx == -1 && length > 0)
        {
          uprintln("There is no next block in the file!");
          return 0;
        }

        
      }

      return 1;
    }
  }
  uprintln("File not found!");
  return 0;
}

/**
 * Write a file from the cursor position
 * Returns 1 on success, or 0 on error
 * file_pointer: file pointer returned by brfs_open_file
 * buffer: buffer to write to the file
 * length: number of words to write
*/
word brfs_write(word file_pointer, word* buffer, word length)
{
  if (file_pointer == 0)
  {
    uprintln("File not open!");
    return 0;
  }

  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;
  word* data_block_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks;

  // Find file pointer
  word i;
  for (i = 0; i < MAX_OPEN_FILES; i++)
  {
    if (brfs_file_pointers[i] == file_pointer)
    {
      if (length < 0)
      {
        uprintln("Length cannot be negative!");
        return 0;
      }

      // Get FAT index of file at cursor
      word current_fat_idx = brfs_get_fat_idx_at_cursor(file_pointer, brfs_cursors[i]);
      if (current_fat_idx == 0)
      {
        uprintln("Error getting FAT index at cursor!");
        return 0;
      }

      // Loop:
      // - calculate words until end of block (or up to length)
      // - write words until end of block (or up to length)
      // - decrease length by words written
      // - get next block from FAT, or find next free block if end of block
      // - if next block is needed, update FAT
      // - repeat until length is 0
      while (length > 0)
      {
        word cursor_in_block = MATH_modU(brfs_cursors[i], superblock->words_per_block);
        word words_until_end_of_block = superblock->words_per_block - cursor_in_block;
        word words_to_write = words_until_end_of_block > length ? length : words_until_end_of_block;

        // Copy words to buffer
        memcpy(data_block_addr + (current_fat_idx * superblock->words_per_block) + cursor_in_block, buffer, words_to_write);

        // Update changed block
        brfs_changed_blocks[current_fat_idx >> 5] |= (1 << (current_fat_idx & 31));

        // Update cursor and length
        brfs_cursors[i] += words_to_write;
        length -= words_to_write;
        buffer += words_to_write;

        // Get next block from FAT, or find next free block if end of block
        if (words_until_end_of_block == words_to_write && length > 0)
        {
          
          word next_fat_idx = brfs_ram_storage[SUPERBLOCK_SIZE + current_fat_idx];
          // Check if next block is already allocated
          if (next_fat_idx != -1)
          {
            current_fat_idx = next_fat_idx;
          }
          else
          {
            // Find next free block
            word next_free_block = brfs_find_next_free_block(brfs_ram_storage + SUPERBLOCK_SIZE, superblock->total_blocks);
            if (next_free_block == -1)
            {
              uprintln("No free blocks left!");
              return 0;
            }
            // Update FAT
            brfs_ram_storage[SUPERBLOCK_SIZE + current_fat_idx] = next_free_block;
            // Go to next block
            current_fat_idx = next_free_block;
            // Set next block to -1 to indicate end of file
            brfs_ram_storage[SUPERBLOCK_SIZE + current_fat_idx] = -1;
            // Update changed block
            brfs_changed_blocks[current_fat_idx >> 5] |= (1 << (current_fat_idx & 31));
          }
        }
      }

      // Update file size in dir entry if we wrote past the current size
      if (brfs_cursors[i] > brfs_dir_entry_pointers[i]->filesize)
      {
        brfs_dir_entry_pointers[i]->filesize = brfs_cursors[i];
      }

      return 1;
    }
  }
  uprintln("File not found!");
  return 0;
}

/**
 * Stat a file or directory
 * Returns the directory entry, or -1 on error
*/
struct brfs_dir_entry* brfs_stat(char* file_path)
{
  // Remove all trailing slashes
  while (strlen(file_path) > 1 && file_path[strlen(file_path)-1] == '/')
  {
    file_path[strlen(file_path)-1] = 0;
  }

  // Split filename from path using basename and dirname
  char dirname_output[MAX_PATH_LENGTH];
  char* file_path_basename = basename(file_path);
  char* file_path_dirname = dirname(dirname_output, file_path);

  // Find data block address of parent directory path
  word dir_fat_idx = brfs_get_fat_idx_of_dir(file_path_dirname);
  if (dir_fat_idx == -1)
  {
    uprint("Parent directory ");
    uprint(file_path_dirname);
    uprintln(" not found!");
    return (struct brfs_dir_entry*)-1;
  }

  // Find file in directory
  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;
  word* dir_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks + (dir_fat_idx * superblock->words_per_block);
  word dir_entries_max = superblock->words_per_block / sizeof(struct brfs_dir_entry);

  word i;
  for (i = 0; i < dir_entries_max; i++)
  {
    struct brfs_dir_entry* dir_entry = (struct brfs_dir_entry*) (dir_addr + (i * sizeof(struct brfs_dir_entry)));
    if (dir_entry->filename[0] != 0)
    {
      char decompressed_filename[17];
      strdecompress(decompressed_filename, (char*)&(dir_entry->filename));
      // Also check for directory flag to be 0
      if (strcmp(decompressed_filename, file_path_basename) == 0)
      {
        return dir_entry;
      }
    }
  }
  uprint("File or directory ");
  uprint(file_path_basename);
  uprintln(" not found!");
  return (struct brfs_dir_entry*)-1;
}

/**
 * Check if a block has changed by comparing it to the flash, returns 1 if changed and 0 if not
 * Note: this is slow and should eventually be replaced by a list of changed blocks
 * block_idx: index of the block
*/
word brfs_check_block_changed(word block_idx)
{
  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;
  word* data_block_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks;

  word spi_data_buffer[256];

  if (superblock->words_per_block > 256)
  {
    uprintln("Error: words_per_block should be <= 256 for this function!");
    return 0;
  }

  // Read block from flash, and enable bytes to word
  spiflash_read_from_address(spi_data_buffer, BRFS_SPIFLASH_BLOCK_ADDR + block_idx * superblock->words_per_block, superblock->words_per_block, 1);

  // Compare block to flash
  return memcmp(data_block_addr + (block_idx * superblock->words_per_block), spi_data_buffer, superblock->words_per_block);
}

/**
 * Write the FAT table to SPI flash by performing three steps:
 * 1. Check which FAT entries have changed
 * 2. Erase the 4KiB sectors that contain these FAT entries
 * 3. Write each changed FAT entry to flash by using 16 page writes per sector
*/
void brfs_write_fat_to_flash()
{
  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;
  word* data_block_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks;

  // 1 sector = 4KiB = 1024 words = 1024 FAT entries
  // 1 word contains 32 flags for changed blocks/FAT entries
  // 1024/32 = 32 words in the changed_blocks array per sector

  uprintln("---Writing FAT to SPI Flash---");

  // Loop over brfs_changed_blocks in 32 word parts
  // Assumes length of brfs_changed_blocks is a multiple of 32
  word i;
  for (i = 0; i < sizeof(brfs_changed_blocks); i+=32)
  {
    // Check if any value within brfs_changed_blocks[i:i+32] is not 0
    word j;
    word changed = 0;
    for (j = 0; j < 32; j++)
    {
      if (brfs_changed_blocks[i+j] != 0)
      {
        changed = 1;
        break;
      }
    }

    if (changed)
    {
      // Erase sector
      word addr = BRFS_SPIFLASH_FAT_ADDR; // Workaround because of large static number
      addr += (i >> 5) * 4096; // Sector idx * bytes per sector
      spiflash_sector_erase(addr);
      uprint("Erased sector ");
      uprintDec(i >> 5);
      uprint(" at address ");
      uprintHex(addr);
      uprintln("");


      // Write sector by writing 16 pages k of 64 words
      // Does not check for boundaries of actual FAT table size,
      //  so it can write garbage if block size is not a multiple of 1024
      word k;
      for (k = 0; k < 1024; k+=64)
      {
        addr = BRFS_SPIFLASH_FAT_ADDR; // Workaround because of large static number
        addr += (i >> 5) * 4096; // Sector idx * bytes per sector
        addr += k << 2; // 64 words * 4 bytes per word

        word* fat_addr_ram = brfs_ram_storage + SUPERBLOCK_SIZE + (i << 5) + k;
        spiflash_write_page_in_words(fat_addr_ram, addr, 64);

        uprint("Wrote FAT entries ");
        uprintDec((i << 5) + k);
        uprint(":");
        uprintDec((i << 5) + k + 63);
        uprint(" from RAM addr ");
        uprintHex((word)fat_addr_ram);
        uprint(" to SPI Flash addr ");
        uprintHex(addr);
        uprintln("");
      }
    }
  }

  uprintln("---Finished writing FAT to SPI Flash---");
}

/**
 * Write a sector (4KiB) to SPI flash
 * sector_idx: index of the sector
*/
void brfs_write_sector_to_flash(word sector_idx)
{
  word spi_addr = BRFS_SPIFLASH_BLOCK_ADDR; // Workaround because of large static number
  spi_addr += sector_idx * 4096; // Sector idx * bytes per sector
  
  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;
  word* data_block_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks;
  word brfs_sector_addr = data_block_addr + sector_idx * (4096 >> 2); // Divided by 4 because of word size

  // Write sector by writing 16 pages k of 64 words
  // Does not check for boundaries of actual FAT table size,
  //  so it can write garbage if block size is not a multiple of 1024
  word k;
  for (k = 0; k < 1024; k+=64)
  {
    spiflash_write_page_in_words(brfs_sector_addr + k, spi_addr + (k << 2), 64);

    uprint("Wrote sector ");
    uprintDec(sector_idx);
    uprint(":");
    uprintDec(sector_idx + 15);
    uprint(" from RAM addr ");
    uprintHex((word)(brfs_sector_addr + k));
    uprint(" to SPI Flash addr ");
    uprintHex(spi_addr + (k << 2));
    uprintln("");
  }
}

/**
 * Write the data blocks to SPI flash by performing three steps:
 * 1. Check which blocks have changed
 * 2. Erase the 4KiB sectors that contain these blocks
 * 3. Write each erased sector with the new block data by using 16 page writes per sector
*/
void brfs_write_blocks_to_flash()
{
  // Loop over all blocks
  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;
  word* data_block_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks;

  // Check if block size is <= 4KiB
  if (superblock->words_per_block > 1024)
  {
    uprintln("Error: block size should be <= 4KiB");
    return;
  }

  // Check if block size is a multiple of 64
  if (superblock->words_per_block & 63)
  {
    uprintln("Error: block size should be a multiple of 64");
    return;
  }

  uprintln("---Writing blocks to SPI Flash---");

  word blocks_per_sector = MATH_divU(4096, superblock->words_per_block * 4);
  uprint("Blocks per sector: ");
  uprintDec(blocks_per_sector);
  uprintln("");

  // Erase 4KiB sectors that contain changed blocks
  // This code is written such that it only erases each sector once, even if multiple blocks in the sector have changed
  word i;
  word sector_to_erase = -1;
  for (i = 0; i < superblock->total_blocks; i++)
  {
    if (brfs_changed_blocks[i >> 5] & (1 << (i & 31)))
    {
      if (sector_to_erase == -1)
      {
        sector_to_erase = MATH_divU(i, blocks_per_sector);
      }
      else if (sector_to_erase != MATH_divU(i, blocks_per_sector))
      {
        word addr = BRFS_SPIFLASH_BLOCK_ADDR; // Workaround because of large static number
        addr += sector_to_erase * 4096;
        spiflash_sector_erase(addr);
        uprint("Erased sector ");
        uprintDec(sector_to_erase);
        uprint(" at address ");
        uprintHex(addr);
        uprintln("");

        brfs_write_sector_to_flash(sector_to_erase);

        sector_to_erase = MATH_divU(i, blocks_per_sector);
      }
    }
  }
  if (sector_to_erase != -1)
  {
    word addr = BRFS_SPIFLASH_BLOCK_ADDR; // Workaround because of large static number
    addr += sector_to_erase * 4096;
    spiflash_sector_erase(addr);
    uprint("Erased sector ");
    uprintDec(sector_to_erase);
    uprint(" at address ");
    uprintHex(addr);
    uprintln("");

    brfs_write_sector_to_flash(sector_to_erase);
  }

  uprintln("---Finished writing blocks to SPI Flash---");
}

/**
 * Write the FAT and data blocks to SPI flash
 * Superblock should already be written to flash during format
*/
void brfs_write_to_flash()
{
  brfs_write_fat_to_flash();
  brfs_write_blocks_to_flash();
  // Reset changed blocks
  memset(brfs_changed_blocks, 0, sizeof(brfs_changed_blocks));
}

/**
 * Checks if given superblock is valid
 * Returns 1 if valid, 0 if invalid
*/
word brfs_superblock_is_valid(struct brfs_superblock* superblock)
{
  // Check if brfs version is correct
  if (superblock->brfs_version != BRFS_SUPPORTED_VERSION)
  {
    uprint("BRFS version ");
    uprintDec(superblock->brfs_version);
    uprint(" is not supported by this implementation (");
    uprintDec(BRFS_SUPPORTED_VERSION);
    uprintln(")!");
    return 0;
  }
  // Check if total blocks is > 0 and a multiple of 64
  if (superblock->total_blocks == 0 || superblock->total_blocks & 63)
  {
    uprintln("Error: total blocks should be > 0 and a multiple of 64");
    return 0;
  }
  // Check if block size is > 0
  if (superblock->words_per_block == 0)
  {
    uprintln("Error: block size should be > 0");
    return 0;
  }
  // Check if words per block is > 0 and <= 2048
  if (superblock->words_per_block == 0 || superblock->words_per_block > 2048)
  {
    uprintln("Error: words per block should be > 0 and <= 2048");
    return 0;
  }

  return 1;
}

/**
 * Read the superblock, FAT and data blocks from SPI flash
 * Returns 1 on success, or 0 on error
*/
word brfs_read_from_flash()
{
  // Read superblock from flash
  spiflash_read_from_address(brfs_ram_storage, BRFS_SPIFLASH_SUPERBLOCK_ADDR, SUPERBLOCK_SIZE, 1);

  // Perform validity checks on superblock
  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;
  if (!brfs_superblock_is_valid(superblock))
  {
    uprintln("Error: superblock is not valid!");
    return 0;
  }
  
  word* data_block_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks;

  // Read FAT from flash
  spiflash_read_from_address(brfs_ram_storage + SUPERBLOCK_SIZE, BRFS_SPIFLASH_FAT_ADDR, superblock->total_blocks, 1);

  // Read data blocks from flash
  spiflash_read_from_address(data_block_addr, BRFS_SPIFLASH_BLOCK_ADDR, superblock->total_blocks * superblock->words_per_block, 1);

  return 1;
}
