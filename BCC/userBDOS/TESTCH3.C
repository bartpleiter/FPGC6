// Test CH376

#define word char

#include "LIB/MATH.C"
#include "LIB/SYS.C"
#include "LIB/STDLIB.C"

#define FS_CMD_GET_IC_VER       0x01

void BOTTOM_spiBeginTransfer()
{
  asm(
    "; backup regs\n"
    "push r1\n"
    "push r2\n"

    "load32 0xC0272C r2      ; r2 = 0xC0272C\n"

    "load 0 r1              ; r1 = 0 (enable)\n"
    "write 0 r2 r1            ; write to SPI1_CS\n"

    "; restore regs\n"
    "pop r2\n"
    "pop r1\n"
    );
}

void BOTTOM_spiEndTransfer()
{
  asm(
    "; backup regs\n"
    "push r1\n"
    "push r2\n"

    "load32 0xC0272C r2      ; r2 = 0xC0272C\n"

    "load 1 r1              ; r1 = 1 (disable)\n"
    "write 0 r2 r1            ; write to SPI1_CS\n"

    "; restore regs\n"
    "pop r2\n"
    "pop r1\n"
    );
}

word BOTTOM_spiTransfer(word dataByte)
{
  word retval = 0;
  asm(
    "load32 0xC0272B r2       ; r2 = 0xC0272B\n"
    "write 0 r2 r4            ; write r4 over SPI1\n"
    "read 0 r2 r2             ; read return value\n"
    "write -4 r14 r2          ; write to stack to return\n"
    );

  return retval;
}


void TOP_spiBeginTransfer()
{
    asm(
        "; backup regs\n"
        "push r1\n"
        "push r2\n"

        "load32 0xC0272F r2                 ; r2 = 0xC0272F\n"

        "load 0 r1                          ; r1 = 0 (enable)\n"
        "write 0 r2 r1                      ; write to SPI2_CS\n"

        "; restore regs\n"
        "pop r2\n"
        "pop r1\n"
        );
}

void TOP_spiEndTransfer()
{
    asm(
        "; backup regs\n"
        "push r1\n"
        "push r2\n"

        "load32 0xC0272F r2                 ; r2 = 0xC0272F\n"

        "load 1 r1                          ; r1 = 1 (disable)\n"
        "write 0 r2 r1                      ; write to SPI2_CS\n"

        "; restore regs\n"
        "pop r2\n"
        "pop r1\n"
        );
}

word TOP_spiTransfer(word dataByte)
{
    word retval = 0;
    asm(
        "load32 0xC0272E r2                 ; r2 = 0xC0272E\n"
        "write 0 r2 r4                      ; write r4 over SPI2\n"
        "read 0 r2 r2                       ; read return value\n"
        "write -4 r14 r2                    ; write to stack to return\n"
        );

    return retval;
}

word BOTTOM_getICver()
{
  BOTTOM_spiBeginTransfer();
  BOTTOM_spiTransfer(FS_CMD_GET_IC_VER);

  word icVer = BOTTOM_spiTransfer(0x00);
  BOTTOM_spiEndTransfer();

  return icVer;
}

word TOP_getICver()
{
  TOP_spiBeginTransfer();
  TOP_spiTransfer(FS_CMD_GET_IC_VER);

  word icVer = TOP_spiTransfer(0x00);
  TOP_spiEndTransfer();

  return icVer;
}


int main() 
{
  BDOS_PrintHexConsole(BOTTOM_getICver());
  BDOS_PrintcConsole('\n');

  BDOS_PrintHexConsole(TOP_getICver());
  BDOS_PrintcConsole('\n');

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