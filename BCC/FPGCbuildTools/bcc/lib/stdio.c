/*
* Stdio replacement library
* Maps common stdio functions to brfs functions
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


// write string to file
word fputs(word fd, char* s)
{
    fs_write(fd, s, strlen(s));
    return 1;
}

// write string to file
word fputc(word fd, char c)
{
    fs_write(fd, &c, 1);
    return 1;
}

word printf(char* s)
{
    bdos_print(s);
}

word printd(word d)
{
    bdos_printdec(d);
}
