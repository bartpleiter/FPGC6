/*
* Stack
* mainly used for backing up registers in assembly
* 32 bits wide, can store an entire register per entry
* 128 words long
* TODO optional: send interrupt when pop on empty stack
*/

module Stack (
    input clk,
    input reset,
    input [31:0] d,
    output reg [31:0] q,
    input push,
    input pop,
    input clear, hold
);

reg [6:0]   ptr;            // stack pointer
reg [31:0]  stack [127:0];  // stack

always @(posedge clk)
begin
    if (reset)
    begin
        ptr <= 7'd0;
    end
    else 
    begin
        if (push)
        begin
            stack[ptr] <= d;
            ptr <= ptr + 1'b1;
            //$display("%d: push @%d := %d", $time, ptr, d);
        end

        if (pop)
        begin
            if (clear)
            begin
                q <= 32'd0;
            end
            else if (hold)
            begin
                q <= q;
            end
            else
            begin
                ptr <= ptr - 1'b1;
                q <= stack[ptr - 1'b1]; // simulation does not like this when ptr = 0
                //$display("%d: pop @%d := %d", $time, ptr, stack[ptr - 1'b1]);
            end
        end
    end
end

integer i;
initial
begin
    q <= 32'd0;
    ptr = 7'd0;
    for (i = 0; i < 128; i = i + 1)
    begin
        stack[i] = 32'd0;
    end
end

endmodule