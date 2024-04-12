/*
* SPI Flash library
* Contains functions to interact with the Winbond W25Q128 SPI Flash chip
*/

// Sets SPI0_CS low
void spiflash_begin_transfer()
{
  asm(
      "; Backup regs\n"
      "push r1\n"
      "push r2\n"

      "load32 0xC02729 r2   ; r2 = 0xC02729\n"

      "load 0 r1            ; r1 = 0 (enable)\n"
      "write 0 r2 r1        ; write to SPI0_CS\n"

      "; Restore regs\n"
      "pop r2\n"
      "pop r1\n"
      );
}

// Sets SPI0_CS high
void spiflash_end_transfer()
{
  asm(
      "; Backup regs\n"
      "push r1\n"
      "push r2\n"

      "load32 0xC02729 r2   ; r2 = 0xC02729\n"

      "load 1 r1            ; r1 = 1 (disable)\n"
      "write 0 r2 r1        ; write to SPI0_CS\n"

      "; Restore regs\n"
      "pop r2\n"
      "pop r1\n"
      );
}

// Write dataByte and return read value
// Write 0x00 for a read
// Writes byte over SPI0
word spiflash_transfer(word dataByte)
{
  word retval = 0;
  asm(
      "load32 0xC02728 r2   ; r2 = 0xC02728\n"
      "write 0 r2 r4        ; write r4 over SPI0\n"
      "read 0 r2 r2         ; read return value\n"
      "write -4 r14 r2      ; write to stack to return\n"
      );
  return retval;
}

// Disable manual operation and set SPI0 back to QSPI mode
// This allows SPI content to be read directly from memory at high speeds
// However, this disables writing and other manual operations
void spiflash_qspi()
{
  // Send QSPI command
  spiflash_begin_transfer();
  spiflash_transfer(0xEB);
  spiflash_end_transfer();

  // Disable SPI0 (connects SPIreader.v and disconnects simpleSPI.v)
  word *p = (word *) 0xC0272A; // Set address (SPI0 enable)
  *p = 0; // Write value
  delay(10);
}

// Initialize manual operation of SPI flash by enabling SPI0 and resetting the chip
// This is needed to get out of continuous read mode and allow manual commands
void spiflash_init()
{
  // Already set CS high before enabling SPI0
  spiflash_end_transfer();

  // Enable SPI0 (connects simpleSPI.v and disconnects SPIreader.v)
  word *p = (word *) 0xC0272A; // Set address (SPI0 enable)
  *p = 1; // Write value
  delay(10);

  // Reset to get out of continuous read mode
  spiflash_begin_transfer();
  spiflash_transfer(0x66);
  spiflash_end_transfer();
  delay(1);
  spiflash_begin_transfer();
  spiflash_transfer(0x99);
  spiflash_end_transfer();
  delay(1);
  spiflash_begin_transfer();
  spiflash_transfer(0x66);
  spiflash_end_transfer();
  delay(1);
  spiflash_begin_transfer();
  spiflash_transfer(0x99);
  spiflash_end_transfer();
  delay(1);
}

// Reads manufacturer and device IDs and prints them over UART
// Should print 239 as Winbond manufacturer, and 23 for W25Q128 device ID
void spiflash_read_chip_ids()
{
  spiflash_begin_transfer();
  spiflash_transfer(0x90);
  spiflash_transfer(0);
  spiflash_transfer(0);
  spiflash_transfer(0);
  word manufacturer = spiflash_transfer(0);
  word deviceID = spiflash_transfer(0);
  spiflash_end_transfer();

  uprint("Manufacturer ID: ");
  uprintDec(manufacturer);
  uprint("\n");

  uprint("Device ID: ");
  uprintDec(deviceID);
  uprint("\n");
}

// Enables write operation on the chip
void spiflash_enable_write()
{
  spiflash_begin_transfer();
  spiflash_transfer(0x06);
  spiflash_end_transfer();
}

