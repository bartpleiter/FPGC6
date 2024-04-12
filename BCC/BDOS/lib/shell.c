// Max length of a single command
#define SHELL_CMD_MAX_LENGTH 128
#define SHELL_BIN_PATH "/bin/"

word shell_cmd[SHELL_CMD_MAX_LENGTH];
word shell_cmd_idx = 0;
word* shell_tokens[SHELL_CMD_MAX_LENGTH >> 1]; // Array of tokens in the command
word shell_num_tokens = 0;

word shell_path[MAX_PATH_LENGTH];

/**
 * Print the shell prompt
*/
void shell_print_prompt()
{
  GFX_PrintConsole(shell_path);
  GFX_PrintConsole("> ");
}

/**
 * Clear the current command buffer
*/
void shell_clear_command()
{
  shell_cmd[0] = 0;
  shell_cmd_idx = 0;
  shell_tokens[0] = 0;
  shell_num_tokens = 0;
}

/**
 * Append character to command buffer
*/
void shell_append_command(char c)
{
  if (shell_cmd_idx < SHELL_CMD_MAX_LENGTH)
  {
    shell_cmd[shell_cmd_idx] = c;
    shell_cmd_idx++;
    shell_cmd[shell_cmd_idx] = 0; // Terminate
  }
}

/**
 * Parse the command buffer into tokens
*/
void shell_parse_command()
{
  // Strip trailing spaces
  word i = shell_cmd_idx - 1;
  while (i >= 0 && shell_cmd[i] == ' ')
  {
    shell_cmd[i] = 0;
    i--;
  }

  shell_num_tokens = 0;
  shell_tokens[shell_num_tokens] = strtok(shell_cmd, " ");
	while((word)shell_tokens[shell_num_tokens] != -1)
  {
    shell_num_tokens++;
    shell_tokens[shell_num_tokens] = strtok(-1, " ");
  }
}

/**
 * Print help
*/
void shell_print_help()
{
  GFX_PrintConsole(
    "************\n"
    "*BDOS Shell*\n"
    "************\n"
    "Available commands:\n"
    "- cd <path>\n"
    "- clear\n"
    "- format <blk size> <blk cnt>\n"
    "- sync\n"
    "- help\n"
    "\n"
    "Programs are run from:\n"
    "- /bin\n"
    "- <current dir>\n"
  );
}

/**
 * Attempt to run program from FS
 * If run_from_path is 1, the program is run from the SHELL_BIN_PATH
 * Returns 1 if program was found, 0 otherwise
*/
word shell_run_program(word run_from_path)
{
  // Create absolute path to program
  char absolute_path[MAX_PATH_LENGTH];
  // Check if program is relative or absolute
  if (shell_tokens[0][0] == '/')
  {
    strcpy(absolute_path, shell_tokens[0]);
  }
  else
  {
    if (run_from_path)
    {
      strcpy(absolute_path, SHELL_BIN_PATH);
      strcat(absolute_path, shell_tokens[0]);
    }
    else
    {
      // Append to current path
      strcpy(absolute_path, shell_path);
      if (strlen(absolute_path) + strlen(shell_tokens[0]) < MAX_PATH_LENGTH)
      {
        // If not root, append slash
        if (strcmp(shell_path, "/") != 0)
        {
          strcat(absolute_path, "/");
        }
        strcat(absolute_path, shell_tokens[0]);
      }
      else
      {
        GFX_PrintConsole("Path too long\n");
        return 0;
      }
    }
  }

  // Get filesize of the program
  struct brfs_dir_entry* dir = brfs_stat(absolute_path);
  if ((word)dir == -1)
  {
    //GFX_PrintConsole("Program not found\n");
    return 0;
  }
  word filesize = dir->filesize;

  // Attempt to open file
  word fp = brfs_open_file(absolute_path);
  if (fp == -1)
  {
    GFX_PrintConsole("Could not open file\n");
    return 0;
  }

  // Set cursor to start of file
  brfs_set_cursor(fp, 0);

  // Read program into memory
  word* program = (word*) RUN_ADDR;
  if (!brfs_read(fp, program, filesize))
  {
    GFX_PrintConsole("Could not read file\n");
    return 0;
  }

  // Run program
  // Indicate that a user program is running
  bdos_userprogram_running = 1;

  uprintln("Running program...");

  // jump to the program
  asm(
    "; backup registers\n"
    "push r1\n"
    "push r2\n"
    "push r3\n"
    "push r4\n"
    "push r5\n"
    "push r6\n"
    "push r7\n"
    "push r8\n"
    "push r9\n"
    "push r10\n"
    "push r11\n"
    "push r12\n"
    "push r13\n"
    "push r14\n"
    "push r15\n"

    //"ccache\n"
    "savpc r1\n"
    "push r1\n"
    "jump 0x400000\n"

    "; restore registers\n"
    "pop r15\n"
    "pop r14\n"
    "pop r13\n"
    "pop r12\n"
    "pop r11\n"
    "pop r10\n"
    "pop r9\n"
    "pop r8\n"
    "pop r7\n"
    "pop r6\n"
    "pop r5\n"
    "pop r4\n"
    "pop r3\n"
    "pop r2\n"
    "pop r1\n"
  );

  // Indicate that no user program is running anymore
  bdos_userprogram_running = 0;

  bdos_restore();

  // Close file
  brfs_close_file(fp);

  return 1;
}

