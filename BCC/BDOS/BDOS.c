/* Bart's Drive Operating System(BDOS)
 * A relatively simple OS that allows for some basic features including:
 * - Running a single user program with full hardware control
 * - Some system calls
 * - A basic shell
 * - Network loaders for sending data and controlling the FPGC
 * - HID fifo including USB keyboard driver
 * - BRFS Filesystem
 */

/* List of reserved stuff:
- Timer2 is used for USB keyboard polling, even when a user program is running
- Socket 7 is used for netHID
*/

/*
 * Defines (also might be used by included libraries below)
 */

// As of writing, BCC assigns 4 memory addresses to ints, so we should use chars instead
// However, this is confusing, so we typedef it to word, since that is what it basically is
#define word char

#define SYSCALL_RETVAL_ADDR 0x200000  // Address for system call communication with user program
#define TEMP_ADDR 0x220000            // Address for (potentially) large temporary outputs/buffers
#define RUN_ADDR 0x400000             // Address of loaded user program

#define NETWORK_LOCAL_IP 213          // local IP address (last byte)

#define MAX_PATH_LENGTH 127           // Max length of a file path
#define BDOS_DEFAULT_BLOCKS 1024      // Default number of blocks for the BRFS filesystem
#define BDOS_DEFAULT_BLOCK_SIZE 128   // Default number of words per block for the BRFS filesystem

// Interrupt IDs for interrupt handler
#define INTID_TIMER1 0x1
#define INTID_TIMER2 0x2
#define INTID_UART0 0x3
#define INTID_GPU 0x4
#define INTID_TIMER3 0x5
#define INTID_PS2 0x6
#define INTID_UART1 0x7
#define INTID_UART2 0x8

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
// Syscalls 17-19 are reserved for future use
#define SYS_SHELL_ARGC 20
#define SYS_SHELL_ARGV 21
#define SYS_USB_KB_BUF 99

/*
 * Global vars (also might be used by included libraries below)
 */

// Flag that indicates whether a user program is running
word bdos_userprogram_running = 0;

/*
 * Included libraries
 */

// Data includes
#include "data/ASCII_BW.c"
#include "data/PS2SCANCODES.c"
#include "data/USBSCANCODES.c"

// Code includes
#include "lib/stdlib.c"
#include "lib/math.c"
#include "lib/gfx.c"
#include "lib/hidfifo.c"
#include "lib/ps2.c"
#include "lib/brfs.c"
#include "lib/shell.c"
#include "lib/usbkeyboard.c"
#include "lib/wiz5500.c"
#include "lib/netloader.c"
#include "lib/nethid.c"
#include "lib/spiflash.c"


/*
 * Functions
 */

/**
 * Initialize the BRFS filesystem
 * Returns 1 if successful, 0 if not
 */
word bdos_init_brfs()
{
  // Initialize the SPI flash
  spiflash_init();

  // Try to read the filesystem from the SPI flash
  GFX_PrintConsole("Reading BRFS from SPI flash...\n");
  if (brfs_read_from_flash())
  {
    GFX_PrintConsole("BRFS read from flash\n");
  }
  else
  {
    GFX_PrintConsole("Could not read BRFS from flash!\n");
    return 0;
  }

  return 1;
}

// Clears all VRAM
//  and copies the default ASCII pattern table and palette table
// also resets the cursor
void bdos_init_vram()
{
  GFX_initVram();
  GFX_copyPaletteTable((word)DATA_PALETTE_DEFAULT);
  GFX_copyPatternTable((word)DATA_ASCII_DEFAULT);
  GFX_cursor = 0;
}

// Initialize the W5500
void bdos_init_network()
{
  word ip_addr[4] = {192, 168, 0, NETWORK_LOCAL_IP};

  word gateway_addr[4] = {192, 168, 0, 1};

  word mac_addr[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0x24, 0x64};

  word sub_mask[4] = {255, 255, 255, 0};

  wiz_Init(ip_addr, gateway_addr, mac_addr, sub_mask);
}


// Restores certain things when returning from a user program
void bdos_restore()
{
  // restore graphics (trying to keep text in window plane)
  GFX_copyPaletteTable((word)DATA_PALETTE_DEFAULT);
  GFX_copyPatternTable((word)DATA_ASCII_DEFAULT);
  GFX_clearBGtileTable();
  GFX_clearBGpaletteTable();
  GFX_clearWindowpaletteTable();
  GFX_clearSprites();

  // restore netloader
  NETLOADER_init(NETLOADER_SOCKET);
}

// Main BDOS code
int main()
{
  uprintln("BDOS_INIT");

  bdos_userprogram_running = 0; // Indicate that no user program is running

  bdos_init_vram();
  GFX_PrintConsole("VRAM initialized\n"); // Print to console now VRAM is initialized
  GFX_PrintConsole("Starting BDOS\n");

  GFX_PrintConsole("Initalizing network\n");
  bdos_init_network();

  GFX_PrintConsole("Initalizing netloader\n");
  NETLOADER_init(NETLOADER_SOCKET);

  GFX_PrintConsole("Initalizing netHID\n");
  NETHID_init(NETHID_SOCKET);

  GFX_PrintConsole("Initalizing USB keyboard\n");
  USBkeyboard_init();

  GFX_PrintConsole("Initalizing filesystem\n");
  if (!bdos_init_brfs())
  {
    GFX_PrintConsole("Error initializing FS\n");
    return 0;
  }

  GFX_PrintConsole("Initalizing shell\n");
  shell_init();

  // Main loop
  while (1)
  {
    shell_loop();                     // Update shell state
    NETLOADER_loop(NETLOADER_SOCKET); // Update netloader state
  }

  return 1;
}

