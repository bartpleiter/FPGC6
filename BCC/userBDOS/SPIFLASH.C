/*
Test program for SPI flash memory
*/

#define word char

#include "LIB/MATH.C"
#include "LIB/STDLIB.C"
#include "LIB/SYS.C"

// Sets SPI0_CS low
void SpiBeginTransfer()
{
  asm(
      "; Backup regs\n"
      "push r1\n"
      "push r2\n"

      "load32 0xC02729 r2                 ; r2 = 0xC02729\n"

      "load 0 r1                          ; r1 = 0 (enable)\n"
      "write 0 r2 r1                      ; write to SPI0_CS\n"

      "; Restore regs\n"
      "pop r2\n"
      "pop r1\n"
      );
}

// Sets SPI0_CS high
void SpiEndTransfer()
{
  asm(
      "; Backup regs\n"
      "push r1\n"
      "push r2\n"

      "load32 0xC02729 r2                 ; r2 = 0xC02729\n"

      "load 1 r1                          ; r1 = 1 (disable)\n"
      "write 0 r2 r1                      ; write to SPI0_CS\n"

      "; Restore regs\n"
      "pop r2\n"
      "pop r1\n"
      );
}

// Write dataByte and return read value
// Write 0x00 for a read
// Writes byte over SPI0
word SpiTransfer(word dataByte)
{
  word retval = 0;
  asm(
      "load32 0xC02728 r2                 ; r2 = 0xC02728\n"
      "write 0 r2 r4                      ; write r4 over SPI0\n"
      "read 0 r2 r2                       ; read return value\n"
      "write -4 r14 r2                    ; write to stack to return\n"
      );

  return retval;
}

// Inits SPI by enabling SPI0 and resetting the chip
void initSPI()
{
    // Already set CS high before enabling SPI0
    SpiEndTransfer();

    // Enable SPI0
    word *p = (word *) 0xC0272A; // Set address (SPI0 enable)
    *p = 1; // Write value
    delay(10);

    // Reset to get out of continuous read mode
    SpiBeginTransfer();
    SpiTransfer(0x66);
    SpiEndTransfer();
    delay(1);
    SpiBeginTransfer();
    SpiTransfer(0x99);
    SpiEndTransfer();
    delay(1);
    SpiBeginTransfer();
    SpiTransfer(0x66);
    SpiEndTransfer();
    delay(1);
    SpiBeginTransfer();
    SpiTransfer(0x99);
    SpiEndTransfer();
    delay(1);
}

// Should print 239 as Winbond manufacturer, and 23 for W25Q128 device ID
void readDeviceID()
{
    SpiBeginTransfer();
    SpiTransfer(0x90);
    SpiTransfer(0);
    SpiTransfer(0);
    SpiTransfer(0);
    word manufacturer = SpiTransfer(0);
    word deviceID = SpiTransfer(0);
    SpiEndTransfer();

    BDOS_PrintConsole("Manufacturer ID: ");
    BDOS_PrintDecConsole(manufacturer);
    BDOS_PrintConsole("\n");

    BDOS_PrintConsole("Device ID: ");
    BDOS_PrintDecConsole(deviceID);
    BDOS_PrintConsole("\n");
}

void enableWrite()
{
    SpiBeginTransfer();
    SpiTransfer(0x06);
    SpiEndTransfer();
}


void read_from_address(word addr, word len)
{
    SpiBeginTransfer();
    SpiTransfer(0x03);
    SpiTransfer(addr >> 16);
    SpiTransfer(addr >> 8);
    SpiTransfer(addr);


    word i;
    for (i = 0; i < len; i++)
    {
        word x = SpiTransfer(0x00);
        // delay a bit here
        uprintDec(x);
        uprint(" ");
    }
    uprint("\n");

    SpiEndTransfer();
}

word readStatusRegister(word reg)
{
    SpiBeginTransfer();

    if (reg == 2)
        SpiTransfer(0x35);
    else if (reg == 3)
        SpiTransfer(0x15);
    else
        SpiTransfer(0x05);

    word status = SpiTransfer(0);
    SpiEndTransfer();

    return status;
}

// Executes chip erase operation
// TAKES AT LEAST 20 SECONDS!
// Returns 1 on success
word chipErase()
{
    enableWrite();

    word status = readStatusRegister(1);

    // Check if write is enabled
    if ((status & 0x2) == 0)
    {
      BDOS_PrintlnConsole("WE disabled!");
        return 0;
    }

    // Send command
    SpiBeginTransfer();
    SpiTransfer(0xC7);
    SpiEndTransfer();


    // Wait for busy bit to be 0
    status = 1;

    while((status & 0x1) == 1) 
    {
      delay(100);
        status = readStatusRegister(1);
    }

    BDOS_PrintlnConsole("Chip erase complete!");

    return 1;
}

void dump_memory()
{
  word page_max = 9000;//65536;
  word page;
  for (page = 0; page < page_max; page++)
  {
    uprintDec(page);
    uprint(": ");
    word addr = page * 256;
    SpiBeginTransfer();
    SpiTransfer(0x03);
    SpiTransfer(addr >> 16);
    SpiTransfer(addr >> 8);
    SpiTransfer(addr);

    word i;
    for (i = 0; i < 16; i++)
    {
        word x = SpiTransfer(0x00);
        
        uprintDec(x);
        uprint(" ");
    }
    uprint("\n");

    SpiEndTransfer();
  }

}

int main() 
{
  // Clear UART screen:
  uprintc(0x1B);
  uprintc(0x5B);
  uprintc(0x32);
  uprintc(0x4A);

  BDOS_PrintConsole("SPI Flash test\n");

  initSPI();
  readDeviceID();

  //chipErase();

  word addr = 2000000;

  read_from_address(addr, 512);

  //dump_memory();

  return 'q';
}

void interrupt()
{
  // Handle all interrupts
  word i = getIntID();
  switch(i)
  {
    case INTID_TIMER1:
      timer1Value = 1; // Notify ending of timer1
      break;

    default:
      break;
  }
}