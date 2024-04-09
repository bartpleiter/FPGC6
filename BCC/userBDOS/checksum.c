#define word char

#include "lib/math.c"
#include "lib/stdlib.c"
#include "lib/sys.c"
#include "lib/brfs.c"

int main() 
{
  // Read number of arguments
  word argc = shell_argc();
  if (argc < 2)
  {
    bdos_println("Usage: checksum <file>");
    return 1;
  }
  
  // Read filename
  char** args = shell_argv();
  char* filename = args[1];

  char absolute_path[MAX_PATH_LENGTH];
  // Check if absolute path
  if (filename[0] != '/')
  {
    char* cwd = fs_getcwd();
    strcpy(absolute_path, cwd);
    strcat(absolute_path, "/");
    strcat(absolute_path, filename);
  }
  else
  {
    strcpy(absolute_path, filename);
  }

  // Get file size
  struct brfs_dir_entry* entry = (struct brfs_dir_entry*)fs_stat(absolute_path);
  if ((word)entry == -1)
  {
    bdos_println("File not found");
    return 1;
  }
  word filesize = entry->filesize;

  // Open file
  word fd = fs_open(absolute_path);
  if (fd == -1)
  {
    bdos_println("File not found");
    return 1;
  }

  // Read file in chunks of 128 words
  word checksum = filesize;
  word buffer[128];
  word read;
  word i;
  // BUG: this will not work for files smaller than 128 words
  // Look at webserv for better way
  while (filesize > 0)
  {
    read = fs_read(fd, (char*)buffer, 128);
    for (i = 0; i < read; i++)
    {
      checksum += buffer[i];
    }
    filesize -= read;
  }

  // Close file
  fs_close(fd);

  // Print checksum
  bdos_print("Checksum: ");
  bdos_printhex(checksum);
  bdos_println("");

  return 'q';
}

void interrupt()
{
  // Handle all interrupts
  word i = get_int_id();
  switch(i)
  {
    case INTID_TIMER1:
      timer1Value = 1;  // Notify ending of timer1
      break;
  }
}