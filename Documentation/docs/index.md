# Welcome

[![FPGC Logo](images/logo_big_alpha.png)](https://www.github.com/b4rt-dev/fpgc6)

!!! warning
    This Wiki is regularly outdated

Use the navigation menu to find the other Wiki pages.

## What is FPGC?
FPGC (FPGA Computer) is my big hobby project. It is a computer where practically every part of it is designed and implemented by me, from the 1's and 0's in the ISA to the operating system, and from the PCB to the 3D printed enclosure.

## Version 6
The largest change in the FPGC6 is the complete redesign of the CPU from the [FPGC5](https://github.com/b4rt-dev/FPGC5) with more advanced techniques and performance in mind. The FPGC6 how has a pipelined CPU including hazard detection/forwarding, with a better architecture for running C code including better signed integer support.
Eventually it will also have memory cache (L1 data and L1 instruction cache), which should drastically speed up performance because of the SDRAM bottleneck. The new CPU, called B32P, was recreated from scratch. Aside from the CPU, all other parts of the system are almost identical to the FPGC5, and the CPU itself still has all old instructions except for COPY implemented. This way, after updating the Assembler, almost all code will still work.

A lot of inspiration for the CPU is taken from MIPS, because many online resources on more advanced CPU techniques like pipelining and caching use MIPS as example. As for implementation specific inspiration, I looked a lot at [mips-cpu by jmahler](https://github.com/jmahler/mips-cpu), since it is a good example of a simple pipelined CPU.

## Current state
- Ready to design and add L1 cache
- All code but the BCC Assembler work with the new CPU
- Waiting for upgraded FPGA board to arrive (double SDRAM bandwidth and capacity, huge FPGA upgrade with almost 10x more block RAM!)
- Need to add some folders from FPGC5 back to FPGC6

## Project Links
FPGC6:

- [Github Repository](https://www.github.com/b4rt-dev/FPGC6)
- [Gogs Mirror](https://www.b4rt.nl/git/bart/FPGC6-mirror)
- [Documentation](https://www.b4rt.nl/fpgc)

FPGC5:

- [Github Repository](https://github.com/b4rt-dev/FPGC5)
- [Gogs Mirror](https://www.b4rt.nl/git/bart/FPGC5-mirror)
- [Documentation](https://www.b4rt.nl/fpgc5)