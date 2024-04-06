/**
 * Contains system functions for user programs
 * Contains code for system calls and interrupt handling
*/

// Interrupt IDs for interrupt handler
#define INTID_TIMER1  0x1
#define INTID_TIMER2  0x2
#define INTID_UART0   0x3
#define INTID_GPU     0x4
#define INTID_TIMER3  0x5
#define INTID_PS2     0x6
#define INTID_UART1   0x7
#define INTID_UART2   0x8

#define SYSCALL_RETVAL_ADDR 0x200000

// System call IDs
#define SYS_HID_CHECKFIFO 1
#define SYS_HID_READFIFO 2
#define SYS_BDOS_PRINTC 3
#define SYS_BDOS_PRINT 4
#define SYS_FS_OPEN 5
#define SYS_FS_CLOSE 6
#define SYS_FS_READ 7
#define SYS_FS_WRITE 8
#define SYS_FS_SETCURSOR 9
#define SYS_FS_GETCURSOR 10
#define SYS_FS_DELETE 11
#define SYS_FS_MKDIR 12
#define SYS_FS_MKFILE 13
#define SYS_FS_STAT 14
#define SYS_FS_READDIR 15
#define SYS_FS_GETCWD 16
#define SYS_FS_SYNCFLASH 17
// Syscalls 18-19 are reserved for future use
#define SYS_SHELL_ARGC 20
#define SYS_SHELL_ARGV 21
#define SYS_USB_KB_BUF 99


/**
 * Returns the interrupt ID
*/
word get_int_id()
{
  word retval = 0;

  asm(
    "readintid r2     ;reads interrupt id to r2\n"
    "write -4 r14 r2  ;write to stack to return\n"
    );

  return retval;
}

/**
 * Executes system call to BDOS
 * Argument specifies the system call ID
 * Returns the address of the return value
*/
word* syscall(word ID)
{
  word* p = (word*) SYSCALL_RETVAL_ADDR;
  *p = ID;

  asm("push r1\n"
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
    "savpc r1\n"
    "push r1\n"
    "jump 4\n"
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
    "pop r1\n");

  return p;
}

/**
 * Exits the user program and returns to BDOS in a somewhat controlled way
*/
void exit()
{
  asm("ccache\n");
  asm("jump Return_BDOS\n");
}

/**
 * Returns 1 if the HID buffer is not empty
*/
word hid_checkfifo()
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  syscall(SYS_HID_CHECKFIFO);
  return p[0];
}

/**
 * Reads a character from the HID buffer
*/
word hid_fiforead()
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  syscall(SYS_HID_READFIFO);
  return p[0];
}

/**
 * Prints a character on the BDOS console
*/
void bdos_printc(char c)
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  p[1] = c;
  syscall(SYS_BDOS_PRINTC);
}

/**
 * Prints a string on the BDOS console
*/
void bdos_print(char* c)
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  p[1] = (char)c;
  syscall(SYS_BDOS_PRINT);
}

/**
 * Prints a string with newline on the BDOS console
*/
void bdos_println(char* str)
{
  bdos_print(str);
  bdos_printc('\n');
}

/**
 * Prints a decimal on the BDOS console
*/
void bdos_printdec(word i)
{
  char buffer[12];

  if (i < 0)
  {
    buffer[0] = '-';
    itoa(MATH_abs(i), &buffer[1]);
  }
  else
  {
    itoa(i, buffer);
  }
  bdos_print(buffer);
}

/**
 * Prints a decimal with newline on the BDOS console
*/
void bdos_printdecln(word i)
{
  bdos_printdec(i);
  bdos_printc('\n');
}

/**
 * Prints a hexadecimal on the BDOS console
*/
void bdos_printhex(word i)
{
  char buffer[11];
  itoah(i, buffer);
  bdos_print(buffer);
}

/**
 * Opens a file in the filesystem
*/
word fs_open(char* filename)
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  p[1] = (char) filename;
  syscall(SYS_FS_OPEN);
  return p[0];
}

/**
 * Closes a file in the filesystem
*/
word fs_close(word fp)
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  p[1] = fp;
  syscall(SYS_FS_CLOSE);
  return p[0];
}

/**
 * Reads from a file in the filesystem
*/
word fs_read(word fp, char* buffer, word len)
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  p[1] = fp;
  p[2] = (char) buffer;
  p[3] = len;
  syscall(SYS_FS_READ);
  return p[0];
}

/**
 * Writes to a file in the filesystem
*/
word fs_write(word fp, char* buffer, word len)
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  p[1] = fp;
  p[2] = (char) buffer;
  p[3] = len;
  syscall(SYS_FS_WRITE);
  return p[0];
}

/**
 * Sets the cursor position in the filesystem
*/
word fs_setcursor(word fp, word pos)
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  p[1] = fp;
  p[2] = pos;
  syscall(SYS_FS_SETCURSOR);
  return p[0];
}

/**
 * Gets the cursor position in the filesystem
*/
word fs_getcursor(word fp)
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  p[1] = fp;
  syscall(SYS_FS_GETCURSOR);
  return p[0];
}

/**
 * Deletes a file in the filesystem
*/
word fs_delete(char* filename)
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  p[1] = (char) filename;
  syscall(SYS_FS_DELETE);
  return p[0];
}

/**
 * Creates a directory in the filesystem
*/
word fs_mkdir(char* dirname)
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  p[1] = (char) dirname;
  syscall(SYS_FS_MKDIR);
  return p[0];
}

/**
 * Creates a file in the filesystem
*/
word fs_mkfile(char* filename)
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  p[1] = (char) filename;
  syscall(SYS_FS_MKFILE);
  return p[0];
}

/**
 * Gets the status of a file in the filesystem
*/
word fs_stat(char* filename)
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  p[1] = (char) filename;
  syscall(SYS_FS_STAT);
  return p[0];
}

/**
 * Lists the contents of a directory in the filesystem
 * Returns the number of entries in the directory
*/
word fs_readdir(char* dirname, char* buffer)
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  p[1] = (char) dirname;
  p[2] = (char) buffer;  
  syscall(SYS_FS_READDIR);
  return p[0];
}

/**
 * Gets the current working directory in the filesystem
 * Note: The pointer returned is only valid until the next syscall
*/
char* fs_getcwd()
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  syscall(SYS_FS_GETCWD);
  return p;
}

/**
 * Synchronizes the filesystem with the flash memory
*/
word fs_syncflash()
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  syscall(SYS_FS_SYNCFLASH);
  return p[0];
}

/**
 * Returns the number of command line arguments
*/
word shell_argc()
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  syscall(SYS_SHELL_ARGC);
  return p[0];
}

/**
 * Returns the command line arguments
 * Note: The pointer returned is only valid until the next syscall
*/
char* shell_argv()
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  syscall(SYS_SHELL_ARGV);
  return p;
}

/**
 * Returns 1 if key is being held on USB keyboard
*/
word bdos_usbkey_held(word c)
{
  char* p = (char*) SYSCALL_RETVAL_ADDR;
  syscall(SYS_USB_KB_BUF);
  word* usbKeyBuffer = (char*) p[0];
  
  word i;
  for (i = 0; i < 8; i++)
  {
    if (usbKeyBuffer[i] == c)
    {
      return 1;
    }
  }
  return 0;
}