/**
 * System calls
*/

// System call handler
// Returns at the same address it reads the command from
void syscall()
{
  word *syscall_data = (word *)SYSCALL_RETVAL_ADDR;
  word id = syscall_data[0];

  // Special case for mkdir and mkfile
  if (id == SYS_FS_MKDIR)
  {
    char dirname_output[MAX_PATH_LENGTH];
    char* file_path_basename = basename((char *)syscall_data[1]);
    char* file_path_dirname = dirname(dirname_output, (char *)syscall_data[1]);
    syscall_data[0] = brfs_create_directory(file_path_dirname, file_path_basename);
  }
  else if (id == SYS_FS_MKFILE)
  {
    char dirname_output[MAX_PATH_LENGTH];
    char* file_path_basename = basename((char *)syscall_data[1]);
    char* file_path_dirname = dirname(dirname_output, (char *)syscall_data[1]);
    syscall_data[0] = brfs_create_file(file_path_dirname, file_path_basename);
  }

  switch (id)
  {
  case SYS_HID_CHECKFIFO:
    syscall_data[0] = HID_FifoAvailable();
    break;
  case SYS_HID_READFIFO:
    syscall_data[0] = HID_FifoRead();
    break;
  case SYS_BDOS_PRINTC:
    GFX_PrintcConsole(syscall_data[1]);
    syscall_data[0] = 0;
    break;
  case SYS_BDOS_PRINT:
    GFX_PrintConsole(syscall_data[1]);
    syscall_data[0] = 0;
    break;
  case SYS_FS_OPEN:
    syscall_data[0] = brfs_open_file((char *)syscall_data[1]);
    break;
  case SYS_FS_CLOSE:
    syscall_data[0] = brfs_close_file(syscall_data[1]);
    break;
  case SYS_FS_READ:
    syscall_data[0] = brfs_read(syscall_data[1], (char *)syscall_data[2], syscall_data[3]);
    break;
  case SYS_FS_WRITE:
    syscall_data[0] = brfs_write(syscall_data[1], (char *)syscall_data[2], syscall_data[3]);
    break;
  case SYS_FS_SETCURSOR:
    syscall_data[0] = brfs_set_cursor(syscall_data[1], syscall_data[2]);
    break;
  case SYS_FS_GETCURSOR:
    syscall_data[0] = brfs_get_cursor(syscall_data[1]);
    break;
  case SYS_FS_DELETE:
    syscall_data[0] = brfs_delete_file((char *)syscall_data[1]);
    break;
  case SYS_FS_STAT:
    syscall_data[0] = brfs_stat((char *)syscall_data[1]);
    break;
  case SYS_FS_READDIR:
    brfs_list_directory((char *)syscall_data[1]);
    syscall_data[0] = 0; // TODO: implement
    break;
  case SYS_FS_GETCWD:
    strcpy(syscall_data, shell_path);
    break;
  case SYS_FS_SYNCFLASH:
    brfs_write_to_flash();
    break;
  case SYS_SHELL_ARGC:
    syscall_data[0] = shell_num_tokens;
    break;
  case SYS_SHELL_ARGV:
    memcpy(syscall_data, shell_tokens, sizeof(shell_tokens));
    break;
  case SYS_USB_KB_BUF:
    syscall_data[0] = USBkeyboard_buffer_parsed;
    break;
  }
}

// Interrupt handler
void interrupt()
{
  // Handle BDOS interrupts
  int i = getIntID();
  switch (i)
  {
  case INTID_TIMER1:
    timer1Value = 1; // Notify ending of timer1
    break;

  case INTID_TIMER2:
    USBkeyboard_HandleInterrupt(); // Handle USB keyboard interrupt
    break;

  case INTID_UART0:
    break;

  case INTID_GPU:
    if (NETHID_isInitialized == 1)
    {
      // Check using CS if we are not interrupting any critical access to the W5500
      word *spi3ChipSelect = (word *)0xC02732; // TODO: use a define for this address
      if (*spi3ChipSelect == 1)
      {
        NETHID_loop(NETHID_SOCKET); // Look for an input sent to netHID
      }
    }
    break;

  case INTID_TIMER3:
    break;

  case INTID_PS2:
    PS2_HandleInterrupt(); // Handle PS2 interrupt
    break;

  case INTID_UART1:
    break;

  case INTID_UART2:
    break;
  }

  // Handle user program interrupts
  if (bdos_userprogram_running)
  {
    // Call interrupt() of user program
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

        "savpc r1\n"
        "push r1\n"
        "jump 0x400001\n"

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
        "pop r1\n");
    return;
  }
  else
  {
    // Code to only run when not running a user program
  }
}
