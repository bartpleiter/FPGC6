# Welcome to the FPGC Wiki!

[![FPGC Logo](images/logo_big_alpha.png)](https://www.github.com/b4rt-dev/fpgc6)


!!! warning
    The documentation is regularly outdated

For more details, use the navigation menu.

## What is FPGC?
The FPGC (FPGA Computer) is my big hobby project. It is a computer where every part of it is designed and implemented by me, from the 1's and 0's in the ISA to the operating system and other software that runs on it, and from the PCB to the 3D printed enclosure.

## Version 6
The FPGC6 is an ambitious attempt to redesign the practically finished but very inefficient [FPGC5](https://github.com/b4rt-dev/FPGC5), with more advanced techniques and performance in mind. This means the FPGC6 will have a pipelined CPU (with hazard detection/forwarding, branch prediction, etc.), memory cache (for data and one for instructions at L1, one for both at L2), and a better architecture for running BCC (FPGC adapted MIPS C compiler) code.

This means that the CPU will have to be recreated from scratch, which allows me to do things more properly this time. If everything goes well then this should give a huge boost in performance. The rest of the system (MU, GPU) can be mostly copied from FPGC5. All software/compilers will have to be rewritten/updated.

A lot of inspiration for the CPU is taken from MIPS, because many online resources on more advanced CPU techniques like pipelining and caching use MIPS as example. As for implementation specific inspiration, I looked a lot at [mips-cpu by jmahler](https://github.com/jmahler/mips-cpu), since it is a good example of a simple pipelined CPU.

## Project Links
- [Github Repository](https://www.github.com/b4rt-dev/FPGC6)
- [Gogs Mirror](https://www.b4rt.nl/git/bart/FPGC6-mirror)
- [Documentation (this site)](https://www.b4rt.nl/fpgc)