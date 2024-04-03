# BRFS (File System)

To get over some of the limitations of the CH376 chip (like 8.3 file naming and no multiple open files at the same time), and to learn how a filesystem actually works, BRFS (Bart's RAM File System) was created.

## General idea

Given the state and speed of the FPGC being similar to a home computer of ~1990, only tens of megabytes of persistent storage should be enough for most applications (Fun fact: HDD sizes for the average home user was <100MB until after 1990).
SPI NOR Flash provides around the same amount of storage, with very little wear compared to NAND Flash, so no wear leveling is needed.
Given that SPI NOR Flash has very fast reads and very slow writes, and that the FPGC already has 64MiB RAM (which is a lot even for 32 bit addressable words), 32MiB of this RAM could easily provide enough space for the entire filesystem with SPI NOR Flash used for persisting the filesystem between reboots/crashes. In contrast to the implementation of CH376, BRFS should be completely managed by BDOS and present via system calls to each application.

## Features

- High speed: BRFS should be very fast as it is fully in RAM
- Low complexity: Inspired by a very simplified version of the FAT filesystem
- Support for nested directories: Each data block can be used for either file data or directory entries
    - Max (words_per_block/8) entries per directory, of which two are "." and ".."
- Filenames of 16 characters: File extensions are part of the filename
- Non-hardcoded values for number of blocks and words per block
- Automatically keeps track which blocks are updated, and only the updated blocks are written to SPI Flash for optimal performance
- Optimized to use 32 bit words instead of bytes, as this is how FPGC operates
- Allows multiple files open at the same time
- Supports cursor operations

## Implementation details

``` text
16 word superblock:
  - (1)  total blocks
  - (1)  bytes per block
  - (10) label [1 char per word]
  - (1)  brfs version
  - (3)  reserved
```

``` text
8 word dir entries:
  - (4) filename.ext [4 chars per word -> 16 chars total]
  - (1) modify date [to be implemented when RTC]
  - (1) flags [max 32 flags, lsb = is_directory]
  - (1) 1st FAT idx of file
  - (1) file size [in words, not bytes]
```

Similar to a linked list, each data block has its own FAT entry (of one word) in the FAT table which points to the next data block of the file, or -1 if it is the last block of the file (or if it is a dir). Each dir entry contains the first FAT index of the file.

## Implementation Notes:

- Current directory is managed by the application/OS, not the FS. Directory (or file) can be checked to exist using stat()
- Updating a file: very useful for appending data (like in logs), but otherwise might be better to delete and create a new one (task of the application instead of the FS)
- Write operation starts writing at the cursor position and jumps to the next block if the end of block is reached (also updates FAT).
- No delete/backspace, only delete entire file or overwrite data

## Implemented operations

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
- [x] Function to read BRFS from SPI Flash (to be used on startup of BDOS)
- [x] Function to write BRFS to SPI Flash (to be used by applications/OS via System Call)
