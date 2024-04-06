/**
 * User library for file system operations
*/

#define MAX_DIR_ENTRIES 128 // Safe bound on max number of entries in a directory (128 -> full dir on block size of 512 words)
#define MAX_PATH_LENGTH 127

struct brfs_dir_entry
{
  word filename[4];       // 4 chars per word
  word modify_date;       // TBD when RTC added to FPGC
  word flags;             // 32 flags, from right to left: directory, hidden 
  word fat_idx;           // idx of first FAT block
  word filesize;          // file size in words, not bytes
};
