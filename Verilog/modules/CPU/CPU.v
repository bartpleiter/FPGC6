/*
* B32P CPU
*/

/* Features:
- 5 stage pipeline
    - fetch             FE   (1)
    - decode            DE   (2)
    - execute           EX   (3)
    - memory            MEM  (4)
    - write back        WB   (5)

- Hazard detection:
    - flush
    - stall (MEM to reg)
    - forward
*/

module CPU(
    input clk, reset
);

// Registers for flush, stall and forwarding
reg flush_FE, flush_DE, flush_EX, flush_MEM, flush_WB;
reg stall_FE, stall_DE, stall_EX, stall_MEM, stall_WB;
reg [1:0] forward_a, forward_b;

/*
* FETCH (FE)
*/

// Program Counter
reg [31:0]  pc_FE = 32'd0;

wire [31:0] pc4_FE;
assign pc4_FE = pc_FE + 3'd4;

always @(posedge clk) 
begin
    if (stall_FE)
    begin
        pc_FE <= pc_FE;
    end
    else if (jumpc_MEM || jumpr_MEM || halt_MEM || (branch_MEM && branch_passed_MEM))
    begin
        pc_FE <= jump_addr_MEM;
    end
    else
    begin
        pc_FE <= pc4_FE;
    end
end


// Instruction Memory
//  should eventually become a memory with variable latency
// writes directly to next stage
wire [31:0] instr_DE;

InstrMem instrMem(
.clk(clk), 
.reset(reset),
.addr(pc_FE),
.q(instr_DE),
.hold(stall_FE),
.clear(flush_FE)
);


// Pass data from FE to DE
wire [31:0] pc4_DE;
Regr #(.N(32)) regr_pc4_FE_DE(
.clk(clk),
.hold(stall_FE),
.clear(flush_FE),
.in(pc4_FE),
.out(pc4_DE)
);


/*
* DECODE (DE)
*/

// Instruction Decoder
wire [3:0] areg_DE, breg_DE, instrOP_DE;
wire he_DE, oe_DE, sig_DE;

InstructionDecoder instrDec_DE(
.instr(instr_DE),

.instrOP(instrOP_DE),
.aluOP(),

.constAlu(),
.const16(),
.const27(),

.areg(areg_DE),
.breg(breg_DE),
.dreg(),

.he(he_DE),
.oe(oe_DE),
.sig(sig_DE)
);

// Control Unit
wire alu_use_const_DE;
wire push_DE, pop_DE;
wire dreg_we_DE;
wire mem_write_DE, mem_read_DE;
wire jumpc_DE, jumpr_DE, branch_DE, halt_DE;
wire getIntID_DE, getPC_DE;
ControlUnit controlUnit(
// in
.instrOP        (instrOP_DE),
.he             (he_DE),

// out
.alu_use_const  (alu_use_const_DE),
.push           (push_DE),
.pop            (pop_DE),
.dreg_we        (dreg_we_DE),
.mem_write      (mem_write_DE),
.mem_read       (mem_read_DE),
.jumpc          (jumpc_DE),
.jumpr          (jumpr_DE),
.halt           (halt_DE),
.branch         (branch_DE),
.getIntID       (getIntID_DE),
.getPC          (getPC_DE)
);


// Register Bank
// writes directly to next stage
wire [31:0] data_a_EX, data_b_EX;
wire [3:0] dreg_WB;
wire dreg_we_WB;
reg [31:0] data_d_WB;

Regbank regbank(
.clk(clk),
.reset(reset),

.addr_a(areg_DE),
.addr_b(breg_DE),
.data_a(data_a_EX),
.data_b(data_b_EX),

// from WB stage
.addr_d(dreg_WB),
.data_d(data_d_WB),
.we(dreg_we_WB),

.hold(stall_DE),
.clear(flush_DE)
);


// Pass data from DE to EX

// Set to 0 during stall (bubble)
wire [31:0] instr_EX;
Regr #(.N(32)) regr_instr_DE_EX(
.clk(clk),
.hold(stall_DE),
.clear(flush_DE || stall_DE),
.in(instr_DE),
.out(instr_EX)
);

wire [31:0] pc4_EX;
Regr #(.N(32)) regr_pc4_DE_EX(
.clk(clk),
.hold(stall_DE),
.clear(flush_DE),
.in(pc4_DE),
.out(pc4_EX)
);

