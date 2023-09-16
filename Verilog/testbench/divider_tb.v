/*
 * Testbench
 * Simulation for divider module
*/

// Set timescale
`timescale 1 ns/1 ns

// Includes
// Memory
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/IO/IDivider.v"


// Define testmodule
module divider_tb;

// clock/reset I/O
reg clk = 1'b0;
reg reset = 1'b0;
reg start = 1'b0;
reg write_a = 1'b0;
reg signed_ope = 1'b0;
wire ready;


reg signed [31:0] a = 0;
reg signed [31:0] b = 0;

wire signed [31:0] quotient;
wire signed [31:0] remainder;

IDivider divider(
.clk    (clk), 
.rst    (reset),
.start(start),  // start calculation
.write_a(write_a),
.ready (ready),
.flush(1'b0),
.signed_ope(signed_ope),   // calculation in progress
.a(a),   // dividend (numerator)
.b(b),   // divisor (denominator)
.quotient(quotient),  // result value: quotient
.remainder(remainder)
);

initial
begin
    // dump everything for GTKwave
    $dumpfile("/home/bart/Documents/FPGA/FPGC6/Verilog/output/wave.vcd");
    $dumpvars;
    
    #10 

    // startup
    repeat(2)
    begin
        clk = ~clk;
        #10 clk = ~clk;
        #10;
    end

    reset = 1;

    repeat(2)
    begin
        clk = ~clk;
        #10 clk = ~clk;
        #10;
    end

    reset = 0;

    repeat(4)
    begin
        clk = ~clk;
        #10 clk = ~clk;
        #10;
    end

    a = 17;
    write_a = 1;
    repeat(1)
    begin
        clk = ~clk;
        #10 clk = ~clk;
        #10;
    end

    write_a = 0;

    repeat(4)
    begin
        clk = ~clk;
        #10 clk = ~clk;
        #10;
    end

    a = 0;
    b = 3;
    start = 1;


    repeat(64)
    begin
        clk = ~clk;
        #10 clk = ~clk;
        #10;
    end


    #1 $finish;
end

endmodule