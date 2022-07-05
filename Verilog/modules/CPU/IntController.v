/*
* Interrupt controller
* On interrupt, CPU should save the PC and jump to ADDR 1
*  and restore everything on reti
*/

module IntController(
    input clk, reset,
    input int1, int2, int3, int4, int5, int6, int7, int8,

    input intDisabled,
    output reg intCPU = 1'b0,
    output reg [7:0] intID = 8'd0

);

reg int1_prev = 1'b0;
reg int2_prev = 1'b0;

reg int1_triggered = 1'b0;
reg int2_triggered = 1'b0;

always @(posedge clk) 
begin
    int1_prev <= int1;
    int2_prev <= int2;

    if (int1 && !int1_prev)
        int1_triggered <= 1'b1;

    if (int2 && !int2_prev)
        int2_triggered <= 1'b1;

    // trigger new interrupt if available
    if (!intDisabled && !intCPU)
    begin
        if (int1_triggered)
        begin
            int1_triggered <= 1'b0;
            intCPU <= 1'b1;
            intID = 8'd1;
        end
        else if (int2_triggered)
        begin
            int2_triggered <= 1'b0;
            intCPU <= 1'b1;
            intID = 8'd2;
        end
    end

    if (intDisabled)
    begin
        intCPU <= 1'b0;
    end
end

endmodule