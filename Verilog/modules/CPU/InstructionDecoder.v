/*
* Instruction Decoder
*/

module InstructionDecoder(
    input   [31:0]  instr,
    
    output  [3:0]   instrOP,
    output  [3:0]   aluOP,

    output  [31:0]  constAlu,
    output  [31:0]  const16,
    output  [15:0]  const16u,
    output  [26:0]  const27,

    output  [3:0]   areg, breg, dreg,

    output          he, oe, sig
);

assign instrOP  = instr[31:28];
assign aluOP    = instr[27:24];

assign constAlu = {{16{instr[23]}}, instr[23:8]}; // sign extend to 32 bit
assign const16  = {{16{instr[27]}}, instr[27:12]}; // sign extend to 32 bit
assign const16u = instr[27:12];
assign const27  = instr[27:1];

assign areg     = instr[11:8];
assign breg     = instr[7:4];
assign dreg     = instr[3:0];

assign he       = instr[8];     // high-enable (loadhi)
assign oe       = instr[0];     // offset-enable (jump[r])
assign sig      = instr[0];     // signed comparison (branch)

endmodule