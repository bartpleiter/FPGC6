/*
* Very simple stdio library for mapping stdio functions to brfs.
* Adds buffer for fgetc to improve performance.
*/

#define EOF -1

#define FGETC_BUFFER_SIZE 512

word fgetc_buffer[FGETC_BUFFER_SIZE];
word fgetc_buffer_cursor = -1;

// returns the current char at cursor within the opened file (EOF if end of file)
// increments the cursor
word fgetc(word fd, word filesize)
{
  if (fgetc_buffer_cursor == -1 || fgetc_buffer_cursor == FGETC_BUFFER_SIZE)
  {
    word cursor = fs_getcursor(fd);
    if (filesize - cursor < FGETC_BUFFER_SIZE)
    {
      fs_read(fd, fgetc_buffer, filesize - cursor);
      fgetc_buffer[filesize - cursor] = EOF;
    }
    else
    {
      fs_read(fd, fgetc_buffer, FGETC_BUFFER_SIZE);
    }
    fgetc_buffer_cursor = 0;
  }

  char c = fgetc_buffer[fgetc_buffer_cursor];
  fgetc_buffer_cursor++;
  return c;
}