// Set to 0 during stall (bubble)
wire alu_use_const_EX;
wire push_EX, pop_EX;
wire dreg_we_EX;
wire mem_write_EX, mem_read_EX;
wire jumpc_EX, jumpr_EX, halt_EX, branch_EX;
wire getIntID_EX, getPC_EX;
Regr #(.N(12)) regr_cuflags_DE_EX(
.clk        (clk),
.hold       (stall_DE),
.clear      (flush_DE || stall_DE),
.in         ({alu_use_const_DE, push_DE, pop_DE, dreg_we_DE, mem_write_DE, mem_read_DE, jumpc_DE, jumpr_DE, halt_DE, branch_DE, getIntID_DE, getPC_DE}),
.out        ({alu_use_const_EX, push_EX, pop_EX, dreg_we_EX, mem_write_EX, mem_read_EX, jumpc_EX, jumpr_EX, halt_EX, branch_EX, getIntID_EX, getPC_EX})
);


/*
* EXECUTE (EX)
*/

// Instruction Decoder
wire [31:0] alu_const16_EX;
wire [3:0] aluOP_EX;
wire [3:0] areg_EX, breg_EX, dreg_EX;

InstructionDecoder instrDec_EX(
.instr(instr_EX),

.instrOP(),
.aluOP(aluOP_EX),

.constAlu(alu_const16_EX),
.const16(),
.const27(),

.areg(areg_EX),
.breg(breg_EX),
.dreg(dreg_EX),

.he(),
.oe(),
.sig()
);


// ALU
wire [31:0] alu_result_EX;

// select constant or register for input b
wire[31:0] alu_input_b_EX;
assign alu_input_b_EX = (alu_use_const_EX) ? alu_const16_EX : data_b_EX;

// if forwarding, select forwarded data instead for input a of ALU
reg [31:0] fw_data_a_EX;
always @(*)
begin
    case (forward_a)
        2'd1:       fw_data_a_EX <= alu_result_MEM;
        2'd2:       fw_data_a_EX <= data_d_WB;
        default:    fw_data_a_EX <= data_a_EX;
    endcase
end

// if forwarding, select forwarded data instead for input b of ALU
reg [31:0] fw_data_b_EX;
always @(*)
begin
    case (forward_b)
        2'd1:       fw_data_b_EX <= alu_result_MEM;
        2'd2:       fw_data_b_EX <= data_d_WB;
        default:    fw_data_b_EX <= alu_input_b_EX;
    endcase
end

ALU alu(
.opcode(aluOP_EX),
.a(fw_data_a_EX),
.b(fw_data_b_EX),
.y(alu_result_EX)
);

// for special instructions, pass other data than alu result
wire [31:0] execute_result_EX;
assign execute_result_EX =  (getPC_EX) ? pc4_EX - 3'd4:
                            (getIntID_EX) ? 32'd0: // TODO add after interrupts are implemented
                            alu_result_EX;


// Pass data from EX to MEM

wire [31:0] instr_MEM;
Regr #(.N(32)) regr_instr_EX_MEM(
.clk(clk),
.hold(stall_EX),
.clear(flush_EX),
.in(instr_EX),
.out(instr_MEM)
);

wire [31:0] data_a_MEM, data_b_MEM;
Regr #(.N(64)) regr_regdata_EX_MEM(
.clk(clk),
.hold(stall_EX),
.clear(flush_EX),
.in({fw_data_a_EX, fw_data_b_EX}), // forwarded data
.out({data_a_MEM, data_b_MEM})
);

wire [31:0] pc4_MEM;
Regr #(.N(32)) regr_pc4_EX_MEM(
.clk(clk),
.hold(stall_EX),
.clear(flush_EX),
.in(pc4_EX),
.out(pc4_MEM)
);

wire push_MEM, pop_MEM;
wire dreg_we_MEM;
wire mem_write_MEM, mem_read_MEM;
wire jumpc_MEM, jumpr_MEM, halt_MEM, branch_MEM;
Regr #(.N(9)) regr_cuflags_EX_MEM(
.clk        (clk),
.hold       (stall_EX),
.clear      (flush_EX),
.in         ({push_EX, pop_EX, dreg_we_EX, mem_write_EX, mem_read_EX, jumpc_EX, jumpr_EX, halt_EX, branch_EX}),
.out        ({push_MEM, pop_MEM, dreg_we_MEM, mem_write_MEM, mem_read_MEM, jumpc_MEM, jumpr_MEM, halt_MEM, branch_MEM})
);

