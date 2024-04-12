#!/bin/bash

# script to sync all C files in UserBDOS/ to /c/src on FPGC

MAINPATH=$(pwd)

echo $MAINPATH

cd userBDOS

# Go to root directory
echo "clear" | python3 "$MAINPATH/../Programmer/sendCommand.py"
echo "cd /" | python3 "$MAINPATH/../Programmer/sendCommand.py"

# Create directories
echo "mkdir c" | python3 "$MAINPATH/../Programmer/sendCommand.py"
echo "mkdir c/src" | python3 "$MAINPATH/../Programmer/sendCommand.py"

for i in $(find . -type f -print)
do
    FNAME=$(basename $i)
    DIR="/c/src$(dirname $i | cut -c 2-)"

    # Create directory
    echo "mkdir $DIR" | python3 "$MAINPATH/../Programmer/sendCommand.py"

    # Move to directory
    echo "cd $DIR" | python3 "$MAINPATH/../Programmer/sendCommand.py"
    
    # Send file
    cd $MAINPATH/../Programmer
    sh sendTextFile.sh $MAINPATH/userBDOS/$i

done