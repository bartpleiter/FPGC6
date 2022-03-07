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


// Define testmodule
module B32P_tb;

//---------------CPU----------------
// CPU I/O
reg clk = 0;
reg reset = 0;

CPU cpu(
.clk        (clk),
.reset      (reset)
);


initial
begin

    // dump everything for GTKwave
    $dumpfile("/home/bart/Documents/FPGA/FPGC6/Verilog/output/wave.vcd");
    $dumpvars;

    repeat(5120) #10 clk = ~clk; // 50MHz

    #1 $finish;
end

endmodule