/*
 * Testbench
 * Simulation for the new SDRAM controller
*/

// Set timescale
`timescale 1 ns/1 ns

// Includes
// Memory
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/Memory/mt48lc16m16a2.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/Memory/SDRAMcontroller.v"


// Define testmodule
module SDRAM_tb;

// clock/reset I/O
reg clk = 1'b0;
reg reset = 1'b0;

/////////
//SDRAM
/////////
wire             SDRAM_CLK;     // SDRAM clock
wire    [31 : 0] SDRAM_DQ;      // SDRAM I/O
wire    [12 : 0] SDRAM_A;       // SDRAM Address
wire    [1 : 0]  SDRAM_BA;      // Bank Address
wire             SDRAM_CKE;     // Synchronous Clock Enable
wire             SDRAM_CSn;     // CS#
wire             SDRAM_RASn;    // RAS#
wire             SDRAM_CASn;    // CAS#
wire             SDRAM_WEn;     // WE#
wire    [3 : 0]  SDRAM_DQM;     // Mask

assign SDRAM_CLK = ~clk;

mt48lc16m16a2 sdram1 (
.Dq     (SDRAM_DQ[15:0]), 
.Addr   (SDRAM_A), 
.Ba     (SDRAM_BA), 
.Clk    (SDRAM_CLK), 
.Cke    (SDRAM_CKE), 
.Cs_n   (SDRAM_CSn), 
.Ras_n  (SDRAM_RASn), 
.Cas_n  (SDRAM_CASn), 
.We_n   (SDRAM_WEn), 
.Dqm    (SDRAM_DQM[1:0])
);

mt48lc16m16a2 sdram2 (
.Dq     (SDRAM_DQ[31:16]), 
.Addr   (SDRAM_A), 
.Ba     (SDRAM_BA), 
.Clk    (SDRAM_CLK), 
.Cke    (SDRAM_CKE), 
.Cs_n   (SDRAM_CSn), 
.Ras_n  (SDRAM_RASn), 
.Cas_n  (SDRAM_CASn), 
.We_n   (SDRAM_WEn), 
.Dqm    (SDRAM_DQM[3:2])
);


////////////////////
//SDRAM Controller
////////////////////

// inputs
reg [23:0]      sdc_addr    = 24'd0;    // address to write or to start reading from
reg [31:0]      sdc_data    = 32'd0;    // data to write
reg             sdc_we      = 1'b0;     // write enable
reg             sdc_start   = 1'b0;     // start trigger

// outputs
wire [255:0]    sdc_q;                  // cache line output
wire            sdc_ack;                // start ack signal
wire            sdc_busy;               // busy status

SDRAMcontroller sdramcontroller(
// clock/reset inputs
.clk        (clk),
.reset      (reset),

// signal inputs
.sdc_addr   (sdc_addr),
.sdc_data   (sdc_data),
.sdc_we     (sdc_we),
.sdc_start  (sdc_start),

// signal outputs
.sdc_q      (sdc_q),
.sdc_ack    (sdc_ack),
.sdc_busy   (sdc_busy),

// SDRAM signals
.SDRAM_CKE  (SDRAM_CKE),
.SDRAM_CSn  (SDRAM_CSn),
.SDRAM_WEn  (SDRAM_WEn),
.SDRAM_CASn (SDRAM_CASn),
.SDRAM_RASn (SDRAM_RASn),
.SDRAM_A    (SDRAM_A),
.SDRAM_BA   (SDRAM_BA),
.SDRAM_DQM  (SDRAM_DQM),
.SDRAM_DQ   (SDRAM_DQ)
);

initial
begin
    // dump everything for GTKwave
    $dumpfile("/home/bart/Documents/FPGA/FPGC6/Verilog/output/wave.vcd");
    $dumpvars;
    
    clk = 1'b0;
    reset = 1'b0;

    repeat(100000) #10 clk = ~clk;

    #1 $finish;
end

endmodule