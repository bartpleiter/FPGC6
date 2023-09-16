/*
* Raycaster, based on lodev engine tutorial
*/

#define word char

#include "LIB/MATH.C"
#include "LIB/STDLIB.C"
#include "LIB/SYS.C"
#include "LIB/GFX.C"
#include "LIB/FP.C"

#define screenWidth 320
#define screenHeight 240
#define mapWidth 24
#define mapHeight 24

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

#define FB_ADDR 0xD00000

// Framebuffer. fb[Y][X] (bottom right is [239][319])
char (*fb)[screenWidth] = (char (*)[screenWidth]) FB_ADDR;

word quitGame = 0;

word worldMap[mapWidth][mapHeight]=
{
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,2,2,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,3,0,0,0,3,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,2,0,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,5,0,0,0,0,0,0,1},
  {1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,0,0,0,5,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

void RAYFX_drawVertLine(word x, word start, word end, char color)
{
  word y = 0;
  while (y < screenHeight)
  {
    if (y >= start && y <= end)
    {
      fb[y][x] = color;
    }
    else
    {
      fb[y][x] = 0;
    }
    y++;
  }
}

int main() 
{
  // clear screen from text
  GFX_clearWindowtileTable();
  GFX_clearWindowpaletteTable();
  GFX_clearBGtileTable();
  GFX_clearBGpaletteTable();

  // tmp precalculated values for rotation
  fixed_point_t sinval = FP_StringToFP("-0.0998");
  fixed_point_t cosval = FP_StringToFP("0.995");

  //x and y start position
  fixed_point_t posX = FP_intToFP(12);
  fixed_point_t posY = FP_intToFP(12);

  //initial direction vector
  fixed_point_t dirX = FP_intToFP(-1);
  fixed_point_t dirY = 0;

  //the 2d raycaster version of camera plane
  fixed_point_t planeX = 0;
  fixed_point_t planeY = FP_StringToFP("0.66");

  fixed_point_t time = 0; //time of current frame
  fixed_point_t oldTime = 0; //time of previous frame

  while(!quitGame)
  {
    word x;
    for(x = 0; x < screenWidth; x++)
    {
      //calculate ray position and direction
      fixed_point_t cameraX = FP_Div(FP_intToFP(x<<1), FP_intToFP(screenWidth)) - FP_intToFP(1); //x-coordinate in camera space
      fixed_point_t rayDirX = dirX + FP_Mult(planeX, cameraX);
      fixed_point_t rayDirY = dirY + FP_Mult(planeY, cameraX);

      //which box of the map we are in
      word mapX = FP_FPtoInt(posX);
      word mapY = FP_FPtoInt(posY);

      //length of ray from current position to next x or y-side
      fixed_point_t sideDistX;
      fixed_point_t sideDistY;

      //length of ray from one x or y-side to next x or y-side
      //these are derived as:
      //deltaDistX = sqrt(1 + (rayDirY * rayDirY) / (rayDirX * rayDirX))
      //deltaDistY = sqrt(1 + (rayDirX * rayDirX) / (rayDirY * rayDirY))
      //which can be simplified to abs(|rayDir| / rayDirX) and abs(|rayDir| / rayDirY)
      //where |rayDir| is the length of the vector (rayDirX, rayDirY). Its length,
      //unlike (dirX, dirY) is not 1, however this does not matter, only the
      //ratio between deltaDistX and deltaDistY matters, due to the way the DDA
      //stepping further below works. So the values can be computed as below.
      // Division through zero is prevented by setting the result to a very high value
      fixed_point_t deltaDistX = (rayDirX == 0) ? 1<<30 : MATH_abs( FP_Div(FP_intToFP(1), rayDirX));
      fixed_point_t deltaDistY = (rayDirY == 0) ? 1<<30 : MATH_abs( FP_Div(FP_intToFP(1), rayDirY));

      fixed_point_t perpWallDist;

      //what direction to step in x or y-direction (either +1 or -1)
      word stepX;
      word stepY;

      word hit = 0; //was there a wall hit?
      word side; //was a NS or a EW wall hit?

      //calculate step and initial sideDist
      if(rayDirX < 0)
      {
        stepX = -1;
        sideDistX = FP_Mult((posX - FP_intToFP(mapX)), deltaDistX);
      }
      else
      {
        stepX = 1;
        sideDistX = FP_Mult((FP_intToFP(mapX + 1) - posX), deltaDistX);
      }
      if(rayDirY < 0)
      {
        stepY = -1;
        sideDistY = FP_Mult((posY - FP_intToFP(mapY)), deltaDistY);
      }
      else
      {
        stepY = 1;
        sideDistY = FP_Mult((FP_intToFP(mapY + 1) - posY), deltaDistY);
      }

      //perform DDA
      while(hit == 0)
      {
        //jump to next map square, either in x-direction, or in y-direction
        if(sideDistX < sideDistY)
        {
          sideDistX += deltaDistX;
          mapX += stepX;
          side = 0;
        }
        else
        {
          sideDistY += deltaDistY;
          mapY += stepY;
          side = 1;
        }
        //Check if ray has hit a wall
        if(worldMap[mapX][mapY] > 0) hit = 1;
      }

      //Calculate distance projected on camera direction. This is the shortest distance from the point where the wall is
      //hit to the camera plane. Euclidean to center camera point would give fisheye effect!
      //This can be computed as (mapX - posX + (1 - stepX) / 2) / rayDirX for side == 0, or same formula with Y
      //for size == 1, but can be simplified to the code below thanks to how sideDist and deltaDist are computed:
      //because they were left scaled to |rayDir|. sideDist is the entire length of the ray above after the multiple
      //steps, but we subtract deltaDist once because one step more into the wall was taken above.
      if(side == 0) perpWallDist = (sideDistX - deltaDistX);
      else          perpWallDist = (sideDistY - deltaDistY);

      //Calculate height of line to draw on screen
      word lineHeight = FP_FPtoInt(FP_Div(FP_intToFP(screenHeight), perpWallDist));

      //calculate lowest and highest pixel to fill in current stripe
      word drawStart = - (lineHeight >> 1) + (screenHeight >> 1);
      if(drawStart < 0) drawStart = 0;
      word drawEnd = (lineHeight >> 1) + (screenHeight >> 1);
      if(drawEnd >= screenHeight) drawEnd = screenHeight - 1;

      //choose wall color
      //give x and y sides different brightness
      char color;
      switch(worldMap[mapX][mapY])
      {
        case 1:  color = (side == 1) ? COLOR_DARK_RED : COLOR_RED;    break;
        case 2:  color = (side == 1) ? COLOR_DARK_GREEN : COLOR_GREEN;    break;
        case 3:  color = (side == 1) ? COLOR_DARK_BLUE : COLOR_BLUE;   break;
        case 4:  color = (side == 1) ? COLOR_GREY : COLOR_WHITE;  break;
        default: color = (side == 1) ? COLOR_DARK_YELLOW : COLOR_YELLOW; break;
      }

      //draw the pixels of the stripe as a vertical line
      RAYFX_drawVertLine(x, drawStart, drawEnd, color);
    }

    
    // TMP go right
    //both camera direction and camera plane must be rotated
    fixed_point_t oldDirX = dirX;
    dirX = FP_Mult(dirX, cosval) - FP_Mult(dirY , sinval);
    dirY = FP_Mult(oldDirX, sinval) + FP_Mult(dirY, cosval);
    fixed_point_t oldPlaneX = planeX;
    planeX = FP_Mult(planeX, cosval) - FP_Mult(planeY, sinval);
    planeY = FP_Mult(oldPlaneX, sinval) + FP_Mult(planeY, cosval);
    
  }

  return 'q';
}

void interrupt()
{
  // handle all interrupts
  word i = getIntID();
  switch(i)
  {
    case INTID_TIMER1:
      timer1Value = 1; // notify ending of timer1
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