wire [31:0] alu_result_MEM;
Regr #(.N(32)) regr_alu_result_EX_MEM(
.clk(clk),
.hold(stall_EX),
.clear(flush_EX),
.in(execute_result_EX), // other data in case of special instructions
.out(alu_result_MEM)
);

/*
* MEMORY (MEM)
*/


// Instruction Decoder
wire [31:0] const16_MEM;
wire [26:0] const27_MEM;
wire [2:0] branchOP_MEM;
wire oe_MEM, sig_MEM;
wire [3:0] dreg_MEM;

InstructionDecoder instrDec_MEM(
.instr(instr_MEM),

.instrOP(),
.aluOP(),
.branchOP(branchOP_MEM),

.constAlu(),
.const16(const16_MEM),
.const27(const27_MEM),

.areg(),
.breg(),
.dreg(dreg_MEM),

.he(),
.oe(oe_MEM),
.sig(sig_MEM)
);


reg [31:0] jump_addr_MEM;

always @(*) 
begin
    jump_addr_MEM <= 32'd0;

    if (jumpc_MEM)
    begin
        if (oe_MEM)
        begin
            // add sign extended to allow negative offsets
            jump_addr_MEM <= pc4_MEM + {{5{const27_MEM[26]}}, const27_MEM[26:0]};
        end
        else
        begin
           jump_addr_MEM <= {5'd0, const27_MEM};
        end
    end

    else if (jumpr_MEM)
    begin
        if (oe_MEM)
        begin
            jump_addr_MEM <= pc4_MEM + (data_b_MEM + const16_MEM);
        end
        else
        begin
            jump_addr_MEM <= data_b_MEM + const16_MEM;
        end
    end
    
    else if (branch_MEM)
    begin
        jump_addr_MEM <= pc4_MEM + const16_MEM;
    end

    else if (halt_MEM)
    begin
        // jump to same address to keep halting
        jump_addr_MEM <= pc4_MEM - 3'd4;
    end
end


// Opcodes
localparam 
    BRANCH_OP_BEQ   = 3'b000, // A == B
    BRANCH_OP_BGT   = 3'b001, // A >  B
    BRANCH_OP_BGE   = 3'b010, // A >= B
    BRANCH_OP_U1    = 3'b011, // Unimplemented 1
    BRANCH_OP_BNE   = 3'b100, // A != B
    BRANCH_OP_BLT   = 3'b101, // A <  B
    BRANCH_OP_BLE   = 3'b110, // A <= B
    BRANCH_OP_U2    = 3'b111; // Unimplemented 2

reg branch_passed_MEM;

always @(*) 
begin
    branch_passed_MEM <= 1'b0;

    case (branchOP_MEM)
        BRANCH_OP_BEQ:
        begin
            branch_passed_MEM <= (data_a_MEM == data_b_MEM);
        end
        BRANCH_OP_BGT:
        begin
            branch_passed_MEM <= (sig_MEM) ? ($signed(data_a_MEM) > $signed(data_b_MEM)) : (data_a_MEM > data_b_MEM);
        end
        BRANCH_OP_BGE:
        begin
            branch_passed_MEM <= (sig_MEM) ? ($signed(data_a_MEM) >= $signed(data_b_MEM)) : (data_a_MEM >= data_b_MEM);
        end
        BRANCH_OP_BNE:
        begin
            branch_passed_MEM <= (data_a_MEM != data_b_MEM);
        end
        BRANCH_OP_BLT:
        begin
            branch_passed_MEM <= (sig_MEM) ? ($signed(data_a_MEM) < $signed(data_b_MEM)) : (data_a_MEM < data_b_MEM);
        end
        BRANCH_OP_BLE:
        begin
            branch_passed_MEM <= (sig_MEM) ? ($signed(data_a_MEM) <= $signed(data_b_MEM)) : (data_a_MEM <= data_b_MEM);
        end
    endcase
end


// Data Memory
//  should eventually become a memory with variable latency
// writes directly to the next stage
wire [31:0] dataMem_q_WB;
wire [31:0] dataMem_addr_MEM;
assign dataMem_addr_MEM = data_a_MEM + const16_MEM;

DataMem dataMem(
.clk(clk),
.addr(dataMem_addr_MEM),
.we(mem_write_MEM),
.data(data_b_MEM),
.q(dataMem_q_WB),
.hold(stall_MEM),
.clear(flush_MEM)
);

// Stack
// writes directly to the next stage
wire [31:0] stack_q_WB;

Stack stack(
.clk(clk),
.reset(reset),
.q(stack_q_WB),
.d(data_b_MEM),
.push(push_MEM),
.pop(pop_MEM),
.hold(stall_MEM),
.clear(flush_MEM)
);


// Pass data from MEM to WB

wire [31:0] instr_WB;
Regr #(.N(32)) regr_instr_MEM_WB(
.clk(clk),
.hold(stall_MEM),
.clear(flush_MEM),
.in(instr_MEM),
.out(instr_WB)
);

