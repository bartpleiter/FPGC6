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
    bdos_println("Usage: rm <file/dir>");
    return 1;
  }
  
  // Read file/dir name
  char** args = shell_argv();
  char* fname = args[1];

  char absolute_path[MAX_PATH_LENGTH];
  // Check if absolute path
  if (fname[0] != '/')
  {
    char* cwd = fs_getcwd();
    strcpy(absolute_path, cwd);
    strcat(absolute_path, "/");
    strcat(absolute_path, fname);
  }
  else
  {
    strcpy(absolute_path, fname);
  }

  // Check if fname is a file or directory
  struct brfs_dir_entry* entry = (struct brfs_dir_entry*)fs_stat(absolute_path);
  if ((word)entry == -1)
  {
    bdos_println("File not found");
    return 1;
  }

  if ((entry->flags & 0x01) == 0)
  {
    // File
    if (fs_delete(absolute_path))
    {
      fs_syncflash();
    }
    else
    {
      bdos_println("Could not delete file");
    }
  }
  else
  {
    // Directory
    uprintln("del dir");
    // TODO: Implement recursive deletion of directory
  }

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