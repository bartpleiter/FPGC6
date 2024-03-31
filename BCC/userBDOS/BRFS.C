// Test implementation of Bart's RAM File System (BRFS)

/*
General Idea:
- HDD for the average home user was <100MB until after 1990
- SPI NOR Flash provides around the same amount of storage, with very little wear
    - Very fast reads in QSPI mode
    - However, writes are extremely slow and need to be performed in pages of 256 bytes (64 words)
- FPGC has 64MiB RAM, which is a lot even for 32 bit addressable words
    - 32MiB is already more than enough for the FPGC in its current form
- Use the other 32MiB as a fully in RAM filesystem
    - That initializes from SPI flash and writes back to Flash at a chosen time
*/

/*
Implementation Idea:
- Use superblock for info/addresses, no hard-coded sizes!
  - Allows for different storage media, easier testing on tiny size, and more future proof
*/

/*
Implementation Details:
--------------------------------------------------
| superblock | FAT | Data+Dir blocks (same size) |
--------------------------------------------------

16 word superblock:
  - (1)  total blocks
  - (1)  bytes per block
  - (10) label [1 char per word]
  - (1)  brfs version
  - (3)  reserved

8 word Dir entries:
  - (4) filename.ext [4 chars per word -> 16 chars total]
  - (1) modify date [to be implemented when RTC]
  - (1) flags [max 32 flags, from right to left: directory, hidden]
  - (1) 1st FAT idx
  - (1) file size [in words, not bytes]
*/

/*
Implementation Notes:
- Current directory is managed by the application/OS, not the FS. Directory (or file) can be checked to exist using stat()
- Updating a file: might be better to delete and create a new one, but this is better be done by the application instead of the FS
- Write should start writing at the cursor and jump to the next block (also create in FAT) if the end is reached
- No delete/backspace, only delete entire file or overwrite data
*/

/*
Required operations:
- [x] Format
- [x] Create directory
- [x] Create file
- [x] Open file (not a dir!) (allow multiple files open at once)
- [x] Close file
- [x] Stat (returns dir entry)
- [x] Set cursor
- [x] Get cursor
- [x] Read file
- [x] Write file
- [x] Delete entire file (deleting part of file is not a thing)
- [x] List directory
*/

#define word char

#include "LIB/MATH.C"
#include "LIB/SYS.C"
#include "LIB/STDLIB.C"

#define BRFS_RAM_STORAGE_ADDR 0x600000

#define MAX_PATH_LENGTH 127
#define MAX_OPEN_FILES 4 // Can be set higher, but 4 is good for testing

// Length of structs, should not be changed
#define SUPERBLOCK_SIZE 16
#define DIR_ENTRY_SIZE 8

// 16 words long
struct brfs_superblock
{
  word total_blocks;
  word bytes_per_block;
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
  if (strcmp(dir_path, "/") == 1)
  {
    return current_dir_fat_idx;
  }

  // Copy dir_path, size + 1 for null terminator
  // Since strtok modifies the string
  char dir_path_copy[MAX_PATH_LENGTH+1];
  strcpy(dir_path_copy, dir_path);

  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;
  word* brfs_data_block_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks;
  word dir_entries_max = superblock->bytes_per_block / sizeof(struct brfs_dir_entry);

