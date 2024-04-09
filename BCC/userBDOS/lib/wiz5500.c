/*
* Wiznet5500 library
* Contains functions to allow for networking
*
* Functions from this library can be used to operate up to 8 sockets
*/

// Wiznet W5500 Op Codes
#define WIZNET_WRITE_COMMON 0x04 //opcode to write to one of the common block of registers
#define WIZNET_READ_COMMON  0x00 //opcode to read one of the common block of registers
#define WIZNET_WRITE_SnR    0x0C // s<<5 (nnn 01 1 00) opcode to write to one of the socket n registers
#define WIZNET_READ_SnR     0x08 // s<<5 (nnn 01 0 00) opcode to read one of the socket n registers
#define WIZNET_WRITE_SnTX   0x14 // s<<5 (nnn 10 1 00) opcode to write to the socket n transmit buffer
#define WIZNET_READ_SnRX    0x18 // s<<5 (nnn 11 0 00) opcode to read from the socket n receive buffer

// Wiznet W5500 Register Addresses
#define WIZNET_MR     0x0000    // Mode
#define WIZNET_GAR    0x0001    // Gateway IP address
#define WIZNET_SUBR   0x0005    // Subnet mask address
#define WIZNET_SHAR   0x0009    // Source MAC address
#define WIZNET_SIPR   0x000F    // Source IP address
#define WIZNET_IR     0x0015    // Interrupt
#define WIZNET_IMR    0x0016    // Interrupt Mask
#define WIZNET_RTR    0x0019    // Timeout address
#define WIZNET_RCR    0x001B    // Retry count
#define WIZNET_UIPR   0x0028    // Unreachable IP address in UDP mode
#define WIZNET_UPORT  0x002C    // Unreachable Port address in UDP mode

//W5500 Socket Registers follow
#define WIZNET_SnMR        0x0000        // Mode
#define WIZNET_SnCR        0x0001        // Command
#define WIZNET_SnIR        0x0002        // Interrupt
#define WIZNET_SnSR        0x0003        // Status
#define WIZNET_SnPORT      0x0004        // Source Port (2 bytes)
#define WIZNET_SnDHAR      0x0006        // Destination Hardw Addr
#define WIZNET_SnDIPR      0x000C        // Destination IP Addr
#define WIZNET_SnDPORT     0x0010        // Destination Port
#define WIZNET_SnMSSR      0x0012        // Max Segment Size
#define WIZNET_SnPROTO     0x0014        // Protocol in IP RAW Mode
#define WIZNET_SnTOS       0x0015        // IP TOS
#define WIZNET_SnTTL       0x0016        // IP TTL
#define WIZNET_SnRX_BSZ    0x001E        // RX Buffer Size
#define WIZNET_SnTX_BSZ    0x001F        // TX Buffer Size
#define WIZNET_SnTX_FSR    0x0020        // TX Free Size
#define WIZNET_SnTX_RD     0x0022        // TX Read Pointer
#define WIZNET_SnTX_WR     0x0024        // TX Write Pointer
#define WIZNET_SnRX_RSR    0x0026        // RX RECEIVED SIZE REGISTER
#define WIZNET_SnRX_RD     0x0028        // RX Read Pointer
#define WIZNET_SnRX_WR     0x002A        // RX Write Pointer (supported?

//Socket n Mode Register (0x0000)
//WIZNET_SnMR
#define WIZNET_MR_CLOSE    0x00    // Unused socket
#define WIZNET_MR_TCP      0x01    // TCP
#define WIZNET_MR_UDP      0x02    // UDP
#define WIZNET_MR_IPRAW    0x03    // IP LAYER RAW SOCK
#define WIZNET_MR_MACRAW   0x04    // MAC LAYER RAW SOCK
#define WIZNET_MR_PPPOE    0x05    // PPPoE
#define WIZNET_MR_ND       0x20    // No Delayed Ack(TCP) flag
#define WIZNET_MR_MULTI    0x80    // support multicating

