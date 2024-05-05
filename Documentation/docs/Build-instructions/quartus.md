# Quartus build instructions

The Quartus folder contains all files for actually implementing the FPGC into hardware on an FPGA using the Quartus Prime Lite edition software from Intel. The targeted development board is the QMTECH 5CEF_A5_F23_I7N core board with 64MiB SDRAM.

!!! info
    There are some slight changes between the code in the Verilog folder and the code in the Quartus folder. For example, the Verilog folder contains simulation files for the SPI flash and SDRAM memory. The Quartus project is on the top level somewhat modified to work on an actual FPGA. This also includes the use of PLLs for creating clocks.

## Flashing configuation to FPGA

To flash the FPGA design, a very cheap Altera USB Blaster from Aliexpress can be used, although they require some work to get working (libusb issues).
