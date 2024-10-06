# Welcome

[![FPGC Logo](images/logo_big_alpha.png)](https://www.github.com/bartpleiter/fpgc6)

!!! warning
    This Wiki is regularly outdated

Use the navigation menu to find the other Wiki pages.

## What is FPGC?

FPGC (FPGA Computer) is my big hobby project. It is a computer where almost every part of it is designed and implemented by me, from the 1's and 0's in the ISA to the Operating System, and from the PCB to the 3D printed enclosure.

TODO: add picture of FPGC running edit on edit.c

## New in version 6 and goals

With FPGC6, the focus is shifting more towards performance improvements. The main new feature of the FPGC6 is a complete redesign of the CPU. Compared to [FPGC5](https://github.com/bartpleiter/FPGC5), the CPU is now 5 stage pipelined including hazard detection/forwarding, has a better architecture for running C code from BCC, has better signed integer and now also fixed point support. For the pipelining, a lot of inspiration is taken from the classic 5 stage MIPS pipeline, because of its simplicity and the availability of educational resources. As for implementation specific inspiration, I looked a lot at [mips-cpu by jmahler](https://github.com/jmahler/mips-cpu).

Version 6 of the FPGC now also contains a better SDRAM controller and cache, which greatly reduces the SDRAM bottleneck. Still, the SDRAM remains a huge bottleneck as the pipeline can never achieve its full potential because of constant instruction and data memory stalls. This brings us to the following goals of the FPGC6:

- [ ] Average amount of cycles per instruction < 2, which requires a redesign of the bus protocol, direct connections to the SDRAM controller, and of course caching
- [ ] Enough computation speed to run a full resolution (320x240) raycaster at 60 FPS, which requires fast memory copy from SDRAM to VRAM
- [ ] Improved Verilog testing setup, as designs and testing is becoming complex
- [ ] Insights of RAM usage gained (e.g. for all software stacks)
- [x] Use own designed file system, instead of relying on the CH376 chip
- [x] Write C code, compile and assemble on device without needing a different computer
- [x] Bitmap (individual pixel) rendering in GPU


## Project Links

FPGC6:

- [Github Repository](https://www.github.com/bartpleiter/FPGC6)
- [Gogs Mirror](https://www.b4rt.nl/git/bart/FPGC6-mirror)
- [Documentation](https://www.b4rt.nl/fpgc)

FPGC5:

- [Github Repository](https://github.com/bartpleiter/FPGC5)
- [Gogs Mirror](https://www.b4rt.nl/git/bart/FPGC5-mirror)
- [Documentation](https://www.b4rt.nl/fpgc5)

## Documentation checklist

- [x] Index page
- [x] Specs
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
- [x] BRFS
- [ ] Running BCC tests