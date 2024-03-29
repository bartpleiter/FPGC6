#!/bin/bash

# script for compiling a bare metal C program, and send it to the FPGC, without doing tests

echo "Processing: $1"
# for each c file, compile and run
echo "Compiling C code to B32P ASM"
if (./bcc $1 ../Assembler/code.asm) # compile c code and write compiled code to code.asm in Assembler folder
then
    echo "C code successfully compiled"

    echo "Assembling B32P ASM code"
    if (cd ../Assembler && python3 Assembler.py > ../Programmer/code.list) # compile and write to code.list in Programmer folder
    then
            echo "B32P ASM code successfully assembled"
            # convert list to binary files and send to FPGC

            if [[ $2 == "flash" ||  $2 == "write" ]]
            then
                (cd ../Programmer && bash compileROM.sh && echo "Flashing binary to FPGC flash" && python3 flash.py write -v verify.bin)
            else
                (cd ../Programmer && bash compileROM.sh noPadding && echo "Sending binary to FPGC" && python3 uartFlasher.py)
            fi
    
    else # assemble failed, run again to show error
        echo "Failed to assemble B32P ASM code"
        cd ../Assembler && python3 Assembler.py
    fi
else # compile failed
    echo "Failed to compile C code"
fi
