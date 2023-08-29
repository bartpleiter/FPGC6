# VRAM
The FPGC contains several VRAM modules for differents parts of the GPU.

## VRAM32
VRAM32 is the 32 bit wide dual port dual clock video RAM (SRAM/Block RAM) used by the CPU and the GPU. It contains the pattern table and palette table for the GPU. It is implemented using internal SRAM/Block RAM. There are two VRAM32 modules in use at the same time (the contents are duplicated), allowing the Background/Window and Sprite rendering part of GPU to access the memory at the same time. This greatly reduces the complexity of the GPU while only slightly increasing the block RAM usage.
The values of this memory at power up are all zero.

## VRAM8
VRAM8 is the 8 bit wide dual port dual clock video RAM (SRAM/Block RAM) used by the CPU and the GPU. It contains the background tile table, background color table, window tile table and window color table for the GPU. It is implemented using internal SRAM/Block RAM. The final two addresses are the horizontal tile offset and horizontal pixel offset for scrolling. Originally the resolution was 320x240 instead of 320x200, so there are certain unused addresses.
The values of this memory at power up are all zero.

## SpriteVRAM
SpriteVRAM is the 9 bit wide dual port dual clock video RAM (SRAM/Block RAM) used by the CPU and the GPU. It contains the sprite table for the GPU. It is implemented using internal SRAM/Block RAM. The memory has room for 64 sprites, where each sprite as a separate address for X position, Y position, tile index and color index+flags.
The values of this memory at power up are all zero.

## PixelVRAM
TODO: add this