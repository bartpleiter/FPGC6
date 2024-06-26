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
    bdos_println("Usage: mkfile <fname>");
    return 1;
  }
  
  // Read fname
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

  // Create file
  if (fs_mkfile(absolute_path))
  {
    //fs_syncflash();
  }
  else
  {
    bdos_println("Could not create file");
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