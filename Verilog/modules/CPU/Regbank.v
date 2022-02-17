/*
* Register Bank
*/

module Regbank(
    input           clk, reset,

    input   [3:0]   addr_a, addr_b,
    output  [31:0]  data_a, data_b, 

    input   [3:0]   addr_d,
    input   [31:0]  data_d,
    input           we, we_high
);


assign data_a = {data_a_h, data_a_l};
assign data_b = {data_b_h, data_b_l};

reg [15:0] regsH [0:15];    // highest 16 bits of regbank
reg [15:0] regsL [0:15];    // lowest 16 bits of regbank

reg [15:0] data_a_l, data_a_h, data_b_l, data_b_h;

// Read
always @(*) 
begin
    if (addr_a == 4'd0)
    begin
        data_a_l <= 16'd0;
        data_a_h <= 16'd0;
    end
    else if ((addr_a == addr_d) && we)
    begin
        if (we_high)
        begin
            data_a_l <= regsL[addr_a];
            data_a_h <= data_d[15:0];
        end
        else
        begin
            data_a_l <= data_d[15:0];
            data_a_h <= data_d[31:16];
        end
    end
    else
    begin
        data_a_l <= regsL[addr_a];
        data_a_h <= regsH[addr_a];
    end

    if (addr_b == 4'd0)
    begin
        data_b_l <= 16'd0;
        data_b_h <= 16'd0;
    end
    else if ((addr_b == addr_d) && we)
    begin
        if (we_high)
        begin
            data_b_l <= regsL[addr_b];
            data_b_h <= data_d[15:0];
        end
        else
        begin
            data_b_l <= data_d[15:0];
            data_b_h <= data_d[31:16];
        end
    end
    else
    begin
        data_b_l <= regsL[addr_b];
        data_b_h <= regsH[addr_b];
    end
end

// Write
// to only write the top 16 bits, both we and we_high must be set
always @(posedge clk) 
begin
    if (we && addr_d != 4'd0)
    begin
        if (we_high)
        begin
            regsH[addr_d] <= data_d[15:0];
        end
        else 
        begin
            regsL[addr_d] <= data_d[15:0];
            regsH[addr_d] <= data_d[31:16];
        end
    end
end

integer i;
initial
begin
    data_a_l = 16'd0;
    data_b_l = 16'd0;
    data_a_h = 16'd0;
    data_b_h = 16'd0;

    for (i = 0; i < 16; i = i + 1)
    begin
        regsL[i] = 16'd0;
        regsH[i] = 16'd0;
    end
end

endmodule