// Reads len bytes from addr and stores them in output
// If bytes_to_word is 1, then each 4 bytes are shifted into one word in output with len in words
void spiflash_read_from_address(word* output, word addr, word len, word bytes_to_word)
{
  spiflash_begin_transfer();
  spiflash_transfer(0x03);
  spiflash_transfer(addr >> 16);
  spiflash_transfer(addr >> 8);
  spiflash_transfer(addr);

  if (bytes_to_word)
  {
    len = len << 2;

    word i;
    for (i = 0; i < len; i++)
    {
      if ((i & 3) == 0)
      {
        output[i >> 2] = 0;
      }
      output[i >> 2] |= spiflash_transfer(0) << (24 - ((i & 3) << 3));
    }
  }
  else
  {
    word i;
    for (i = 0; i < len; i++)
    {
      output[i] = spiflash_transfer(0);
    }
  }

  spiflash_end_transfer();
}

// Writes up to 64 words as 256 bytes to the given address
// Address must be aligned to a page boundary
void spiflash_write_page_in_words(word* input, word addr, word len)
{
  // Check if length exceeds page size
  if (len > 64)
  {
    uprintln("Error: cannot write more than a page!");
    return;
  }

  // Check if address is aligned to a page boundary
  if (addr & 0x3F)
  {
    uprintln("Error: address must be aligned to a page boundary!");
    return;
  }

  spiflash_enable_write();

  word status = spiflash_read_status_reg(1);

  // Check if write is enabled
  if ((status & 0x2) == 0)
  {
    uprintln("WE disabled!");
    return;
  }

  spiflash_begin_transfer();
  spiflash_transfer(0x02);
  spiflash_transfer(addr >> 16);
  spiflash_transfer(addr >> 8);
  spiflash_transfer(addr);

  word i;
  for (i = 0; i < len; i++)
  {
    spiflash_transfer(input[i] >> 24);
    spiflash_transfer(input[i] >> 16);
    spiflash_transfer(input[i] >> 8);
    spiflash_transfer(input[i]);
  }

  spiflash_end_transfer();

  // Wait for busy bit to be 0
  status = 1;

  while((status & 0x1) == 1) 
  {
    status = spiflash_read_status_reg(1);
  }
}

// Reads status register 1, 2, or 3
word spiflash_read_status_reg(word reg_idx)
{
  if (reg_idx < 1 || reg_idx > 3)
    return 0;

  spiflash_begin_transfer();

  word command;
  switch (reg_idx)
  {
    case 1:
      command = 0x05;
      break;
    case 2:
      command = 0x35;
      break;
    case 3:
      command = 0x15;
      break;
  }
  spiflash_transfer(command);
  word status = spiflash_transfer(0);
  spiflash_end_transfer();

  return status;
}

// Executes sector erase operation
// Erases the 4KiB sector of the given address
// (e.g. addr 4096 erases the second sector)
// This is the smallest unit that can be erased
// Returns 1 on success
word spiflash_sector_erase(word addr)
{
  spiflash_enable_write();

  word status = spiflash_read_status_reg(1);

  // Check if write is enabled
  if ((status & 0x2) == 0)
  {
    uprintln("WE disabled!");
    return 0;
  }

  // Send command
  spiflash_begin_transfer();
  spiflash_transfer(0x20);
  spiflash_transfer(addr >> 16);
  spiflash_transfer(addr >> 8);
  spiflash_transfer(addr);
  spiflash_end_transfer();


  // Wait for busy bit to be 0
  status = 1;

  while((status & 0x1) == 1) 
  {
    status = spiflash_read_status_reg(1);
  }

  return 1;
}

// Executes chip erase operation
// TAKES AT LEAST 20 SECONDS!
// Returns 1 on success
word spiflash_chip_erase()
{
  spiflash_enable_write();

  word status = spiflash_read_status_reg(1);

  // Check if write is enabled
  if ((status & 0x2) == 0)
  {
    uprintln("WE disabled!");
    return 0;
  }

  // Send erase chip command
  spiflash_begin_transfer();
  spiflash_transfer(0xC7);
  spiflash_end_transfer();

  // Wait for busy bit to be 0
  status = 1;

  while((status & 0x1) == 1) 
  {
    delay(100);
    status = spiflash_read_status_reg(1);
  }

  return 1;
}
