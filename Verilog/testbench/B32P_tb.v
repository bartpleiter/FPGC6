/*
 * Testbench
 * Simulates the B32P CPU
*/

// Set timescale
`timescale 1 ns/1 ns

// Include modules
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/CPU/CPU.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/CPU/ALU.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/CPU/ControlUnit.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/CPU/InstructionDecoder.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/CPU/Regbank.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/CPU/Stack.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/CPU/InstrMem.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/CPU/DataMem.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/CPU/Regr.v"

`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/CPU/Arbiter.v"

`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/Memory/VRAM.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/Memory/mt48lc16m16a2.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/Memory/w25q128jv.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/Memory/SDRAMcontroller.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/Memory/SPIreader.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/Memory/ROM.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/Memory/MemoryUnit.v"

`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/IO/Keyboard.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/IO/OStimer.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/IO/UARTtx.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/IO/UARTrx.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/IO/SimpleSPI.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/IO/LEDvisualizer.v"


// Define testmodule
module B32P_tb;

//---------------CPU----------------
// CPU I/O
reg clk = 0;
reg clk_SDRAM = 0;
reg reset = 0;

//Bus
wire [26:0] bus_addr;
wire [31:0] bus_data;
wire        bus_we;
wire        bus_start;
wire [31:0] bus_q;
wire        bus_done;

CPU cpu(
.clk        (clk),
.reset      (reset),

// bus
.bus_addr(bus_addr),
.bus_data(bus_data),
.bus_we(bus_we),
.bus_start(bus_start),
.bus_q(bus_q),
.bus_done(bus_done)
);

//----------DEV-------------


//---------------------------VRAM32---------------------------------
//VRAM32 I/O
wire        vram32_gpu_clk;
wire [13:0] vram32_gpu_addr;
wire [31:0] vram32_gpu_d;
wire        vram32_gpu_we;
wire [31:0] vram32_gpu_q;

wire        vram32_cpu_clk;
wire [13:0] vram32_cpu_addr;
wire [31:0] vram32_cpu_d;
wire        vram32_cpu_we; 
wire [31:0] vram32_cpu_q;

//because FSX will not write to VRAM
assign vram32_gpu_we    = 1'b0;
assign vram32_gpu_d     = 32'd0;

VRAM #(
.WIDTH(32), 
.WORDS(1056), 
.LIST("/home/bart/Documents/FPGA/FPGC5/Verilog/memory/vram32.list")
)   vram32(
//CPU port
.cpu_clk    (clk),
.cpu_d      (vram32_cpu_d),
.cpu_addr   (vram32_cpu_addr),
.cpu_we     (vram32_cpu_we),
.cpu_q      (vram32_cpu_q),

//GPU port
.gpu_clk    (clkMuxOut),
.gpu_d      (vram32_gpu_d),
.gpu_addr   (vram32_gpu_addr),
.gpu_we     (vram32_gpu_we),
.gpu_q      (vram32_gpu_q)
);


//---------------------------VRAM322--------------------------------
//VRAM322 I/O
wire        vram322_gpu_clk;
wire [13:0] vram322_gpu_addr;
wire [31:0] vram322_gpu_d;
wire        vram322_gpu_we;
wire [31:0] vram322_gpu_q;

//because FSX will not write to VRAM
assign vram322_gpu_we    = 1'b0;
assign vram322_gpu_d     = 32'd0;

VRAM #(
.WIDTH(32), 
.WORDS(1056), 
.LIST("/home/bart/Documents/FPGA/FPGC5/Verilog/memory/vram32.list")
)   vram322(
//CPU port
.cpu_clk    (clk),
.cpu_d      (vram32_cpu_d),
.cpu_addr   (vram32_cpu_addr),
.cpu_we     (vram32_cpu_we),
.cpu_q      (),

//GPU port
.gpu_clk    (clkMuxOut),
.gpu_d      (vram322_gpu_d),
.gpu_addr   (vram322_gpu_addr),
.gpu_we     (vram322_gpu_we),
.gpu_q      (vram322_gpu_q)
);


