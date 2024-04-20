/*
* Stdio replacement library
* Maps common stdio functions to brfs functions
* Adds buffers to improve performance.
* Outputfile is written in memory first and then written to disk.
*/

#define EOF -1

#define FGETC_BUFFER_SIZE 512

word fgetc_buffer_main[FGETC_BUFFER_SIZE];
word fgetc_buffer_lib[FGETC_BUFFER_SIZE];
word fgetc_buffer_cursor_main = -1;
word fgetc_buffer_cursor_lib = -1;
word fgetc_buffer_fd_main = -1;
word fgetc_buffer_fd_lib = -1;

char *outfileData = (char*) OUTFILE_DATA_ADDR;
word outfileCursor = 0;

// returns the current char at cursor within the opened file (EOF if end of file)
// increments the cursor
// For the buffer, it assumes that not more than two files are open for reading at the same time
word fgetc(word fd, word filesize)
{
  if (fgetc_buffer_fd_main == -1)
  {
    fgetc_buffer_fd_main = fd;
  }

  if (fd != fgetc_buffer_fd_main)
  {
    if (fd != fgetc_buffer_fd_lib)
    {
      // Clear buffer for new lib file
      fgetc_buffer_cursor_lib = -1;
    }
    fgetc_buffer_fd_lib = fd;
  }


  if (fd == fgetc_buffer_fd_main)
  {
    if (fgetc_buffer_cursor_main == -1 || fgetc_buffer_cursor_main == FGETC_BUFFER_SIZE)
    {
      word cursor = fs_getcursor(fd);
      if (filesize - cursor < FGETC_BUFFER_SIZE)
      {
        fs_read(fd, fgetc_buffer_main, filesize - cursor);
        fgetc_buffer_main[filesize - cursor] = EOF;
      }
      else
      {
        fs_read(fd, fgetc_buffer_main, FGETC_BUFFER_SIZE);
      }
      fgetc_buffer_cursor_main = 0;
    }

    char c = fgetc_buffer_main[fgetc_buffer_cursor_main];
    fgetc_buffer_cursor_main++;
    return c;
  }
  else if (fd == fgetc_buffer_fd_lib)
  {
    if (fgetc_buffer_cursor_lib == -1 || fgetc_buffer_cursor_lib == FGETC_BUFFER_SIZE)
    {
      word cursor = fs_getcursor(fd);
      if (filesize - cursor < FGETC_BUFFER_SIZE)
      {
        fs_read(fd, fgetc_buffer_lib, filesize - cursor);
        fgetc_buffer_lib[filesize - cursor] = EOF;
      }
      else
      {
        fs_read(fd, fgetc_buffer_lib, FGETC_BUFFER_SIZE);
      }
      fgetc_buffer_cursor_lib = 0;
    }

    char c = fgetc_buffer_lib[fgetc_buffer_cursor_lib];
    fgetc_buffer_cursor_lib++;
    return c;
  }
  bdos_println("UNEXPECTED ERROR IN FGETC");
  return EOF;
}

// Ignore fp as we assume only one output file
void stdio_setcursor(word fp, word pos)
{
  outfileCursor = pos;
}

// Ignore fp as we assume only one output file
word stdio_getcursor(word fp)
{
  return outfileCursor;
}

word fputs(word fd, char* s)
{
  word len = strlen(s);
  memcpy(outfileData + outfileCursor, s, len);
  outfileCursor += len;
}

word fputc(word fd, char c)
{
  outfileData[outfileCursor] = c;
  outfileCursor++;
}

// Flush output buffer to filesystem
void stdio_flush(word fd)
{
  fs_write(fd, outfileData, outfileCursor);
  outfileCursor = 0;
}

word printf(char* s)
{
  bdos_print(s);
}

word printd(word d)
{
  bdos_printdec(d);
}
