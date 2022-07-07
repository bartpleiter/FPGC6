#!/bin/bash

echo "Assembling B32P ASM code"
if (python3 Assembler.py > ../Programmer/code.list) # compile and write to code.list in Programmer folder
then
    echo "B32P ASM code successfully assembled"
    # convert list to binary files and send to FPGC

    if [[ $1 == "flash" ||  $1 == "write" ]]
    then
        (cd ../Programmer && bash compileROM.sh && echo "Flashing binary to SPI flash" && python3 flash.py write)
    else
        (cd ../Programmer && bash compileROM.sh noPadding && echo "Sending binary to FPGC" && python3 uartFlasher.py)
    fi

else # assemble failed, run again to show error
    echo "Failed to assemble B32P ASM code"
    python3 Assembler.py
fi