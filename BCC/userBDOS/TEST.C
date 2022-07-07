#define word char

#include "LIB/MATH.C"
#include "LIB/STDLIB.C"
#include "LIB/SYS.C"

int main() 
{

  BDOS_PrintConsole("Running user program\n");

  while (1)
  {
    if (HID_FifoAvailable())
    {
      word c = HID_FifoRead();
      if (c == 27) // escape
      {
        return 'q';
      }
      if (c < 255)
      {
        BDOS_PrintcConsole(c);
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