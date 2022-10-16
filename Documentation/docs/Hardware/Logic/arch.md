# Architecture
The FPGC consists of three main parts: the CPU, GPU and MU.

The CPU, called the B32P, executes the instructions. It reads from and writes to the MU.
 
The GPU, called the FSX2, is completely separate from the CPU. It contains the logic for generating a video signal and for creating an image on this signal based on the contents of VRAM. The GPU has its own read port with clock on the dual port VRAM (SRAM/FPGA block RAM), allowing it to run on a completely different clock domain than the rest of the FPGC.

The MU, or memory unit, handles all memory access between the CPU and all the different memories and I/O devices used in the FPGC. The most important memories here are SDRAM, ROM, VRAM and SPI flash. The goal of the MU is to have the CPU access all memories without the CPU having to care about the type or timing of the memory, making an easy memory interface for the CPU. This is achieved a memory map and a bus protocol with a busy/wait signal. However, this currently does cost one cycle of overhead per operation on the MU in most cases.

Block diagram of FPGC:

``` text
                  +---------------------+
                  |                     |
                  |        B32P         |
                  |         CPU         |
                  |                     |
                  |                     |
                  +---------------------+
                             ^
                             |
                             v
+---------+       +---------------------+       +---------+       +---------+
|         |       |                     |       |         |       |         |
|  SDRAM  |<----->|                     |       |         |       |         |
|         |       |                     |       |         |       |         |
+---------+       |                     |       |         |       |  FSX2   |
                  |       Memory        |<----->|  VRAM   |<----->|   GPU   |
+---------+       |        Unit         |       |         |       |         |
|         |       |                     |       |         |       |         |
|   ROM   |<----->|                     |       |         |       |         |
|         |       |                     |       |         |       |         |
+---------+       +---------------------+       +---------+       +---------+
                        ^          ^
                        |          |
                        v          v
                    +-------+  +-------+
                    |       |  |       |
                    |  SPI  |  |  I/O  |
                    | flash |  |       |
                    |       |  +-------+
                    +-------+ 
```