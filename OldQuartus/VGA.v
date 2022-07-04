module VGA(
   input clk,
   input  [1:0] switch,
   input  [7:0] piano,
   output [7:0] dataout,
   output [7:0] en,
   output wire [11:0] leds
);

wire [7:0] nPiano;
assign nPiano = ~piano;

wire [11:0] nLeds;
assign nLeds = {~switch[0], 10'd0}; //{4'd0, nPiano};
assign leds = ~nLeds;


wire [31:0] r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15;
CPU cpu(
.clk(clk),
.reset(~switch[0]),
.r1(r1),
.r2(r2),
.r3(r3),
.r4(r4),
.r5(r5),
.r6(r6),
.r7(r7),
.r8(r8),
.r9(r9),
.r10(r10),
.r11(r11),
.r12(r12),
.r13(r13),
.r14(r14),
.r15(r15)
);


reg [31:0] leftDigit;
always @(*) 
begin
    leftDigit <= 32'd0;
    case ({nPiano[4], nPiano[5], nPiano[6], nPiano[7]})
    4'd0: leftDigit <= 32'd0;
    4'd1: leftDigit <= r1;
    4'd2: leftDigit <= r2;
    4'd3: leftDigit <= r3;
    4'd4: leftDigit <= r4;
    4'd5: leftDigit <= r5;
    4'd6: leftDigit <= r6;
    4'd7: leftDigit <= r7;
    4'd8: leftDigit <= r8;
    4'd9: leftDigit <= r9;
    4'd10: leftDigit <= r10;
    4'd11: leftDigit <= r11;
    4'd12: leftDigit <= r12;
    4'd13: leftDigit <= r13;
    4'd14: leftDigit <= r14;
    4'd15: leftDigit <= r15;
    endcase
end

reg [31:0] rightDigit;
always @(*) 
begin
    rightDigit <= 32'd0;
    case ({nPiano[0], nPiano[1], nPiano[2], nPiano[3]})
    4'd0: rightDigit <= 32'd0;
    4'd1: rightDigit <= r1;
    4'd2: rightDigit <= r2;
    4'd3: rightDigit <= r3;
    4'd4: rightDigit <= r4;
    4'd5: rightDigit <= r5;
    4'd6: rightDigit <= r6;
    4'd7: rightDigit <= r7;
    4'd8: rightDigit <= r8;
    4'd9: rightDigit <= r9;
    4'd10: rightDigit <= r10;
    4'd11: rightDigit <= r11;
    4'd12: rightDigit <= r12;
    4'd13: rightDigit <= r13;
    4'd14: rightDigit <= r14;
    4'd15: rightDigit <= r15;
    endcase
end

wire [3:0] SSD0, SSD1, SSD2, SSD3, SSD4, SSD5, SSD6, SSD7;

BCD16 bcdR1(
.clk(clk),
.bin(leftDigit),
.tk(),
.k(SSD3),
.h(SSD2),
.t(SSD1),
.s(SSD0)
);

BCD16 bcdR2(
.clk(clk),
.bin(rightDigit),
.tk(),
.k(SSD7),
.h(SSD6),
.t(SSD5),
.s(SSD4)
);

LedTube ledTube (
.clk(clk),
.dataout(dataout),
.en(en),
.d1(SSD0),
.d2(SSD1),
.d3(SSD2),
.d4(SSD3),
.d5(SSD4),
.d6(SSD5),
.d7(SSD6),
.d8(SSD7)
);


endmodule
