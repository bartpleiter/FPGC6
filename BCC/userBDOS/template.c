#define word char

#include "lib/math.c"
#include "lib/stdlib.c"
#include "lib/sys.c"

int main() 
{
  bdos_println("Template program.");
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