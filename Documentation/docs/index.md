# Welcome

[![FPGC Logo](images/logo_big_alpha.png)](https://www.github.com/b4rt-dev/fpgc6)

!!! warning
    This Wiki is regularly outdated

Use the navigation menu to find the other Wiki pages.

## What is FPGC?

FPGC (FPGA Computer) is my big hobby project. It is a computer where almost every part of it is designed and implemented by me, from the 1's and 0's in the ISA to the Operating System, and from the PCB to the 3D printed enclosure.

TODO: add picture of FPGC running edit on edit.c

## New in version 6

The largest change in the FPGC6 is the complete redesign of the CPU from the [FPGC5](https://github.com/b4rt-dev/FPGC5) with more advanced techniques and performance in mind. The new CPU (built from scratch again) is now a pipelined CPU including hazard detection/forwarding with a better architecture for running C code including better signed integer support. It also allows for L1 cache (although currently not implemented). A lot of inspiration for the CPU is taken from MIPS, because many online resources on more advanced CPU techniques like pipelining and caching use MIPS as example. As for implementation specific inspiration, I looked a lot at [mips-cpu by jmahler](https://github.com/jmahler/mips-cpu), since it is a good example of a simple pipelined CPU.

Version 6 of the FPGC now also contains a better SDRAM controller and cache, which greatly reduces the SDRAM bottleneck.
Aside from this and the CPU, all other parts of the system are mostly identical to the FPGC5, and the CPU itself still has all old instructions implemented (although using different opcodes and argument placement) except for COPY. Therefore, after updating the Assembler, almost all code will still work meaning I do not have to rewrite most of my code base.

## Current state

- All existing FPGC5 code but the BCC Assembler works with the new CPU
- FPGA module upgraded to Cyclone V (10x more block RAM!) with double the SDRAM bandwith and capacity
- Added bitmap GPU layer that allows for accessing individual pixels
- L2 cache working, no need for L1I cache with the current arbiter implementation
- Ready to design and add L1D cache

## Project Links

FPGC6:

- [Github Repository](https://www.github.com/b4rt-dev/FPGC6)
- [Gogs Mirror](https://www.b4rt.nl/git/bart/FPGC6-mirror)
- [Documentation](https://www.b4rt.nl/fpgc)

FPGC5:

- [Github Repository](https://github.com/b4rt-dev/FPGC5)
- [Gogs Mirror](https://www.b4rt.nl/git/bart/FPGC5-mirror)
- [Documentation](https://www.b4rt.nl/fpgc5)

## Next steps

- Create 3D raycaster using new bitmap GPU layer
- Update BCC assembler for new ISA
- Add data memory cache (at 100MHz)
- Implement true GPIO
- Implement I2S Audio

## Documentation checklist

- [x] Index page
- [ ] CPU
- [ ] GPU
- [ ] MU
- [ ] PCB
- [ ] 3D printed enclosure
- [ ] NTSC encoder
- [ ] HDMI encoder
- [ ] SDRAM controller
- [ ] SPI flash reader
- [ ] Assembler
- [ ] BCC
- [ ] BDOS (MemoryMap, shell, usb&PS2 keyboard, HID&NetHID, netloader, GFX, syscalls)
- [ ] userBDOS
- [ ] Compiling code on BCC (ASM, BDOS, EDIT)
- [ ] BCC programs (EDIT, WEBSERV, etc.)
- [ ] All programmers (UART, SPI flasher, BDOS send/upload)
- [ ] BDOS sync files
- [ ] Running BCC tests