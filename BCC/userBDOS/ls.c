#define word char

#include "lib/math.c"
#include "lib/stdlib.c"
#include "lib/sys.c"
#include "lib/brfs.c"

/**
 * List the contents of a directory
 * Sort by filename
*/
void list_dir(char* path)
{
  struct brfs_dir_entry entries[MAX_DIR_ENTRIES];
  word num_entries = fs_readdir(path, entries);

  // Keep track of the longest filename for formatting
  word max_filename_length = 0;

  // Sort entries by filename
  word i, j;
  for (i = 0; i < num_entries - 1; i++)
  {
    for (j = 0; j < num_entries - i - 1; j++)
    {
      char decompressed_filename1[17];
      strdecompress(decompressed_filename1, entries[j].filename);

      char decompressed_filename2[17];
      strdecompress(decompressed_filename2, entries[j + 1].filename);

      // Update max_filename_length
      // This works because . is always the first entry (skipped by this check)
      if (strlen(decompressed_filename2) > max_filename_length)
      {
        max_filename_length = strlen(decompressed_filename2);
      }

      // Sort by filename
      if (strcmp(decompressed_filename1, decompressed_filename2) > 0)
      {
        // Swap filenames
        struct brfs_dir_entry temp = entries[j];
        entries[j] = entries[j + 1];
        entries[j + 1] = temp;
      }
    }
  }

  for (i = 0; i < num_entries; i++)
  {
    // Create entire line with blank spaces
    char output_line[40];
    memset(output_line, ' ', 40);
    output_line[40] = 0;

    // Add filename
    struct brfs_dir_entry entry = entries[i];
    strdecompress(output_line, entry.filename);
    output_line[strlen(output_line)] = ' ';

    // Add filesize if file
    if ((entry.flags & 0x01) == 0)
    {
      char buffer[11];
      itoa(entry.filesize, buffer);
      memcpy(output_line + max_filename_length + 1, buffer, strlen(buffer));
    }
    
    bdos_print(output_line);
  }
}

int main() 
{
  // Read number of arguments
  word argc = shell_argc();
  char** args = shell_argv();

  // Read file/dir name if provided
  char* fname;
  if (argc < 2)
  {
    fname = fs_getcwd();
  }
  else
  {
    fname = args[1];
  } 

  // Create absolute path
  char absolute_path[MAX_PATH_LENGTH];
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

  // If path is root, list root directory
  if (strcmp(absolute_path, "/") == 0)
  {
    list_dir("/");
    return 'q';
  }

  // Check if fname is a file or directory
  struct brfs_dir_entry* entry = (struct brfs_dir_entry*)fs_stat(absolute_path);
  if ((word)entry == -1)
  {
    bdos_println("Dir not found");
    return 'q';
  }

  if ((entry->flags & 0x01) == 0)
  {
    // File
    bdos_println("Not a directory");
  }
  else
  {
    // Directory
    list_dir(absolute_path);
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