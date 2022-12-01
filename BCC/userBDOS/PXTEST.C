#define word char

#include "LIB/MATH.C"
#include "LIB/STDLIB.C"
#include "LIB/SYS.C"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define FB_ADDR 0xD00000

// Framebuffer. fb[Y][X] (bottom right is [239][319])
char (*fb)[SCREEN_WIDTH] = (char (*)[SCREEN_WIDTH]) FB_ADDR;

int main() 
{

  word pxcount = 0;
  word x, y;
  


  while (1)
  {
    if (HID_FifoAvailable())
    {
      word c = HID_FifoRead();

      if (c == 'a')
      {
        for (x = 0; x < SCREEN_WIDTH; x++)
        {
          for (y = 0; y < SCREEN_HEIGHT; y++)
          {
            fb[y][x] = 0;
          }
        }
      }

      if (c == 'b')
      {
        for (x = 0; x < SCREEN_WIDTH; x++)
        {
          for (y = 0; y < SCREEN_HEIGHT; y++)
          {
            fb[y][x] = x;
          }
        }
      }

      if (c == 'c')
      {
        for (x = 0; x < SCREEN_WIDTH; x++)
        {
          for (y = 0; y < SCREEN_HEIGHT; y++)
          {
            fb[y][x] = y;
          }
        }
      }

      if (c == 'd')
      {
        pxcount = 0;
        for (x = 0; x < SCREEN_WIDTH; x++)
        {
          for (y = 0; y < SCREEN_HEIGHT; y++)
          {
            fb[y][x] = pxcount;
            pxcount++;
          }
        }
      }

      if (c == 27) // escape
      {
        for (x = 0; x < SCREEN_WIDTH; x++)
        {
          for (y = 0; y < SCREEN_HEIGHT; y++)
          {
            fb[y][x] = 0;
          }
        }
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