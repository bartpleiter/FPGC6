#define word char

#include "LIB/MATH.C"
#include "LIB/STDLIB.C"
#include "LIB/SYS.C"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define FB_ADDR 0xD00000

// Framebuffer. fb[Y][X] (bottom right is [239][319])
char (*fb)[SCREEN_WIDTH] = (char (*)[SCREEN_WIDTH]) FB_ADDR;

char r[] = {7,7,7,7,7,7,7,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,0};
char g[] = {0,1,2,3,4,5,6,7,7,7,7,7,7,7,7,7,7,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0};
char b[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0};

void clearScreen()
{
  word x, y;
  for (x = 0; x < SCREEN_WIDTH; x++)
  {
    for (y = 0; y < SCREEN_HEIGHT; y++)
    {
      fb[y][x] = 0;
    }
  }
}

word colorMap(word i)
{
  word rgb = 0;
  rgb += r[i] << 5;
  rgb += g[i] << 2;
  rgb += b[i];

  return rgb;
}


int main() 
{

  clearScreen();

  word i = 0;
  for (i = 0; i < 24; i++)
  {
    BDOS_PrintcConsole('\n');
  }

  word row = 0;
  word pixel = 0;

  word cx = 0;
  word x = 0;
  word y = 0;
  word x2 = 0;
  word y2 = 0;
  word iter = 0;

  word xmin = -8601;
  word xmax =  2867;
  word ymin = -4915;
  word ymax =  4915;

  /*xmin = MATH_div(xmin, 2);
  xmax = MATH_div(xmax, 2);

  xmin -= 2000;
  xmax -= 2000;

  ymin = MATH_div(ymin, 2);
  ymax = MATH_div(ymax, 2);
  */

  word maxiter = 32;

  word dx = MATH_div((xmax-xmin),310);
  word dy = MATH_div((ymax-ymin),230);

  
  word cy = ymin;
  while(cy <= ymax)
  {
    cx = xmin;
    while(cx <= xmax)
    {
      x = 0;
      y = 0;
      x2 = 0;
      y2 = 0;
      iter = 0;
      while(iter < maxiter)
      {
        if (x2 + y2 > 16384)
        {
          break;
        }

        y = x*y;
        y = y >> 11;
        y += cy;

        x = x2 - y2;
        x += cx;

        x2 = x * x;
        x2 = x2 >> 12;

        y2 = y * y;
        y2 = y2 >> 12;

        iter++;
      }
      //BDOS_PrintcConsole(' ' + iter);
      fb[row][pixel] = colorMap(iter);
      pixel++;
      cx += dx;
    }
    //BDOS_PrintcConsole('\n');
    row++;
    pixel = 0;
    cy += dy;
  }

  while (1)
  {
    if (HID_FifoAvailable())
      {
        word c = HID_FifoRead();

        if (c == 27)
        {
          clearScreen();
          return 'q';
        }
      }
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