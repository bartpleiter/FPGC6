# CPU (B32P)
The B32P (B4rt 32 bit Pipelined processor) is a 32 bit RISC CPU with a 5-stage pipeline, implementing the self designed B32P instruction set.
Both the B32P and its ISA are a big improvement over the previous B322 version, making it the largest difference between the FPGC5 and FPGC6.

!!! info 
    [link to the B322 CPU for comparison](https://www.b4rt.nl/fpgc5/cpu.html)

## B32P ISA
The B322 instruction set architecture is a RISC architecture.
Each instruction is 32 bits and can be one of the following instructions:
``` text
         |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
----------------------------------------------------------------------------------------------------------
1 HALT     1  1  1  1| 1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1 
2 READ     1  1  1  0||----------------16 BIT CONSTANT---------------||--A REG---| x  x  x  x |--D REG---|
3 WRITE    1  1  0  1||----------------16 BIT CONSTANT---------------||--A REG---||--B REG---| x  x  x  x 
4 INTID    1  1  0  0| x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x |--D REG---|
5 PUSH     1  0  1  1| x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x |--B REG---| x  x  x  x 
6 POP      1  0  1  0| x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x |--D REG---|
7 JUMP     1  0  0  1||--------------------------------27 BIT CONSTANT--------------------------------||O|
8 JUMPR    1  0  0  0||----------------16 BIT CONSTANT---------------| x  x  x  x |--B REG---| x  x  x |O|
9 XXX
10 BRANCH  0  1  1  0||----------------16 BIT CONSTANT---------------||--A REG---||--B REG---||-OPCODE||S|
11 SAVPC   0  1  0  1| x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x |--D REG---|
12 RETI    0  1  0  0| x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x 
13 XXX
14 XXX
15 ARITHC  0  0  0  1||--OPCODE--||----------------16 BIT CONSTANT---------------||--A REG---||--D REG---|
16 ARITH   0  0  0  0||--OPCODE--| x  x  x  x  x  x  x  x  x  x  x  x |--A REG---||--B REG---||--D REG---|
```

1.  `HALT`:   Will prevent the CPU to go to the next instruction by jumping to the same address. Can be interrupted.
2.  `READ`:   Read from memory at address in AREG + (signed) 16 bit offset, store value in DREG.
3.  `WRITE`:  Write value from BREG to memory at address stored in AREG + (signed) 16 bit offset.
4.  `INTID`:  Store the interrupt ID in DREG
5.  `PUSH`:   Pushes value in AREG to stack.
6.  `POP`:    Pops value from stack into DREG.
7.  `JUMP`:   Set PC to 27 bit constant if O is 0. If O is 1, then add the 27 bit constant to PC. 
8.  `JUMPR`:  Set PC to BREG + (signed) 16 bit constant if O is 0. If O is 1, then add the value from BREG + (signed) 16 bit constant to PC. 
9.  `XXX`:    Currently reserved for future instructions
10. `BRANCH`: Compare AREG to BREG depending on the branch opcode. If S is 1, then used signed comparison. If branch pass, add (signed) 16 bit constant to PC.
11. `SAVPC`:  Save current PC to DREG.
12. `RETI`:   Restore PC after interrupt and re-enable interrupts.
13. `XXX`:    Currently reserved for future instructions
14. `XXX`:    Currently reserved for future instructions
15. `ARITHC`: Execute operation specified by OPCODE on AREG and (signed) 16 bit constant. Write result to DREG.
16. `ARITH`:  Execute operation specified by OPCODE on AREG and BREG. Write result to DREG.

### BRANCH opcodes
The type of branch operation can be specified by the branch opcode:
``` text
Operation|Opcode|Description
-------------------------
BEQ       000    Branch if A == B
BGT       001    Branch if A >  B
BGE       010    Branch if A >= B
XXX       011    Reserved
BNE       100    Branch if A != B
BLT       101    Branch if A <  B
BLE       110    Branch if A <= B
XXX       111    Reserved
```
Signed comparisons can be enabled by setting the S (sign) bit, creating the BGTS, BGES, BLTS and BLES operations.

### ARITH opcodes
The type of ALU operation can be specified by the ARITH opcode:
``` text
Operation|Opcode|Description
-------------------------
OR        0000   A OR   B
AND       0001   A AND  B
XOR       0010   A XOR  B
ADD       0011   A  +   B
SUB       0100   A  -   B
SHIFTL    0101   A  <<  B
SHIFTR    0110   A  >>  B
NOT(A)    0111   ~A
MULTS     1000   A  *   B (signed)
MULTU     1001   A  *   B (unsigned)
SLT       1010   1 if A < B, else 0 (signed)
SLTU      1011   1 if A < B, else 0 (unsigned)
LOAD      1100   16 bit constant (or B)
LOADHI    1101   {16 bit constant, A[15:0]} (if AREG == DREG, this is equivalent to LOADHI)
```

The remaining opcodes are reserved for future operations.

## Pipeline
The B32P has a 5-stage pipeline, very similar to the MIPS CPU. The stages are as follows:

- FE (fetch): Obtain instruction from memory
- DE (decode): Decode instruction and read register bank
- EX (execute): ALU operation and jump/branch calculation
- MEM (memory access): Stack and memory access
- WB (write back): Write to register bank


### Hazard detection and branch prediction
The CPU detects pipeline hazards, removing the need for the programmer to account for this, by doing the following things depending on the situation:

- Flush (if jump/branch/halt)
- Stall (if EX uses result of MEM)
- Forwarding (MEM -> EX and WB -> EX)

Branch prediction is done by always assuming that the branch did not pass (which is the easiest to implement).

### Memory access and latency
Because the FPGC does not have a separate instruction and data memory, both stages could need access to the MU at the same time.
To handle this, an arbiter is used that gives priority to the MEM request (since these only occur for READ/WRITE instructions) and lets the FETCH request stall until the bus is free.
Access to memory via the MU will take a variable amount of cycles depending on the memory type that is being accessed. The pipeline will stall during this delay.

Memory access is still word addressable, as byte addressable memory add too much complications for now.

## Register bank
Contains 16 registers which are all 32 bit wide. Aside from `R0` which is always zero, all registers are basically GP register. However, to maintain some kind of coding consistency, some registers have a special function assigned (though their hardware implementation are the same).

The 16 32 bit registers have the current functions:
``` text
Register|Hardware   |Assembly   |C
-----------------------------------------------------
R0      |Always 0   |Always 0   |Always 0
R1      |GP         |Arg|retval |Very local temp reg
R2      |GP         |Arg|retval |Ret0
R3      |GP         |Arg|retval |Ret1
R4      |GP         |GP         |Arg0
R5      |GP         |GP         |Arg1
R6      |GP         |GP         |Arg2
R7      |GP         |GP         |Arg3
R8      |GP         |GP         |GP0
R9      |GP         |GP         |GP1
R10     |GP         |GP         |GP2
R11     |GP         |GP         |Temp
R12     |GP         |GP         |Temp
R13     |GP         |GP         |SP
R14     |GP         |GP         |BP
R15     |GP         |Ret Ptr    |Ret addr
```
The register bank has two read ports and one write port. Internally on the FPGA, the registers are implemented in block RAM to increase performance and save space. Note that the program counter is not part of the register bank, as it is a separate part of the CPU, which can be obtained using the SAVPC instruction.

## Stack
Stack memory with internal stack pointer. The stack is mostly used in assembly coding for jumping to functions and backing up or restoring registers in interrupt handlers or functions. In combination with the SavPC instruction, one can easily jump to (and return from) functions in assembly (C uses a software stack).

The pointer wraps around in case of a push when the stack is full or in case of a pop when the stack is empty.
The stack is 128 words deep, allowing for 8 full register bank backups. The stack pointer and stack memory are not accessible by the rest of the CPU. This and the small size make the stack mostly unusable for the C compiler. For this, a software stack implementation using the SDRAM main memory and some registers are used. However, the hardware stack is used to quickly backup and restore all 15 GP registers during an interrupt.

## Interrupts
The CPU has an extendable amount of interrupt pins. The interrupt controller detects rising edges for each interrupt pin and handles them one at a time, with higher priority for lower pin numbers. If the CPU has interrupts disabled (because it might be in one already or the system is not ready yet), then the interrupts will be processed when the CPU has them enabled again. This way the CPU will not miss any, unless a new one triggers before the previous one on the same pin was sent to the CPU. When the CPU handles an interrupt, it will disable interrupts, save the PC and jump to a certain address (TODO: this will probably be address 1). Then, the interrupt ID can de obtained using the INTID instruction. Of course, it is recommended to push all registers to the stack before doing this. After a RETI instruction, interrupts will be enabled again and the CPU will jump to the saved PC.

## Main differences with B322
- Pipelining (5-stage)
- Possibility for adding both an Instruction and Data cache (L1-cache) (currently not implemented)
- Possibility for adding a L2 cache (currently not implemented)
- More branch instructions while only using a single instruction opcode
- Better support for signed numbers, 16 bit constant is now signed
- LOAD and LOADHI are now ARITH instructions
- More ALU opcodes, like SLT and SLTU which are very useful for the MIPS-based C compiler
- Better and more extendable interrupts

## TODO
- proper reset
- more testing
