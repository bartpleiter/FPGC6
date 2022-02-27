# Welcome to the FPGC Wiki!

[![FPGC Logo](images/logo_big_alpha.png)](https://www.github.com/b4rt-dev/fpgc6)


!!! danger
    I do not update this documentation site much

For more details, use the navigation menu.

## What is FPGC?
The FPGC (FPGA Computer) is my big hobby project. It is a computer where every part of it is designed and implemented by me, from the 1's and 0's in the ISA to the operating system and other software that runs on it, and from the PCB to the 3D printed enclosure.

## Version 6?
The FPGC6 is an ambitious attempt to redesign the practically finished but very inefficient [FPGC5](https://github.com/b4rt-dev/FPGC5), with more advanced techniques and performance in mind. This means the FPGC6 will have a pipelined CPU (with hazard detection/forwarding, branch prediction, etc.), memory cache (for data and one for instructions), byte addressable memory (FPGC5 was still word addressable!), and a better architecture for running BCC (FPGC adapted C compiler) code.

This does mean that I will have to reimplement everything, but this is good because I now have a better idea of what I am doing and what is useful to have at the upper layers (for example certain instructions that will be useful for the C compiler). If everything goes well then this should give a huge boost in performance, and more importantly: an even better understanding in how computers work.

A lot of inspiration is taken from MIPS, because many online resources on more advanced CPU techniques like pipelining and caching use MIPS as example. As for implementation specific inspiration, I looked a lot at [mips-cpu by jmahler](https://github.com/jmahler/mips-cpu), since it is a good example of a simple pipelined CPU.

## Project Links
- [Github Repository](https://www.github.com/b4rt-dev/FPGC6)
- [Gogs Mirror](https://www.b4rt.nl/git/bart/FPGC6-mirror)
- [Documentation (this site)](https://www.b4rt.nl/fpgc)