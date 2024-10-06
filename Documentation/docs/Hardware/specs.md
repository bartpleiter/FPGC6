# Specifications
These are the current specifications of the FPGC6.

## CPU

- 50MHz 5-stage pipelined CPU (cache not fully implemented yet)
- 32bit instructions
- 16 32bit registers, of which 15 are General Purpose, R0 is always 0
- 27bit program counter for a possible address space of 0.5GiB at 32bit
- Easily extendable amount of hardware interrupts, currently 8 in use

## GPU

- Two layer tile-based render engine at 320x200 with 256 colors, with horizontal hardware scrolling support for one layer
- Bitmap render engine at 320x240 with 24bit color (will be changed to 256 colors in the future to save SRAM)
- Integer scaled 640x480 HDMI output over 3.3V LVDS transmitters (not the best compatibility with displays/adapters)

## Memory

- 16MiB external SPI flash in two modes:
	- QSPI with continuous read mode @ 25MHz: 32bit addresses. Read Only!
	- SPI bus mode @ 25MHz: Accessible as a normal SPI device
- 64MiB SDRAM @ 100MHz. 32bit addresses. Used as main memory (currently only 32MiB is accessible until memory map is updated)
- ~16.4KiB VRAM (SRAM) for tile-based rendering. Combination of 32, 8 and 9bit addresses
- 320x240xbitdepth amount of VRAM (SRAM) for bitmap rendering
- 2KiB internal ROM for the Bootloader. 32bit addresses
- 4KiB Hardware Stack (SRAM). 32bit addresses, internal to CPU

## I/O

- Memory mapped I/O
- 3 One Shot (OS) timers
- PS/2 Keyboard support (to be removed in next hardware revision)
- UART and power over USB
- 4 GPI and 4 GPO pins (will become 8 true GPIO pins eventually when needed)
- I2S DAC for future audio support
- 2 USB host ports using a CH376T controller over SPI
- Ethernet using W5500 over SPI
- TODO: list all I/O connected on current PCB version
