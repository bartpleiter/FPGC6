#define word char

#include "lib/math.c"
#include "lib/stdlib.c"
#include "lib/sys.c"
#include "lib/brfs.c"

void remove_dir(char* path)
{
  struct brfs_dir_entry entries[MAX_DIR_ENTRIES];
  word num_entries = fs_readdir(path, entries);

  word i;
  for (i = 0; i < num_entries; i++)
  {
    struct brfs_dir_entry entry = entries[i];
    char decompressed_filename[17];
    strdecompress(decompressed_filename, entry.filename);
    
    if (strcmp(decompressed_filename, ".") == 0 || strcmp(decompressed_filename, "..") == 0)
    {
      continue;
    }

    char absolute_path[MAX_PATH_LENGTH];
    strcpy(absolute_path, path);
    strcat(absolute_path, "/");
    strcat(absolute_path, decompressed_filename);

    if ((entry.flags & 0x01) == 0)
    {
      // File
      if (fs_delete(absolute_path))
      {
        // Do not sync flash after each file deletion
      }
      else
      {
        bdos_println("Could not delete file");
      }
    }
    else
    {
      // Directory
      remove_dir(absolute_path);
    }
  }

  // Delete directory after contents are deleted
  if (fs_delete(path))
  {
    // Do not sync flash after each directory deletion
  }
  else
  {
    bdos_println("Could not delete directory");
  }
  
  
}

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
    if (absolute_path[strlen(absolute_path) - 1] != '/')
    {
      strcat(absolute_path, "/");
    }
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

  // Check if root directory
  if (entry->fat_idx == 0)
  {
    bdos_println("Cannot delete root directory");
    return 1;
  }

  if ((entry->flags & 0x01) == 0)
  {
    // File
    if (fs_delete(absolute_path))
    {
      //fs_syncflash();
    }
    else
    {
      bdos_println("Could not delete file");
    }
  }
  else
  {
    // Directory
    remove_dir(absolute_path);
    //fs_syncflash();
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