wire [31:0] alu_result_WB;
Regr #(.N(32)) regr_alu_result_MEM_WB(
.clk(clk),
.hold(stall_MEM),
.clear(flush_MEM),
.in(alu_result_MEM),
.out(alu_result_WB)
);

wire [31:0] pc4_WB;
Regr #(.N(32)) regr_pc4_MEM_WB(
.clk(clk),
.hold(stall_MEM),
.clear(flush_MEM),
.in(pc4_MEM),
.out(pc4_WB)
);

wire pop_WB, mem_read_WB;
//wire dreg_we_WB;
Regr #(.N(3)) regr_cuflags_MEM_WB(
.clk        (clk),
.hold       (stall_MEM),
.clear      (flush_MEM),
.in         ({pop_MEM, dreg_we_MEM, mem_read_MEM}),
.out        ({pop_WB, dreg_we_WB, mem_read_WB})
);

/*
* WRITE BACK (WB)
*/
wire [15:0] const16u_WB;

InstructionDecoder instrDec_WB(
.instr(instr_WB),

.instrOP(),
.aluOP(),

.constAlu(),
.const16(),
.const16u(const16u_WB),
.const27(),

.areg(),
.breg(),
.dreg(dreg_WB),

.he(),
.oe(),
.sig()
);

always @(*) 
begin
    case (1'b1)
        pop_WB:
        begin
            data_d_WB <= stack_q_WB;
        end
        mem_read_WB:
        begin
            data_d_WB <= dataMem_q_WB;
        end
        default: // (ALU, savPC, IntID)
        begin
            data_d_WB <= alu_result_WB;
        end
    endcase
end


/*
* FLUSH
*/
always @(*)
begin
    flush_FE <= 1'b0;
    flush_DE <= 1'b0;
    flush_EX <= 1'b0;
    flush_MEM <= 1'b0;
    flush_WB <= 1'b0;

    // flush on jumps
    if (jumpc_MEM || jumpr_MEM || halt_MEM || (branch_MEM && branch_passed_MEM))
    begin
        flush_FE <= 1'b1;
        flush_DE <= 1'b1;
        flush_EX <= 1'b1;
    end
end

/*
* STALL
*/
always @(*)
begin
    stall_FE <= 1'b0;
    stall_DE <= 1'b0;
    stall_EX <= 1'b0;
    stall_MEM <= 1'b0;
    stall_WB <= 1'b0;

    // stall if an instruction in EX uses the result of a some operation in MEM (dreg_mem)
    if ((mem_read_EX || pop_EX) && ( (dreg_EX == areg_DE) || (dreg_EX == breg_DE)) )
    begin
        stall_FE <= 1'b1;
        stall_DE <= 1'b1;
    end
end

/*
* FORWARDING
* TODO: find a fix for loadhi
*/

// MEM (4) -> EX (3)
// WB  (5) -> EX (3)
always @(*) begin

    // input a of ALU
    forward_a <= 2'd0;  // default to no forwarding
    if (dreg_we_MEM && (dreg_MEM == areg_EX) && (areg_EX != 4'd0))
    begin
        forward_a <= 2'd1;  // priority 1: forward from MEM to EX
    end
    else if (dreg_we_WB && (dreg_WB == areg_EX) && (areg_EX != 4'd0))
    begin
        forward_a <= 2'd2;  // priority 2: forward from WB to EX
    end

    // input b of ALU
    forward_b <= 2'd0;  // default to no forwarding
    if (dreg_we_MEM && (dreg_MEM == breg_EX) && (breg_EX != 4'd0))
    begin
        forward_b <= 2'd1;  // priority 1: forward from MEM to EX
    end
    else if (dreg_we_WB && (dreg_WB == breg_EX) && (breg_EX != 4'd0))
    begin
        forward_b <= 2'd2;  // priority 2: forward from WB to EX
    end
        
end


endmodule