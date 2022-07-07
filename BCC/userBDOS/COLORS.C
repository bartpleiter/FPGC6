// Prints some rgb colors for testing purposes

#define word char

#include "LIB/MATH.C"
#include "LIB/STDLIB.C"
#include "LIB/SYS.C"
#include "LIB/GFX.C"
#include "DATA/COLOR.C"

#define DATA_OFFSET 3

void testTiles()
{
  asm(".dw 0 1 2 3\n");
}

int main() 
{
  GFX_initVram(); // clear all VRAM
  GFX_copyPaletteTable((word)DATA_PALETTE_COLOR);
  GFX_copyPatternTable((word)DATA_PATTERN_COLOR);

  word y = 0;
  word c = 1;

  word x;
  for (x = 0; x < 5; x++)
  {
    GFX_printWindowColored(((word)testTiles) + DATA_OFFSET, 4, GFX_WindowPosFromXY(x*4, y), x + 1);
    GFX_printBGColored(((word)testTiles) + DATA_OFFSET, 4, GFX_BackgroundPosFromXY(x*4, y+2), x + 1);
  }

  // Any key to quit
  while (1)
  {
    if (HID_FifoAvailable())
    {
      word c = HID_FifoRead();
      return 'q';
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