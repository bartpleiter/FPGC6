#define word char

#include "lib/math.c"
#include "lib/stdlib.c"
#include "lib/sys.c"
#include "lib/brfs.c"

#define READ_BUFFER_SIZE 64

int main() 
{
  // Read number of arguments
  word argc = shell_argc();
  if (argc < 2)
  {
    bdos_println("Usage: print <file>");
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


  // Read file in chunks of READ_BUFFER_SIZE
  word file_buffer[READ_BUFFER_SIZE + 1]; // +1 for null terminator
  word chunk_to_read;

  while (filesize > 0)
  {
    chunk_to_read = filesize > READ_BUFFER_SIZE ? READ_BUFFER_SIZE : filesize;
    memset(file_buffer, 0, READ_BUFFER_SIZE + 1);
    fs_read(fd, file_buffer, chunk_to_read);
    file_buffer[chunk_to_read] = 0; // Null terminator
    bdos_print(file_buffer);
    filesize -= chunk_to_read;
  }

  // Close file
  fs_close(fd);

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