# Register bank
The register bank contains 16 registers which are all 32 bits wide. Aside from `R0` which is always zero, all registers are basically GP register. However, to maintain some kind of coding consistency, some registers have a special function assigned (though their hardware implementation are the same).

The 16 32 bit registers have the current functions:
``` text
Register|Hardware   |Assembly   |C
-----------------------------------------------------
R0      |Always 0   |Always 0   |Always 0
R1      |GP         |Arg|retval |Very local temp reg
R2      |GP         |Arg|retval |Ret0
R3      |GP         |Arg|retval |Ret1
R4      |GP         |GP         |Arg0
R5      |GP         |GP         |Arg1
R6      |GP         |GP         |Arg2
R7      |GP         |GP         |Arg3
R8      |GP         |GP         |GP0
R9      |GP         |GP         |GP1
R10     |GP         |GP         |GP2
R11     |GP         |GP         |Temp
R12     |GP         |GP         |Temp
R13     |GP         |GP         |SP
R14     |GP         |GP         |BP
R15     |GP         |Ret Ptr    |Ret addr
```
The register bank has two read ports and one write port. Internally on the FPGA, the registers are implemented in block RAM to increase performance and save space. Note that the program counter is not part of the register bank, as it is a separate part of the CPU, which can be obtained using the SAVPC instruction.
