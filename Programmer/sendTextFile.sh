#!/bin/bash

if [ $# -eq 0 ]
then
    echo "No file to send"
    exit 1
fi

echo "Converting $1 to wordfile"

# Get base name of file
filename=$(basename -- "$1")

xxd -p -c 1 $1 | sed 's/\(..\)/000000\1/g' | xxd -r -p > wordfile

echo "Sending wordfile to FPGC"
python3 netUpload.py wordfile $filename

echo "File sent"

# Clean up
rm wordfile
