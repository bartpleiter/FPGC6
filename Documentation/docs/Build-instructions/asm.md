# ASM build instructions

## Assembling a program

To assemble a program, run `python3 Assembler.py > {outfile.list}`. The code is always stored in `code.asm`. The assembler will produce a text file with 32 1's and 0's and some comments for each line. To convert this into a binary, run `compileROM.sh` from the `Programmer` directory.

The arguments `os` or `bdos {offset}` can be used when assembling BDOS and a bdos user program. More details can be found in the Assembler wiki page.

## Assembling a userBDOS program from FPGC

A user program can also be assembled from the FPGC itself using the `asm` userBDOS program found in `BCC/FPGCbuildTools/asm/`. Within BDOS, run `asm {code.asm} {out.bin}`. This assembler will directly assemble to an output binary that can be run from BDOS.

## Assembling a program for simulation in Verilog

See `buildToVerilog.sh` and `simulate.sh` for examples on what is needed to assemble a program into the simulated rom/flash of the FPGC. This is useful for testing very small programs or individual instructions in simulation without having to write binary manually.
