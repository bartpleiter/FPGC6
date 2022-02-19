/*
* Register Bank
*/

module Regbank(
    input               clk, reset,

    input       [3:0]   addr_a, addr_b,
    output reg  [31:0]  data_a, data_b, 

    input       [3:0]   addr_d,
    input       [31:0]  data_d,
    input               we
);

reg [31:0] regs [0:15]; // 16 registers of 32 bit, although reg0 is unused

// Read
always @(*) 
begin
    if (addr_a == 4'd0)
    begin
        data_a <= 32'd0;
    end
    else if ((addr_a == addr_d) && we)
    begin
        data_a <= data_d;
    end
    else
    begin
        data_a <= regs[addr_a];
    end

    if (addr_b == 4'd0)
    begin
        data_b <= 32'd0;
    end
    else if ((addr_b == addr_d) && we)
    begin
        data_b <= data_d;
    end
    else
    begin
        data_b <= regs[addr_b];
    end
end

// Write
always @(posedge clk) 
begin
    if (we && addr_d != 4'd0)
    begin
        regs[addr_d] <= data_d;
        $display("%d: reg%d := %d", $time, addr_d, data_d);
    end
end

integer i;
initial
begin
    for (i = 0; i < 16; i = i + 1)
    begin
        regs[i] = 32'd0;
    end
end

endmodule