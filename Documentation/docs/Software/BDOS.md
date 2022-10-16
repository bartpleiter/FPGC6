# BDOS

!!! warning
    Because I am updating BDOS a lot, this page can become quickly outdated.

BDOS (or Bart's Drive Operating System) is a single-tasking operating system for the FPGC written in C and assembly. Its goal is to provide a user interface (shell) to load programs from storage an to do other basic drive operations. These programs still have full access to all hardware as if it is running on bare metal. The only difference is that the program is ran from `0x400000` instead of `0x000000` and that interrupts first go through BDOS before reaching the program. Therefore, some compiler parameters will have to be passed for running programs on BDOS. Furthermore, BDOS user programs have access to the OS via system calls. These system calls can be used, for example, to get HID inputs from the HID FIFO, or to print characters to the shell. While technically not required, they do make the programs a lot simpler.

While this is definitely not a complex OS like Linux or Windows, it does add a lot of functionality to the FPGC. Mainly the shell and the ability to load programs from USB and Network are a huge step-up over the MCU-like programming experience where you can only run one program until it is reprogrammed. By adding BCC, ASM (assembler) and a file editor as userBDOS programs, and loading BDOS from the SPI flash, the system can be fully used as a personal computer without the need for an external programmer!

!!! danger
    Timer2 is used for USB keyboard polling, and Socket 7 is used for netHID, so don't use these in user code!

## Functionality
Currently, with BDOS you can:
- Do operations on the file system on a USB drive using the shell
- Run programs from the file system
- Run programs sent over the network
- Download and store files/programs sent over the network
- Use a PS/2 keyboard and/or USB keyboard to interface with shell, or use the network via the netHID interface
- Use system calls in your user programs
- Return to BDOS after a user program terminates (sucessfully)
- Using some user programs, edit code, compile, assemble and run userBDOS programs
- Use up and down arrows to browse through command history
- Execute programs from /BIN directly from all locations


## Memory Map
BDOS has its own layout of the 32MiB SDRAM, which you can see here. There currently is no need for such big chunks of memory, but I have no other purpose for it for now.

Note: addresses are 32 bit, so 0x100000 is 4MiB

``` text
SDRAM
$000000 +------------------------+
        |                        |
        |                        |
        |         (8MiB)         |
        |   BDOS Program Code    |
        |                        |
        |                        |
        |                        | $1FFFFF
$200000 +------------------------+
        |        (256KiB)        |
        | Syscall Arg + Retval   | $20FFFF
$210000 +------------------------+
        |       (7.75MiB)        |
        |   TMP Output Buffer    |
        |           &            |
        | Syscall Stack (at end) | $3FFFFF
$400000 +------------------------+
        |         (13MiB)        |
        |   User Program Memory  |
        |           &            |
        |User Main Stack (at end)|
        |                        | $73FFFF
$740000 +------------------------+
        |         (1MiB)         |
        |BDOS Main Stack (at end)|
        |                        | $77FFFF
$780000 +------------------------+
        |         (1MiB)         |
        | User Int Stack (at end)|
        |                        | $7BFFFF
$7C0000 +------------------------+
        |         (1MiB)         |
        | BDOS Int Stack (at end)|
        |                        | $7FFFFF
        +------------------------+

```


## BDOS OS Libraries
BDOS has it own set of libraries and data, which you can see in the `Ccompiler/BDOS/` folder. These are not accessable to user programs. Those have their own set of libraries.

### gfx.h
The graphics library provides, on top of the basic gfx functions, functionality to write characters to screen as if it was a console. This means that the screen scrolls vertically when the last line is written. A cursor is also kept in memory. Newlines and backspaces are also supported.

### ps2.h
The PS2 keyboard library provides an interrupt handler for extracting keypresses from scancodes. It can handle shifted keys and extended keys. See ps2.h for more details on what is supported and what is not.

### usbkeyboard.h
A library for reading keypresses from an USB keyboard. Works with most, but not all keyboards. The top USB port is used for this. The keyboard is polled using a timer (timer2, so don't use it in user code).

### netHID.h
A very simple network program that reads for HID codes and forwards them to the HID fifo (after some parsing in case of an escape character)

### hidfifo.h
A FIFO for storing keypresses from PS/2, USB keyboard and NEThid. The shell can read these key presses without needing to know from which device.

### stdlib.h
Contains basic functions, including timer and memory functions

### math.h
Provides very very basic math operations.

### fs.h
Provides functionality for operating the filesystem on a flash drive in the bottom usb port.

### wiz5500.h
Provides functionality for operating the Wiznet W5500 chip. Supports eight sockets.

### netloader.h
A network based bootloader using the wiz5500.h library.

### shell.h
Provides the implementation of the shell for operating the system.


## BDOS user program libraries
User programs have their own set of libraries and data, which you can see in the `Ccompiler/userBDOS/` folder. While mostly similar to the BDOS libraries, some libraries like HID related drivers and the shell are removed or different, since those are only useful for the OS or should be called using system calls. Most importantly, there is a library `SYS.H` that allows for system calls to BDOS. All files are stored in 8.3 DOS format, so it can be synced with the FPGC itself.