//Socket n Command Register (0x0001)
//WIZNET_SnCR
#define WIZNET_CR_OPEN          0x01   // Initialize or open socket
#define WIZNET_CR_LISTEN        0x02   // Wait connection request in tcp mode(Server mode)
#define WIZNET_CR_CONNECT       0x04   // Send connection request in tcp mode(Client mode)
#define WIZNET_CR_DISCON        0x08   // Send closing reqeuset in tcp mode
#define WIZNET_CR_CLOSE         0x10   // Close socket
#define WIZNET_CR_SEND          0x20   // Update Tx memory pointer and send data
#define WIZNET_CR_SEND_MAC      0x21   // Send data with MAC address, so without ARP process
#define WIZNET_CR_SEND_KEEP     0x22   // Send keep alive message
#define WIZNET_CR_RECV          0x40   // Update Rx memory buffer pointer and receive data

//Socket n Interrupt Register (0x0002)
//WIZNET_SnIR
// Bit 0: CON
// Bit 1: DISCON
// Bit 2: RECV
// Bit 3: TIMEOUT
// Bit 4: SEND_OK

//Socket n Status Register (0x0003)
//WIZNET_SnSR 
#define WIZNET_SOCK_CLOSED      0x00   // Closed
#define WIZNET_SOCK_INIT        0x13   // Init state
#define WIZNET_SOCK_LISTEN      0x14   // Listen state
#define WIZNET_SOCK_SYNSENT     0x15   // Connection state
#define WIZNET_SOCK_SYNRECV     0x16   // Connection state
#define WIZNET_SOCK_ESTABLISHED 0x17   // Success to connect
#define WIZNET_SOCK_FIN_WAIT    0x18   // Closing state
#define WIZNET_SOCK_CLOSING     0x1A   // Closing state
#define WIZNET_SOCK_TIME_WAIT   0x1B   // Closing state
#define WIZNET_SOCK_CLOSE_WAIT  0x1C   // Closing state
#define WIZNET_SOCK_LAST_ACK    0x1D   // Closing state
#define WIZNET_SOCK_UDP         0x22   // UDP socket
#define WIZNET_SOCK_IPRAW       0x32   // IP raw mode socket
#define WIZNET_SOCK_MACRAW      0x42   // MAC raw mode socket
#define WIZNET_SOCK_PPPOE       0x5F   // PPPOE socket

//Socket n Source Port Register (0x0004, 0x0005)
//WIZNET_SnPORT
// MSByte: 0x0004
// LSByte: 0x0005

#define WIZNET_MAX_RBUF 2048 // buffer for receiving data (max rx packet size!)
#define WIZNET_MAX_TBUF 2048 // buffer for sending data (max tx packet size!)

#define WIZNET_WAIT_BUFFER_TIMEOUT_MS 1000

//-------------------
//BASIC READ AND WRITE FUNCTIONS
//-------------------

// Set wiznet CS low
void wiz_spi_begin_transfer()
{
  asm(
      "; backup regs\n"
      "push r1\n"
      "push r2\n"

      "load32 0xC02732 r2                 ; r2 = 0xC02732\n"

      "load 0 r1                          ; r1 = 0 (enable)\n"
      "write 0 r2 r1                      ; write to SPI3_CS\n"

      "; restore regs\n"
      "pop r2\n"
      "pop r1\n"
    );
}

// Set wiznet CS high
void wiz_spi_end_transfer()
{
  asm(
      "; backup regs\n"
      "push r1\n"
      "push r2\n"

      "load32 0xC02732 r2                 ; r2 = 0xC02732\n"

      "load 1 r1                          ; r1 = 1 (disable)\n"
      "write 0 r2 r1                      ; write to SPI3_CS\n"

      "; restore regs\n"
      "pop r2\n"
      "pop r1\n"
    );
}

