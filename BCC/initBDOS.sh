#!/bin/bash

# Script to initialize BDOS (as in an intitial setup) by uploading all the programs in userBDOS to the /bin directory

MAINPATH=$(pwd)

echo $MAINPATH

# Create /bin directory
echo "cd /" | python3 "$MAINPATH/../Programmer/sendCommand.py"
sh uploadToBDOS.sh userBDOS/mkdir.c mkdir
echo "mkdir /bin" | python3 "$MAINPATH/../Programmer/sendCommand.py"
echo "cd /bin" | python3 "$MAINPATH/../Programmer/sendCommand.py"
#sleep 0.5

# Upload all the programs in a very hardcoded way given the directory structure of the project
cd userBDOS
for j in $(find . -maxdepth 1 -type f -print)
do
    cd $MAINPATH
    FNAME=$(basename $j)
    # Remove extension from FNAME
    FNAME="${FNAME%.*}"
    # Remove leading ./
    j="${j#./}"
    bash uploadToBDOS.sh userBDOS/$j $FNAME
    #sleep 0.5
done

echo "rm /mkdir" | python3 "$MAINPATH/../Programmer/sendCommand.py"
