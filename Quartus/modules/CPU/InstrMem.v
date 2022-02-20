/*
* Instruction Memory
*/

module InstrMem(
    input clk,
    input reset,
    input [31:0] addr,
    output reg [31:0] q,
    input clear, hold
);

reg [31:0] rom [0:255];

always @(posedge clk) 
begin
    if (clear)
    begin
        q <= 32'd0;
    end
    else if (hold)
    begin
        q <= q;
    end
    else
    begin
        q <= rom[addr[8:2]]; // divide by 4 because not byte addressable
    end
end

initial
begin
    q <= 32'd0;
    $readmemb("/home/bart/Documents/FPGA/FPGC6/Verilog/memory/instrMem.list", rom);
end

endmodule