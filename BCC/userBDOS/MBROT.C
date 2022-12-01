#define word char

#include "LIB/MATH.C"
#include "LIB/STDLIB.C"
#include "LIB/SYS.C"
#include "LIB/GFX.C"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define FB_ADDR 0xD00000

// Framebuffer. fb[Y][X] (bottom right is [239][319])
char (*fb)[SCREEN_WIDTH] = (char (*)[SCREEN_WIDTH]) FB_ADDR;

char MBROT_r[] = {7,7,7,7,7,7,7,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,0};
char MBROT_g[] = {0,1,2,3,4,5,6,7,7,7,7,7,7,7,7,7,7,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0};
char MBROT_b[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0};


word MBROT_xmin = -8601;
word MBROT_xmax =  2867;
word MBROT_ymin = -4915;
word MBROT_ymax =  4915;
word MBROT_maxiter = 32;

// Render mandelbrot on display
// INPUT:
//   r4 = dx
//   r5 = dy
void MBROT_render(word dx, word dy)
{
  asm(
  "; backup registers\n"
  "push r1\n"
  "push r2\n"
  "push r3\n"
  "push r4\n"
  "push r5\n"
  "push r6\n"
  "push r7\n"
  "push r8\n"
  "push r9\n"
  "push r10\n"
  "push r11\n"
  "push r12\n"
  "push r13\n"
  "push r14\n"
  "push r15\n"
  );

  asm(
  "; r13,r14,r15 = temp reg\n"
  "load 0 r1            ; r1 = row\n"
  "load 0 r2            ; r2 = pixel\n"
  "load 0 r3            ; r3 = cx\n"
  ";                      r4 = dx\n"
  ";                      r5 = dy\n"
  "load 0 r6            ; r6 = x\n"
  "load 0 r7            ; r7 = y\n"
  "load 0 r8            ; r8 = x2\n"
  "load 0 r9            ; r9 = y2\n"
  "load 0 r10           ; r10 = iter\n"

  "addr2reg MBROT_ymin r15\n"
  "read 0 r15 r11       ; r11 = cy\n"

  "load32 0xD00000 r12  ; r12 = fb[] at 0xD00000\n"

  "MBROT_whileCyLoop:\n"
  "; while(cy <= MBROT_ymax)\n"
    "addr2reg MBROT_ymax r15\n"
    "read 0 r15 r15\n"
    "bles r11 r15 2\n"
    "jump MBROT_whileCyLoop_done\n"

    "addr2reg MBROT_xmin r15\n"
    "read 0 r15 r3       ; cx = MBROT_xmin;\n"

    "MBROT_whileCxLoop:\n"
    "; while(cx <= MBROT_xmax)\n"
      "addr2reg MBROT_xmax r15\n"
      "read 0 r15 r15\n"
      "bles r3 r15 2\n"
      "jump MBROT_whileCxLoop_done\n"

      "load 0 r6            ; x = 0\n"
      "load 0 r7            ; y = 0\n"
      "load 0 r8            ; x2 = 0\n"
      "load 0 r9            ; y2 = 0\n"
      "load 0 r10           ; iter = 0\n"

      "MBROT_whileIterLoop:\n"
        "; while(iter < MBROT_maxiter)\n"
        "addr2reg MBROT_maxiter r15\n"
        "read 0 r15 r15\n"
        "blts r10 r15 2\n"
        "jump MBROT_whileIterLoop_done\n"

        "; break if (x2 + y2 > 16384)\n"
        "add r8 r9 r14        ; x2 + y2\n"
        "load32 16384 r15\n"
        "bles r14 r15 2\n"
        "jump MBROT_whileIterLoop_done\n"

        "mults r6 r7 r7       ; y = x * y;\n"
        "shiftrs r7 11 r7     ; y = y >> 11;\n"
        "add r7 r11 r7        ; y += cy;\n"

        "sub r8 r9 r6         ; x = x2 - y2;\n"
        "add r6 r3 r6         ; x += cx;\n"

        "mults r6 r6 r8       ; x2 = x * x;\n"
        "shiftrs r8 12 r8     ; x2 = x2 >> 12;\n"

        "mults r7 r7 r9       ; y2 = y * y;\n"
        "shiftrs r9 12 r9     ; y2 = y2 >> 12;\n"

        "add r10 1 r10        ; iter++;\n"
        "jump MBROT_whileIterLoop\n"
        "MBROT_whileIterLoop_done:\n"

      "multu r1 320 r13            ; r13 = row * 320\n"
      "add r13 r2 r13              ; r13 += pixel\n"
      "add r13 r12 r13             ; r13 += fb base address\n"

      "; r14 = rgb pixel value\n"
      "addr2reg MBROT_r r15\n"
      "add r15 r10 r15             ; r15 = MBROT_r[iter]\n"
      "read 0 r15 r15\n"
      "shiftl r15 5 r14            ; r14 = r << 5\n"

      "addr2reg MBROT_g r15\n"
      "add r15 r10 r15             ; r15 = MBROT_r[iter]\n"
      "read 0 r15 r15\n"
      "shiftl r15 2 r15            ; r15 = g << 2\n"
      "add r15 r14 r14             ; r14 += g (shifted)\n"

      "addr2reg MBROT_b r15\n"
      "add r15 r10 r15             ; r15 = MBROT_r[iter]\n"
      "read 0 r15 r15\n"
      "add r15 r14 r14             ; r14 += b\n"

      ";write rgb value to pixel address\n"
      "write 0 r13 r14\n"

      "add r2 1 r2      ; pixel++;\n"
      "add r3 r4 r3     ; cx += dx;\n"
      "jump MBROT_whileCxLoop\n"
      "MBROT_whileCxLoop_done:\n"

    "add r1 1 r1          ; row++;\n"
    "load 0 r2            ; pixel = 0;\n"
    "add r11 r5 r11       ; cy += dy;\n"
    "jump MBROT_whileCyLoop\n"
    "MBROT_whileCyLoop_done:\n"
  );

  asm(
  "; restore registers\n"
  "pop r15\n"
  "pop r14\n"
  "pop r13\n"
  "pop r12\n"
  "pop r11\n"
  "pop r10\n"
  "pop r9\n"
  "pop r8\n"
  "pop r7\n"
  "pop r6\n"
  "pop r5\n"
  "pop r4\n"
  "pop r3\n"
  "pop r2\n"
  "pop r1\n"
  );
}

