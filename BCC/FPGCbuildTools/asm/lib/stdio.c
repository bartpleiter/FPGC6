/*
* Very simple stdio library for mapping stdio functions to brfs.
*/

#define EOF -1

// returns the current char at cursor within the opened file (EOF if end of file)
// increments the cursor
word fgetc(word fd, word filesize)
{
  word cursor = fs_getcursor(fd);
  if (cursor == filesize)
  {
    return EOF;
  }

  char c;
  fs_read(fd, &c, 1);
  return c;
}
