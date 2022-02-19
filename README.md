# FPGC6

NOTE: for a more finished project, look at [FPGC5](https://github.com/b4rt-dev/FPGC5) for now.

Ambitious attempt to redesign the [FPGC5](https://github.com/b4rt-dev/FPGC5) but with a pipelined CPU (with hazard detection, branch prediction, etc.), memory cache (for data and one for instructions, both probably 1 or 2-way associative), byte addressable memory, and a better architecture for running BCC code.

This does mean that I will have to reimplement everything, but this is good because I now have a better idea of what I am doing and what is useful to have at the upper layers (like the C compiler). If everything goes well then this should give a huge boost in performance, and more importantly: an even better understanding in how computers work at the lower levels.

A lot of inspiration is taken from MIPS, because many online info use MIPS as example (especially on pipelining). As for implementation specific inspiration, I looked a lot at [mips-cpu by jmahler](https://github.com/jmahler/mips-cpu).

## Current status:
- Working on the new CPU design in Verilog (simulator)

## Next steps:
- Test CPU in hardware (Quartus)
- (hardest step) Add instruction memory and data memory cache, which both read from the same main memory (via a new MU with new bus protocol) on cache miss
	- Make use of SDRAM burst to reduce the open/close costs
- Extend MU with IO and other memory interfaces (VRAM, SPI flash, ROM, etc.)
- Add GPU (can mostly copy paste from FPGC5 since it is completely separate from the CPU and MU)
- Update assembler for new ISA
- Update UART/SPI-flash bootloader
- Update/redo the C compiler
- Update BDOS
- Update user BDOS programs
- Profit?

## Random notes for in documentation later:
- All constants are signed, except for the load instruction
- All jump constants/offsets/registers represent 8-bit addresses, which is easier for addr2reg in the assembler (works the same for labels and data)