/**
 * Process dots in path so that ".." goes up one directory and "." is skipped
*/
void shell_process_dots(char* path)
{
  // Create a copy of the path
  char path_copy[MAX_PATH_LENGTH];
  strcpy(path_copy, path);

  // Split path by '/' and traverse directories
  word num_tokens = 0;
  char* tokens[MAX_PATH_LENGTH >> 1];
  tokens[num_tokens] = strtok(path_copy+1, "/");
  while((word)tokens[num_tokens] != -1)
  {
    num_tokens++;
    tokens[num_tokens] = strtok(-1, "/");
  }

  if (num_tokens == 0)
  {
    return;
  }

  strcpy(path, "/");

  // Traverse path
  word i = 0;
  while (i < num_tokens)
  {
    if (strcmp(tokens[i], ".") == 0)
    {
      // Skip
    }
    else if (strcmp(tokens[i], "..") == 0)
    {
      // Remove all characters after last slash
      word j = strlen(path) - 1;
      while (j > 0 && path[j] != '/')
      {
        path[j] = 0;
        j--;
      }
    }
    else
    {
      // Copy token
      if (strlen(path) > 1)
      {
        strcat(path, "/");
      }
      strcat(path, tokens[i]);
    }
    i++;
  }
}

/**
 * Change directory
*/
void shell_change_directory()
{
  if (shell_num_tokens != 2)
  {
    GFX_PrintConsole("Usage: cd <path>\n");
    return;
  }

  // Remove trailing slashes, skipping the first character
  word i = strlen(shell_tokens[1]) - 1;
  while (i >= 1 && shell_tokens[1][i] == '/')
  {
    shell_tokens[1][i] = 0;
    i--;
  }

  // Check if path is root
  if (strcmp(shell_tokens[1], "/") == 0)
  {
    strcpy(shell_path, "/");
    return;
  }

  char absolute_path[MAX_PATH_LENGTH];

  // Create absolute path
  // Check if argument is relative or absolute
  if (shell_tokens[1][0] == '/')
  {
    strcpy(absolute_path, shell_tokens[1]);
  }
  else
  {
    // Append to current path
    strcpy(absolute_path, shell_path);
    if (strlen(absolute_path) + strlen(shell_tokens[1]) < MAX_PATH_LENGTH)
    {
      // If not root, append slash
      if (strcmp(shell_path, "/") != 0)
      {
        strcat(absolute_path, "/");
      }
      strcat(absolute_path, shell_tokens[1]);
    }
    else
    {
      GFX_PrintConsole("Path too long\n");
    }
  }

  // Get dir entry of the path
  struct brfs_dir_entry* dir = brfs_stat(absolute_path);

  // Check if valid and is a directory
  if ((word)dir != -1 && dir->flags & (1 << 0))
  {
    // Set new path
    strcpy(shell_path, absolute_path);
    // Process dots in path
    shell_process_dots(shell_path);
  }
  else
  {
    GFX_PrintConsole("Directory not found\n");
  }
}

/**
 * Format the filesystem
*/
void shell_format_filesystem()
{
  if (shell_num_tokens < 3)
  {
    GFX_PrintConsole("Usage: format <blk size> <blk cnt>\n");
    return;
  }

  word block_size = strToInt(shell_tokens[1]);
  word block_count = strToInt(shell_tokens[2]);
  word full_format = 1;
  brfs_format(block_count, block_size, "FPGC", full_format);
  brfs_write_to_flash();
}

