/*
* Raycaster, based on lodev engine tutorial
* Plan:
* - Improve collision detection:
*   - During movement function,
*   - Check if new pos has distance from possible wall hor and ver
* - Make rendering variables global with prefix
* - Convert rendering function to assembly
* - Add sprites (as this is done after walls)
*/

#define word char

#include "LIB/MATH.C"
#include "LIB/STDLIB.C"
#include "LIB/SYS.C"
#include "LIB/GFX.C"
#include "LIB/FP.C"

// Note: these are also hardcoded in the render assembly, so update there as well!
#define FB_ADDR       0xD00000
#define screenWidth   320
#define screenHeight  240
#define texWidth      64
#define texHeight     64
#define mapWidth  24
#define mapHeight 24

// Input
#define BTN_LEFT  256
#define BTN_RIGHT 257
#define BTN_UP    258
#define BTN_DOWN  259

// Colors
#define COLOR_RED         224
#define COLOR_DARK_RED    96
#define COLOR_GREEN       28
#define COLOR_DARK_GREEN  12
#define COLOR_BLUE        3
#define COLOR_DARK_BLUE   2
#define COLOR_WHITE       0xFF
#define COLOR_GREY        0xB6
#define COLOR_YELLOW      0xFC
#define COLOR_DARK_YELLOW 0x90

// Data
/*
- LUTdirX
- LUTdirY
- LUTplaneX
- LUTplaney
- texture
*/
#include "DATA/RAYDAT.C"

// Framebuffer. fb[Y][X] (bottom right is [239][319])
char (*fb)[screenWidth] = (char (*)[screenWidth])FB_ADDR;

word worldMap[mapWidth][mapHeight] = {
    {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 7, 7, 7, 7, 7, 7, 7, 7},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 7},
    {4, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7},
    {4, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7},
    {4, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 7},
    {4, 0, 4, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 7, 7, 0, 7, 7, 7, 7, 7},
    {4, 0, 5, 0, 0, 0, 0, 2, 0, 1, 0, 1, 0, 1, 0, 2, 7, 0, 0, 0, 7, 7, 7, 1},
    {4, 0, 6, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 7, 0, 0, 0, 0, 0, 0, 8},
    {4, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 7, 7, 1},
    {4, 0, 8, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 5, 7, 0, 0, 0, 0, 0, 0, 8},
    {4, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 5, 7, 0, 0, 0, 7, 7, 7, 1},
    {4, 0, 0, 0, 0, 0, 0, 5, 5, 5, 5, 0, 5, 5, 5, 5, 7, 7, 7, 7, 7, 7, 7, 1},
    {6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6},
    {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6},
    {4, 4, 4, 4, 4, 4, 0, 4, 4, 4, 6, 0, 6, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 4, 6, 0, 6, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 2, 0, 0, 5, 0, 0, 2, 0, 0, 0, 2},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 4, 6, 0, 6, 2, 0, 0, 0, 0, 0, 2, 2, 0, 2, 2},
    {4, 0, 6, 0, 6, 0, 0, 0, 0, 4, 6, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 2},
    {4, 0, 0, 5, 0, 0, 0, 0, 0, 4, 6, 0, 6, 2, 0, 0, 0, 0, 0, 2, 2, 0, 2, 2},
    {4, 0, 6, 0, 6, 0, 0, 0, 0, 4, 6, 0, 6, 2, 0, 0, 5, 0, 0, 2, 0, 0, 0, 2},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 4, 6, 0, 6, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2},
    {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3}};

word frameCounter = 0;
word frameCounterLoop = 0;

// Global render variables
word drawStart = 0;
word drawEnd = 0;
word texNum = 0;
word side = 0; // was a NorthSouth or a EastWest wall hit?

// x and y start position
fixed_point_t RAY_posX;
fixed_point_t RAY_posY;

// initial direction vector
fixed_point_t RAY_dirX;
fixed_point_t RAY_dirY;

// the 2d raycaster version of camera plane
fixed_point_t RAY_planeX;
fixed_point_t RAY_planeY;

