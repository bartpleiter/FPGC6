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

// Map
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

// Global variables, so render function can access it
word drawStart = 0;
word drawEnd = 0;
word texNum = 0;
word side = 0; // was a NorthSouth or a EastWest wall hit?

// Render vertical line in pixel plane with textures
// INPUT:
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
void RAYFX_drawTextureVertLine(fixed_point_t texPos, fixed_point_t step, word texX, word x)
{
  // reg 4 5 6 7 (args) and 2 3 (retval) are safe
  asm(
  "; backup registers\n"
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

  // setup registers
  asm(
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

  "; draw until start\n"
  "RAYFX_drawVlineLoopCeiling:\n"
  "  write 0 r8 r14     ; write ceiling pixel\n"
  "  add r8 320 r8     ; go to next line pixel\n"

  "  bge r8 r9 2       ; keep looping until reached wall\n"
  "  jump RAYFX_drawVlineLoopCeiling\n"




  "multu r3 320 r9        ; r9 = drawEnd VRAM offset\n"
  "add r9 r1 r9           ; r9 = last FB pos of wall\n"

  "load32 8355711 r2      ; r2 = mask for darken color\n"

  "; draw until floor\n"
  "RAYFX_drawVlineLoopWall:\n"
  "  shiftrs r4 16 r11  ; r11 = texY = FPtoInt(texPos)\n"
  "  and r11 63 r11     ; r11 = r11 & (texHeight-1)\n"
  "  add r4 r5 r4       ; texPos += step\n"

  "  multu r11 64 r11   ; r11 = texHeight * texY \n"
  "  add r11 r6 r11     ; r11 += texX\n"

  "  add r11 r12 r13    ; r13 = addr of color in texture array\n"
  "  read 0 r13 r13     ; r13 = pixel color\n"

  "  beq r15 r0 3       ; skip next two lines if not side of wall is hit\n"
  "    shiftrs r13 1 r13  ; r13 >> 1\n"
  "    and r13 r2 r13     ; r13 & darken color mask\n"


  "  write 0 r8 r13     ; write texture pixel\n"
  "  add r8 320 r8      ; go to next line pixel\n"

  "  bge r8 r9 2        ; keep looping until reached floor\n"
  "  jump RAYFX_drawVlineLoopWall\n"




  "load 239 r9            ; r9 = last y position\n"
  "multu r9 320 r9        ; r9 = screen end VRAM offset\n"
  "add r9 r1 r9           ; r9 = last FB pos of line\n"

  "load32 0x9E9E9E r14    ; r14 = floor color\n"

  "; draw until end of screen\n"
  "RAYFX_drawVlineLoopFloor:\n"
  "  write 0 r8 r14    ; write floor pixel\n"
  "  add r8 320 r8     ; go to next line pixel\n"

  "  bgt r8 r9 2       ; keep looping until reached end of screen\n"
  "  jump RAYFX_drawVlineLoopFloor\n"
  );

  asm(
  "; restore registers\n"
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

// x and y start position
fixed_point_t RAY_posX;
fixed_point_t RAY_posY;

// initial direction vector
fixed_point_t RAY_dirX;
fixed_point_t RAY_dirY;

// the 2d raycaster version of camera plane
fixed_point_t RAY_planeX;
fixed_point_t RAY_planeY;

word RAY_linex; // current vertical line being rendered (x of screen)

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
  fixed_point_t movePadding = FP_StringToFP("0.3");

  word quitGame = 0;

  while (!quitGame) {
    
    for (RAY_linex = 0; RAY_linex < screenWidth; RAY_linex++) 
    {
      // calculate ray position and direction
      fixed_point_t cameraX =
          FP_Div(FP_intToFP(RAY_linex << 1), FP_intToFP(screenWidth)) -
          FP_intToFP(1);  // x-coordinate in camera space

      fixed_point_t rayDirX = RAY_dirX + FP_Mult(RAY_planeX, cameraX);
      fixed_point_t rayDirY = RAY_dirY + FP_Mult(RAY_planeY, cameraX);

      // which box of the map we are in
      word mapX = FP_FPtoInt(RAY_posX);
      word mapY = FP_FPtoInt(RAY_posY);

      // length of ray from current position to next x or y-side
      fixed_point_t sideDistX;
      fixed_point_t sideDistY;

      // length of ray from one x or y-side to next x or y-side
      // these are derived as:
      // deltaDistX = sqrt(1 + (rayDirY * rayDirY) / (rayDirX * rayDirX))
      // deltaDistY = sqrt(1 + (rayDirX * rayDirX) / (rayDirY * rayDirY))
      // which can be simplified to abs(|rayDir| / rayDirX) and abs(|rayDir| /
      // rayDirY) where |rayDir| is the length of the vector (rayDirX, rayDirY).
      // Its length, unlike (RAY_dirX, RAY_dirY) is not 1, however this does not matter,
      // only the ratio between deltaDistX and deltaDistY matters, due to the
      // way the DDA stepping further below works. So the values can be computed
      // as below.
      //  Division through zero is prevented by setting the result to a very
      //  high value
      fixed_point_t deltaDistX =
          (rayDirX == 0) ? 1 << 30 : MATH_abs(FP_Div(FP_intToFP(1), rayDirX));
      fixed_point_t deltaDistY =
          (rayDirY == 0) ? 1 << 30 : MATH_abs(FP_Div(FP_intToFP(1), rayDirY));

      fixed_point_t perpWallDist;

      // what direction to step in x or y-direction (either +1 or -1)
      word stepX;
      word stepY;

      word hit = 0;  // was there a wall hit?

      // calculate step and initial sideDist
      if (rayDirX < 0) {
        stepX = -1;
        sideDistX = FP_Mult((RAY_posX - FP_intToFP(mapX)), deltaDistX);
      } else {
        stepX = 1;
        sideDistX = FP_Mult((FP_intToFP(mapX + 1) - RAY_posX), deltaDistX);
      }
      if (rayDirY < 0) {
        stepY = -1;
        sideDistY = FP_Mult((RAY_posY - FP_intToFP(mapY)), deltaDistY);
      } else {
        stepY = 1;
        sideDistY = FP_Mult((FP_intToFP(mapY + 1) - RAY_posY), deltaDistY);
      }

      // perform DDA
      while (hit == 0) {
        // jump to next map square, either in x-direction, or in y-direction
        if (sideDistX < sideDistY) {
          sideDistX += deltaDistX;
          mapX += stepX;
          side = 0;
        } else {
          sideDistY += deltaDistY;
          mapY += stepY;
          side = 1;
        }
        // Check if ray has hit a wall
        if (worldMap[mapX][mapY] > 0) hit = 1;
      }

      // Calculate distance projected on camera direction. This is the shortest
      // distance from the point where the wall is hit to the camera plane.
      // Euclidean to center camera point would give fisheye effect! This can be
      // computed as (mapX - RAY_posX + (1 - stepX) / 2) / rayDirX for side == 0, or
      // same formula with Y for size == 1, but can be simplified to the code
      // below thanks to how sideDist and deltaDist are computed: because they
      // were left scaled to |rayDir|. sideDist is the entire length of the ray
      // above after the multiple steps, but we subtract deltaDist once because
      // one step more into the wall was taken above.
      if (side == 0)
        perpWallDist = (sideDistX - deltaDistX);
      else
        perpWallDist = (sideDistY - deltaDistY);

      // Calculate height of line to draw on screen
      word lineHeight =
          FP_FPtoInt(FP_Div(FP_intToFP(screenHeight), perpWallDist));

      // calculate lowest and highest pixel to fill in current stripe
      drawStart = -(lineHeight >> 1) + (screenHeight >> 1);
      if (drawStart < 0) drawStart = 0;
      drawEnd = (lineHeight >> 1) + (screenHeight >> 1);
      if (drawEnd >= screenHeight) drawEnd = screenHeight - 1;

      // texturing calculations
      texNum = worldMap[mapX][mapY] -
               1;  // 1 subtracted from it so that texture 0 can be used!

      // calculate value of wallX
      fixed_point_t wallX;  // where exactly the wall was hit
      if (side == 0)
        wallX = RAY_posY + FP_Mult(perpWallDist, rayDirY);
      else
        wallX = RAY_posX + FP_Mult(perpWallDist, rayDirX);
      word floormask = 0xFFFF;
      wallX &= floormask;  // wallX-=floor(wallX)

      // x coordinate on the texture
      word texX = FP_FPtoInt(FP_Mult(wallX, FP_intToFP(texWidth)));
      if (side == 0 && rayDirX > 0) texX = texWidth - texX - 1;
      if (side == 1 && rayDirY < 0) texX = texWidth - texX - 1;

      // How much to increase the texture coordinate per screen pixel
      fixed_point_t step =
          FP_Div(FP_intToFP(texHeight), FP_intToFP(lineHeight));
      // Starting texture coordinate
      fixed_point_t texPos = FP_Mult(
          FP_intToFP(drawStart - (screenHeight >> 1) + (lineHeight >> 1)),
          step);

      // TMP fix for first line not rendering correctly and prevent crash when
      // start > screen end
      if (RAY_linex > 0 && drawStart < screenHeight) {
        RAYFX_drawTextureVertLine(texPos, step, texX, RAY_linex);
      }
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
        GFX_clearPXframebuffer();
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