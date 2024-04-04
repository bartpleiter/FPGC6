// Simple calculator to integers

#define word char

#include "lib/math.c"
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

word CALC_a = 0;
word CALC_b = 0;

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
        bdos_print("Result =      ");
        bdos_printdec(CALC_a + CALC_b);
        CALC_state = CALC_STATE_INPUTA;
        bdos_print("\n\nInput A:      ");
      }

      else if (c == '-')
      {
        bdos_println("-");
        bdos_print("Result =      ");
        bdos_printdec(CALC_a - CALC_b);
        CALC_state = CALC_STATE_INPUTA;
        bdos_print("\n\nInput A:      ");
      }

      else if (c == '*')
      {
        bdos_println("*");
        bdos_print("Result =      ");
        bdos_printdec(CALC_a * CALC_b);
        CALC_state = CALC_STATE_INPUTA;
        bdos_print("\n\nInput A:      ");
      }

      else if (c == '/')
      {
        bdos_println("/");
        bdos_print("Result =      ");
        bdos_printdec( MATH_div(CALC_a, CALC_b) );
        CALC_state = CALC_STATE_INPUTA;
        bdos_print("\n\nInput A:      ");
      }

      else if (c == '%')
      {
        bdos_println("%");
        bdos_print("Result =      ");
        bdos_printdec( MATH_mod(CALC_a, CALC_b) );
        CALC_state = CALC_STATE_INPUTA;
        bdos_print("\n\nInput A:      ");
      }

      else if (c == 'u')
      {
        bdos_println("u/");
        bdos_print("Result =      ");
        bdos_printdec( MATH_divU(CALC_a, CALC_b) );
        CALC_state = CALC_STATE_INPUTA;
        bdos_print("\n\nInput A:      ");
      }

      else if (c == 'w')
      {
        bdos_println("u%");
        bdos_print("Result =      ");
        bdos_printdec( MATH_modU(CALC_a, CALC_b) );
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
    else if ( (c >= '0' && c <= '9') || c == '-')
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
            
            CALC_a = strToInt(CALC_buffer);
            CALC_buffer_idx = 0;

            bdos_print("\nInput B:      ");
            CALC_state = CALC_STATE_INPUTB;
          }
          break;

        case CALC_STATE_INPUTB:
          if (CALC_buffer_idx > 0)
          {
            bdos_printc('\n');

            CALC_b = strToInt(CALC_buffer);
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
  bdos_println("Integer calculator test\n");

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