// Render entire screen
// Registers (global):
//   r1   x         current vertical line (x of screen)
void RAYFX_renderScreen()
{
  // reg 4 5 6 7 (args) and 2 3 (retval) are safe
  asm(
  // backdup registers
  "push r1\n"
  "push r8\n"
  "push r9\n"
  "push r10\n"
  "push r11\n"
  "push r12\n"
  "push r13\n"
  "push r14\n"
  "push r15\n"
  );

  asm(
  
  // r1: current vertical line (x of screen)
  "load32 0 r1\n"

  // for each vertical line (x)
  "RAYFX_screenXloop:\n"

    // cameraX
    "addr2reg LUTcameraX r15\n" // r15 = LUTcameraX
    "add r15 r1 r15\n"          // r15 = LUTcameraX[x] addr
    "read 0 r15 r15\n"          // r15 = LUTcameraX[x] value

    //   r2   rayDirx   [FP] x dir of ray
    //   r3   rayDiry   [FP] y dir of ray

    // r2: rayDirX
    "addr2reg RAY_dirX r14\n"   // r14 = RAY_dirX addr
    "read 0 r14 r14\n"          // r14 = RAY_dirX value
    "addr2reg RAY_planeX r13\n" // r13 = RAY_planeX addr
    "read 0 r13 r13\n"          // r13 = RAY_planeX value
    "multfp r13 r15 r2\n"       // r2 = FP_Mult(RAY_planeX, cameraX)
    "add r2 r14 r2\n"           // r2 += RAY_dirX
    
    // r3: rayDirY
    "addr2reg RAY_dirY r14\n"   // r14 = RAY_dirY addr
    "read 0 r14 r14\n"          // r14 = RAY_dirY value
    "addr2reg RAY_planeY r13\n" // r13 = RAY_planeY addr
    "read 0 r13 r13\n"          // r13 = RAY_planeY value
    "multfp r13 r15 r3\n"       // r3 = FP_Mult(RAY_planeY, cameraX)
    "add r3 r14 r3\n"           // r3 += RAY_dirY

    // r4: deltaDistX
    "load32 1 r15\n"            // r15 = 1
    "shiftl r15 16 r15\n"       // r15 = FP(1)
    "load32 0xC02742 r14\n"     // r14 = addr fpdiv_writea
    "write 0 r14 r15\n"         // write a (fp1) to divider
    "write 1 r14 r2\n"          // write b (rayDirX) to divider and perform division
    "read 1 r14 r4\n"           // read result to r4
    "shiftrs r4 31 r13\n"       // r13 = y = x >>(signed) 31 (start of abs)
    "xor r4 r13 r4\n"           // r4 = (x ^ y)
    "sub r4 r13 r4\n"           // r4 -= y (result of abs)

    // r5: deltaDistY (uses r14 and r15 from above)
    "write 0 r14 r15\n"         // write a (fp1) to divider
    "write 1 r14 r3\n"          // write b (rayDirY) to divider and perform division
    "read 1 r14 r5\n"           // read result to r5
    "shiftrs r5 31 r13\n"       // r13 = y = x >>(signed) 31 (start of abs)
    "xor r5 r13 r5\n"           // r5 = (x ^ y)
    "sub r5 r13 r5\n"           // r5 -= y (result of abs)

    // if (rayDirX < 0), else
    // r6: stepX
    // r7: stepY
    // r8: sideDistX
    // r9: sideDistY
    // r10: mapX
    // r11: mapY
    "blts r2 r0 2\n"            // if (rayDirX < 0) skip next line
    "jump RAYFX_rayDirXge0\n"   // goto RAYFX_rayDirXge0

      "load32 -1 r6\n"          // r6 = stepX = -1
      "addr2reg RAY_posX r15\n" // r15 = RAY_posX addr
      "read 0 r15 r15\n"        // r15 = RAY_posX value
      "shiftrs r15 16 r10\n"    // mapX = int(RAY_posX)
      "load32 0xFFFF r14\n"     // r14 = decimal part mask
      "and r15 r14 r15\n"       // r15 = RAY_posX decimal part
      "multfp r15 r4 r8\n"      // sideDistX = RAY_posX decimal part * deltaDistX
      "jump RAYFX_rayDirXltEnd\n" // skip the else part

    "RAYFX_rayDirXge0:\n"
      "load32 1 r6\n"           // stepX = 1
      "addr2reg RAY_posX r15\n" // r15 = RAY_posX addr
      "read 0 r15 r15\n"        // r15 = RAY_posX value
      "shiftrs r15 16 r10\n"    // r10 (mapX) = r15 >> 16 (to int)
      "add r10 1 r14\n"         // r14 = int(RAY_posX) + 1
      "shiftl r14 16 r14\n"     // r14 <<=16 (to FP)
      "sub r14 r15 r15\n"       // r15 = r14 - RAY_posX
      "multfp r15 r4 r8\n"      // sideDistX = RAY_posX-ish * deltaDistX

    "RAYFX_rayDirXltEnd:\n"

    // if (rayDirY < 0), else
    "blts r3 r0 2\n"            // if (rayDirY < 0) skip next line
    "jump RAYFX_rayDirYge0\n"   // goto RAYFX_rayDirYge0

      "load32 -1 r7\n"          // r7 = stepY = -1
      "addr2reg RAY_posY r15\n" // r15 = RAY_posY addr
      "read 0 r15 r15\n"        // r15 = RAY_posY value
      "shiftrs r15 16 r11\n"    // mapY = int(RAY_posY)
      "load32 0xFFFF r14\n"     // r14 = decimal part mask
      "and r15 r14 r15\n"       // r15 = RAY_posY decimal part
      "multfp r15 r5 r9\n"      // sideDistY = RAY_posY decimal part * deltaDistY
      "jump RAYFX_rayDirYltEnd\n" // skip the else part

    "RAYFX_rayDirYge0:\n"
      "load32 1 r7\n"           // stepY = 1
      "addr2reg RAY_posY r15\n" // r15 = RAY_posY addr
      "read 0 r15 r15\n"        // r15 = RAY_posY value
      "shiftrs r15 16 r11\n"    // r11 (mapY) = r15 >> 16 (to int)
      "add r11 1 r14\n"         // r14 = int(RAY_posY) + 1
      "shiftl r14 16 r14\n"     // r14 <<=16 (to FP)
      "sub r14 r15 r15\n"       // r15 = r14 - RAY_posX
      "multfp r15 r5 r9\n"      // sideDistY = RAY_posX-ish * deltaDistY

    "RAYFX_rayDirYltEnd:\n"

    // DDA (uses tmp r15)
    "load32 0 r15\n"            // hit = 0
    "RAYFX_DDAwhileNoHit:\n"    // while (hit == 0)
      "beq r15 r0 2\n"          // while check
      "jump RAYFX_DDAwhileNoHitDone\n"

      // if (sideDistX < sideDistY), else
      "blts r8 r9 2\n"              // if (sideDistX < sideDistY) skip next line
      "jump RAYFX_sideDistXltY\n"   // goto RAYFX_sideDistXltY

        "add r8 r4 r8\n"            // sideDistX += deltaDistX;
        "add r10 r6 r10\n"          // mapX += stepX;
        "addr2reg side r14\n"       // side;
        "write 0 r14 r0\n"          // side = 0;
        "jump RAYFX_sideDistXltYend\n" // skip the else part

      "RAYFX_sideDistXltY:\n"
        "add r9 r5 r9\n"            // sideDistY += deltaDistY;
        "add r11 r7 r11\n"          // mapY += stepY;
        "addr2reg side r14\n"       // side;
        "load32 1 r13\n"            // 1
        "write 0 r14 r13\n"         // side = 1;

      "RAYFX_sideDistXltYend:\n"

      "addr2reg worldMap r14\n"     // r14 = worldMap addr;
      "multu r10 24 r13\n"          // r13 = mapX offset using mapHeight
      "add r13 r11 r13\n"           // r13 += mapY offset
      "add r13 r14 r14\n"           // r14 = worldMap[mapX][mapY] addr
      "read 0 r14 r14\n"            // r14 = worldMap[mapX][mapY] value
      "bles r14 r0 2\n"             // skip next instruction if worldMap[mapX][mapY] <= 0
        "load 1 r15\n"            // hit = 1

      "jump RAYFX_DDAwhileNoHit\n" // return to while loop start

    "RAYFX_DDAwhileNoHitDone:\n"

    // From this point, r10 r11 mapx mapy (and r15) are free

    // texNum = worldMap[mapX][mapY] - 1;
    // assumes worldMap[mapX][mapY] value is still in r14
    "addr2reg texNum r13\n"     // r13 = texNum addr;
    "sub r14 1 r14\n"           // r14 = worldMap[mapX][mapY] - 1
    "write 0 r13 r14\n"         // write texNum;

    // r10: perpWallDist (FP)
    "addr2reg side r15\n"       // side addr
    "read 0 r15 r15\n"          // side value

    // if (side == 0), else
    "bne r15 r0 3\n"            // if (side != 0) skip next two lines
    "sub r8 r4 r10\n"           // perpWallDist = (sideDistX - deltaDistX); (match)
    "jumpo 2\n"                 // skip else part
    "sub r9 r5 r10\n"           // perpWallDist = (sideDistY - deltaDistY); (else)

    // From this point sideDistX&Y and deltaDistX&Y are free
    // this frees up regs 4 5 8 and 9
    // leaving to use: 4, 5, 8, 9, 11, 12, 13, 14, 15

    // r4: lineHeight
    "load32 240 r15\n"          // r15 = screenHeight
    "shiftl r15 16 r15\n"       // r15 = FP(screenHeight)
    "load32 0xC02742 r14\n"     // r14 = addr fpdiv_writea
    "write 0 r14 r15\n"         // write a (FP(screenHeight) to divider
    "write 1 r14 r10\n"         // write b (perpWallDist) to divider and perform division
    "read 1 r14 r4\n"           // read result to r4
    "shiftrs r4 16 r4\n"        // r4 = lineHeight = int(r4)

    "shiftrs r4 1 r15\n"        // r15 = lineHeight >> 1
    "load32 -1 r14\n"           // r14 = -1
    "mults r15 r14 r15\n"       // r15 = -r15
    "load32 120 r14\n"          // r14 = screenHeight >> 1
    "add r14 r15 r15\n"         // r15 = drawStart value (r14+r15)
    "bges r15 r0 2\n"           // skip next line if drawStart >= 0
      "load 0 r15\n"            // set drawStart to 0 if < 0
    "addr2reg drawStart r14\n"  // r14 = drawStart addr
    "write 0 r14 r15\n"         // write drawStart value

    // skip render if start > 238
    "load32 238 r14\n"
    "blts r15 r14 2 \n"
      "jump RAYFX_skipRenderLine\n"

    "shiftrs r4 1 r15\n"        // r15 = lineHeight >> 1
    "load32 120 r14\n"          // r14 = screenHeight >> 1
    "add r14 r15 r15\n"         // r15 = drawEnd value (r14+r15)
    "load32 240 r14\n"          // r14 = screenHeight
    "blts r15 r14 2\n"          // skip next line if drawEnd < screenHeight
      "load 239 r15\n"          // set drawEnd to screenHeight - 1
    "addr2reg drawEnd r14\n"    // r14 = drawEnd addr
    "write 0 r14 r15\n"         // write drawEnd value


    // texture calculations
    // r8: side
    // r5: wallX
    "addr2reg side r8\n"      // r8 = side addr
    "read 0 r8 r8\n"          // r8 = side value
    "bne r8 r0 7\n"           // if side != 0, goto else part (addr2reg is two instructions!)
      "addr2reg RAY_posY r15\n" // r15 = RAY_posY addr
      "read 0 r15 r15\n"        // r15 = RAY_posY value
      "multfp r10 r3 r14\n"     // r14 = perpWallDist * rayDirY
      "add r14 r15 r5\n"        // r5 = wallX = r14 + RAY_posY
      "jumpo 5\n"               // skip else

    // else
      "addr2reg RAY_posX r15\n" // r15 = RAY_posX addr
      "read 0 r15 r15\n"        // r15 = RAY_posX value
      "multfp r10 r2 r14\n"     // r14 = perpWallDist, rayDirX
      "add r14 r15 r5\n"        // r5 = wallX = r14 + RAY_posX

    "load32 0xFFFF r15\n"       // r15 = floormask
    "and r5 r15 r5\n"           // wallX-=floor(wallX)

    // r6: texX
    "load32 64 r15\n"           // r15 = texWidth
    "shiftl r15 16 r14\n"       // r14 = FP(texWidth)
    "multfp r14 r5 r6\n"        // r6 = FP_Mult(wallX, FP_intToFP(texWidth))
    "shiftrs r6 16 r6\n"        // r6 = int(r6)

    "bne r8 r0 4\n"             // skip if side != 0
    "bles r2 r0 3\n"            // skip if rayDirX <= 0

      "sub r15 r6 r6\n"           // texX = texWidth - texX
      "sub r6 1 r6\n"             // texX -= 1

    "beq r8 r0 4\n"             // skip if side == 0
    "bges r3 r0 3\n"            // skip if rayDirY >= 0

      "sub r15 r6 r6\n"           // texX = texWidth - texX
      "sub r6 1 r6\n"             // texX -= 1

    // r2 and r3 are free now (no rayDirX&Y needed)
    // r5: step
    "load32 64 r15\n"           // r15 = texHeight
    "shiftl r15 16 r15\n"       // r15 = FP(texHeight)
    "shiftl r4 16 r14\n"        // r14 = FP(lineHeight)
    "load32 0xC02742 r13\n"     // r13 = addr fpdiv_writea
    "write 0 r13 r15\n"         // write a (FP(screenHeight) to divider
    "write 1 r13 r14\n"         // write b (FP(lineHeight)) to divider and perform division
    "read 1 r13 r5\n"           // read result to r5

    // r4: texPos
    "addr2reg drawStart r15\n"  // r15 = drawStart addr
    "read 0 r15 r15\n"          // r15 = drawStart value
    "sub r15 120 r15\n"         // r15 -= (screenHeight >> 1)
    "shiftrs r4 1 r14\n"        // r14 = lineHeight >> 1
    "add r15 r14 r15\n"         // r15 += lineHeight >> 1
    "shiftl r15 16 r15\n"       // r15 = FP(r15)
    "multfp r15 r5 r4\n"        // r4 = r15 * step

    "or r1 r0 r7\n"             // move x to r7


    // Render vertical line in pixel plane with textures
    // Registers:
    //   r1        first pixel addr   VRAM addr of first pixel in line (top pixel)
    //   r2 (a2r)  drawStart          Starting pixel of wall
    //   r3 (a2r)  drawEnd            Ending pixel of wall
    //   r4        texPos             Starting texture coordinate
    //   r5        step               How much to increase the texture coordinate per screen pixel
    //   r6        texX               X coordinate on the texture
    //   r7        x                  X position of line to render
    //   r8        current FB pos     Current framebuffer position in VRAM (top to bottom)
    //   r9        end loop FB pos    Last framebuffer position in VRAM of current loop
    //   r10       texY               Y coordinate on the texture
    //   r11       gp                 Used as temp reg in calculations
    //   r12       texture[texNum]    Texture array of texture to draw
    //   r13       color              Pixel color
    //   r14       ceil or floor col  Ceiling or floor color
    //   r15       side               North South or East West wall side

    "push r1\n"             // backup x loop var

    "addr2reg drawStart r2  ; r2 = drawStart addr\n"
    "read 0 r2 r2           ; r2 = drawStart value\n"

    "addr2reg drawEnd r3    ; r3 = drawEnd addr\n"
    "read 0 r3 r3           ; r3 = drawEnd value\n"

    "load32 0xD00000 r1     ; r1 = framebuffer addr\n"
    "add r1 r7 r1           ; r1 = first pixel in line (fb+x)\n"

    "or r0 r1 r8            ; r8 = current pixel\n"

    "multu r2 320 r9        ; r9 = drawStart VRAM offset\n"
    "add r9 r1 r9           ; r9 = last FB pos of before wall\n"

    "addr2reg texNum r12    ; r12 = texNum addr\n"
    "read 0 r12 r12         ; r12 = texNum value\n"
    "multu r12 4096 r12     ; r12 = texture offset (64*64 per texture)\n"

    "addr2reg texture r11   ; r11 = texture array\n"
    "add r12 r11 r12        ; r12 = texture[texNum]\n"

    "load32 0x0082F0 r14    ; r14 = ceiling color\n"

    "addr2reg side r15      ; r15 = side addr\n"
    "read 0 r15 r15         ; r15 = side value\n"

    // draw until start
    "RAYFX_drawVlineLoopCeiling:\n"
    "  write 0 r8 r14     ; write ceiling pixel\n"
    "  add r8 320 r8     ; go to next line pixel\n"

    "  bge r8 r9 2       ; keep looping until reached wall\n"
    "  jump RAYFX_drawVlineLoopCeiling\n"




    "multu r3 320 r9        ; r9 = drawEnd VRAM offset\n"
    "add r9 r1 r9           ; r9 = last FB pos of wall\n"

    "load32 8355711 r2      ; r2 = mask for darken color\n"

    // draw until floor
    "RAYFX_drawVlineLoopWall:\n"
    "  shiftrs r4 16 r11  ; r11 = texY = FPtoInt(texPos)\n"
    "  and r11 63 r11     ; r11 = r11 & (texHeight-1)\n"
    "  add r4 r5 r4       ; texPos += step\n"

    "  multu r11 64 r11   ; r11 = texHeight * texY \n"
    "  add r11 r6 r11     ; r11 += texX\n"

    "  add r11 r12 r13    ; r13 = addr of color in texture array\n"
    "  read 0 r13 r13     ; r13 = pixel color\n"

    "  beq r15 r0 3       ; skip next two lines if not side of wall is hit\n"
    "    shiftrs r13 1 r13\n" // r13 >> 1
    "    and r13 r2 r13     ; r13 & darken color mask\n"


    "  write 0 r8 r13     ; write texture pixel\n"
    "  add r8 320 r8      ; go to next line pixel\n"

    "  bge r8 r9 2        ; keep looping until reached floor\n"
    "  jump RAYFX_drawVlineLoopWall\n"




    "load32 239 r9          ; r9 = last y position\n"
    "multu r9 320 r9        ; r9 = screen end VRAM offset\n"
    "add r9 r1 r9           ; r9 = last FB pos of line\n"

    "load32 0x9E9E9E r14    ; r14 = floor color\n"

    "; draw until end of screen\n"
    "RAYFX_drawVlineLoopFloor:\n"
    "  write 0 r8 r14    ; write floor pixel\n"
    "  add r8 320 r8     ; go to next line pixel\n"

    "  bgt r8 r9 2       ; keep looping until reached end of screen\n"
    "  jump RAYFX_drawVlineLoopFloor\n"


    "pop r1\n"              // restore x loop var

    "RAYFX_skipRenderLine:\n"
    

    "add r1 1 r1\n"             // r1 = x++
    "load32 320 r15\n"          // r15 = stop x loop (screenWidth)
    "bge r1 r15 2\n"            // keep looping until reached final vline (x) of screen
    "jump RAYFX_screenXloop\n"
  );

  asm(
  // restore registers
  "pop r15\n"
  "pop r14\n"
  "pop r13\n"
  "pop r12\n"
  "pop r11\n"
  "pop r10\n"
  "pop r9\n"
  "pop r8\n"
  "pop r1\n"
  );
}