// SPI write to wiznet chip
word wiz_spi_transfer(word dataByte)
{
  word retval = 0;
  asm(
      "load32 0xC02731 r2                 ; r2 = 0xC02731\n"
      "write 0 r2 r4                      ; write r4 over SPI3\n"
      "read 0 r2 r2                       ; read return value\n"
      "write -4 r14 r2                    ; write to stack to return\n"
    );

  return retval;
}

// Write data to W5500
void wiz_write(word addr, word cb, char *buf, word len)
{
  wiz_spi_begin_transfer();

  // Send address
  word addr_msb = (unsigned)addr >> 8;
  wiz_spi_transfer(addr_msb);
  wiz_spi_transfer(addr);

  // Send control byte
  wiz_spi_transfer(cb);

  // Send data
  word i;
  for (i = 0; i < len; i++)
  {
    wiz_spi_transfer(buf[i]);
  }

  wiz_spi_end_transfer();
}

// Write single byte to W5500
word wiz_write_single(word addr, word cb, word data)
{
  wiz_spi_begin_transfer();

  // Send address
  word addr_msb = (unsigned)addr >> 8;
  wiz_spi_transfer(addr_msb);
  wiz_spi_transfer(addr);

  // Send control byte
  wiz_spi_transfer(cb);

  // Send data
  wiz_spi_transfer(data);

  wiz_spi_end_transfer();

  return data;
}

// Write two bytes to W5500
void wiz_write_double(word addr, word cb, word data)
{
  wiz_spi_begin_transfer();

  // Send address
  word addr_msb = (unsigned)addr >> 8;
  wiz_spi_transfer(addr_msb);
  wiz_spi_transfer(addr);

  // Send control byte
  wiz_spi_transfer(cb);

  // Send data
  word dataMSB = (unsigned)data >> 8;
  wiz_spi_transfer(dataMSB);
  wiz_spi_transfer(data);

  wiz_spi_end_transfer();
}

void wiz_read(word addr, word cb, char *buf, word len)
{
  wiz_spi_begin_transfer();

  // Send address
  word addr_msb = (unsigned)addr >> 8;
  wiz_spi_transfer(addr_msb);
  wiz_spi_transfer(addr);

  // Send control byte
  wiz_spi_transfer(cb);

  // Read data
  word i;
  for (i = 0; i < len; i++)
  {
    buf[i] = wiz_spi_transfer(0);
  }

  wiz_spi_end_transfer();
}

word wiz_read_single(word addr, word cb)
{
  wiz_spi_begin_transfer();

  // Send address
  word addr_msb = (unsigned)addr >> 8;
  wiz_spi_transfer(addr_msb);
  wiz_spi_transfer(addr);

  // Send control byte
  wiz_spi_transfer(cb);

  // Read data
  word retval = wiz_spi_transfer(0);

  wiz_spi_end_transfer();

  // Return read value
  return retval;
}

word wiz_read_double(word addr, word cb)
{
  wiz_spi_begin_transfer();

  // Send address
  word addr_msb = (unsigned)addr >> 8;
  wiz_spi_transfer(addr_msb);
  wiz_spi_transfer(addr);

  // Send control byte
  wiz_spi_transfer(cb);

  // Read data
  word retval = wiz_spi_transfer(0) << 8;
  retval = retval + wiz_spi_transfer(0);

  wiz_spi_end_transfer();

  // Return read value
  return retval;
}


//-------------------
//W5500 SOCKET REGISTER FUNCTIONS
//-------------------

// Send a command cmd to socket s
void wiz_send_cmd(word s, word cmd)
{
  wiz_spi_begin_transfer();
  wiz_spi_transfer(0);
  wiz_spi_transfer(WIZNET_SnCR);
  wiz_spi_transfer(WIZNET_WRITE_SnR + (s << 5));
  wiz_spi_transfer(cmd);
  wiz_spi_end_transfer();

  // Wait until done
  while (wiz_read_single(WIZNET_SnCR, WIZNET_READ_SnR));
}

