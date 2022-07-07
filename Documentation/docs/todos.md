# TODOs
Things I want to do or I already have done but did not update this page:

## Next steps:
- Update documentation
    - Parts that are worth documentation:
        - CPU
        - GPU
        - MU
        - NTSC encoder
        - HDMI encoder
        - SDRAM controller
        - SPI flash reader
        - Assembler
        - BCC
        - BDOS (MemoryMap, shell, usb&PS2 keyboard, HID&NetHID, netloader, GFX, syscalls)
        - userBDOS
        - Compiling code on BCC (ASM, BDOS, EDIT)
        - BCC programs (EDIT, WEBSERV, etc.)
        - All programmers (UART, SPI flasher, BDOS send/upload)
        - BDOS sync files
        - Running BCC tests
- Update BCC assembler for new ISA
- Improve the C compiler with new instructions
- Add instruction memory and data memory cache
- Implement true GPIO
- Implement I2S Audio
- Add L2 cache (within arbiter or MU)
    - Make use of SDRAM burst to reduce the open/close costs