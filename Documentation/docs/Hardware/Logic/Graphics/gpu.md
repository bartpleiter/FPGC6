# GPU (FSX2)
The GPU, called the FSX2 (Frame Synthesizer 2), generates a video signal based on the contents of VRAM. The actual active resolution of the output is 320x200 pixels (40x25 characters). The GPU outputs this at 480p over HDMI (2x scaling), or 240p over NTSC composite. The output can be selected at any moment using the second dip switch on the PCB.

The GPU consists of three main parts:

- Timing generators
- Render engines
- Output encoders

!!! info
    Looking back at the code, I could structure things better to clearly make the distinction between timing generators, render engines and output encoders. For example, currently the `TimingGenerator` is only used for HDMI, and the `RGB332toNTSC` contains its own different timing generator within the module. I will improve it when upgrading the GPU to have a pixel plane when the Cyclone V board arrives.

## Timing generators
The timing generators generate a blank unencoded video signal for an output protocol. For HDMI, this is a 480p signal and for NTCS this is a 240p signal. These generators create sync signals and horizontal/vertical counters that can be used by a render engine to draw the graphics on the video output.

## Render engines
The GPU uses render engines to draw different graphic planes, which are then combined in some way and added to the video signal.

!!! info "TODO"
	Rename `renderer` to `engine` or `render engine` or something like that.

### BGWrenderer
The main, and currently only, render engine is the BGWrenderer, which stands for Background and Window plane renderer. It uses a tile based rendering system inspired by the NES PPU in order to prevent having to use a frame buffer and therefore save video RAM. This also makes it easy to draw characters by loading an ASCII tileset. However, this also makes it impossible to draw individual pixels. The BGWrenderer has a scale2x input signal to double the resolution in case of HDMI output. The BGWrenderer works as follows:

#### Tile based rendering process
For each tile, the GPU has to read the following tables in order to know which color to draw (in the following order):

- BG Tile table
- Pattern table
- BG Color table
- Palette table
- Window Tile table
- Pattern table
- Window Color table
- Palette table

The Pattern table allows for 256 different tiles on a single screen.

The Palette table allows for 32 different palettes per screen with four colors per palette.

Each address in the tile tables and the color tables is mapped to one tile on screen.

Two layers of tiles are rendered, the background layer which can be scrolled horizontally, and the window layer which draws on top of the background layer and does not scroll.

#### Background layer
The background layer consists of 512x200 pixels. They are indexed by tiles of 8x8 pixels making 64x25 tiles. The background is horizontally scrollable by using the tile offset parameter and fine offset parameter. The tile offset parameter specifies how many tiles the background has to be scrolled to the left. The fine offset parameter specifies how many pixels (ranging from 0 to 7) the background has to be scrolled to the left. The background wraps around horizontally. This means no vertical scrolling (in hardware).

#### Window layer
The window layer consists of 320x200 pixels. They are indexed by tiles of 8x8 pixels making 40x25 tiles. The window is not scrollable and is rendered above the background. When a pixel is black, it will not be rendered which makes the background visible. The window is especially useful for static UI elements in games like text, score and a life bar.

### SpriteRenderer

!!! info
    The SpriteRenderer is currently unused as it was removed after adding the functionality to switch between NTSC and HDMI. It is still added to the Wiki in case I want to bring it back.

The SpriteRenderer is very similar to the BGWrenderer, as it also uses the same tile based render system. The main difference is that it allows for single tiles (called sprites) to be placed anywhere on the screen. This sprite layer can consist of a maximum of 64 sprites. Only 16 of these sprites can be rendered on the same horizontal line (which is double the amount a NES can render on a single line). Each sprite has four different addresses that can be written to, with the following functions:

- X position (9 bits)
- Y position (8 bits)
- Tile index (8 bits)
- Color index (5 bits), hflip, vflip, priority, disable (all 1 bit)

hflip, vflip, priority and disable are currently not implemented.
The sprites are rendered on top of the window and background layers. When a pixel is black, it will not be rendered which makes the window or background visible. Sprites are useful for things that move per pixel on the screen independently, such as the ball in pong or a mouse cursor.

### PixelPlane Engine

!!! info 
	The PixelPlane engine is currently not made yet because of the limited amount of block RAM in the EP4CE15 FPGA. However, after upgrading to the Cyclone V 5CEA5 this will not be an issue anymore.

The PixelPlane Engine is a very simple rendering engine to allow changing individual pixels, which is also known as bitmap graphics, to overcome the limitation of the BGWrenderer. For every 320x200 pixels there is an address in the pixel table where the 8 bits indicate the color of that pixel. All pixels are sequentially stored in this pixel table, meaning that the second pixel of the second line will be stored in address 321. The rendering engine uses a simple counter that increments when the video signal in in the active area, and resets to 0 at vsync. This counter is used as the address of the pixel table and the resulting data is the RGB value of that pixel.

Because the PixelPlane Engine is very simplistic, the CPU has to do more work for generating graphics. For example, for drawing a line, every pixel has to be calculated and set by the CPU.