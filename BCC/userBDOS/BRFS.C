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
- [] Open file (not a dir!) (allow multiple files open at once)
- [] Close file (update dir entry and check/update all FAT entries)
- [] Stat (returns dir entry)
- [] Set cursor
- [] Get cursor
- [] Read file
- [] Write file
- [] Delete entire file (deleting part of file is not a thing)
- [x] List directory
*/

#define word char

#include "LIB/MATH.C"
#include "LIB/SYS.C"
#include "LIB/STDLIB.C"

#define BRFS_RAM_STORAGE_ADDR 0x600000

#define MAX_PATH_LENGTH 127

// Length of structs, should not be changed
#define SUPERBLOCK_SIZE 16
#define DIR_ENTRY_SIZE 8

word *brfs_ram_storage = (word*) BRFS_RAM_STORAGE_ADDR; // RAM storage of file system

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
  brfs_dump_section(brfs_ram_storage+SUPERBLOCK_SIZE, fatsize, 8);

  // Datablock dump
  uprintln("\nData:");
  brfs_dump_section(brfs_ram_storage+SUPERBLOCK_SIZE+fatsize, datasize, 32);

  uprintc('\n');
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
        if (strcmp(decompressed_filename, token) == 1)
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
}

/**
 * Create a new directory in the directory of parent_dir_path
 * parent_dir_path: full path of the parent directory
 * dirname: name of the new directory
*/
void brfs_create_directory(char* parent_dir_path, char* dirname)
{
  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;

  word* brfs_data_block_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks;

  // Find first free FAT block
  word next_free_block = brfs_find_next_free_block(brfs_ram_storage + SUPERBLOCK_SIZE, superblock->total_blocks);
  if (next_free_block == -1)
  {
    uprintln("No free blocks left!");
    return;
  }

  // Find data block address of parent directory path
  word parent_dir_fat_idx = brfs_get_fat_idx_of_dir(parent_dir_path);
  if (parent_dir_fat_idx == -1)
  {
    uprint("Parent directory ");
    uprint(parent_dir_path);
    uprintln(" not found!");
    return;
  }

  // Find first free dir entry
  word next_free_dir_entry = brfs_find_next_free_dir_entry(
    brfs_data_block_addr + (parent_dir_fat_idx * superblock->bytes_per_block), 
    superblock->bytes_per_block / sizeof(struct brfs_dir_entry)
  );
  if (next_free_dir_entry == -1)
  {
    uprintln("No free dir entries left!");
    return;
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
  word dir_entries_max = superblock->bytes_per_block / sizeof(struct brfs_dir_entry);
  brfs_init_directory(
    brfs_data_block_addr + (next_free_block * superblock->bytes_per_block),
    dir_entries_max,
    next_free_block,
    parent_dir_fat_idx
  );
}

/**
 * Create a new file in the directory of parent_dir_path
 * parent_dir_path: full path of the parent directory
 * filename: name of the new file
*/
void brfs_create_file(char* parent_dir_path, char* filename)
{
  struct brfs_superblock* superblock = (struct brfs_superblock*) brfs_ram_storage;

  word* brfs_data_block_addr = brfs_ram_storage + SUPERBLOCK_SIZE + superblock->total_blocks;

  // Find first free FAT block
  word next_free_block = brfs_find_next_free_block(brfs_ram_storage + SUPERBLOCK_SIZE, superblock->total_blocks);
  if (next_free_block == -1)
  {
    uprintln("No free blocks left!");
    return;
  }

  // Find data block address of parent directory path
  word parent_dir_fat_idx = brfs_get_fat_idx_of_dir(parent_dir_path);
  if (parent_dir_fat_idx == -1)
  {
    uprint("Parent directory ");
    uprint(parent_dir_path);
    uprintln(" not found!");
    return;
  }

  // Find first free dir entry
  word next_free_dir_entry = brfs_find_next_free_dir_entry(
    brfs_data_block_addr + (parent_dir_fat_idx * superblock->bytes_per_block), 
    superblock->bytes_per_block / sizeof(struct brfs_dir_entry)
  );
  if (next_free_dir_entry == -1)
  {
    uprintln("No free dir entries left!");
    return;
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

  word blocks = 8;
  word bytes_per_block = 32;
  word full_format = 1;

  brfs_format(blocks, bytes_per_block, "Label", full_format);

  brfs_dump(blocks, blocks*bytes_per_block);

  brfs_list_directory("/");

  brfs_create_file("/", "file1.txt");

  brfs_list_directory(".");

  brfs_create_directory("..", "dir1");
  brfs_create_file("dir1", "file2.txt");

  brfs_list_directory(".");
  brfs_list_directory("dir1");

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