//--------------------------VRAM8--------------------------------
//VRAM8 I/O
wire        vram8_gpu_clk;
wire [13:0] vram8_gpu_addr;
wire [7:0]  vram8_gpu_d;
wire        vram8_gpu_we;
wire [7:0]  vram8_gpu_q;

wire        vram8_cpu_clk;
wire [13:0] vram8_cpu_addr;
wire [7:0]  vram8_cpu_d;
wire        vram8_cpu_we;
wire [7:0]  vram8_cpu_q;

//because FSX will not write to VRAM
assign vram8_gpu_we     = 1'b0;
assign vram8_gpu_d      = 8'd0;

VRAM #(
.WIDTH(8), 
.WORDS(8194), 
.LIST("/home/bart/Documents/FPGA/FPGC5/Verilog/memory/vram8.list")
)   vram8(
//CPU port
.cpu_clk    (clk),
.cpu_d      (vram8_cpu_d),
.cpu_addr   (vram8_cpu_addr),
.cpu_we     (vram8_cpu_we),
.cpu_q      (vram8_cpu_q),

//GPU port
.gpu_clk    (clkMuxOut),
.gpu_d      (vram8_gpu_d),
.gpu_addr   (vram8_gpu_addr),
.gpu_we     (vram8_gpu_we),
.gpu_q      (vram8_gpu_q)
);


//--------------------------VRAMSPR--------------------------------
//VRAMSPR I/O
wire        vramSPR_gpu_clk;
wire [13:0] vramSPR_gpu_addr;
wire [8:0]  vramSPR_gpu_d;
wire        vramSPR_gpu_we;
wire [8:0]  vramSPR_gpu_q;

wire        vramSPR_cpu_clk;
wire [13:0] vramSPR_cpu_addr;
wire [8:0]  vramSPR_cpu_d;
wire        vramSPR_cpu_we;
wire [8:0]  vramSPR_cpu_q;

//because FSX will not write to VRAM
assign vramSPR_gpu_we     = 1'b0;
assign vramSPR_gpu_d      = 9'd0;

VRAM #(
.WIDTH(9), 
.WORDS(256), 
.LIST("/home/bart/Documents/FPGA/FPGC5/Verilog/memory/vramSPR.list")
)   vramSPR(
//CPU port
.cpu_clk    (clk),
.cpu_d      (vramSPR_cpu_d),
.cpu_addr   (vramSPR_cpu_addr),
.cpu_we     (vramSPR_cpu_we),
.cpu_q      (vramSPR_cpu_q),

//GPU port
.gpu_clk    (clkMuxOut),
.gpu_d      (vramSPR_gpu_d),
.gpu_addr   (vramSPR_gpu_addr),
.gpu_we     (vramSPR_gpu_we),
.gpu_q      (vramSPR_gpu_q)
);


//-------------------ROM-------------------------
//ROM I/O
wire [8:0] rom_addr;
wire [31:0] rom_q;


ROM rom(
.clk            (clk),
.reset          (reset),
.address        (rom_addr),
.q              (rom_q)
);




//SPI0 Flash
wire SPI0_clk;
wire SPI0_cs; 
wire SPI0_data; 
wire SPI0_wp;
wire SPI0_q;  
wire SPI0_hold; 

W25Q128JV spiFlash (
.CLK    (SPI0_clk), 
.DIO    (SPI0_data), 
.CSn    (SPI0_cs), 
.WPn    (SPI0_wp), 
.HOLDn  (SPI0_hold), 
.DO     (SPI0_q)
);

//SDRAM
wire             SDRAM_CLK;     // SDRAM clock
wire    [15 : 0] SDRAM_DQ;      // SDRAM I/O
wire    [12 : 0] SDRAM_A;    // SDRAM Address
wire    [1 : 0]  SDRAM_BA;      // Bank Address
wire             SDRAM_CKE;     // Synchronous Clock Enable
wire             SDRAM_CSn;    // CS#
wire             SDRAM_RASn;   // RAS#
wire             SDRAM_CASn;   // CAS#
wire             SDRAM_WEn;    // WE#
wire    [1 : 0]  SDRAM_DQM;     // Mask