int main() {
  // clear screen from text
  GFX_clearWindowtileTable();
  GFX_clearWindowpaletteTable();
  GFX_clearBGtileTable();
  GFX_clearBGpaletteTable();

    // x and y start position
  RAY_posX = FP_intToFP(15);
  RAY_posY = FP_StringToFP("11.5");

  // initial direction vector
  RAY_dirX = LUTdirX[0];
  RAY_dirY = LUTdirY[0];

  // the 2d raycaster version of camera plane
  RAY_planeX = LUTplaneX[0];
  RAY_planeY = LUTplaneY[0];

  // rotation angle (loops at 360)
  word rotationAngle = 0;
  word rotationSpeed = 5;  // degrees per frame

  fixed_point_t moveSpeed = FP_StringToFP("0.15");
  fixed_point_t movePadding = 0;//FP_StringToFP("0.3");

  word quitGame = 0;

  while (!quitGame) {
    
    // render screen
    RAYFX_renderScreen();

    // calculate and draw FPS
    if (frameCounterLoop == 10)
    {
      word fps = MATH_div(600, frameCounter);
      frameCounter = 0;
      char buffer[11];
      itoa(fps, buffer);
      // clear fps digits (assumes fps <= 999)
      asm(
      "push r1\n"
      "load32 0xC01420 r1       ; r1 = vram addr\n"
      "write 0 r1 r0            ; write char to vram\n"
      "write 1 r1 r0            ; write char to vram\n"
      "write 2 r1 r0            ; write char to vram\n"
      "pop r1\n"
      );
      
      GFX_printWindowColored(buffer, strlen(buffer), 0, 0);
      frameCounterLoop = 0;
    }
    else
    {
      frameCounterLoop++;
    }

    // check which button is held
    if (BDOS_USBkeyHeld(BTN_LEFT)) {
      // both camera direction and camera plane must be rotated
      rotationAngle -= rotationSpeed;
      if (rotationAngle < 0) {
        rotationAngle += 360;
      }
      RAY_dirX = LUTdirX[rotationAngle];
      RAY_dirY = LUTdirY[rotationAngle];
      RAY_planeX = LUTplaneX[rotationAngle];
      RAY_planeY = LUTplaneY[rotationAngle];
    } else if (BDOS_USBkeyHeld(BTN_RIGHT)) {
      // both camera direction and camera plane must be rotated
      rotationAngle += rotationSpeed;
      if (rotationAngle >= 360) {
        rotationAngle -= 360;
      }
      RAY_dirX = LUTdirX[rotationAngle];
      RAY_dirY = LUTdirY[rotationAngle];
      RAY_planeX = LUTplaneX[rotationAngle];
      RAY_planeY = LUTplaneY[rotationAngle];
    }

    if (BDOS_USBkeyHeld(BTN_UP)) {
      word worldMapX = FP_FPtoInt(RAY_posX + FP_Mult(RAY_dirX, moveSpeed + movePadding));
      word worldMapY = FP_FPtoInt(RAY_posY);

      if (worldMap[worldMapX][worldMapY] == 0) {
        RAY_posX += FP_Mult(RAY_dirX, moveSpeed);
      }

      worldMapX = FP_FPtoInt(RAY_posX);
      worldMapY = FP_FPtoInt(RAY_posY + FP_Mult(RAY_dirY, moveSpeed + movePadding));
      if (worldMap[worldMapX][worldMapY] == 0) {
        RAY_posY += FP_Mult(RAY_dirY, moveSpeed);
      }
    } else if (BDOS_USBkeyHeld(BTN_DOWN)) {
      word worldMapX = FP_FPtoInt(RAY_posX - FP_Mult(RAY_dirX, moveSpeed + movePadding));
      word worldMapY = FP_FPtoInt(RAY_posY);

      if (worldMap[worldMapX][worldMapY] == 0) {
        RAY_posX -= FP_Mult(RAY_dirX, moveSpeed);
      }

      worldMapX = FP_FPtoInt(RAY_posX);
      worldMapY = FP_FPtoInt(RAY_posY - FP_Mult(RAY_dirY, moveSpeed + movePadding));
      if (worldMap[worldMapX][worldMapY] == 0) {
        RAY_posY -= FP_Mult(RAY_dirY, moveSpeed);
      }
    }

    if (HID_FifoAvailable()) {
      word c = HID_FifoRead();

      if (c == 27)  // escape
      {
        // clean up and exit
        GFX_clearPXframebuffer();
        GFX_clearWindowtileTable();
        GFX_clearWindowpaletteTable();
        GFX_clearBGtileTable();
        GFX_clearBGpaletteTable();
        quitGame = 1;
      }
    }
  }

  return 'q';
}

void interrupt() {
  // handle all interrupts
  word i = getIntID();
  switch (i) {
    case INTID_TIMER1:
      timer1Value = 1;  // notify ending of timer1
      break;

    case INTID_TIMER2:
      break;

    case INTID_UART0:
      break;

    case INTID_GPU:
      frameCounter++;
      break;

    case INTID_TIMER3:
      break;

    case INTID_PS2:
      break;

    case INTID_UART1:
      break;

    case INTID_UART2:
      break;
  }
}