module DataMem(
    input wire          clk,
    input wire  [31:0]  addr,
    input wire          we,
    input wire  [31:0]  data,
    output reg  [31:0]  q,
    input wire          clear, hold
);

reg [31:0] mem [0:127];  // 32-bit memory with 128 entries

// write
always @(posedge clk)
begin
    if (we)
    begin
        mem[addr] <= data;
        //$display("%d: addr%d := %d", $time, addr[7:0], data);
    end
end

// read
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
    else if (we) // during a write, avoid the one cycle delay by reading from 'wdata'
    begin
        q <= data;
    end
    else
    begin
        q <= mem[addr]; 
    end
end

integer i;
initial
begin
    for (i = 0; i < 128; i = i + 1)
    begin
        mem[i] = 32'd0;
    end
end

endmodule