# Syscalls

Syscalls, short for system calls, are a fundamental concept in operating systems. They provide a way for user-level (userBDOS) programs to interact with the underlying operating system (BDOS). Syscalls act as an interface between the user space and the OS space, allowing programs to request services from BDOS. Examples of syscalls include filesystem operations or writing to console.

Syscalls are essential for performing low-level operations and accessing system resources that are not directly/easily accessible from user-level programs. They provide a standardized and controlled way for programs to interact with the operating system.

## Implementation

Syscalls are implemented by using some hardcoded reserved memory in the BDOS memory map. The program starts by writing the ID to this address. Then, the program adds optional arguments to each following word/address (so one word per arg: might be good to use a pointer if it does not fit in one word). Afterwards, the program backs up its registers to the hw stack, pushes the program counter to the hw stack and jumps to address 4.

By jumping to address 4, the CPU will then jump to the `syscall()` function of BDOS, as this jump statement is added during compilation of BDOS by the assembler. Then, BDOS will handle the syscall by reading the reserved memory to obtain the ID and arguments. The output is then written to the same address. Note, during this, a different stack is used to not interfere with the BDOS or program's stack.

Afterwards, BDOS will return from the syscall() function, pop the program counter saved by the user program, and jumpt to this program counter with an offset to land right after the jump to address 4. Afterwards, the registers are restored and the userprogram continues normally. Then, it can read the return values of the syscall by reading from the reserved memory address.

## Syscall table


| ID  | Name           | arg1       | arg2      | arg3       | Description                                      | Implemented |
| --- | -------------- | ---------- | --------- | ---------- | ------------------------------------------------ | ----------- |
| 1   | hid_checkfifo  | -          | -         | -          | Check if there is data in the HID FIFO           | &#9745;     |
| 2   | hid_readfifo   | -          | -         | -          | Read next input from the HID FIFO                | &#9745;     |
| 3   | bdos_printc    | char c     | -         | -          | Print a character to the console                 | &#9745;     |
| 4   | bdos_print     | char* c    | -         | -          | Print a null-terminated string to the console    | &#9745;     |
| 5   | fs_open        | char* path | -         | -          | Open a file                                      | &#9745;     |
| 6   | fs_close       | word fp    | -         | -          | Close a file                                     | &#9745;     |
| 7   | fs_read        | word fp    | word len  | word* data | Read data from a file                            | &#9745;     |
| 8   | fs_write       | word fp    | word len  | word* data | Write data to a file                             | &#9745;     |
| 9   | fs_setcursor   | word fp    | word pos  | -          | Set the cursor position in a file                | &#9745;     |
| 10  | fs_getcursor   | word fp    | -         | -          | Get the cursor position in a file                | &#9745;     |
| 11  | fs_delete      | char* path | -         | -          | Delete a file or directory                       | &#9745;     |
| 12  | fs_mkdir       | char* path | -         | -          | Create a directory                               | &#9745;     |
| 13  | fs_mkfile      | char* path | -         | -          | Create a file                                    | &#9745;     |
| 14  | fs_stat        | char* path | -         | -          | Get file or directory information                | &#9745;     |
| 15  | fs_readdir     | char* path | -         | -          | Read a directory (TODO: think what to return)    | &#9744;     |
| 16  | fs_getcwd      | -          | -         | -          | Get the current working directory                | &#9745;     |
| 17  | fs_syncflash   | -          | -         | -          | Sync filesystem changes to SPI Flash             | &#9745;     |
|18-19| *reserved*     | -          | -         | -          | Reserved for future use                          |             |
| 20  | shell_argc     | -          | -         | -          | Get the number of command-line arguments         | &#9745;     |
| 21  | shell_argv     | -          | -         | -          | Get the command-line arguments                   | &#9745;     |
