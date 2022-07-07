#!/bin/bash

# script for compiling a C program for BDOS, and send it to the FPGC over the network

if [ "$1" == "" ]
then
    echo "No program to compile given"
    exit 1
fi

echo "Processing: $1"
# compile and send
echo "Compiling C code to B32P ASM"
if (./bcc --bdos $1 ../Assembler/code.asm) # compile c code with BDOS flag and write compiled code to code.asm in Assembler folder
then
    echo "C code successfully compiled"

    echo "Assembling B32P ASM code"
    if (cd ../Assembler && python3 Assembler.py bdos 0x400000 -O > ../Programmer/code.list) # assemble with offset for BDOS and write to code.list in Programmer folder
    then
            echo "B32P ASM code successfully assembled"
            # convert list to binary files and send to FPGC

            (cd ../Programmer && bash compileROM.sh noPadding && echo "Sending binary to FPGC over Network" && python3 netFlash.py code.bin)
    
    else # assemble failed, run again to show error
        echo "Failed to assemble B32P ASM code"
        cd ../Assembler && python3 Assembler.py bdos 0x400000 -O
    fi
else # compile failed
    echo "Failed to compile C code"
fi
