/*
 * Testbench
 * Simulates the bus protocol
*/

`timescale 1 ns/1 ns

`include "/home/bart/Documents/FPGA/FPGC6/Verilog/testbench/busProtocol/cpu.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/testbench/busProtocol/mu.v"
`include "/home/bart/Documents/FPGA/FPGC6/Verilog/modules/Memory/SRAM.v"

module busProtocol_tb;

reg clk;
reg reset;

reg clear;
reg hold;


// Bus
wire [26:0] bus_addr;
wire [31:0] bus_data;
wire        bus_we;
wire        bus_start;
wire [31:0] bus_q;
wire        bus_done;

MemoryUnit memoryunit(
// Clocks
.clk            (clk),
.reset          (reset),

// Bus
.bus_addr       (bus_addr),
.bus_data       (bus_data),
.bus_we         (bus_we),
.bus_start      (bus_start),
.bus_q          (bus_q),
.bus_done       (bus_done),
.bus_ready       (bus_ready)
);

//---------------CPU----------------
CPU cpu(
.clk            (clk),
.reset          (reset),

// bus
.bus_addr       (bus_addr),
.bus_data       (bus_data),
.bus_we         (bus_we),
.bus_start      (bus_start),
.bus_q          (bus_q),
.bus_done       (bus_done),
.bus_ready       (bus_ready),
.clear          (clear),
.hold           (hold)
);

initial
begin
    //Dump everything for GTKwave
    $dumpfile("/home/bart/Documents/FPGA/FPGC6/Verilog/output/wave.vcd");
    $dumpvars;

    clk = 0;
    reset = 0;
    clear = 0;
    hold = 0;


    repeat(3)
    begin
        #10 clk = ~clk; //50MHz
        #10 clk = ~clk;
    end

    reset = 1;

    repeat(3)
    begin
        #10 clk = ~clk; //50MHz
        #10 clk = ~clk;
    end

    reset = 0;

    repeat(100)
    begin
        #10 clk = ~clk; //50MHz
        #10 clk = ~clk;
    end

    #1 $finish;
end

endmodule
