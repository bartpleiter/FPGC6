/*
 * Testbench
 * Simulation for divider module
*/

// Set timescale
`timescale 1 ns/1 ns

// Includes
// Memory
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/IO/Divider.v"


// Define testmodule
module divider_tb;

// clock/reset I/O
reg clk = 1'b0;
reg reset = 1'b0;
reg start = 1'b0;
reg write_a = 1'b0;

wire busy;
wire done;
wire valid;
wire dbz;
wire ovf;


reg signed [31:0] a = 0;
reg signed [31:0] b = 0;

wire signed [31:0] val;

wire signed [15:0] val_int;
assign val_int = val [31:16];

Divider divider(
.clk    (clk), 
.rst    (reset),
.start(start),  // start calculation
.write_a(write_a),
.busy(busy),   // calculation in progress
.done(done),   // calculation is complete (high for one tick)
.valid(valid),  // result is valid
.dbz(dbz),    // divide by zero
.ovf(ovf),    // overflow
.a_in(a),   // dividend (numerator)
.b(b),   // divisor (denominator)
.val(val)  // result value: quotient
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

    a = 17 << 16;
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
    b = 3 << 16;
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