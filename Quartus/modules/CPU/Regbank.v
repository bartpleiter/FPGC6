/*
* Register Bank
*/

module Regbank(
    input               clk, reset,

    input       [3:0]   addr_a, addr_b,
    output reg  [31:0]  data_a, data_b, 

    input       [3:0]   addr_d,
    input       [31:0]  data_d,
    input               we, clear, hold,

    output wire [31:0] r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15
);

reg [31:0] regs [0:15]; // 16 registers of 32 bit, although reg0 is unused

assign r1 = regs[1];
assign r2 = regs[2];
assign r3 = regs[3];
assign r4 = regs[4];
assign r5 = regs[5];
assign r6 = regs[6];
assign r7 = regs[7];
assign r8 = regs[8];
assign r9 = regs[9];
assign r10 = regs[10];
assign r11 = regs[11];
assign r12 = regs[12];
assign r13 = regs[13];
assign r14 = regs[14];
assign r15 = regs[15];

// Read
always @(posedge clk) 
begin
    if (clear)
    begin
        data_a <= 32'd0;
    end
    else if (hold)
    begin
        data_a <= data_a;
    end
    else if (addr_a == 4'd0)
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

    if (clear)
    begin
        data_b <= 32'd0;
    end
    else if (hold)
    begin
        data_b <= data_b;
    end
    else if (addr_b == 4'd0)
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