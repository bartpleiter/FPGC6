/*
* Top level design of the FPGC6
*/
module FPGC6(
    input           clk, // 100MHz
    input           nreset,
    
    //Led for debugging
    output          led
);

//--------------------Reset&Stabilizers-----------------------
//Reset signals
wire nreset_stable, reset;

MultiStabilizer multistabilizer (
.clk(clk),
.u0(nreset),
.s0(nreset_stable)
);

assign reset = ~nreset_stable;


// Bus
wire [26:0] bus_addr;
wire [31:0] bus_data;
wire        bus_we;
wire        bus_start;
wire [31:0] bus_q;
wire        bus_done;

MemoryUnit memoryunit(
// Clocks
.clk            (clk),
.reset          (reset),

// Bus
.bus_addr       (bus_addr),
.bus_data       (bus_data),
.bus_we         (bus_we),
.bus_start      (bus_start),
.bus_q          (bus_q),
.bus_done       (bus_done)
);

//---------------CPU----------------
CPU cpu(
.clk            (clk),
.reset          (reset),

// bus
.bus_addr       (bus_addr),
.bus_data       (bus_data),
.bus_we         (bus_we),
.bus_start      (bus_start),
.bus_q          (bus_q),
.bus_done       (bus_done),

.led            (led)
);

endmodule