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
Required operations:
- Format
- Create directory
- Create file
- Open file (allow multiple files open at once)
- Close file (update dir entry and check/update all FAT entries)
- Set cursor
- Get cursor
- Read file
- Write file
- Delete entire file (deleting part of file is not a thing)
- Change directory
- List directory
*/

#define word char

#include "LIB/MATH.C"
#include "LIB/SYS.C"
#include "LIB/STDLIB.C"

#define BRFS_RAM_STORAGE_ADDR 0x600000

#define SUPERBLOCK_SIZE 16

word *brfs_ram_storage = (word*) BRFS_RAM_STORAGE_ADDR; // RAM storage of file system

word brfs_current_dir = 0; // Current directory index, points to block in data section

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

void brfs_create_single_dir_entry(struct brfs_dir_entry* dir_entry, char* filename, word fat_idx, word filesize, word flags)
{
  // Initialize to 0
  memset(dir_entry, 0, sizeof(*dir_entry));

  // Set filename
  char compressed_filename[4] = {0,0,0,0};
  strcompress(compressed_filename, filename);
  memcpy(&(dir_entry->filename), compressed_filename, sizeof(compressed_filename));

  // Set other fields
  dir_entry->fat_idx = fat_idx;
  dir_entry->flags = flags;
  dir_entry->filesize = filesize;
}

void brfs_init_directory(word* dir_addr, word dir_entries_max, word dir_fat_idx, word parent_fat_idx)
{
  // Create . entry
  struct brfs_dir_entry dir_entry;
  brfs_create_single_dir_entry(&dir_entry, ".", dir_fat_idx, dir_entries_max*sizeof(struct brfs_dir_entry), 1);
  // Copy to first data entry
  memcpy(dir_addr, &dir_entry, sizeof(dir_entry));

  // Create .. entry
  brfs_create_single_dir_entry(&dir_entry, "..", parent_fat_idx, dir_entries_max*sizeof(struct brfs_dir_entry), 1);
  // Copy to second data entry
  memcpy(dir_addr+sizeof(dir_entry), &dir_entry, sizeof(dir_entry));

  // Set FAT table
  brfs_ram_storage[SUPERBLOCK_SIZE + dir_fat_idx] = -1;
}

// Creates hexdump like dump
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

void brfs_dump(word* ram_addr, word fatsize, word datasize)
{
  // Superblock dump
  uprintln("Superblock:");
  brfs_dump_section(ram_addr, SUPERBLOCK_SIZE, 16);

  // FAT dump
  uprintln("\nFAT:");
  brfs_dump_section(ram_addr+SUPERBLOCK_SIZE, fatsize, 8);

  // Datablock dump
  uprintln("\nData:");
  brfs_dump_section(ram_addr+SUPERBLOCK_SIZE+fatsize, datasize, 32);

  uprintc('\n');
}

void brfs_list_directory(word* ram_addr)
{
  struct brfs_superblock* superblock = (struct brfs_superblock*) ram_addr;
  word* dir_addr = ram_addr + SUPERBLOCK_SIZE + superblock->total_blocks + (brfs_current_dir * superblock->bytes_per_block);
  word dir_entries_max = superblock->bytes_per_block / sizeof(struct brfs_dir_entry);

  uprintln("-------------------");

  word i;
  for (i = 0; i < dir_entries_max; i++)
  {
    struct brfs_dir_entry* dir_entry = (struct brfs_dir_entry*) (dir_addr + (i * sizeof(struct brfs_dir_entry)));
    if (dir_entry->filename[0] != 0)
    {
      uprint("Filename: ");
      char decompressed_filename[16];
      strdecompress(decompressed_filename, &(dir_entry->filename));
      uprintln(decompressed_filename);
      uprint("FAT idx: ");
      uprintDec((dir_entry->fat_idx));
      uprint("Flags: ");
      uprintDec((dir_entry->flags));
      uprint("Filesize: ");
      uprintDec((dir_entry->filesize));
      uprintc('\n');
    }
  }
}

void brfs_change_directory(word* ram_addr, char* dirname)
{
  struct brfs_superblock* superblock = (struct brfs_superblock*) ram_addr;
  word* dir_addr = ram_addr + SUPERBLOCK_SIZE + superblock->total_blocks + (brfs_current_dir * superblock->bytes_per_block);
  word dir_entries_max = superblock->bytes_per_block / sizeof(struct brfs_dir_entry);

  word i;
  for (i = 0; i < dir_entries_max; i++)
  {
    struct brfs_dir_entry* dir_entry = (struct brfs_dir_entry*) (dir_addr + (i * sizeof(struct brfs_dir_entry)));
    if (dir_entry->filename[0] != 0)
    {
      char decompressed_filename[16];
      strdecompress(decompressed_filename, &(dir_entry->filename));
      if (strcmp(decompressed_filename, dirname) == 1)
      {
        brfs_current_dir = dir_entry->fat_idx;
        return;
      }
    }
  }

  uprintln("Directory not found!");
}

