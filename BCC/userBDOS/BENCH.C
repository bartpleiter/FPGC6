// Benchmarking tool

#define word char

#include "LIB/MATH.C"
#include "LIB/STDLIB.C"
#include "LIB/SYS.C"

#define N    256  // Decimals of pi to compute.
#define LEN  854  // (10*N) / 3 + 1
#define TMPMEM_LOCATION 0x440000

word frameCount = 0;

//word a[LEN];
word *a = (char*) TMPMEM_LOCATION;

void spigotPiBench()
{
  frameCount = 0;
  while (frameCount == 0); // wait until next frame to start
  frameCount = 0;

  word j = 0;
  word predigit = 0;
  word nines = 0;
  word x = 0;
  word q = 0;
  word k = 0;
  word len = 0;
  word i = 0;
  word y = 0;

  for(j=N; j; ) 
  {
    q = 0;
    k = LEN+LEN-1;

    for(i=LEN; i; --i) 
    {
      if (j == N)
      {
        x = 20 + q*i;
      }
      else
      {
        x = (10*a[i-1]) + q*i;
      }
      q = MATH_div(x, k);
      a[i-1] = (x-q*k);
      k -= 2;
    }

    k = MATH_mod(x, 10);

    if (k==9)
    {
      ++nines;
    }

    else 
    {
      if (j)
      {
        --j;
        y = predigit+MATH_div(x,10);
        BDOS_PrintDecConsole(y);
      }

      for(; nines; --nines)
      {
        if (j)
        {
          --j;
          if (x >= 10)
          {
            BDOS_PrintcConsole('0');
          }
          else
          {
            BDOS_PrintcConsole('9');
          }
        }
      }

      predigit = k;
    }
  }

  BDOS_PrintConsole("\nPiBench256 took    ");
  BDOS_PrintDecConsole(frameCount);
  BDOS_PrintConsole(" frames\n");
}


// LoopBench: a simple increase and loop bench for a set amount of time.
// reads framecount from memory in the loop.
// no pipeline clears within the loop.
int loopBench()
{
  word retval = 0;
  asm(
      "push r1\npush r2\npush r3\npush r4\n"
      "addr2reg frameCount r2\n"
      "write 0 r2 r0 ; reset frameCount\n"
      "load 0 r4 ; score\n"

      "Label_ASM_Loop:\n"
      "read 0 r2 r3 ; read frameCount\n"
      "slt r3 300 r3 ; TESTDURATION here in frames\n"
      "beq r3 r0 3 ; check if done \n"
      "add r4 1 r4 ; increase score and loop\n"
      "jump Label_ASM_Loop\n"

      "jump Label_ASM_Done\n"
      

      "Label_ASM_Done:\n"
      "or r4 r0 r2 ; set return value\n"
      "write -4 r14 r2 ; write to stack to return\n"
      "pop r4\npop r3\npop r2\npop r1\n"
      );

  return retval;
}


// CountMillionBench: how many frames it takes to do a C for loop to a million
int countMillionBench()
{
  frameCount = 0;
  while (frameCount == 0); // wait until next frame to start
  frameCount = 0;
  int i;
  for (i = 0; i < 1000000; i++);
  return frameCount;
}


int main() 
{

  BDOS_PrintlnConsole("---------------FPGCbench---------------\n");


  BDOS_PrintConsole("LoopBench:         ");
  frameCount = 0;
  while (frameCount == 0); // wait until next frame to start
  BDOS_PrintDecConsole(loopBench());
  BDOS_PrintcConsole('\n');

  BDOS_PrintConsole("\nCountMillionBench: ");
  BDOS_PrintDecConsole(countMillionBench());
  BDOS_PrintConsole(" frames\n");

  BDOS_PrintConsole("\nPiBench256:\n");
  spigotPiBench();

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
      frameCount++;
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