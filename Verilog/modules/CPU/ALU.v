/*
* ALU
*/

module ALU(
    input       [31:0]  a, b,
    input       [3:0]   opcode,
    output reg  [31:0]  y
);

// Opcodes
localparam 
    OP_OR       = 4'b0000, // OR
    OP_AND      = 4'b0001, // AND
    OP_XOR      = 4'b0010, // XOR
    OP_ADD      = 4'b0011, // Add
    OP_SUB      = 4'b0100, // Substract
    OP_SHIFTL   = 4'b0101, // Shift left
    OP_SHIFTR   = 4'b0110, // Shift right
    OP_NOTA     = 4'b0111, // ~A
    OP_MULTS    = 4'b1000, // Multiplication signed
    OP_MULTU    = 4'b1001, // Multiplication unsigned
    OP_SLT      = 4'b1010, // Set on less than signed
    OP_SLTU     = 4'b1011, // Set on less than unsigned
    OP_LOAD     = 4'b1100, // Load input B ( y := b )
    OP_LOADHI   = 4'b1101, // Loadhi input B ( y := {(b<<16), a} )
    OP_U6       = 4'b1110, // Unimplemented
    OP_U7       = 4'b1111; // Unimplemented

always @ (*) 
begin
    case (opcode)
        OP_OR:      y <= a | b;
        OP_AND:     y <= a & b;
        OP_XOR:     y <= a ^ b;
        OP_ADD:     y <= a + b;
        OP_SUB:     y <= a - b;
        OP_SHIFTL:  y <= a << b[5:0];
        OP_SHIFTR:  y <= a >> b[5:0];
        OP_NOTA:    y <= ~a;
        OP_MULTS:   y <= $signed(a) * $signed(b);
        OP_MULTU:   y <= a * b;
        OP_SLT:     y <= {{31{1'b0}}, ($signed(a) < $signed(b))};
        OP_SLTU:    y <= {{31{1'b0}}, (a < b)};
        OP_LOAD:    y <= b;
        OP_LOADHI:  y <= {b[15:0], a[15:0]};
        default:    y <= 32'd0;
    endcase
end

endmodule