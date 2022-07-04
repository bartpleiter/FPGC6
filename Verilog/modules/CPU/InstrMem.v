/*
* Instruction Memory
*/

module InstrMem(
    input clk,
    input reset,
    input [31:0] addr,
    output hit,
    output [31:0] q,

    // bus
    output [31:0] bus_addr,
    output [31:0] bus_data,
    output        bus_we,
    output        bus_start,
    input [31:0]  bus_q,
    input         bus_done,

    input clear, hold
);


// in case of a clear mid transaction, ignore the next result and start again
reg ignoreNext = 1'b0;

assign hit = bus_done && !ignoreNext;
assign q =  (bus_done && !ignoreNext) ? bus_q : 32'd0;

assign bus_addr = addr;
assign bus_data = 32'd0;
assign bus_we = 1'b0;

assign bus_start = !bus_done && !hold;

always @(posedge clk) 
begin
    if (ignoreNext)
    begin
        if (bus_done)
        begin
            ignoreNext <= 1'b0;
        end
    end
    else if (clear)
    begin
        if (bus_start)
        begin
            ignoreNext <= 1'b1;
        end
    end
end

endmodule