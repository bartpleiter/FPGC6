# Specifications
These are the current specifications of the FPGC6.

## CPU

- 50MHz 5-stage pipelined CPU with option to add L1 cache
- 32bit instructions
- 16 32bit registers, of which 15 are General Purpose, R0 is always 0
- 27bit program counter for a possible address space of 0.5GiB at 32bit
- Easily extendable amount of hardware interrupts, currently 8 in use

## GPU

- 320x200 at 256 colors Tile-based and bitmap (at 320x240) rendering GPU with selectable HDMI (480P) and NTSC (240P) output
- Two layers of 8x8 Tiles of which one layer (background) has horizontal hardware scrolling support
- (currently not working anymore) One Sprite layer with support for 64 Sprites. Max 16 Sprites per horizontal line
- Bitmap rendering layer allowing for access to each of the 230x240 individual pixels supporting 256 colors

## Memory

- 16MiB external SPI flash in two modes:
	- QSPI with continuous read mode @ 25MHz: 32bit addresses. Read Only!
	- SPI bus mode @ 25MHz: Accessible as a normal SPI device
- 64MiB SDRAM @ 100MHz. 32bit addresses. Used as main memory (currently only 32MiB is accessible until memory map is updated)
- ~16.4KiB VRAM (SRAM) for tile-based rendering. Combination of 32, 8 and 9bit addresses
- 75 KiB VRAM (SRAM) for bitmap rendering
- 2KiB internal ROM for the Bootloader. 32bit addresses
- 4KiB Hardware Stack (SRAM). 32bit addresses, internal to CPU
- 4KiB L2 cache (SRAM)

## I/O

- Memory mapped I/O
- 3 One Shot (OS) timers
- PS/2 Keyboard support
- UART and power over USB
- 4 GPI and 4 GPO pins (will become 8 true GPIO pins eventually)
- I2S DAC for future audio support
- 2 USB host ports with FAT(12/16/32) file system support using a CH376T controller over SPI
- Ethernet using W5500 over SPI
