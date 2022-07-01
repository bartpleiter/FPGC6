/*
* Data Memory
*/

module DataMem(
    input wire          clk,
    input wire  [31:0]  addr,
    input wire          we,
    input wire          re,
    input wire  [31:0]  data,
    output reg  [31:0]  q,
    output wire         busy,
    input wire          clear, hold
);

//reg [31:0] qreg = 32'd0;
//assign q = (!busy) ? qreg : 32'd0;

reg [31:0] mem [0:127];  // 32-bit memory with 128 entries


reg [2:0] count = 3'd0;
assign busy = count != 3'd0;

always @(posedge clk)
begin
    count <= count + 1'b1;
end

// write
always @(posedge clk)
begin
    if (we)
    begin
        mem[addr] <= data;
        $display("%d: addr%d := %d", $time, addr[7:0], data);
    end
end

// read
always @(posedge clk)
begin
    // skip clear, because currently not needed
    /*if (clear)
    begin
        qreg <= 32'd0;
    end
    else */
    if (hold)
    begin
        q <= q;
    end
    else if (we) // during a write, pass input directly to the output
    begin
        q <= data;
    end
    else if (re)
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