//Run SDRAM at 100MHz
assign SDRAM_CLK = clk_SDRAM;

mt48lc16m16a2 sdram (
.Dq     (SDRAM_DQ), 
.Addr   (SDRAM_A), 
.Ba     (SDRAM_BA), 
.Clk    (SDRAM_CLK), 
.Cke    (SDRAM_CKE), 
.Cs_n   (SDRAM_CSn), 
.Ras_n  (SDRAM_RASn), 
.Cas_n  (SDRAM_CASn), 
.We_n   (SDRAM_WEn), 
.Dqm    (SDRAM_DQM)
);

//HDMI
wire [3:0] TMDS_p;
wire [3:0] TMDS_n;

//SPI1
wire SPI1_clk;
wire SPI1_cs;
wire SPI1_mosi;
wire SPI1_miso;
wire SPI1_rst;
reg  SPI1_nint;

//SPI2
wire SPI2_clk;
wire SPI2_cs;
wire SPI2_mosi;
wire SPI2_miso;
wire SPI2_rst;
reg  SPI2_nint;

//SPI3
wire SPI3_clk;
wire SPI3_cs;
wire SPI3_mosi;
wire SPI3_miso;
wire SPI3_nrst;
reg  SPI3_int;

//SPI4
wire SPI4_clk;
wire SPI4_cs;
wire SPI4_mosi;
wire SPI4_miso;
reg  SPI4_gp;

//UART0
reg  UART0_in;
wire UART0_out;
reg  UART0_dtr;

//UART1
//reg  UART1_in;
//wire UART1_out;

//UART2
reg  UART2_in;
wire UART2_out;

//PS/2
reg PS2_clk;
reg PS2_data;

//Led
wire led;

//GPIO
wire [3:0]  GPO;
reg  [3:0]  GPI;

//DIP Switch
reg [3:0] DIPS;



//----------------Memory Unit--------------------
//Memory Unit I/O

//Interrupt signals
wire        OST1_int, OST2_int, OST3_int;
wire        UART0_rx_int, UART2_rx_int;
wire        PS2_int;
wire        SPI0_QSPI;

