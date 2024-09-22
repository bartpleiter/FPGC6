/*
 * Testbench
 * Simulates the CPU while using a simplified version of the rest of the system
*/

// Set timescale
`timescale 1 ns/1 ns

// tld
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/FPGC6Simplified.v"

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
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/CPU/IntController.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/CPU/L1Icache.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/CPU/L1Dcache.v"

// memory
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/Memory/SRAM.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/Memory/MemoryUnitSimplified.v"


// Define testmodule
module FPGC_tb;

//Clock I/O
reg clk;
reg clk100;
reg nreset;

FPGC6 fpgc (
.clk(clk),
.clk100(clk100),
.nreset(nreset)
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
        #5 clk100 = ~clk100; clk = ~clk; //50MHz
        #5 clk100 = ~clk100; //100MHz
        #5 clk100 = ~clk100; clk = ~clk; //50MHz
        #5 clk100 = ~clk100; //100MHz
    end

    nreset = 0;

    repeat(3)
    begin
        #5 clk100 = ~clk100; clk = ~clk; //50MHz
        #5 clk100 = ~clk100; //100MHz
        #5 clk100 = ~clk100; clk = ~clk; //50MHz
        #5 clk100 = ~clk100; //100MHz
    end

    nreset = 1;

    repeat(1000)
    begin
        #5 clk100 = ~clk100; clk = ~clk; //50MHz
        #5 clk100 = ~clk100; //100MHz
        #5 clk100 = ~clk100; clk = ~clk; //50MHz
        #5 clk100 = ~clk100; //100MHz
    end

    #1 $finish;
end

endmodule
