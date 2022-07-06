/*
* Data Memory
*/

module DataMem(
    input wire          clk, reset,
    input wire  [31:0]  addr,
    input wire          we,
    input wire          re,
    input wire  [31:0]  data,
    output wire [31:0]  q,
    output              busy,

    // bus
    output [31:0] bus_addr,
    output [31:0] bus_data,
    output        bus_we,
    output        bus_start,
    input [31:0]  bus_q,
    input         bus_done,

    input wire          clear, hold
);

reg [31:0] qreg = 32'd0;

assign bus_addr = addr;
assign bus_data = data;
assign bus_we = we;
assign bus_start = !bus_done && (we || re);
assign busy = bus_start;
assign q = (bus_done) ? bus_q : qreg;

always @(posedge clk)
begin
    // skip clear, because currently not needed
    /*if (clear)
    begin
        q <= 32'd0;
    end
    else */
    // skip hold, because currently not needed
    /*
    if (hold)
    begin
        q <= q;
    end
    else */

    if (reset)
    begin
        qreg <= 32'd0;
    end
    else
    begin
        if (bus_done)
        begin
            qreg <= bus_q;
        end
    end
end


endmodule