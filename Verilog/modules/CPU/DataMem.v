module DataMem(
    input wire          clk,
    input wire  [31:0]  addr,
    input wire          we,
    input wire  [31:0]  data,
    output wire [31:0]  q
);

reg [31:0] mem [0:127];  // 32-bit memory with 128 entries

always @(posedge clk)
begin
    if (we)
    begin
        mem[addr] <= data;
    end
end

assign q = we ? data : mem[addr];
// During a write, avoid the one cycle delay by reading from 'wdata'

integer i;
initial
begin
    for (i = 0; i < 128; i = i + 1)
    begin
        mem[i] = 32'd0;
    end
end

endmodule