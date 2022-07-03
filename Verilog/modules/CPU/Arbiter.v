/*
* Arbiter
* Regulates access to the CPU memory bus from both Instruction and Data memory
* In case of two requests at the same time, Data memory is granted first
*/

module Arbiter(
    input clk,
    input reset,

    // port a (Instr)
    input [31:0] addr_a,
    input [31:0] data_a,
    input        we_a,
    input        start_a,
    output       done_a,

    // port b (Data)
    input [31:0] addr_b,
    input [31:0] data_b,
    input        we_b,
    input        start_b,
    output       done_b,

    // output (both ports)
    output [31:0] q,

    // bus
    output reg [26:0] bus_addr = 27'd0,
    output reg [31:0] bus_data = 32'd0,
    output reg       bus_we = 1'b0,
    output       bus_start,
    input [31:0]  bus_q,
    input         bus_done
);

assign q = bus_q;

assign done_a = busy_a && !busy_b && bus_done;
assign done_b = busy_b && bus_done;

reg busy_a = 1'b0;
reg busy_b = 1'b0;

reg bus_start_reg = 1'b0;
assign bus_start = (bus_start_reg) && (!bus_done);

always @(posedge clk) 
begin
    
    if (start_b && (!busy_a || bus_done))
    begin
        bus_start_reg <= 1'b1;
        bus_addr <= addr_b;
        bus_data <= data_b;
        bus_we <= we_b;
        busy_b <= 1'b1;
    end
    else if (start_a && (!busy_b || bus_done))
    begin
        bus_start_reg <= 1'b1;
        bus_addr <= addr_a;
        bus_data <= data_a;
        bus_we <= we_a;
        busy_a <= 1'b1;
    end
    
    if (bus_done)
    begin
        if (!busy_b)
        begin
            busy_a <= 1'b0;
        end

        busy_b <= 1'b0;
        bus_start_reg <= 1'b0;
    end

end


endmodule