/*
 * Testbench
 * Simulates the entire FPGC
 * 100MHz experiment
*/

// Set timescale
`timescale 1 ns/1 ns

// tld
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/FPGC6.v"

// other logic
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/MultiStabilizer.v"

// cpu
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
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/CPU/IntController.v"

// memory
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/Memory/SRAM.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/Memory/L1Dcache.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/Memory/MemoryUnit.v"


// Define testmodule
module FPGC_tb;

//Clock I/O
reg clk;
reg nreset;

//Led I/O
wire led;

FPGC6 fpgc (
.clk(clk),
.nreset(nreset),
     
//Led for debugging
.led(led)
);



initial
begin
    //Dump everything for GTKwave
    $dumpfile("/home/bart/Documents/FPGA/FPGC6/Verilog/output/wave.vcd");
    $dumpvars;
    
    clk = 0;
    nreset = 1;


    repeat(3)
    begin
        #5 clk = ~clk; //100MHz
        #5 clk = ~clk;
    end

    nreset = 0;

    repeat(3)
    begin
        #5 clk = ~clk; //100MHz
        #5 clk = ~clk;
    end

    nreset = 1;

    repeat(1000)
    begin
        #5 clk = ~clk; //100MHz
        #5 clk = ~clk;
    end

    #1 $finish;
end

endmodule