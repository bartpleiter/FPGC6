// Test implementation of Bart's RAM File System (BRFS)

/*
General Idea:
- HDD for the average home user was <100MB until after 1990
- SPI NOR Flash provides around the same amount of storage, with relatively little wear
    - However, writes are extremely slow
- FPGC has 64MiB RAM, which is a lot even for 64 bit addressable words
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
  - (4) filename.ext [4 chars per word]
  - (1) modify date [to be implemented when RTC]
  - (1) flags [max 32 flags, TBD]
  - (1) 1st FAT idx
  - (1) file size [in words, not bytes]
*/

#define word char

#include "LIB/MATH.C"
#include "LIB/SYS.C"
#include "LIB/STDLIB.C"

#define BRFS_RAM_STORAGE_ADDR 0x500000

#define SUPERBLOCK_SIZE 16

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

// 16 words long
struct brfs_dir_entry
{
  word filename[4];       // 4 chars per word
  word modify_date;       // TBD when RTC added to FPGC
  word flags;
  word fat_idx;           // idx of first FAT block
  word filesize;          // file size in words, not bytes
};


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
  brfs_dump_section(ram_addr+SUPERBLOCK_SIZE+fatsize, datasize, 16);

  uprintc('\n');
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
  
  // Create root dir entry in first block
  struct brfs_dir_entry root_dir;

  // Initialize to 0
  memset(&root_dir, 0, sizeof(root_dir));

  // Copy root dir entry to first block
  memcpy(ram_addr + SUPERBLOCK_SIZE + blocks, &root_dir, sizeof(root_dir));

  
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
  word bytes_per_block = 16;
  word full_format = 1;

  brfs_format(brfs_ram_storage, blocks, bytes_per_block, "Label", full_format);

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