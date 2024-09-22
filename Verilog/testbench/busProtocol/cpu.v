module CPU(
    input clk, reset,
    output [26:0] bus_addr,
    output [31:0] bus_data,
    output bus_we,
    output reg bus_start,
    input [31:0] bus_q,
    input bus_done, bus_ready,
    input clear, hold
);

reg ignoreNext;
reg  [31:0] addr = 0;

assign bus_addr = addr;
assign bus_data = 32'd0;
assign bus_we = 1'b0;

always @(posedge clk) 
begin
    if (reset)
        ignoreNext <= 1'b0;
    else if (ignoreNext && bus_done)
        ignoreNext <= 1'b0;
    else if (clear && bus_start)
        ignoreNext <= 1'b1;
end

always @(posedge clk)
begin
    if (reset)
    begin
        addr <= 0;
        bus_start <= 0;
    end
    else
    begin

        if (bus_ready && !hold && !(bus_start && !bus_done))
        begin
            bus_start <= 1'b1;
            addr <= addr + 1;
        end
        else
            bus_start <= 1'b0;
    end
end

endmodule
