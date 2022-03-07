/*
* Instruction Memory
*/

module InstrMem(
    input clk,
    input reset,
    input [31:0] addr,
    output hit,
    output [31:0] q,
    input clear, hold
);

assign hit = count == 3'd7; // 1'b1 if no delay
assign q = (hit) ? qreg : 32'd0;

reg [31:0] qreg = 32'd0;

reg [31:0] rom [0:255];

reg [2:0] count = 3'd0;

always @(posedge clk) 
begin
    count <= count + 1'b1;

    if (clear)
    begin
        qreg <= 32'd0;
    end
    else if (hold)
    begin
        qreg <= qreg;
    end
    else
    begin
        qreg <= rom[addr[8:2]]; // divide by 4 because not byte addressable
    end
end

initial
begin
    $readmemb("/home/bart/Documents/FPGA/FPGC6/Verilog/memory/instrMem.list", rom);
end

endmodule