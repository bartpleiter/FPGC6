/*
* Instruction Memory
*/

module InstrMem(
    input clk,
    input reset,
    input [31:0] addr,
    output reg [31:0] q
);

reg [31:0] rom [0:255];

always @(*) 
begin
    q <= rom[addr[8:2]]; // divide by 4 because not byte addressable
end

initial
begin
	q <= 32'd0;
    $readmemb("/home/bart/Documents/FPGA/FPGC6/Verilog/memory/instrMem.list", rom);
end

endmodule