#define word char

#include "lib/math.c"
#include "lib/stdlib.c"
#include "lib/sys.c"

int main() 
{
  // Read number of arguments
  word argc = shell_argc();
  if (argc < 2)
  {
    bdos_println("Usage: mkdir <directory>");
    return 1;
  }
  
  // Read directory name
  char** dirname = shell_argv(1);

  char absolute_path[MAX_PATH_LENGTH];
  // Check if absolute path
  if (dirname[1][0] != '/')
  {
    char* cwd = fs_getcwd();
    uprintln("CWD:");
    uprintln(cwd);
    strcpy(absolute_path, cwd);
    strcat(absolute_path, "/");
    strcat(absolute_path, dirname[1]);
  }
  else
  {
    strcpy(absolute_path, dirname[1]);
  }

  // Create directory
  if (fs_mkdir(absolute_path))
  {
    bdos_println("Directory created");
  }
  else
  {
    bdos_println("Could not create directory");
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