void initGraphics()
{
  GFX_clearWindowtileTable();
  GFX_clearWindowpaletteTable();
  GFX_clearBGtileTable();
  GFX_clearBGpaletteTable();
  GFX_clearPXframebuffer();

  word* paletteAddress = (word*) 0xC00400;
  paletteAddress[1] = 0xC0;
}

void displayInfo()
{
  char* xmin = "X-:               ";
  if (MBROT_xmin < 0)
    xmin[3] = '-';
  itoa(MATH_abs(MBROT_xmin), &xmin[4]);
  GFX_printWindowColored(xmin, 18, 0, 1);

  char* xmax = "X+:               ";
  if (MBROT_xmax < 0)
    xmax[3] = '-';
  itoa(MATH_abs(MBROT_xmax), &xmax[4]);
  GFX_printWindowColored(xmax, 18, 40, 1);

  char* ymin = "Y-:               ";
  if (MBROT_ymin < 0)
    ymin[3] = '-';
  itoa(MATH_abs(MBROT_ymin), &ymin[4]);
  GFX_printWindowColored(ymin, 18, 120, 1);

  char* ymax = "Y+:               ";
  if (MBROT_ymax < 0)
    ymax[3] = '-';
  itoa(MATH_abs(MBROT_ymax), &ymax[4]);
  GFX_printWindowColored(ymax, 18, 160, 1);
}


int main() 
{

  initGraphics();

  word dx = MATH_div((MBROT_xmax-MBROT_xmin),310);
  word dy = MATH_div((MBROT_ymax-MBROT_ymin),230);

  MBROT_render(dx, dy);

  while (1)
  {
    if (HID_FifoAvailable())
      {
        word c = HID_FifoRead();

        if (c == '=') // +
        {
          MBROT_xmin += MATH_div(MATH_abs(MBROT_xmin), 2);
          MBROT_xmax -= MATH_div(MATH_abs(MBROT_xmax), 2);
          MBROT_ymin += MATH_div(MATH_abs(MBROT_ymin), 2);
          MBROT_ymax -= MATH_div(MATH_abs(MBROT_ymax), 2);
        }

        if (c == '-') // -
        {
          MBROT_xmin -= MATH_div(MATH_abs(MBROT_xmin), 2);
          MBROT_xmax += MATH_div(MATH_abs(MBROT_xmax), 2);
          MBROT_ymin -= MATH_div(MATH_abs(MBROT_ymin), 2);
          MBROT_ymax += MATH_div(MATH_abs(MBROT_ymax), 2);
        }

        if (c == 256) // left
        {
          MBROT_xmin -= MATH_div(MATH_abs(MBROT_xmax-MBROT_xmin), 4) + 1;
          MBROT_xmax -= MATH_div(MATH_abs(MBROT_xmax-MBROT_xmin), 4) + 1;
        }

        if (c == 257) // right
        {
          MBROT_xmin += MATH_div(MATH_abs(MBROT_xmax-MBROT_xmin), 4) + 1;
          MBROT_xmax += MATH_div(MATH_abs(MBROT_xmax-MBROT_xmin), 4) + 1;
        }

        if (c == 258) // up
        {
          MBROT_ymin -= MATH_div(MATH_abs(MBROT_ymax-MBROT_ymin), 4) + 1;
          MBROT_ymax -= MATH_div(MATH_abs(MBROT_ymax-MBROT_ymin), 4) + 1;
        }

        if (c == 259) // down
        {
          MBROT_ymin += MATH_div(MATH_abs(MBROT_ymax-MBROT_ymin), 4) + 1;
          MBROT_ymax += MATH_div(MATH_abs(MBROT_ymax-MBROT_ymin), 4) + 1;
        }

        if (c == 27) // escape
        {
          GFX_clearPXframebuffer();
          return 'q';
        }

        word dx = MATH_div((MBROT_xmax-MBROT_xmin),310);
        word dy = MATH_div((MBROT_ymax-MBROT_ymin),230);
        displayInfo();
        MBROT_render(dx, dy);
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