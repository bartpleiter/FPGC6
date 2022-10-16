# USB (CH376)
Using the CH376T USB controller chip over SPI, it is relatively really simple to read and write files to an USB stick with a FAT or FAT32 partition table. It is also possible to do other things, like reading USB MIDI keyboards and HID devices, although a bit more difficult because of the lack of (English) documentation on the chip. I have working code for polling a USB keyboard. The n_interrupt pin from the CH376 is also accessible from the memory map, which makes getting status codes a lot easier.

There are two USB ports on the FPGC. Currently the bottom port is used for storage for the OS. The top one is currently used for an USB keyboard.

In the far future, when the FPGC is a lot faster, I might implement my own filesystem handler (FAT or something custom).

!!! info "Note:"
	The "Set file name" command cannot handle input data with more than 14 characters (excluding terminator?), so to open a file in a subdirectory you need to send the directory name and use "File open" first before sending the filename itself. All filenames should be CAPITAL LETTERS ONLY, following the old 8.3 file name format. Also, the chip can be a bit unreliable when the flash drive is 'incorrectly' formatted. I know it works for FAT32 with a cluster size of 2KB. Also with the V3 PCB design, physically smaller USB drives seem to not work well for some reason. Micro SD to USB adapters are recommended (small ones do work well afaik).