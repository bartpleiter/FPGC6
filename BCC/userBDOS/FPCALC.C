// Simple calculator to test floating points

#define word char

#include "LIB/MATH.C"
#include "LIB/FP.C"
#include "LIB/STDLIB.C"
#include "LIB/SYS.C"

#define CALC_BUFFER_MAX_LEN 32

#define CALC_STATE_INPUTSTART 0
#define CALC_STATE_INPUTA 1
#define CALC_STATE_INPUTB 2
#define CALC_STATE_INPUTOP 3

// The current number that is being typed
char CALC_buffer[CALC_BUFFER_MAX_LEN];
word CALC_buffer_idx = 0;

fixed_point_t CALC_a = 0;
fixed_point_t CALC_b = 0;

char CALC_state = CALC_STATE_INPUTSTART;

word calcLoop()
{
  if (CALC_state == CALC_STATE_INPUTSTART)
  {
    BDOS_PrintConsole("Input A:      ");
    CALC_state = CALC_STATE_INPUTA;
  }


  if (HID_FifoAvailable())
  {
    word c = HID_FifoRead();

    if (CALC_state == CALC_STATE_INPUTOP)
    {
      if (c == '+')
      {
        BDOS_PrintlnConsole("+");
        char buffer[24];
        FP_FPtoString(CALC_a + CALC_b, buffer, 5);
        BDOS_PrintConsole("Result =      ");
        BDOS_PrintlnConsole(buffer);
        CALC_state = CALC_STATE_INPUTA;
        BDOS_PrintConsole("\n\nInput A:      ");
      }

      else if (c == '-')
      {
        BDOS_PrintlnConsole("-");
        char buffer[24];
        FP_FPtoString(CALC_a - CALC_b, buffer, 5);
        BDOS_PrintConsole("Result =      ");
        BDOS_PrintlnConsole(buffer);
        CALC_state = CALC_STATE_INPUTA;
        BDOS_PrintConsole("\n\nInput A:      ");
      }

      else if (c == '*')
      {
        BDOS_PrintlnConsole("*");
        char buffer[24];
        FP_FPtoString(FP_Mult(CALC_a, CALC_b), buffer, 5);
        BDOS_PrintConsole("Result =      ");
        BDOS_PrintlnConsole(buffer);
        CALC_state = CALC_STATE_INPUTA;
        BDOS_PrintConsole("\n\nInput A:      ");
      }

      else if (c == 0x1b) // escape
      {
        BDOS_PrintcConsole('\n');
        return 1;
      }
    }
    // number input, dot or sign
    else if ( (c >= '0' && c <= '9') || c == '.' || c == '-')
    {
      if (CALC_state == CALC_STATE_INPUTA || CALC_state == CALC_STATE_INPUTB)
      {
        // add to buffer and print character
        CALC_buffer[CALC_buffer_idx] = c;
        CALC_buffer_idx++;
        CALC_buffer[CALC_buffer_idx] = 0; // terminate
        BDOS_PrintcConsole(c);
      }
    }
    else if (c == 0x8) // backspace
    {
      if (CALC_state == CALC_STATE_INPUTA || CALC_state == CALC_STATE_INPUTB)
      {
        // replace last char in buffer by 0 (if not at start)
        if (CALC_buffer_idx != 0)
        {
          CALC_buffer_idx--;
          CALC_buffer[CALC_buffer_idx] = 0;
          BDOS_PrintcConsole(c);
        }
      }
    }
    else if (c == 0xa) // newline/enter
    {
      switch(CALC_state)
      {
        case CALC_STATE_INPUTA:
          if (CALC_buffer_idx > 0)
          {
            BDOS_PrintcConsole('\n');

            CALC_a = FP_StringToFP(CALC_buffer);
            CALC_buffer_idx = 0;

            BDOS_PrintConsole("Input B:      ");
            CALC_state = CALC_STATE_INPUTB;
          }
          break;

        case CALC_STATE_INPUTB:
          if (CALC_buffer_idx > 0)
          {
            BDOS_PrintcConsole('\n');

            CALC_b = FP_StringToFP(CALC_buffer);
            CALC_buffer_idx = 0;

            BDOS_PrintConsole("Operation:    ");
            CALC_state = CALC_STATE_INPUTOP;
          }
          break;
      }
    }
    else if (c == 0x1b) // escape
    {
      BDOS_PrintcConsole('\n');
      return 1;
    }
  }

  return 0;
}

int main() 
{
  BDOS_PrintlnConsole("Fixed-point calculator test\n");

  word stop = 0;
  while (!stop)
  {
    stop = calcLoop();
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