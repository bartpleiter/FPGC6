# Welcome to the FPGC Wiki!

[![FPGC Logo](images/logo_big_alpha.png)](https://www.github.com/b4rt-dev/fpgc6)


!!! warning
    The documentation is regularly outdated

For more details, use the navigation menu.

## What is FPGC?
The FPGC (FPGA Computer) is my big hobby project. It is a computer where every part of it is designed and implemented by me, from the 1's and 0's in the ISA to the operating system and other software that runs on it, and from the PCB to the 3D printed enclosure.

## Version 6
The largest change in the FPGC6 is the complete redesign of the CPU from the [FPGC5](https://github.com/b4rt-dev/FPGC5) with more advanced techniques and performance in mind. The FPGC6 how has a pipelined CPU including hazard detection/forwarding, with a better architecture for running BCC (variant of C) code (including better signed number support).
Eventually it will also have memory cache (for data and one for instructions at L1, one for both at L2), which should drastically speed up performance because of the SDRAM bottleneck. The new CPU, called B32P, was recreated from scratch which allowed me to do things more properly this time. Aside from the CPU, all other parts of the system are almost identical to the FPGC5, and the CPU itself still has all old instructions except for COPY implemented. This way, after updating the Assembler, almost all code will still work saving a lot of time.

A lot of inspiration for the CPU is taken from MIPS, because many online resources on more advanced CPU techniques like pipelining and caching use MIPS as example. As for implementation specific inspiration, I looked a lot at [mips-cpu by jmahler](https://github.com/jmahler/mips-cpu), since it is a good example of a simple pipelined CPU.

## Current state
- CPU implementation done (except for caching). All major bugs fixed, tested in hardware
- Can run all programs from FPGC5 after recompilation with new assembler and slight change in interrupt handler code
- Ready to optimize BCC compiler to use new instructions
- Ready to design and add CPU cache

## Project Links
- [Github Repository](https://www.github.com/b4rt-dev/FPGC6)
- [Gogs Mirror](https://www.b4rt.nl/git/bart/FPGC6-mirror)
- [Documentation (this site)](https://www.b4rt.nl/fpgc)