// Write 8 bits to a sockets control register
void wiz_set_sock_reg_8(word s, word addr, word val)
{
  wiz_spi_begin_transfer();
  wiz_spi_transfer(0);
  wiz_spi_transfer(addr);
  wiz_spi_transfer(WIZNET_WRITE_SnR + (s << 5));
  wiz_spi_transfer(val);
  wiz_spi_end_transfer();
}

// Read 8 bits from a sockets control register
word wiz_get_sock_reg_8(word s, word addr)
{
  wiz_spi_begin_transfer();

  // Send address
  word addr_msb = (unsigned) addr >> 8;
  wiz_spi_transfer(addr_msb);
  wiz_spi_transfer(addr);

  word cb = WIZNET_READ_SnR + (s << 5);

  // Send control byte
  wiz_spi_transfer(cb);

  // Read data
  word retval = wiz_spi_transfer(0);

  wiz_spi_end_transfer();

  // Return read value
  return retval;
}

// Write 16 bits to a sockets control register
void wiz_set_sock_reg_16(word s, word addr, word val)
{
  wiz_spi_begin_transfer();
  wiz_spi_transfer(0);
  wiz_spi_transfer(addr);
  wiz_spi_transfer(WIZNET_WRITE_SnR + (s << 5));
  word val_msb = (unsigned) val >> 8;
  wiz_spi_transfer(val_msb);
  wiz_spi_transfer(val);
  wiz_spi_end_transfer();
}

// Read 16 bits from a sockets control register
word wiz_get_sock_reg_16(word s, word addr)
{
  //return wiz_read_double(addr, WIZNET_READ_SnR + (s << 5));
  wiz_spi_begin_transfer();

  // Send address
  word addr_msb = (unsigned) addr >> 8;
  wiz_spi_transfer(addr_msb);
  wiz_spi_transfer(addr);

  word cb = WIZNET_READ_SnR + (s << 5);

  // Send control byte
  wiz_spi_transfer(cb);

  // Read data
  word retval = wiz_spi_transfer(0) << 8;
  retval = retval + wiz_spi_transfer(0);

  wiz_spi_end_transfer();

  // Return read value
  return retval;
}


//-------------------
//W5500 SETUP FUNCTIONS
//-------------------

// Initialize W5500 chip
void wiz_init(char *ip_addr, char *gateway_addr, char *mac_addr, char *sub_mask)
{
  wiz_spi_end_transfer();
  wiz_write(WIZNET_SIPR, WIZNET_WRITE_COMMON, ip_addr, 4);
  wiz_write(WIZNET_GAR, WIZNET_WRITE_COMMON, gateway_addr, 4);
  wiz_write(WIZNET_SHAR, WIZNET_WRITE_COMMON, mac_addr, 6);
  wiz_write(WIZNET_SUBR, WIZNET_WRITE_COMMON, sub_mask, 4);
}

// Initialize socket s for TCP host mode
void wiz_init_socket_tcp_host(word s, word port)
{
  wiz_send_cmd(s, WIZNET_CR_CLOSE);
  wiz_set_sock_reg_8(s, WIZNET_SnIR, 0xFF);          // Reset interrupt register
  wiz_set_sock_reg_8(s, WIZNET_SnMR, WIZNET_MR_TCP); // Set mode register to tcp
  wiz_set_sock_reg_16(s, WIZNET_SnPORT, port);       // Set tcp port
  wiz_send_cmd(s, WIZNET_CR_OPEN);
  wiz_send_cmd(s, WIZNET_CR_LISTEN);
  while (wiz_get_sock_reg_8(s, WIZNET_SnSR) != WIZNET_SOCK_LISTEN);
}

// Initialize socket s for TCP client mode
void wiz_init_socket_tcp_client(word s, word port)
{
  wiz_send_cmd(s, WIZNET_CR_CLOSE);
  wiz_set_sock_reg_8(s, WIZNET_SnIR, 0xFF);          // Reset interrupt register
  wiz_set_sock_reg_8(s, WIZNET_SnMR, WIZNET_MR_TCP); // Set mode register to tcp
  wiz_set_sock_reg_16(s, WIZNET_SnPORT, port);       // Set tcp port
  wiz_send_cmd(s, WIZNET_CR_OPEN);
  while (wiz_get_sock_reg_8(s, WIZNET_SnSR) != WIZNET_SOCK_INIT);
}