void brfs_format(word* ram_addr, word blocks, word bytes_per_block, char* label, word full_format)
{
  // Create a superblock
  struct brfs_superblock superblock;

  // Initialize to 0
  memset(&superblock, 0, sizeof(superblock));

  // Set values of superblock
  superblock.total_blocks = blocks;
  superblock.bytes_per_block = bytes_per_block;
  strcpy(&(superblock.label), label);
  superblock.brfs_version = 1;

  // Copy superblock to head of ram addr
  memcpy(ram_addr, &superblock, sizeof(superblock));


  // Create FAT
  memset(ram_addr + SUPERBLOCK_SIZE, 0, blocks);

  // Create Data section
  if (full_format)
  {
    memset(ram_addr + SUPERBLOCK_SIZE + blocks, 0, blocks * bytes_per_block);
  }
  
  // Initialize root dir
  word dir_entries_max = bytes_per_block / sizeof(struct brfs_dir_entry);
  brfs_init_directory(ram_addr + SUPERBLOCK_SIZE + blocks, dir_entries_max, 0, 0);

  brfs_current_dir = 0;
}

/*
* Creates directory in current directory
*/
void brfs_create_directory(word* ram_addr, char* dirname)
{
  struct brfs_superblock* superblock = (struct brfs_superblock*) ram_addr;

  // Find first free FAT block
  word next_free_block = brfs_find_next_free_block(ram_addr + SUPERBLOCK_SIZE, superblock->total_blocks);
  if (next_free_block == -1)
  {
    uprintln("No free blocks left!");
    return;
  }

  // Find first free dir entry
  word next_free_dir_entry = brfs_find_next_free_dir_entry(
    ram_addr + SUPERBLOCK_SIZE + superblock->total_blocks + (brfs_current_dir * superblock->bytes_per_block), 
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
    ram_addr + SUPERBLOCK_SIZE + superblock->total_blocks + (brfs_current_dir * superblock->bytes_per_block) + (next_free_dir_entry * sizeof(struct brfs_dir_entry)),
    &new_entry,
    sizeof(new_entry)
  );

  // Initialize directory
  word dir_entries_max = superblock->bytes_per_block / sizeof(struct brfs_dir_entry);
  brfs_init_directory(
    ram_addr + SUPERBLOCK_SIZE + superblock->total_blocks + (next_free_block * superblock->bytes_per_block),
    dir_entries_max,
    next_free_block,
    brfs_current_dir
  );
}

/*
* Creates an empty file in current directory
*/
void brfs_create_file(word* ram_addr, char* filename)
{
  struct brfs_superblock* superblock = (struct brfs_superblock*) ram_addr;

  // Find first free FAT block
  word next_free_block = brfs_find_next_free_block(ram_addr + SUPERBLOCK_SIZE, superblock->total_blocks);
  if (next_free_block == -1)
  {
    uprintln("No free blocks left!");
    return;
  }

  // Find first free dir entry
  word next_free_dir_entry = brfs_find_next_free_dir_entry(
    ram_addr + SUPERBLOCK_SIZE + superblock->total_blocks + (brfs_current_dir * superblock->bytes_per_block), 
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
    ram_addr + SUPERBLOCK_SIZE + superblock->total_blocks + (brfs_current_dir * superblock->bytes_per_block) + (next_free_dir_entry * sizeof(struct brfs_dir_entry)),
    &new_entry,
    sizeof(new_entry)
  );

  // Initialize file by setting data to 0
  memset(
    ram_addr + SUPERBLOCK_SIZE + superblock->total_blocks + (next_free_block * superblock->bytes_per_block),
    0,
    superblock->bytes_per_block
  );

  // Update FAT
  ram_addr[SUPERBLOCK_SIZE + next_free_block] = -1;
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

  brfs_format(brfs_ram_storage, blocks, bytes_per_block, "Label", full_format);

  brfs_create_file(brfs_ram_storage, "file1");

  brfs_create_directory(brfs_ram_storage, "dir1");

  brfs_list_directory(brfs_ram_storage);

  brfs_change_directory(brfs_ram_storage, "dir1");

  brfs_create_file(brfs_ram_storage, "file2");

  brfs_list_directory(brfs_ram_storage);

  brfs_change_directory(brfs_ram_storage, "..");

  brfs_list_directory(brfs_ram_storage);

  brfs_dump(brfs_ram_storage, blocks, blocks*bytes_per_block);

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

    case INTID_TIMER2:
      break;

    case INTID_UART0:
      break;

    case INTID_GPU:
      break;

    case INTID_TIMER3:
      break;

    case INTID_PS2:
      break;

    case INTID_UART1:
      break;

    case INTID_UART2:
      break;
  }
}