# FPGC6


## Notes:
- All constants are signed, except for the load instruction
- All jump constants/offsets/registers represent 8-bit addresses, which is easier for addr2reg in the assembler (works the same for labels and data)