# TODOs
Things I want to do or I already have done but did not update this page:

## Next steps:
- (hardest step) Add instruction memory and data memory cache, which both read from the same main memory (via a new MU with new bus protocol) on cache miss
    - Make use of SDRAM burst to reduce the open/close costs
- Implement new, better and more efficient interrupt system
- Extend MU with IO and other memory interfaces (VRAM, SPI flash, ROM, etc.)
- Add GPU (can mostly copy paste from FPGC5 since it is completely separate from the CPU and MU)
- Update assembler for new ISA
- Update UART/SPI-flash bootloader
- Update/redo the C compiler
- Update BDOS
- Update user BDOS programs

## Random notes for in documentation later:
- All constants are signed, except for the load instruction
- All jump constants/offsets/registers represent 8-bit addresses, which is easier for addr2reg in the assembler (works the same for labels and data)