/**
 * Show filesystem usage
*/
void shell_show_fs_usage()
{
  struct brfs_superblock* superblock = (struct brfs_superblock*)brfs_ram_storage;
  word total_blocks = superblock->total_blocks;
  word block_size = superblock->words_per_block;

  char* fat_addr = brfs_ram_storage + SUPERBLOCK_SIZE;

  // Loop through FAT table and count free blocks
  word free_blocks = 0;
  word i = 0;
  while (i < total_blocks)
  {
    if (fat_addr[i] == 0)
    {
      free_blocks++;
    }
    i++;
  }

  word used_blocks = total_blocks - free_blocks;
  word used_space = used_blocks * block_size;
  word free_space = free_blocks * block_size;
  word total_space = total_blocks * block_size;

  GFX_PrintConsole("Usage:\n");
  GFX_PrintConsole("------\n");

  GFX_PrintConsole("Used blocks : ");
  GFX_PrintDecConsole(used_blocks);
  GFX_PrintConsole("/");
  GFX_PrintDecConsole(total_blocks);
  GFX_PrintConsole("\n");
  GFX_PrintConsole("Used space  : ");
  GFX_PrintDecConsole(MATH_divU(used_space, 1000));
  GFX_PrintConsole("/");
  GFX_PrintDecConsole(MATH_divU(total_space, 1000));
  GFX_PrintConsole(" kwords\n");

  GFX_PrintConsole("\nFree space:\n");
  GFX_PrintConsole("-----------\n");

  GFX_PrintConsole("Free blocks : ");
  GFX_PrintDecConsole(free_blocks);
  GFX_PrintConsole("\n");
  GFX_PrintConsole("Free space  : ");
  GFX_PrintDecConsole(MATH_divU(free_space, 1000));
  GFX_PrintConsole(" kwords\n");
  

  GFX_PrintConsole("\nBlocks:\n");
  GFX_PrintConsole("-------\n");

  GFX_PrintConsole("Total blocks: ");
  GFX_PrintDecConsole(total_blocks);
  GFX_PrintConsole("\n");
  GFX_PrintConsole("Block size  : ");
  GFX_PrintDecConsole(block_size);
  GFX_PrintConsole(" words\n");
}

/**
 * Handle the command
*/
void shell_handle_command()
{
  if (strcmp(shell_tokens[0], "cd") == 0)
  {
    shell_change_directory();
  }
  else if (strcmp(shell_tokens[0], "clear") == 0)
  {
    // clear screen by clearing window tables and resetting the cursor
    GFX_clearWindowtileTable();
    GFX_clearWindowpaletteTable();
    GFX_cursor = 0;
  }
  else if (strcmp(shell_tokens[0], "help") == 0)
  {
    shell_print_help();
  }
  else if (strcmp(shell_tokens[0], "sync") == 0)
  {
    brfs_write_to_flash();
  }
  else if (strcmp(shell_tokens[0], "dump") == 0)
  {
    // Dump FAT
    brfs_dump_section(brfs_ram_storage+SUPERBLOCK_SIZE, strToInt(shell_tokens[1]), 16);
  }
  else if (strcmp(shell_tokens[0], "format") == 0)
  {
    shell_format_filesystem();
  }
  else if (strcmp(shell_tokens[0], "df") == 0)
  {
    shell_show_fs_usage();
  }
  // Attempt to run program both from local dir and from SHELL_BIN_PATH
  else if (!shell_run_program(0))
  {
    if (!shell_run_program(1))
    {
      GFX_PrintConsole("Command not found\n");
    }
  } 
}

/**
 * Initialize shell
*/
void shell_init()
{
  // Clear screen by clearing window tables and resetting the cursor
  GFX_clearWindowtileTable();
  GFX_clearWindowpaletteTable();
  GFX_cursor = 0;

  shell_clear_command();
  // Set path to root
  strcpy(shell_path, "/");
  shell_print_prompt();
}

/**
 * Main shell loop
*/
void shell_loop()
{
  // Read command from HID FIFO
  if (HID_FifoAvailable())
  {
    char c = HID_FifoRead();
    if (c == '\n')
    {
      GFX_PrintcConsole('\n');
      shell_parse_command();
      if (shell_num_tokens != 0)
      {
        shell_handle_command();
      }
      shell_clear_command();
      shell_print_prompt();
    }
    else if (c == 0x8) // backspace
    {
      if (shell_cmd_idx > 0)
      {
        shell_cmd_idx--;
        shell_cmd[shell_cmd_idx] = 0;
        GFX_PrintcConsole(0x8);
      }
    }
    else
    {
      // Append character to command buffer and print it
      shell_append_command(c);
      GFX_PrintcConsole(c);
    }
  }
}