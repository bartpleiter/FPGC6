module MemoryUnit(
    // Clocks
    input           clk,
    input           reset,

    // Bus
    input [26:0]    bus_addr,
    input [31:0]    bus_data,
    input           bus_we,
    input           bus_start,
    output [31:0]   bus_q,
    output          bus_done,
    output          bus_ready
);

reg bus_done_reg = 1'b0;
reg bus_done_next = 1'b0;
reg bus_ready_reg = 1'b0;

//---------------------------SRAM---------------------------------
//SRAM I/O
wire        sram_cpu_clk;
wire [11:0] sram_cpu_addr;
wire [31:0] sram_cpu_d;
wire        sram_cpu_we; 
wire [31:0] sram_cpu_q;

assign sram_cpu_addr = bus_addr;
assign sram_cpu_d    = bus_data;
assign sram_cpu_we   = bus_we;

assign bus_q = sram_cpu_q;

SRAM #(
.WIDTH(32),
.WORDS(4096),
.ADDR_BITS(12),
.LIST("/home/bart/Documents/FPGA/FPGC6/Verilog/memory/sram.list")
)   sram(
//CPU port
.cpu_clk    (clk),
.cpu_d      (sram_cpu_d),
.cpu_addr   (sram_cpu_addr),
.cpu_we     (sram_cpu_we),
.cpu_q      (sram_cpu_q)
);

assign bus_done = (bus_addr < 8) ? bus_start : bus_done_reg;
assign bus_ready = bus_ready_reg;


always @(posedge clk)
begin
    if (reset)
    begin
        bus_done_reg <= 1'b0;
        bus_ready_reg <= 1'b0;
        bus_done_next <= 1'b0;
    end
    else
    begin
        bus_done_reg <= 1'b0;
        bus_ready_reg <= 1'b1;

        if (bus_done_next)
        begin
            bus_done_next <= 1'b0;
            bus_ready_reg <= 1'b0;
            bus_done_reg <= 1'b1;
        end
        else if (bus_addr >= 8)
        begin
            if (bus_start)
            begin
                bus_ready_reg <= 1'b0;
            end
            else if (!bus_ready_reg)
            begin
                bus_done_next <= 1'b1;
                bus_ready_reg <= 1'b1;
            end
        end
    end
end

endmodule