  // Split path by '/' and traverse directories
  char* token = strtok(dir_path_copy, "/");
  while (token != (word*)-1)
  {
    // Find token in current directory
    word* dir_addr = brfs_data_block_addr + (current_dir_fat_idx * superblock->bytes_per_block);
    word found_dir = 0; // Keep track if token is found in current directory
    word i;
    for (i = 0; i < dir_entries_max; i++)
    {
      struct brfs_dir_entry* dir_entry = (struct brfs_dir_entry*) (dir_addr + (i * sizeof(struct brfs_dir_entry)));
      if (dir_entry->filename[0] != 0)
      {
        char decompressed_filename[16];
        strdecompress(decompressed_filename, (char*)&(dir_entry->filename));
        // Also check for directory flag
        if (strcmp(decompressed_filename, token) == 1 && dir_entry->flags == 1)
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
}

/**
 * Format the ram storage as a BRFS filesystem
 * blocks: number of blocks in the filesystem
 * bytes_per_block: number of bytes per block
 * label: label of the filesystem
 * full_format: if 1, initialize data section to 0
*/
void brfs_format(word blocks, word bytes_per_block, char* label, word full_format)
{
  // Create a superblock
  struct brfs_superblock superblock;

  // Initialize to 0
  memset((char*)&superblock, 0, sizeof(superblock));

  // Set values of superblock
  superblock.total_blocks = blocks;
  superblock.bytes_per_block = bytes_per_block;
  strcpy((char*)&superblock.label, label);
  superblock.brfs_version = 1;

  // Copy superblock to head of ram addr
  memcpy(brfs_ram_storage, (char*)&superblock, sizeof(superblock));


  // Create FAT
  memset(brfs_ram_storage + SUPERBLOCK_SIZE, 0, blocks);

  // Create Data section
  if (full_format)
  {
    memset(brfs_ram_storage + SUPERBLOCK_SIZE + blocks, 0, blocks * bytes_per_block);
  }
  
  // Initialize root dir
  word dir_entries_max = bytes_per_block / sizeof(struct brfs_dir_entry);
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
}

/**
 * Create a new directory in the directory of parent_dir_path
 * Returns 1 on success, 0 on error
 * parent_dir_path: full path of the parent directory
 * dirname: name of the new directory
*/
word brfs_create_directory(char* parent_dir_path, char* dirname)
{
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
  word* parent_dir_addr = brfs_data_block_addr + (parent_dir_fat_idx * superblock->bytes_per_block);
  word dir_entries_max = superblock->bytes_per_block / sizeof(struct brfs_dir_entry);
  word i;
  for (i = 0; i < dir_entries_max; i++)
  {
    struct brfs_dir_entry* dir_entry = (struct brfs_dir_entry*) (parent_dir_addr + (i * sizeof(struct brfs_dir_entry)));
    if (dir_entry->filename[0] != 0)
    {
      char decompressed_filename[16];
      strdecompress(decompressed_filename, (char*)&(dir_entry->filename));
      if (strcmp(decompressed_filename, dirname) == 1)
      {
        uprint(dirname);
        uprintln(" already exists!");
        return 0;
      }
    }
  }

  // Find first free dir entry
  word next_free_dir_entry = brfs_find_next_free_dir_entry(
    brfs_data_block_addr + (parent_dir_fat_idx * superblock->bytes_per_block), 
    superblock->bytes_per_block / sizeof(struct brfs_dir_entry)
  );
  if (next_free_dir_entry == -1)
  {
    uprintln("No free dir entries left!");
    return 0;
  }

  // Create dir entry
  struct brfs_dir_entry new_entry;
  brfs_create_single_dir_entry(&new_entry, dirname, next_free_block, 0, 1);

  // Copy dir entry to first free dir entry
  memcpy(
    brfs_data_block_addr + (parent_dir_fat_idx * superblock->bytes_per_block) + (next_free_dir_entry * sizeof(struct brfs_dir_entry)),
    (char*)&new_entry,
    sizeof(new_entry)
  );

  // Initialize directory
  brfs_init_directory(
    brfs_data_block_addr + (next_free_block * superblock->bytes_per_block),
    dir_entries_max,
    next_free_block,
    parent_dir_fat_idx
  );

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
  word* parent_dir_addr = brfs_data_block_addr + (parent_dir_fat_idx * superblock->bytes_per_block);
  word dir_entries_max = superblock->bytes_per_block / sizeof(struct brfs_dir_entry);
  word i;
  for (i = 0; i < dir_entries_max; i++)
  {
    struct brfs_dir_entry* dir_entry = (struct brfs_dir_entry*) (parent_dir_addr + (i * sizeof(struct brfs_dir_entry)));
    if (dir_entry->filename[0] != 0)
    {
      char decompressed_filename[16];
      strdecompress(decompressed_filename, (char*)&(dir_entry->filename));
      if (strcmp(decompressed_filename, filename) == 1)
      {
        uprint(filename);
        uprintln(" already exists!");
        return 0;
      }
    }
  }

  // Find first free dir entry
  word next_free_dir_entry = brfs_find_next_free_dir_entry(
    brfs_data_block_addr + (parent_dir_fat_idx * superblock->bytes_per_block), 
    superblock->bytes_per_block / sizeof(struct brfs_dir_entry)
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
    brfs_data_block_addr + (parent_dir_fat_idx * superblock->bytes_per_block) + (next_free_dir_entry * sizeof(struct brfs_dir_entry)),
    (char*)&new_entry,
    sizeof(new_entry)
  );

  // Initialize file by setting data to 0
  memset(
    brfs_data_block_addr + (next_free_block * superblock->bytes_per_block),
    0,
    superblock->bytes_per_block
  );

  // Update FAT
  brfs_ram_storage[SUPERBLOCK_SIZE + next_free_block] = -1;

  return 1;
}

/**
 * List the contents of a directory over UART
 * dir_path: full path of the directory
*/
void brfs_list_directory(char* dir_path)
{
  uprint("Listing directory ");
  uprintln(dir_path);
  uprintln("-------------------");

  // Find data block address of parent directory path
  word dir_fat_idx = brfs_get_fat_idx_of_dir(dir_path);
  if (dir_fat_idx == -1)
  {
    uprint("Parent directory ");
    uprint(dir_path);
    uprintln(" not found!");
    return;
  }

  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;
  word* dir_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks + (dir_fat_idx * superblock->bytes_per_block);
  word dir_entries_max = superblock->bytes_per_block / sizeof(struct brfs_dir_entry);

  word i;
  for (i = 0; i < dir_entries_max; i++)
  {
    struct brfs_dir_entry* dir_entry = (struct brfs_dir_entry*) (dir_addr + (i * sizeof(struct brfs_dir_entry)));
    if (dir_entry->filename[0] != 0)
    {
      uprint("Filename: ");
      char decompressed_filename[16];
      strdecompress(decompressed_filename, (char*)&(dir_entry->filename));
      uprint(decompressed_filename);
      uprint(" FAT idx: ");
      uprintDec((dir_entry->fat_idx));
      uprint(" Flags: ");
      uprintDec((dir_entry->flags));
      uprint(" Filesize: ");
      uprintDec((dir_entry->filesize));
      uprintc('\n');
    }
  }
  uprintln("");
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
  word* dir_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks + (dir_fat_idx * superblock->bytes_per_block);
  word dir_entries_max = superblock->bytes_per_block / sizeof(struct brfs_dir_entry);

  word i;
  for (i = 0; i < dir_entries_max; i++)
  {
    struct brfs_dir_entry* dir_entry = (struct brfs_dir_entry*) (dir_addr + (i * sizeof(struct brfs_dir_entry)));
    if (dir_entry->filename[0] != 0)
    {
      char decompressed_filename[16];
      strdecompress(decompressed_filename, (char*)&(dir_entry->filename));
      // Also check for directory flag to be 0
      if (strcmp(decompressed_filename, file_path_basename) == 1 && dir_entry->flags == 0)
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
 * Returns 1 on success, 0 on error
 * file_path: full path of the file
*/
word brfs_delete_file(char* file_path)
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
  word* dir_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks + (dir_fat_idx * superblock->bytes_per_block);
  word dir_entries_max = superblock->bytes_per_block / sizeof(struct brfs_dir_entry);

  word i;
  for (i = 0; i < dir_entries_max; i++)
  {
    struct brfs_dir_entry* dir_entry = (struct brfs_dir_entry*) (dir_addr + (i * sizeof(struct brfs_dir_entry)));
    if (dir_entry->filename[0] != 0)
    {
      char decompressed_filename[16];
      strdecompress(decompressed_filename, (char*)&(dir_entry->filename));
      // Also check for directory flag to be 0
      if (strcmp(decompressed_filename, file_path_basename) == 1 && dir_entry->flags == 0)
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
          current_fat_idx = next_fat_idx;
        }

        // Delete file
        memset((char*)dir_entry, 0, sizeof(struct brfs_dir_entry));
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
  while (cursor > superblock->bytes_per_block)
  {
    current_fat_idx = brfs_ram_storage[SUPERBLOCK_SIZE + current_fat_idx];
    if (current_fat_idx == -1)
    {
      uprintln("Cursor is out of bounds!");
      return 0;
    }
    cursor -= superblock->bytes_per_block;
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
        word words_until_end_of_block = superblock->bytes_per_block - (MATH_modU(brfs_cursors[i], superblock->bytes_per_block));
        word words_to_read = words_until_end_of_block > length ? length : words_until_end_of_block;

        // Copy words to buffer
        memcpy(buffer, data_block_addr + (current_fat_idx * superblock->bytes_per_block) + brfs_cursors[i], words_to_read);

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
        word cursor_in_block = MATH_modU(brfs_cursors[i], superblock->bytes_per_block);
        word words_until_end_of_block = superblock->bytes_per_block - cursor_in_block;
        word words_to_write = words_until_end_of_block > length ? length : words_until_end_of_block;

        // Copy words to buffer
        memcpy(data_block_addr + (current_fat_idx * superblock->bytes_per_block) + cursor_in_block, buffer, words_to_write);

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
  word* dir_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks + (dir_fat_idx * superblock->bytes_per_block);
  word dir_entries_max = superblock->bytes_per_block / sizeof(struct brfs_dir_entry);

  word i;
  for (i = 0; i < dir_entries_max; i++)
  {
    struct brfs_dir_entry* dir_entry = (struct brfs_dir_entry*) (dir_addr + (i * sizeof(struct brfs_dir_entry)));
    if (dir_entry->filename[0] != 0)
    {
      char decompressed_filename[16];
      strdecompress(decompressed_filename, (char*)&(dir_entry->filename));
      // Also check for directory flag to be 0
      if (strcmp(decompressed_filename, file_path_basename) == 1)
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


int main() 
{
  // Clear UART screen:
  uprintc(0x1B);
  uprintc(0x5B);
  uprintc(0x32);
  uprintc(0x4A);
  uprintln("------------------------");
  uprintln("BRFS test implementation");
  uprintln("------------------------");

  word blocks = 16;
  word bytes_per_block = 32;
  word full_format = 1;

  brfs_format(blocks, bytes_per_block, "Label", full_format);
  //brfs_list_directory("/");
  //brfs_dump(blocks, blocks*bytes_per_block);
  //uprintln("");

  // Create directories
  if (!brfs_create_directory("/", "dir1"))
  {
    uprintln("Error creating dir1!");
  }
  if (!brfs_create_directory("/", "dir2"))
  {
    uprintln("Error creating dir2!");
  }

  // Create files
  if (!brfs_create_file("/dir1", "file1.txt"))
  {
    uprintln("Error creating file1!");
  }
  if (!brfs_create_file("/dir1", "file2.txt"))
  {
    uprintln("Error creating file2!");
  }

  // Open file and write
  word file_pointer = brfs_open_file("/dir1/file1.txt");
  if (file_pointer == -1)
  {
    uprintln("Error opening file1!");
  }
  else
  {
    char* write_string = "This message should exceed the length of a single block, it even should exceed the length of two blocks! I am adding this part here to keep increasing the number of blocks used. This is the end of the message.";
    if (!brfs_write(file_pointer, write_string, strlen(write_string)))
    {
      uprintln("Error writing to file1!");
    }

    // Update two blocks in the middle of the file
    brfs_set_cursor(file_pointer, 57);
    char* write_string2 = "THIS PART IS WRITTEN IN THE MIDDLE OF THE FILE!";
    if (!brfs_write(file_pointer, write_string2, strlen(write_string2)))
    {
      uprintln("Error writing to file1!");
    }

    brfs_close_file(file_pointer);
  }

  // Open second file and write
  word file_pointer2 = brfs_open_file("/dir1/file2.txt");
  if (file_pointer2 == -1)
  {
    uprintln("Error opening file2!");
  }
  else
  {
    char* write_string = "Small message in file2!";
    if (!brfs_write(file_pointer2, write_string, strlen(write_string)))
    {
      uprintln("Error writing to file2!");
    }

    // Update within the first block
    brfs_set_cursor(file_pointer2, 6);
    char* write_string2 = "UPDATES";
    if (!brfs_write(file_pointer2, write_string2, strlen(write_string2)))
    {
      uprintln("Error writing to file2!");
    }

    // Skip closing the file to see data in dump
    //brfs_close_file(file_pointer2);
  }

  // Delete file1
  if (!brfs_delete_file("/dir1/file1.txt"))
  {
    uprintln("Error deleting file1!");
  }
  
  brfs_list_directory("/");
  brfs_list_directory("/dir1");
  brfs_dump(blocks, blocks*bytes_per_block);

  return 'q';
}

void interrupt()
{
  // handle all interrupts
  word i = getIntID();
  switch(i)
  {
    case INTID_TIMER1:
      timer1Value = 1; // notify ending of timer1
      break;

    default:
      break;
  }
}