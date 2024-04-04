// Simple calculator to test floating points

#define word char

#include "lib/math.c"
#include "lib/fp.c"
#include "lib/stdlib.c"
#include "lib/sys.c"

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
    bdos_print("Input A:      ");
    CALC_state = CALC_STATE_INPUTA;
  }


  if (hid_checkfifo())
  {
    word c = hid_fiforead();

    if (CALC_state == CALC_STATE_INPUTOP)
    {
      if (c == '+')
      {
        bdos_println("+");
        char buffer[24];
        FP_FPtoString(CALC_a + CALC_b, buffer, 5);
        bdos_print("Result =      ");
        bdos_println(buffer);
        CALC_state = CALC_STATE_INPUTA;
        bdos_print("\n\nInput A:      ");
      }

      else if (c == '-')
      {
        bdos_println("-");
        char buffer[24];
        FP_FPtoString(CALC_a - CALC_b, buffer, 5);
        bdos_print("Result =      ");
        bdos_println(buffer);
        CALC_state = CALC_STATE_INPUTA;
        bdos_print("\n\nInput A:      ");
      }

      else if (c == '*')
      {
        bdos_println("*");
        char buffer[24];
        FP_FPtoString(FP_Mult(CALC_a, CALC_b), buffer, 5);
        bdos_print("Result =      ");
        bdos_println(buffer);
        CALC_state = CALC_STATE_INPUTA;
        bdos_print("\n\nInput A:      ");
      }

      else if (c == '/')
      {
        bdos_println("/");
        char buffer[24];
        FP_FPtoString(FP_Div(CALC_a, CALC_b), buffer, 5);
        bdos_print("Result =      ");
        bdos_println(buffer);
        CALC_state = CALC_STATE_INPUTA;
        bdos_print("\n\nInput A:      ");
      }

      else if (c == 0x1b) // escape
      {
        bdos_printc('\n');
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
        bdos_printc(c);
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
          bdos_printc(c);
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
            bdos_printc('\n');

            CALC_a = FP_StringToFP(CALC_buffer);
            CALC_buffer_idx = 0;

            bdos_print("Input B:      ");
            CALC_state = CALC_STATE_INPUTB;
          }
          break;

        case CALC_STATE_INPUTB:
          if (CALC_buffer_idx > 0)
          {
            bdos_printc('\n');

            CALC_b = FP_StringToFP(CALC_buffer);
            CALC_buffer_idx = 0;

            bdos_print("Operation:    ");
            CALC_state = CALC_STATE_INPUTOP;
          }
          break;
      }
    }
    else if (c == 0x1b) // escape
    {
      bdos_printc('\n');
      return 1;
    }
  }

  return 0;
}

int main() 
{
  bdos_println("Fixed-point calculator test\n");

  word stop = 0;
  while (!stop)
  {
    stop = calcLoop();
  }

  return 'q';
}

void interrupt()
{
  // Handle all interrupts
  word i = get_int_id();
  switch(i)
  {
    case INTID_TIMER1:
      timer1Value = 1;  // Notify ending of timer1
      break;
  }
}