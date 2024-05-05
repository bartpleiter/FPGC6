# Assembler for B32P

The basic way to write code for the B32P is by using the B32P assembly language. The assembler compiles the assembly code to 32 bit machine instructions. The input file is currently hardcoded to `code.asm`, and the output is printed to stdout. The assembler is written in Python3. A [special version written in BCC](#bcc-version) can be run from the FPGC itself.

!!! info
    Since the addition of the C compiler BCC, the focus of the assembler now lies more on assembling BCC's output instead. Still, the bootloaders are written in assembly and some BCC libraries, like the graphics library, also make extensive use of assembly code.

## Command line arguments
There are three types of programs the assembler can create, indicated by command line arguments:

- BDOS OS [arg `os`]. Special mode for assembling the BDOS operating system. This adds a jumpt to the syscall function on address 4, and adds the length of the binary to address 2.
- BDOS User Program [arg `bdos {offset}`]. Special mode for assembling BDOS user programs. No program length is included and the assembler will offset labels to make the code executeable from the given offset.
- Bare metal program [no args]. Basic mode for assembling bare metal programs without OS. Adds length of the binary to address 2.

Finally, by adding the `-O` argument at the end will cause the assembler to remove unreached code, which is quite useful for compiled C code because of the lack of dynamic linking. This will save quite some space as C libraries are getting more functions over time.

## Output
The assembler does not directly create a binary. Instead, it outputs the assembled code as a text file containing binary strings of 32 ones and zeros, follwed by a space and comments starting with `//`. This is very useful to see what each instruction is supposed to do, and makes it easy to verify this with the ISA.

Example output:
```
00010011000000000000110011011101 //Compute r13 + 12 and write result to r13
00011100001001110100010000100010 //Set r2 to 10052
```

The output can be converted into a binary using other tools. For example: `perl -ne 'print pack("B32", $_)' < filename_containing_assembler_output > executable.bin`

## Assembly process
The assembler performs the following steps in order

1. Read input file into list of lines while removing all comments
2. Move all `.data`, `.rdata` and `.bss` sections down so the `.code` section becomes one part at the top (only relevant for C compiled code)
3. Insert libraries (only relevant for non-C compiled code)
4. Remove unreachable code if requested
5. Process the define statements
7. Do pass 1: Compiles lines that can be compiled directly (so without labels) and create new lines for instructions that become multiple lines
8. Add header code
9. Process labels
10. Do pass 2: Compile instructions with labels
11. Check for remaining labels (there should be none left)
12. Add length of program if requested
13. Print result to stdout

## Header

The assembler creates the first few lines of the program, depending on the program type (header code). Because of a bug in B32P, in some cases the CPU will start at address 3 instead of address 0 after the bootloader. Until this is fixed, the 4th instruction also is a jump to Main. For a `-os` program, this looks as follows:
``` text
Jump Main
Jump Int
[Length of program]
Jump Main
Jump Syscall
```

## Line types
Each line is parsed on its own. There are six types of lines:

- Assembler directives
- Includes
- Comments
- Defines
- Labels
- Instructions

### Assembler directives
Because the BCC compiler is a modified MIPS compiler, its output contains directives indicating the purpose of the code below. These directives include:

- `.code`, normal code
- `.data` and `.rdata`, static data
- `.bss`, object data

The assembler will move each non-code section down so it does not interfere with the code.

### Includes
By adding an \`include namehere.asm statement, it is possible to add code from other files, like libraries. The way this works in the assembler is by just adding all lines of that file to the code, while recursively importing includes from other files. The assembler makes sure that the same file is never included more than one time. The path to the file is relative to the assembler. This is only relevant for non-C code, as the C compiler should not produce assembly includes.

### Comments
Comments can be added by using the ';' character. For each line, only the part until the first ';' occurrence will be used by the assembler. This means that anything can be written after the ';'. This all does not hold for .ds lines. They must not have any comments. This way it is not needed to use escape characters in the strings.

### Defines
Defines are the first type of lines that are processed by the assembler. A define line should have the following structure:
``` asm
define TEXTTOREPLACE = textToReplaceWith
```
It is not necessary for 'define' to be in lower caps and 'TEXTTOREPLACE' to be in all caps. However, it is recommended to do so as a coding style. Also, it is recommended to place all define statements at the top of the file before the first instruction or label.
The define statement is used as a textual replacement. This means that no values or anything will be processed or converted during the processing of the define statements. This also means that is not smart to replace text that are used in instructions or labels.
Furthermore, 'TEXTTOREPLACE' should be unique for each define statement. If not, the assembler will complain.

### Labels
Labels can be used to get the address in the assembled code of the instruction below the label. To define a label, one should use the following syntax:
``` asm
LabelName:
```
LabelName can consist of any character, including numbers and special characters, as long as it is just one word (so no spaces, newlines etc.) The label must end with a ':' character. On the same line after this ':' character, no other text is allowed, except comments.

A label can be referenced by using the LabelName in certain instructions at specific places (see section Instructions).
The assembler will eventually compute the address of the label and replace all LabelName occurrences with this address. One can make forward and backward references, which means that one does not have to define a label earlier in the code before referencing, as long as the label is defined somewhere in the code.

Each program should at least contain the following labels (if not, the assembler will complain):
``` asm
Main: ; this is where the CPU will initially jump to
Int: ; interrupt handler
```

It is recommended to start each label with a capital letter, however this is not mandatory.
You can use two labels directly after each other without any instruction in between, but you should not use a label at the end of the file without anything under it. The assembler will complain if this happens.
When two identical labels are defined, the assembler will complain.
It does not matter if a label is never referenced.
However, it does matter when a reference is made to a label that is not defined. In that case the assembler will complain.
Each of the interrupt handler label sections should finish with a reti instruction, otherwise the CPU will not return from the interrupt and will probably lock up. However, this is not checked by the assembler.

### Instructions
The instructions are the lines that will be assembled into machine code. Each instruction has its own format with the following description:
``` text
Instr   | Arg1  | Arg2 | Arg3   || Description
================================||=====================================================================
HALT    |       |       |       || Halts CPU by jumping to the current address
READ    | C16   | R     | R     || Read from addr in Arg2 with 16 bit offset from Arg1. Write to Arg3
WRITE   | C16   | R     | R     || Write to addr in Arg2 with 16 bit offset from Arg1. Data to write is in Arg3
READINTID| R    |       |       || Stores the interrupt ID from the CPU to Arg1
PUSH    | R     |       |       || Push Arg1 to stack
POP     | R     |       |       || Pop from stack to Arg1
JUMP    | L/C27 |       |       || Jump to Label or 27 bit constant in Arg1
JUMPO   | C27   |       |       || Jump to (unsigned) 27 bit constant offset in Arg1
JUMPR   | C16   | R     |       || Jump to Arg2 with 16 bit offset in Arg1
JUMPRO  | C16   | R     |       || Jump to offset in Arg2 with 16 bit offset in Arg1
BEQ     | R     | R     | C16   || If Arg1 == Arg2, jump to 16 bit offset in Arg3
BGT     | R     | R     | C16   || If Arg1 >  Arg2, jump to 16 bit offset in Arg3
BGTS    | R     | R     | C16   || (signed) If Arg1 >  Arg2, jump to 16 bit offset in Arg3
BGE     | R     | R     | C16   || If Arg1 >= Arg2, jump to 16 bit offset in Arg3
BGES    | R     | R     | C16   || (signed) If Arg1 >= Arg2, jump to 16 bit offset in Arg3
BNE     | R     | R     | C16   || If Arg1 != Arg2, jump to 16 bit offset in Arg3
BLT     | R     | R     | C16   || If Arg1 <  Arg2, jump to 16 bit offset in Arg3
BLTS    | R     | R     | C16   || (signed) If Arg1 <  Arg2, jump to 16 bit offset in Arg3
BLE     | R     | R     | C16   || If Arg1 <= Arg2, jump to 16 bit offset in Arg3
BLES    | R     | R     | C16   || (signed) If Arg1 <= Arg2, jump to 16 bit offset in Arg3
SAVPC   | R     |       |       || Save program counter to Arg1
RETI    |       |       |       || Return from interrupt
OR      | R     | C16/R | R     || Compute Arg1 OR  Arg2, write result to Arg3
AND     | R     | C16/R | R     || Compute Arg1 AND Arg2, write result to Arg3
XOR     | R     | C16/R | R     || Compute Arg1 XOR Arg2, write result to Arg3
ADD     | R     | C16/R | R     || Compute Arg1 +   Arg2, write result to Arg3
SUB     | R     | C16/R | R     || Compute Arg1 -   Arg2, write result to Arg3
SHIFTL  | R     | C16/R | R     || Compute Arg1 <<  Arg2, write result to Arg3
SHIFTR  | R     | C16/R | R     || Compute Arg1 >>  Arg2, write result to Arg3
SHIFTRS | R     | C16/R | R     || Compute Arg1 (signed)>> Arg2, write result to Arg3
NOT     | R     | R     |       || Compute NOT  (~) Arg1, write result to Arg2
MULTS   | R     | C16/R | R     || (signed) Compute Arg1 * Arg2, write result to Arg3
MULTU   | R     | C16/R | R     || (unsigned) Compute Arg1 * Arg2, write result to Arg3
MULTFP  | R     | C16/R | R     || (signed, FixedPoint16.16) Compute Arg1 * Arg2, write result to Arg3
SLT     | R     | C16/R | R     || (signed) If Arg1 < Arg2, write 1 to Arg3, else write 0 to Arg3
SLTU    | R     | C16/R | R     || (unsigned) If Arg1 < Arg2, write 1 to Arg3, else write 0 to Arg3
LOAD    | C16   | R     |       || Load (unsigned) 16 bit constant from Arg1 into Arg2
LOADHI  | C16   | R     |       || Load (unsigned) 16 bit constant from Arg1 into highest 16 bits of Arg2
LOAD32  | C32   | R     |       || Load signed 32 bit constant from Arg1 into Arg2. Is converted into LOAD and LOADHI
NOP     |       |       |       || Does nothing, is converted to the instruction OR r0 r0 r0
ADDR2REG| L     | R     |       || Loads address from Arg1 to Arg2. Is converted into LOAD and LOADHI
.DW     | N32   | *     | *     || Data: Each argument is converted to 32bit binary
.DD     | N16   | *     | *     || Data: Each argument is converted to 16bit binary **
.DB     | N8    | *     | *     || Data: Each argument is converted to 8bit binary **
.DS     | N8    | S     |       || Data: Each character of the string is converted to 8bit ASCII **

/   = Or
R   = Register
Cx  = Constant that fits within x bits
L   = Label
S   = String

Note: All constants are signed except stated otherwise
*  Optional argument with same type as Arg1. Has no limit on number of arguments
** Data is placed after each other to make blocks of 32 bits. If a block cannot be made, it will be padded by zeros
```

Each Cx type argument (constant) can be written in decimal, binary (with 0b prefix) or hex (with 0x prefix).

## BCC version
To allow for assembling code directly from the FPGC/BDOS itself, a special version of the assembler is written in BCC. This version lacks several features like includes, defines and line number errors, since its goal is to only assemble the output of BCC (specifically the BCC version running on the FPGC/BDOS as well). It is also more optimized for speed (the compiler, not the code). It also only assembles for BDOS User Programs, so if you want to assemble manually written assembly, you will need to add the userBDOS assembly wrappers yourself.