MemoryUnit mu(
//clock
.clk            (clk),
.clk_SDRAM      (clk_SDRAM),
.reset          (reset),

//CPU connection (Bus)
.bus_addr       (bus_addr),
.bus_data       (bus_data),
.bus_we         (bus_we),
.bus_start      (bus_start),
.bus_q          (bus_q),
.bus_done       (bus_done),

/********
* MEMORY
********/

//SPI Flash / SPI0
.SPIflash_data  (SPI0_data), 
.SPIflash_q     (SPI0_q), 
.SPIflash_wp    (SPI0_wp), 
.SPIflash_hold  (SPI0_hold),
.SPIflash_cs    (SPI0_cs), 
.SPIflash_clk   (SPI0_clk),

//SDRAM
.SDRAM_CSn      (SDRAM_CSn), 
.SDRAM_WEn      (SDRAM_WEn), 
.SDRAM_CASn     (SDRAM_CASn),
.SDRAM_CKE      (SDRAM_CKE), 
.SDRAM_RASn     (SDRAM_RASn),
.SDRAM_A        (SDRAM_A),
.SDRAM_BA       (SDRAM_BA),
.SDRAM_DQM      (SDRAM_DQM),
.SDRAM_DQ       (SDRAM_DQ),

//VRAM32 cpu port
.VRAM32_cpu_d       (vram32_cpu_d),
.VRAM32_cpu_addr    (vram32_cpu_addr), 
.VRAM32_cpu_we      (vram32_cpu_we),
.VRAM32_cpu_q       (vram32_cpu_q),

//VRAM8 cpu port
.VRAM8_cpu_d        (vram8_cpu_d),
.VRAM8_cpu_addr     (vram8_cpu_addr), 
.VRAM8_cpu_we       (vram8_cpu_we),
.VRAM8_cpu_q        (vram8_cpu_q),

//VRAMspr cpu port
.VRAMspr_cpu_d      (vramSPR_cpu_d),
.VRAMspr_cpu_addr   (vramSPR_cpu_addr), 
.VRAMspr_cpu_we     (vramSPR_cpu_we),
.VRAMspr_cpu_q      (vramSPR_cpu_q),

//ROM
.ROM_addr           (rom_addr),
.ROM_q              (rom_q),

/********
* I/O
********/

//UART0 (Main USB)
.UART0_in           (UART0_in),
.UART0_out          (UART0_out),
.UART0_rx_interrupt (UART0_rx_int),

//UART1 (APU)
/*.UART1_in           (),
.UART1_out          (),
.UART1_rx_interrupt (),
*/

//UART2 (GP)
.UART2_in           (UART2_in),
.UART2_out          (UART2_out),
.UART2_rx_interrupt (UART2_rx_int),

//SPI0 (Flash)
//declared under MEMORY
.SPI0_QSPI      (SPI0_QSPI),

//SPI1 (USB0/CH376T)
.SPI1_clk       (SPI1_clk),
.SPI1_cs        (SPI1_cs),
.SPI1_mosi      (SPI1_mosi),
.SPI1_miso      (SPI1_miso),
.SPI1_nint      (SPI1_nint_stable),

//SPI2 (USB1/CH376T)
.SPI2_clk       (SPI2_clk),
.SPI2_cs        (SPI2_cs),
.SPI2_mosi      (SPI2_mosi),
.SPI2_miso      (SPI2_miso),
.SPI2_nint      (SPI2_nint_stable),

//SPI3 (W5500)
.SPI3_clk       (SPI3_clk),
.SPI3_cs        (SPI3_cs),
.SPI3_mosi      (SPI3_mosi),
.SPI3_miso      (SPI3_miso),
.SPI3_int       (SPI3_int_stable),

//SPI4 (EXT/GP)
.SPI4_clk       (SPI4_clk),
.SPI4_cs        (SPI4_cs),
.SPI4_mosi      (SPI4_mosi),
.SPI4_miso      (SPI4_miso),
.SPI4_GP        (SPI4_gp_stable),

//GPIO (Separated GPI and GPO until GPIO module is implemented)
.GPI        (GPI[3:0]),
.GPO        (GPO[3:0]),

//OStimers
.OST1_int   (OST1_int),
.OST2_int   (OST2_int),
.OST3_int   (OST3_int),

//SNESpad
/*
.SNES_clk   (),
.SNES_latch (),
.SNES_data  (),
*/

//PS/2
.PS2_clk    (PS2_clk),
.PS2_data   (PS2_data),
.PS2_int    (PS2_int), //Scan code ready signal

//Boot mode
.boot_mode  (boot_mode_stable)
);




initial
begin

    // dump everything for GTKwave
    $dumpfile("/home/bart/Documents/FPGA/FPGC6/Verilog/output/wave.vcd");
    $dumpvars;

    reset = 0;

    //repeat(5120) #10 clk = ~clk; // 50MHz

    repeat(4)
    begin
        #5 clk_SDRAM = ~clk_SDRAM; clk = ~clk; //50MHz
        #5 clk_SDRAM = ~clk_SDRAM; //100MHz
    end

    reset = 1;

    repeat(4)
    begin
        #5 clk_SDRAM = ~clk_SDRAM; clk = ~clk; //50MHz
        #5 clk_SDRAM = ~clk_SDRAM; //100MHz
    end

    reset = 0;

    repeat(1024)
    begin
        #5 clk_SDRAM = ~clk_SDRAM; clk = ~clk; //50MHz
        #5 clk_SDRAM = ~clk_SDRAM; //100MHz
    end

    repeat(512)
    begin
        #5 clk_SDRAM = ~clk_SDRAM; clk = ~clk; //50MHz
        #5 clk_SDRAM = ~clk_SDRAM; //100MHz
    end


    #1 $finish;
end

endmodule