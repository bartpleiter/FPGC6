/**
 * User library for file system operations
*/

struct brfs_dir_entry
{
  word filename[4];       // 4 chars per word
  word modify_date;       // TBD when RTC added to FPGC
  word flags;             // 32 flags, from right to left: directory, hidden 
  word fat_idx;           // idx of first FAT block
  word filesize;          // file size in words, not bytes
};
