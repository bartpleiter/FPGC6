/*
* Arbiter
* Regulates access to the CPU memory bus from both Instruction and Data memory
* Port a (instruction memory) will directly access the bus (no latency)
* When port b (data memory) requests an access, it will stop port a when it is finished and give access to port b
* This will give port b quite some latency, but as port b requests are more rare, this should be okay (and can be reduced by l1d cache)
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
    output [26:0] bus_addr,
    output [31:0] bus_data,
    output        bus_we ,
    output        bus_start,
    input [31:0]  bus_q,
    input         bus_done
);

assign q = bus_q;

assign bus_addr     = (!port_b_access) ? addr_a   : bus_addr_reg;
assign bus_data     = (!port_b_access) ? data_a   : bus_data_reg;
assign bus_we       = (!port_b_access) ? we_a     : bus_we_reg;
assign bus_start    = (!port_b_access) ? start_a  : (bus_start_reg && !bus_done);

assign done_a       = (!port_b_access) && bus_done;
assign done_b       = (state == state_wait_b_done) && bus_done;


reg port_b_access = 1'b0;

reg [26:0] bus_addr_reg = 27'd0;
reg [31:0] bus_data_reg = 32'd0;
reg bus_we_reg          = 1'b0;
reg bus_start_reg       = 1'b0;

// state machine
reg [2:0] state = 3'd0; // 0-7 states limit
parameter state_idle            = 3'd0;
parameter state_wait_b_done     = 3'd1;

always @(posedge clk) 
begin
    if (reset)
    begin
        port_b_access   <= 1'b0;
        
        bus_addr_reg    <= 27'd0;
        bus_data_reg    <= 32'd0;
        bus_we_reg      <= 1'b0;
        bus_start_reg   <= 1'b0;

        state           <= state_idle;
    end
    else
    begin
        case(state)
            state_idle: 
            begin
                // if port b is requested and port a is just finished
                if (!start_a && bus_done && start_b)
                begin
                    // give access to port b before a starts a new request
                    port_b_access   <= 1'b1;

                    bus_addr_reg    <= addr_b;
                    bus_data_reg    <= data_b;
                    bus_we_reg      <= we_b;
                    bus_start_reg   <= 1'b1;

                    state <= state_wait_b_done;
                end
                
            end
            state_wait_b_done:
            begin
                if (bus_done)
                begin
                    // return access to port a
                    state           <= state_idle;
                    port_b_access   <= 1'b0;
                end
            end
        endcase
    end
end

endmodule