//-------------------
//W5500 READING AND WRITING FUNCTIONS
//-------------------

// Write data from buf of length buflen to socket s
// Returns 1 if successful, 0 if not
word wiz_write_data(word s, char *buf, word buflen)
{
  // Make sure there is something to send
  if (buflen == 0)
  {
    return 0;
  }

  word bytes_sent = 0;

  // Loop until all bytes are sent
  while (bytes_sent != buflen)
  {
    if (wiz_get_sock_reg_8(s, WIZNET_SnSR) == WIZNET_SOCK_CLOSED)
    {
      return 0;
    }

    // Send in chunks of WIZNET_MAX_TBUF
    word part_to_send = buflen - bytes_sent;
    if (part_to_send > WIZNET_MAX_TBUF)
    {
      part_to_send = WIZNET_MAX_TBUF;
    }

    // Wait until there is room in the transmit buffer for what we want to send
    word txfree = wiz_get_sock_reg_16(s, WIZNET_SnTX_FSR);
    word timeout = 0;
    while (txfree < part_to_send)
    {
      timeout++;
      delay(1);
      txfree = wiz_get_sock_reg_16(s, WIZNET_SnTX_FSR);

      if (timeout > WIZNET_WAIT_BUFFER_TIMEOUT_MS)
      {
        wiz_send_cmd(s, WIZNET_CR_DISCON); // Disconnect
        return 0;
      }
    }

    // Read the Tx Write Pointer
    word txwr = wiz_get_sock_reg_16(s, WIZNET_SnTX_WR);

    // Write the outgoing data to the transmit buffer
    wiz_write(txwr, WIZNET_WRITE_SnTX + (s << 5), buf + bytes_sent, part_to_send);

    // Update the buffer pointer
    word newSize = txwr + part_to_send;
    wiz_set_sock_reg_16(s, WIZNET_SnTX_WR, newSize);

    // Now send the SEND command which tells the wiznet the pointer is updated
    wiz_send_cmd(s, WIZNET_CR_SEND);

    // Update the amount of bytes sent
    bytes_sent += part_to_send;
  }

  return 1;
}

// Read received data on socket s of length buflen to buf
void wiz_read_recv_data(word s, char *buf, word buflen)
{
  if (buflen == 0)
  {
    return;
  }

  if (buflen > WIZNET_MAX_RBUF) // If the request size > WIZNET_MAX_RBUF, truncate it to prevent overflow
  {
    buflen = WIZNET_MAX_RBUF;
  }

  // Get the address where the wiznet is holding the data
  word rxrd = wiz_get_sock_reg_16(s, WIZNET_SnRX_RD);

  // Read the data into the buffer
  wiz_read(rxrd, WIZNET_READ_SnRX + (s << 5), buf, buflen);

  // Remove read data from rxbuffer to make space for new data
  word nsize = rxrd + buflen;
  wiz_set_sock_reg_16(s, WIZNET_SnRX_RD, nsize); // replace read data pointer
  // Tell the wiznet we have retrieved the data
  wiz_send_cmd(s, WIZNET_CR_RECV);

  return;
}

// Remove the received data
void wiz_flush(word s, word rsize)
{
  if (rsize > 0)
  {
    word rxrd = wiz_get_sock_reg_16(s, WIZNET_SnRX_RD); // Retrieve read data pointer
    word nsize = rxrd + rsize;
    wiz_set_sock_reg_16(s, WIZNET_SnRX_RD, nsize); // Replace read data pointer
    // Tell the wiznet we have retrieved the data
    wiz_send_cmd(s, WIZNET_CR_RECV);
  }
}
