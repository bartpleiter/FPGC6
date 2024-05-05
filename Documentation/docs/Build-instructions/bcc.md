# BCC build instructions

This page shows how to build C written software for the FPGC using BCC

## Compiling BCC using GCC

To compile the compiler, run `make` in the `BCC/` directory. This will build the `bcc` binary which can then be used to compile C code into ASM code.

## Compile bare metal C program

To compile a C program to run directly on the FPGC, run `bcc {code.c} {file.asm}`. To also assemble and program the FPGC, a convenience script `compileBareMetal.sh` can be used.

## Compile BDOS

To compile the operating system BDOS, run `bcc --os {BDOS.c} {file.asm}`. To also assemble and program the FPGC, a convenience script `compileBDOS.sh` can be used.

## Compile userBDOS program

To compile a program to be run as a program within BDOS, run `bcc --bdos {program.c} {file.asm}`. To also assemble, send/upload and run the program on the FPGC, two convenience scripts can be used: `sendToBDOS.sh` to send the program over the network and run it directly from memory, and `uploadToBDOS.sh [filename to store]` to upload the program over the network to store it in the filesystem.

## Compile userBDOS program from FPGC

A user program can also be compiled from the FPGC itself using the `bcc` userBDOS program found in `BCC/FPGCbuildTools/bcc/`. Within BDOS, run `bcc {code.c} {file.asm}`. As of writing there is no scripting support, so no convenience script to also assemble the program exists.