/*
Copyright (c) 2021-2022, bartpleiter
Copyright (c) 2012-2015, Alexey Frunze
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*****************************************************************************/
/*                                                                           */
/*                           BCC (B32P C Compiler)                           */
/*                                                                           */
/*                           C compiler for B32P                             */
/*                                                                           */
/*                            Based on SmallerC:                             */
/*                 A simple and small single-pass C compiler                 */
/*                                                                           */
/*                           B32P code generator                             */
/*                  Modified from the MIPS code generator                    */
/*                                                                           */
/*****************************************************************************/

// NOTE: Pretty inefficient generated code, because of the direct-ish translation from MIPS to B32P

/* MAIN TODOs:
- Improve using new B32P instructions
- Remove all MIPS code leftovers
- Do optimizations (like size stuff)
*/

/* WORDSIZE issue tags (to look into if I will ever try to fix it)
//WORDSIZE
//CurFxnLocalOfs (gcc.c)
*/


STATIC
void GenInit(void)
{
  // initialization of target-specific code generator
  // Assembler should move all .data and .rdata parts away from the code
  SizeOfWord = 4;
  OutputFormat = FormatSegmented;
  CodeHeaderFooter[0] = ".code";
  DataHeaderFooter[0] = ".data";
  RoDataHeaderFooter[0] = ".rdata";
  BssHeaderFooter[0] = ".bss"; // object data
  UseLeadingUnderscores = 0;
}

STATIC
int GenInitParams(int argc, char** argv, int* idx)
{
  return 0;
}

STATIC
void GenInitFinalize(void)
{
  // finalization of initialization of target-specific code generator
  // Put all C specific wrapper code (start) here

  if (compileUserBDOS)
  {
    printf2(
      ".code\n"
      "; Setup stack and return function before jumping to Main of BDOS user program\n"
      "; BDOS user programs have their stack to keep the other stacks intact\n"
      "Main:\n"
      "    ccache                  ; clear cache\n"
      "    load32 0 r14            ; initialize base pointer address\n"
      "    load32 0x73FFFF r13     ; initialize user main stack address\n"
      "    addr2reg Return_BDOS r1 ; get address of return function\n"
      "    or r0 r1 r15            ; copy return addr to r15\n"
      "    jump main               ; jump to main of C program\n"
      "    halt                    ; should not get here\n"
      "\n"
      "; Function that is called after Main of C user program has returned\n"
      "; Here we do some post processing before jumping back to BDOS\n"
      "; Its adrress should be on top of the hardware stack\n"
      "Return_BDOS:\n"
      "    ; notify program has ended\n"
      "    load32 0x100300 r1\n"
      "    write 0 r1 r0\n"
      "\n"
      "    pop r1\n"
      "    jumpr 3 r1\n"
      "    halt    ; should not get here\n"
      "\n"
      "; COMPILED C CODE HERE\n");
  }
  else
  {
    printf2(
      ".code\n"
      "; Setup stack and return function before jumping to Main of C program\n"
      "Main:\n"
      "    ccache                  ; clear cache\n"
      "    load32 0 r14            ; initialize base pointer address\n"
      "    load32 0x77FFFF r13     ; initialize main stack address\n"
      "    addr2reg Return_UART r1 ; get address of return function\n"
      "    or r0 r1 r15            ; copy return addr to r15\n"
      "    jump main               ; jump to main of C program\n"
      "                            ; should return to the address in r15\n"
      "    halt                    ; should not get here\n"
      "\n"
      "; Function that is called after Main of C program has returned\n"
      "; Return value should be in R1\n"
      "; Send it over UART and halt afterwards\n"
      "Return_UART:\n"
      "    load32 0xC02723 r1          ; r1 = 0xC02723 | UART tx\n"
      "    write 0 r1 r2               ; write r2 over UART\n"
      "    halt                        ; halt\n"
      "\n"
      "; COMPILED C CODE HERE\n");
  }
}

STATIC
void GenStartCommentLine(void)
{
  printf2(" ; ");
}

// No alignment needed on B32P
STATIC
void GenWordAlignment(int bss)
{
  (void)bss;
  printf2("; .align 2\n");
}

STATIC
void GenLabel(char* Label, int Static)
{
  {
    if (!Static && GenExterns)
      printf2("; .globl %s\n", Label);
    printf2("%s:\n", Label);
  }
}

STATIC
void GenPrintLabel(char* Label)
{
  {
    if (isdigit(*Label))
      printf2("Label_%s", Label);
    else
      printf2("%s", Label);
  }
}

STATIC
void GenNumLabel(int Label)
{
  printf2("Label_%d:\n", Label);
}

STATIC
void GenPrintNumLabel(int label)
{
  printf2("Label_%d", label);
}

STATIC
void GenZeroData(unsigned Size, int bss)
{
  (void)bss;
  printf2("; .space %u\n", truncUint(Size));

  // B32P implementation of .space:
  if (Size > 0)
  {
    printf2(".dw");
    int i;
    for (i = 0; i < Size; i++)
    {
      printf2(" 0");
    }
    printf2("\n");
  }

}

STATIC
void GenIntData(int Size, int Val)
{
  Val = truncInt(Val);

  // Print multiple times, since the compiler does not know yet B32P is word addressable
  if (Size == 1)
    printf2(" .dw %d\n", Val);
  else if (Size == 2)
    printf2(" .dw %d %d\n", Val, Val);
  else if (Size == 4)
    printf2(" .dw %d %d %d %d\n", Val, Val, Val, Val);
  
}

STATIC
void GenStartAsciiString(void)
{
  printf2(".dw "); // String should be converted into 1 character per word
}

STATIC
void GenAddrData(int Size, char* Label, int ofs)
{
  ofs = truncInt(ofs);

  int i;
  for (i = 0; i < 4; i++) // label is 4 "bytes", hotfix since the compiler does not know yet B32P is word addressable
  {
    printf2(".dl ");

    GenPrintLabel(Label);

    // Still not sure if this ever gets called (and probably will not work until an Assembler update)
    if (ofs)
      printf2(" %+d", ofs);
    puts2("");
  }
}

STATIC
int GenFxnSizeNeeded(void)
{
  return 0;
}

STATIC
void GenRecordFxnSize(char* startLabelName, int endLabelNo)
{
  (void)startLabelName;
  (void)endLabelNo;
}

#define B32PInstrHalt      0x30
#define B32PInstrRead      0x31
#define B32PInstrWrite     0x32
#define B32PInstrIntID     0x33
#define B32PInstrPush      0x34
#define B32PInstrPop       0x35
#define B32PInstrJump      0x36
#define B32PInstrJumpo     0x37
#define B32PInstrJumpr     0x38
#define B32PInstrJumpro    0x39
#define B32PInstrBEQ       0x3A
#define B32PInstrBGT       0x3B
#define B32PInstrBGTS      0x3C
#define B32PInstrBGE       0x3D
#define B32PInstrBGES      0x3E
#define B32PInstrBNE       0x3F
#define B32PInstrBLT       0x40
#define B32PInstrBLTS      0x41
#define B32PInstrBLE       0x42
#define B32PInstrBLES      0x43
#define B32PInstrSavPC     0x44
#define B32PInstrReti      0x45
#define B32PInstrOR        0x46
#define B32PInstrAND       0x47
#define B32PInstrXOR       0x48
#define B32PInstrADD       0x49
#define B32PInstrSUB       0x4A
#define B32PInstrSHIFTL    0x4B
#define B32PInstrSHIFTR    0x4C
#define B32PInstrNOT       0x4D
#define B32PInstrMULTS     0x4E
#define B32PInstrMULTU     0x4F
#define B32PInstrSLT       0x50
#define B32PInstrSLTU      0x51
#define B32PInstrLoad      0x52
#define B32PInstrLoadHi    0x53
#define B32PInstrAddr2reg  0x54
#define B32PInstrLoad32    0x55
#define B32PInstrNOP       0x56
#define B32PInstrSHIFTRS   0x57

STATIC
void GenPrintInstr(int instr, int val)
{
  char* p = "";

  (void)val;

  switch (instr)
  {
  case B32PInstrHalt      : p = "halt"; break;
  case B32PInstrRead      : p = "read"; break;
  case B32PInstrWrite     : p = "write"; break;
  case B32PInstrIntID     : p = "readintid"; break;
  case B32PInstrPush      : p = "push"; break;
  case B32PInstrPop       : p = "pop"; break;
  case B32PInstrJump      : p = "jump"; break;
  case B32PInstrJumpo     : p = "jumpo"; break;
  case B32PInstrJumpr     : p = "jumpr"; break;
  case B32PInstrJumpro    : p = "jumpro"; break;
  case B32PInstrBEQ       : p = "beq"; break;
  case B32PInstrBGT       : p = "bgts"; break; // HACK: Default signed comparison, because of MIPS
  case B32PInstrBGTS      : p = "bgts"; break;
  case B32PInstrBGE       : p = "bges"; break; // HACK: Default signed comparison, because of MIPS
  case B32PInstrBGES      : p = "bges"; break;
  case B32PInstrBNE       : p = "bne"; break;
  case B32PInstrBLT       : p = "blts"; break; // HACK: Default signed comparison, because of MIPS
  case B32PInstrBLTS      : p = "blts"; break;
  case B32PInstrBLE       : p = "bles"; break; // HACK: Default signed comparison, because of MIPS
  case B32PInstrBLES      : p = "bles"; break;
  case B32PInstrSavPC     : p = "savpc"; break;
  case B32PInstrReti      : p = "reti"; break;
  case B32PInstrOR        : p = "or"; break;
  case B32PInstrAND       : p = "and"; break;
  case B32PInstrXOR       : p = "xor"; break;
  case B32PInstrADD       : p = "add"; break;
  case B32PInstrSUB       : p = "sub"; break;
  case B32PInstrSHIFTL    : p = "shiftl"; break;
  case B32PInstrSHIFTR    : p = "shiftr"; break;
  case B32PInstrSHIFTRS   : p = "shiftrs"; break;
  case B32PInstrNOT       : p = "not"; break;
  case B32PInstrMULTS     : p = "mults"; break;
  case B32PInstrMULTU     : p = "multu"; break;
  case B32PInstrSLT       : p = "slt"; break;
  case B32PInstrSLTU      : p = "sltu"; break;
  case B32PInstrLoad      : p = "load32"; break;
  case B32PInstrLoadHi    : p = "loadhi"; break;
  case B32PInstrAddr2reg  : p = "addr2reg"; break;
  case B32PInstrLoad32    : p = "load32"; break;
  }

  printf2(" %s ", p);
}

#define B32POpRegZero                    0x00 //0  0
#define B32POpRegAt                      0x01 //1  at
#define B32POpRegV0                      0x02 //2  ret0
#define B32POpRegV1                      0x03 //3  ret1
#define B32POpRegA0                      0x04 //4  arg0
#define B32POpRegA1                      0x05 //5  arg1
#define B32POpRegA2                      0x06 //6  arg2
#define B32POpRegA3                      0x07 //7  arg3
#define B32POpRegT0                      0x08 //8  gp0
#define B32POpRegT1                      0x09 //9  gp1
#define B32POpRegT2                      0x0A //10 gp2
#define B32POpRegT8                      0x0B //11 tempa
#define B32POpRegT9                      0x0C //12 tempb
#define B32POpRegSp                      0x0D //13 sp
#define B32POpRegFp                      0x0E //14 fp
#define B32POpRegRa                      0x0F //15 retaddr

#define B32POpIndRegZero                 0x20
#define B32POpIndRegAt                   0x21
#define B32POpIndRegV0                   0x22
#define B32POpIndRegV1                   0x23
#define B32POpIndRegA0                   0x24
#define B32POpIndRegA1                   0x25
#define B32POpIndRegA2                   0x26
#define B32POpIndRegA3                   0x27
#define B32POpIndRegT0                   0x28
#define B32POpIndRegT1                   0x29
#define B32POpIndRegSp                   0x2D
#define B32POpIndRegFp                   0x2E
#define B32POpIndRegRa                   0x2F

#define B32POpConst                      0x80
#define B32POpLabel                      0x81
#define B32POpNumLabel                   0x82
#define B32POpLabelLo                    0x83
#define B32POpIndLocal                   B32POpIndRegFp

#define MAX_TEMP_REGS 3 // this many temp registers used beginning with T0 to hold subexpression results
#define TEMP_REG_A B32POpRegT8 // two temporary registers used for momentary operations, similarly to the AT register
#define TEMP_REG_B B32POpRegT9

STATIC
void GenPrintOperand(int op, int val)
{
  if (op >= B32POpRegZero && op <= B32POpRegRa)
  {
    printf2("r%d", op);
  }
  else if (op >= B32POpIndRegZero && op <= B32POpIndRegRa)
  {
    printf2("%d r%d", truncInt(val), op - B32POpIndRegZero);
  }
  else
  {
    switch (op)
    {
    case B32POpConst: printf2("%d", truncInt(val)); break;
    case B32POpLabelLo:
      printf2("%%lo(");
      GenPrintLabel(IdentTable + val);
      printf2(")(r1)");
      break;
    case B32POpLabel: GenPrintLabel(IdentTable + val); break;
    case B32POpNumLabel: GenPrintNumLabel(val); break;

    default:
      errorInternal(100);
      break;
    }
  }
}

STATIC
void GenPrintOperandSeparator(void)
{
  printf2(" ");
}

STATIC
void GenPrintNewLine(void)
{
  puts2("");
}

STATIC
void GenPrintInstr1Operand(int instr, int instrval, int operand, int operandval)
{
  GenPrintInstr(instr, instrval);
  GenPrintOperand(operand, operandval);
  GenPrintNewLine();
}

STATIC
void GenPrintInstr2Operands(int instr, int instrval, int operand1, int operand1val, int operand2, int operand2val)
{
  // TODO: figure out if this ever happens because ADD and SUB need 3 args
  if (operand2 == B32POpConst && operand2val == 0 &&
      (instr == B32PInstrADD || instr == B32PInstrSUB))
    return;

  GenPrintInstr(instr, instrval);
  GenPrintOperand(operand1, operand1val);
  GenPrintOperandSeparator();
  GenPrintOperand(operand2, operand2val);
  GenPrintNewLine();
}

STATIC
void GenPrintInstr3Operands(int instr, int instrval,
                            int operand1, int operand1val,
                            int operand2, int operand2val,
                            int operand3, int operand3val)
{
  if (operand3 == B32POpConst && operand3val == 0 &&
      (instr == B32PInstrADD || instr == B32PInstrSUB) &&
      operand1 == operand2)
    return;

  // If constant is negative, swap B32PInstrADD for B32PInstrSUB and vice versa
  // and flip sign of constant
  if (operand2 == B32POpConst && operand2val < 0)
  {
    if (instr == B32PInstrADD)
    {
      instr = B32PInstrSUB;
      operand2val = -operand2val;
    }
    else if (instr == B32PInstrSUB)
    {
      instr = B32PInstrADD;
      operand2val = -operand2val;
    }
  }

  GenPrintInstr(instr, instrval);
  GenPrintOperand(operand1, operand1val);
  GenPrintOperandSeparator();
  GenPrintOperand(operand2, operand2val);
  GenPrintOperandSeparator();
  GenPrintOperand(operand3, operand3val);
  GenPrintNewLine();
}


// Currently we do not want to "extend" any reg
STATIC
void GenExtendRegIfNeeded(int reg, int opSz)
{
  
}

STATIC
void GenJumpUncond(int label)
{
  GenPrintInstr1Operand(B32PInstrJump, 0,
                        B32POpNumLabel, label);
}

extern int GenWreg; // GenWreg is defined below

STATIC
void GenJumpIfEqual(int val, int label)
{

  GenPrintInstr2Operands(B32PInstrLoad, 0,
                         B32POpConst, val,
                         TEMP_REG_B, 0);
  /*
  GenPrintInstr3Operands(MipsInstrBEQ, 0,
                         GenWreg, 0,
                         TEMP_REG_B, 0,
                         B32POpNumLabel, label);
  */
  GenPrintInstr3Operands(B32PInstrBNE, 0,
                         GenWreg, 0,
                         TEMP_REG_B, 0,
                         B32POpConst, 2);
  GenPrintInstr1Operand(B32PInstrJump, 0,
                         B32POpNumLabel, label);
}

STATIC
void GenJumpIfZero(int label)
{
#ifndef NO_ANNOTATIONS
  printf2(" ; JumpIfZero\n");
#endif
  /* if Wreg == 0, jump to label
  GenPrintInstr3Operands(MipsInstrBEQ, 0,
                         GenWreg, 0,
                         B32POpRegZero, 0,
                         B32POpNumLabel, label);
  */
  GenPrintInstr3Operands(B32PInstrBNE, 0,
                         GenWreg, 0,
                         B32POpRegZero, 0,
                         B32POpConst, 2);
  GenPrintInstr1Operand(B32PInstrJump, 0,
                         B32POpNumLabel, label);
}

STATIC
void GenJumpIfNotZero(int label)
{
#ifndef NO_ANNOTATIONS
  printf2(" ; JumpIfNotZero\n");
#endif
  /* if Wreg != 0, jump to label
  GenPrintInstr3Operands(MipsInstrBNE, 0,
                         GenWreg, 0,
                         B32POpRegZero, 0,
                         B32POpNumLabel, label);
  */
  GenPrintInstr3Operands(B32PInstrBEQ, 0,
                         GenWreg, 0,
                         B32POpRegZero, 0,
                         B32POpConst, 2);
  GenPrintInstr1Operand(B32PInstrJump, 0,
                         B32POpNumLabel, label);
}

fpos_t GenPrologPos;
int GenLeaf;

STATIC
void GenWriteFrameSize(void) //WORDSIZE
{
  unsigned size = 8/*RA + FP*/ - CurFxnMinLocalOfs;
  //printf2(" subu r13, r13, %10u\n", size); // 10 chars are enough for 32-bit unsigned ints
  printf2(" sub r13 %10u r13\n", size); // r13 = r13 - size

  //printf2(" sw r14, %10u r13\n", size - 8);
  printf2(" write %10u r13 r14\n", size - 8); // write r14 to memory[r13+(size-8)]
  
  //printf2(" addu r14, r13, %10u\n", size - 8);
  printf2(" add r13 %10u r14\n", size - 8); // r14 = r13 + (size-8)

  //printf2(" %csw r15, 4 r14\n", GenLeaf ? ';' : ' ');
  printf2(" %c write 4 r14 r15\n", GenLeaf ? ';' : ' '); // write r15 to memory[r14+4]
}

STATIC
void GenUpdateFrameSize(void)
{
  fpos_t pos;
  fgetpos(OutFile, &pos);
  fsetpos(OutFile, &GenPrologPos);
  GenWriteFrameSize();
  fsetpos(OutFile, &pos);
}

STATIC
void GenFxnProlog(void)
{
  if (CurFxnParamCntMin && CurFxnParamCntMax)
  {
    int i, cnt = CurFxnParamCntMax;
    if (cnt > 4)
      cnt = 4; //WORDSIZE?
    // TBD!!! for structure passing use the cumulative parameter size
    // instead of the number of parameters. Currently this bug is masked
    // by the subroutine that pushes structures on the stack (it copies
    // all words except the first to the stack). But passing structures
    // in registers from assembly code won't always work.
    for (i = 0; i < cnt; i++)
      GenPrintInstr2Operands(B32PInstrWrite, 0,
                             B32POpIndRegSp, 4 * i, //WORDSIZE
                             B32POpRegA0 + i, 0);
  }

  GenLeaf = 1; // will be reset to 0 if a call is generated

  fgetpos(OutFile, &GenPrologPos);
  GenWriteFrameSize();
}

STATIC
void GenGrowStack(int size) //WORDSIZE
{
  if (!size)
    return;

  if (size > 0)
  {
    GenPrintInstr3Operands(B32PInstrSUB, 0,
                           B32POpRegSp, 0,
                           B32POpConst, size,
                           B32POpRegSp, 0);
  }
  else
  {
    GenPrintInstr3Operands(B32PInstrADD, 0,
                           B32POpRegSp, 0,
                           B32POpConst, -size,
                           B32POpRegSp, 0);
  }
}

STATIC
void GenFxnEpilog(void)
{
  GenUpdateFrameSize();

  if (!GenLeaf)
    GenPrintInstr2Operands(B32PInstrRead, 0,
                           B32POpIndRegFp, 4, //WORDSIZE
                           B32POpRegRa, 0);

  GenPrintInstr2Operands(B32PInstrRead, 0,
                         B32POpIndRegFp, 0,
                         B32POpRegFp, 0);

  GenPrintInstr3Operands(B32PInstrADD, 0,
                         B32POpRegSp, 0,
                         B32POpConst, 8/*RA + FP*/ - CurFxnMinLocalOfs, //WORDSIZE
                         B32POpRegSp, 0);

  GenPrintInstr2Operands(B32PInstrJumpr, 0,
                        B32POpConst, 0,
                        B32POpRegRa, 0);
}

STATIC
int GenMaxLocalsSize(void)
{
  return 0x7FFFFFFF;
}

STATIC
int GenGetBinaryOperatorInstr(int tok)
{
  switch (tok)
  {
  case tokPostAdd:
  case tokAssignAdd:
  case '+':
    return B32PInstrADD;
  case tokPostSub:
  case tokAssignSub:
  case '-':
    return B32PInstrSUB;
  case '&':
  case tokAssignAnd:
    return B32PInstrAND;
  case '^':
  case tokAssignXor:
    return B32PInstrXOR;
  case '|':
  case tokAssignOr:
    return B32PInstrOR;
  case '<':
  case '>':
  case tokLEQ:
  case tokGEQ:
  case tokEQ:
  case tokNEQ:
  case tokULess:
  case tokUGreater:
  case tokULEQ:
  case tokUGEQ:
    return B32PInstrNOP;
  case '*':
  case tokAssignMul:
    return B32PInstrMULTS;
  case '/':
  case '%':
  case tokAssignDiv:
  case tokAssignMod:
    printf("DIVISION/MOD is not supported!\n");
    return B32PInstrHalt;
  case tokUDiv:
  case tokUMod:
  case tokAssignUDiv:
  case tokAssignUMod:
    printf("DIVISION/MOD is not supported!\n");
    return B32PInstrHalt;
  case tokLShift:
  case tokAssignLSh:
    return B32PInstrSHIFTL;
  case tokRShift:
  case tokAssignRSh:
    return B32PInstrSHIFTRS;
  case tokURShift:
  case tokAssignURSh:
    return B32PInstrSHIFTR;

  default:
    //error("Error: Invalid operator\n");
    errorInternal(101);
    return 0;
  }
}

// Should not be needed, AT register is not used by B32P assembler
// Although the lui instruction probably should stay?
STATIC
void GenPreIdentAccess(int label)
{
  printf2("; .set noat\n lui r1, %%hi(");
  GenPrintLabel(IdentTable + label);
  puts2(")");
}

// Should not be needed, AT register is not used by B32P assembler
STATIC
void GenPostIdentAccess(void)
{
  puts2("; .set at");
}

STATIC
void GenReadIdent(int regDst, int opSz, int label)
{
  GenPrintInstr2Operands(B32PInstrAddr2reg, 0,
                         B32POpLabel, label,
                         B32POpRegAt, 0);

  GenPrintInstr3Operands(B32PInstrRead, 0,
                         B32POpConst, 0,
                         B32POpRegAt, 0,
                         regDst, 0);
}

STATIC
void GenReadLocal(int regDst, int opSz, int ofs)
{
  int instr = B32PInstrRead;
  GenPrintInstr2Operands(instr, 0,
                         B32POpIndRegFp, ofs,
                         regDst, 0);
}

STATIC
void GenReadIndirect(int regDst, int regSrc, int opSz)
{
  int instr = B32PInstrRead;
  GenPrintInstr2Operands(instr, 0,
                         regSrc + B32POpIndRegZero, 0,
                         regDst, 0);
}

STATIC
void GenWriteIdent(int regSrc, int opSz, int label)
{
  GenPrintInstr2Operands(B32PInstrAddr2reg, 0,
                         B32POpLabel, label,
                         B32POpRegAt, 0);

  GenPrintInstr3Operands(B32PInstrWrite, 0,
                         B32POpConst, 0,
                         B32POpRegAt, 0,
                         regSrc, 0);
}

STATIC
void GenWriteLocal(int regSrc, int opSz, int ofs)
{
  int instr = B32PInstrWrite;

  GenPrintInstr2Operands(instr, 0,
                         B32POpIndRegFp, ofs,
                         regSrc, 0);
}

STATIC
void GenWriteIndirect(int regDst, int regSrc, int opSz)
{
  int instr = B32PInstrWrite;

  GenPrintInstr2Operands(instr, 0,
                         regDst + B32POpIndRegZero, 0,
                         regSrc, 0);
}

STATIC
void GenIncDecIdent(int regDst, int opSz, int label, int tok)
{
  int instr = B32PInstrADD;

  if (tok != tokInc)
    instr = B32PInstrSUB;

  GenReadIdent(regDst, opSz, label);
  GenPrintInstr3Operands(instr, 0,
                         regDst, 0,
                         B32POpConst, 1,
                         regDst, 0);
  GenWriteIdent(regDst, opSz, label);
  GenExtendRegIfNeeded(regDst, opSz);
}

STATIC
void GenIncDecLocal(int regDst, int opSz, int ofs, int tok)
{
  int instr = B32PInstrADD;

  if (tok != tokInc)
    instr = B32PInstrSUB;

  GenReadLocal(regDst, opSz, ofs);
  GenPrintInstr3Operands(instr, 0,
                         regDst, 0,
                         B32POpConst, 1,
                         regDst, 0);
  GenWriteLocal(regDst, opSz, ofs);
  GenExtendRegIfNeeded(regDst, opSz);
}

STATIC
void GenIncDecIndirect(int regDst, int regSrc, int opSz, int tok)
{
  int instr = B32PInstrADD;

  if (tok != tokInc)
    instr = B32PInstrSUB;

  GenReadIndirect(regDst, regSrc, opSz);
  GenPrintInstr3Operands(instr, 0,
                         regDst, 0,
                         B32POpConst, 1,
                         regDst, 0);
  GenWriteIndirect(regSrc, regDst, opSz);
  GenExtendRegIfNeeded(regDst, opSz);
}

STATIC
void GenPostIncDecIdent(int regDst, int opSz, int label, int tok)
{
  int instr = B32PInstrADD;

  if (tok != tokPostInc)
    instr = B32PInstrSUB;

  GenReadIdent(regDst, opSz, label);
  GenPrintInstr3Operands(instr, 0,
                         regDst, 0,
                         B32POpConst, 1,
                         regDst, 0);
  GenWriteIdent(regDst, opSz, label);

  GenPrintInstr3Operands(instr, 0,
                         regDst, 0,
                         B32POpConst, -1,
                         regDst, 0);
  GenExtendRegIfNeeded(regDst, opSz);
}

STATIC
void GenPostIncDecLocal(int regDst, int opSz, int ofs, int tok)
{
  int instr = B32PInstrADD;

  if (tok != tokPostInc)
    instr = B32PInstrSUB;

  GenReadLocal(regDst, opSz, ofs);
  GenPrintInstr3Operands(instr, 0,
                         regDst, 0,
                         B32POpConst, 1,
                         regDst, 0);
  GenWriteLocal(regDst, opSz, ofs);

  GenPrintInstr3Operands(instr, 0,
                         regDst, 0,
                         B32POpConst, -1,
                         regDst, 0);
  GenExtendRegIfNeeded(regDst, opSz);
}

STATIC
void GenPostIncDecIndirect(int regDst, int regSrc, int opSz, int tok)
{
  int instr = B32PInstrADD;

  if (tok != tokPostInc)
    instr = B32PInstrSUB;

  GenReadIndirect(regDst, regSrc, opSz);
  GenPrintInstr3Operands(instr, 0,
                         regDst, 0,
                         B32POpConst, 1,
                         regDst, 0);
  GenWriteIndirect(regSrc, regDst, opSz);

  GenPrintInstr3Operands(instr, 0,
                         regDst, 0,
                         B32POpConst, -1,
                         regDst, 0);
  GenExtendRegIfNeeded(regDst, opSz);
}

int CanUseTempRegs;
int TempsUsed;
int GenWreg = B32POpRegV0; // current working register (V0 or Tn or An)
int GenLreg, GenRreg; // left operand register and right operand register after GenPopReg()

/*
  General idea behind GenWreg, GenLreg, GenRreg:

  - In expressions w/o function calls:

    Subexpressions are evaluated in V0, T0, T1, ..., T<MAX_TEMP_REGS-1>. If those registers
    aren't enough, the stack is used additionally.

    The expression result ends up in V0, which is handy for returning from
    functions.

    In the process, GenWreg is the current working register and is one of: V0, T0, T1, ... .
    All unary operators are evaluated in the current working register.

    GenPushReg() and GenPopReg() advance GenWreg as needed when handling binary operators.

    GenPopReg() sets GenWreg, GenLreg and GenRreg. GenLreg and GenRreg are the registers
    where the left and right operands of a binary operator are.

    When the exression runs out of the temporary registers, the stack is used. While it is being
    used, GenWreg remains equal to the last temporary register, and GenPopReg() sets GenLreg = TEMP_REG_A.
    Hence, after GenPopReg() the operands of the binary operator are always in registers and can be
    directly manipulated with.

    Following GenPopReg(), binary operator evaluation must take the left and right operands from
    GenLreg and GenRreg and write the evaluated result into GenWreg. Care must be taken as GenWreg
    will be the same as either GenLreg (when the popped operand comes from T0-T<MAX_TEMP_REGS-1>)
    or GenRreg (when the popped operand comes from the stack in TEMP_REG_A).

  - In expressions with function calls:

    GenWreg is always V0 in subexpressions that aren't function parameters. These subexpressions
    get automatically pushed onto the stack as necessary.

    GenWreg is always V0 in expressions, where return values from function calls are used as parameters
    into other called functions. IOW, this is the case when the function call depth is greater than 1.
    Subexpressions in such expressions get automatically pushed onto the stack as necessary.

    GenWreg is A0-A3 in subexpressions that are function parameters when the function call depth is 1.
    Basically, while a function parameter is evaluated, it's evaluated in the register from where
    the called function will take it. This avoids some of unnecessary register copies and stack
    manipulations in the most simple and very common cases of function calls.
*/

STATIC
void GenWregInc(int inc)
{
  if (inc > 0)
  {
    // Advance the current working register to the next available temporary register
    if (GenWreg == B32POpRegV0)
      GenWreg = B32POpRegT0;
    else
      GenWreg++;
  }
  else
  {
    // Return to the previous current working register
    if (GenWreg == B32POpRegT0)
      GenWreg = B32POpRegV0;
    else
      GenWreg--;
  }
}

STATIC
void GenPushReg(void)
{
  if (CanUseTempRegs && TempsUsed < MAX_TEMP_REGS)
  {
    GenWregInc(1);
    TempsUsed++;
    return;
  }

  GenPrintInstr3Operands(B32PInstrSUB, 0,
                         B32POpRegSp, 0,
                         B32POpConst, 4, //WORDSIZE
                         B32POpRegSp, 0);

  GenPrintInstr2Operands(B32PInstrWrite, 0,
                         B32POpIndRegSp, 0,
                         GenWreg, 0);

  TempsUsed++;
}

STATIC
void GenPopReg(void)
{
  TempsUsed--;

  if (CanUseTempRegs && TempsUsed < MAX_TEMP_REGS)
  {
    GenRreg = GenWreg;
    GenWregInc(-1);
    GenLreg = GenWreg;
    return;
  }

  GenPrintInstr2Operands(B32PInstrRead, 0,
                         B32POpIndRegSp, 0,
                         TEMP_REG_A, 0);

  GenPrintInstr3Operands(B32PInstrADD, 0,
                         B32POpRegSp, 0,
                         B32POpConst, 4, //WORDSIZE
                         B32POpRegSp, 0);
  GenLreg = TEMP_REG_A;
  GenRreg = GenWreg;
}

#define tokRevIdent    0x100
#define tokRevLocalOfs 0x101
#define tokAssign0     0x102
#define tokNum0        0x103

STATIC
void GenPrep(int* idx)
{
  int tok;
  int oldIdxRight, oldIdxLeft, t0, t1;

  if (*idx < 0)
    //error("GenFuse(): idx < 0\n");
    errorInternal(100);

  tok = stack[*idx][0];

  oldIdxRight = --*idx;

  switch (tok)
  {
  case tokUDiv:
  case tokUMod:
  case tokAssignUDiv:
  case tokAssignUMod:
    if (stack[oldIdxRight][0] == tokNumInt || stack[oldIdxRight][0] == tokNumUint)
    {
      // Change unsigned division to right shift and unsigned modulo to bitwise and
      unsigned m = truncUint(stack[oldIdxRight][1]);
      if (m && !(m & (m - 1)))
      {
        if (tok == tokUMod || tok == tokAssignUMod)
        {
          stack[oldIdxRight][1] = (int)(m - 1);
          tok = (tok == tokUMod) ? '&' : tokAssignAnd;
        }
        else
        {
          t1 = 0;
          while (m >>= 1) t1++;
          stack[oldIdxRight][1] = t1;
          tok = (tok == tokUDiv) ? tokURShift : tokAssignURSh;
        }
        stack[oldIdxRight + 1][0] = tok;
      }
    }
  }

  switch (tok)
  {
  case tokNumUint:
    stack[oldIdxRight + 1][0] = tokNumInt; // reduce the number of cases since tokNumInt and tokNumUint are handled the same way
    // fallthrough
  case tokNumInt:
  case tokNum0:
  case tokIdent:
  case tokLocalOfs:
    break;

  case tokPostAdd:
  case tokPostSub:
  case '-':
  case '/':
  case '%':
  case tokUDiv:
  case tokUMod:
  case tokLShift:
  case tokRShift:
  case tokURShift:
  case tokLogAnd:
  case tokLogOr:
  case tokComma:
    GenPrep(idx);
    // fallthrough
  case tokShortCirc:
  case tokGoto:
  case tokUnaryStar:
  case tokInc:
  case tokDec:
  case tokPostInc:
  case tokPostDec:
  case '~':
  case tokUnaryPlus:
  case tokUnaryMinus:
  case tok_Bool:
  case tokVoid:
  case tokUChar:
  case tokSChar:
  case tokShort:
  case tokUShort:
    GenPrep(idx);
    break;

  case '=':
    if (oldIdxRight + 1 == sp - 1 &&
        (stack[oldIdxRight][0] == tokNumInt || stack[oldIdxRight][0] == tokNumUint) &&
        truncUint(stack[oldIdxRight][1]) == 0)
    {
      // Special case for assigning 0 while throwing away the expression result value
      // TBD??? ,
      stack[oldIdxRight][0] = tokNum0; // this zero constant will not be loaded into a register
      stack[oldIdxRight + 1][0] = tokAssign0; // change '=' to tokAssign0
    }
    // fallthrough
  case tokAssignAdd:
  case tokAssignSub:
  case tokAssignMul:
  case tokAssignDiv:
  case tokAssignUDiv:
  case tokAssignMod:
  case tokAssignUMod:
  case tokAssignLSh:
  case tokAssignRSh:
  case tokAssignURSh:
  case tokAssignAnd:
  case tokAssignXor:
  case tokAssignOr:
    GenPrep(idx);
    oldIdxLeft = *idx;
    GenPrep(idx);
    // If the left operand is an identifier (with static or auto storage), swap it with the right operand
    // and mark it specially, so it can be used directly
    if ((t0 = stack[oldIdxLeft][0]) == tokIdent || t0 == tokLocalOfs)
    {
      t1 = stack[oldIdxLeft][1];
      memmove(stack[oldIdxLeft], stack[oldIdxLeft + 1], (oldIdxRight - oldIdxLeft) * sizeof(stack[0]));
      stack[oldIdxRight][0] = (t0 == tokIdent) ? tokRevIdent : tokRevLocalOfs;
      stack[oldIdxRight][1] = t1;
    }
    break;

  case '+':
  case '*':
  case '&':
  case '^':
  case '|':
  case tokEQ:
  case tokNEQ:
  case '<':
  case '>':
  case tokLEQ:
  case tokGEQ:
  case tokULess:
  case tokUGreater:
  case tokULEQ:
  case tokUGEQ:
    GenPrep(idx);
    oldIdxLeft = *idx;
    GenPrep(idx);
    // If the right operand isn't a constant, but the left operand is, swap the operands
    // so the constant can become an immediate right operand in the instruction
    t1 = stack[oldIdxRight][0];
    t0 = stack[oldIdxLeft][0];
    if (t1 != tokNumInt && t0 == tokNumInt)
    {
      int xor;

      t1 = stack[oldIdxLeft][1];
      memmove(stack[oldIdxLeft], stack[oldIdxLeft + 1], (oldIdxRight - oldIdxLeft) * sizeof(stack[0]));
      stack[oldIdxRight][0] = t0;
      stack[oldIdxRight][1] = t1;

      switch (tok)
      {
      case '<':
      case '>':
        xor = '<' ^ '>'; break;
      case tokLEQ:
      case tokGEQ:
        xor = tokLEQ ^ tokGEQ; break;
      case tokULess:
      case tokUGreater:
        xor = tokULess ^ tokUGreater; break;
      case tokULEQ:
      case tokUGEQ:
        xor = tokULEQ ^ tokUGEQ; break;
      default:
        xor = 0; break;
      }
      tok ^= xor;
    }
    // Handle a few special cases and transform the instruction
    if (stack[oldIdxRight][0] == tokNumInt)
    {
      unsigned m = truncUint(stack[oldIdxRight][1]);
      switch (tok)
      {
      case '*':
        // Change multiplication to left shift, this helps indexing arrays of ints/pointers/etc
        if (m && !(m & (m - 1)))
        {
          t1 = 0;
          while (m >>= 1) t1++;
          stack[oldIdxRight][1] = t1;
          tok = tokLShift;
        }
        break;
      case tokLEQ:
        // left <= const will later change to left < const+1, but const+1 must be <=0x7FFFFFFF
        if (m == 0x7FFFFFFF)
        {
          // left <= 0x7FFFFFFF is always true, change to the equivalent left >= 0u
          stack[oldIdxRight][1] = 0;
          tok = tokUGEQ;
        }
        break;
      case tokULEQ:
        // left <= const will later change to left < const+1, but const+1 must be <=0xFFFFFFFFu
        if (m == 0xFFFFFFFF)
        {
          // left <= 0xFFFFFFFFu is always true, change to the equivalent left >= 0u
          stack[oldIdxRight][1] = 0;
          tok = tokUGEQ;
        }
        break;
      case '>':
        // left > const will later change to !(left < const+1), but const+1 must be <=0x7FFFFFFF
        if (m == 0x7FFFFFFF)
        {
          // left > 0x7FFFFFFF is always false, change to the equivalent left & 0
          stack[oldIdxRight][1] = 0;
          tok = '&';
        }
        break;
      case tokUGreater:
        // left > const will later change to !(left < const+1), but const+1 must be <=0xFFFFFFFFu
        if (m == 0xFFFFFFFF)
        {
          // left > 0xFFFFFFFFu is always false, change to the equivalent left & 0
          stack[oldIdxRight][1] = 0;
          tok = '&';
        }
        break;
      }
    }
    stack[oldIdxRight + 1][0] = tok;
    break;

  case ')':
    while (stack[*idx][0] != '(')
    {
      GenPrep(idx);
      if (stack[*idx][0] == ',')
        --*idx;
    }
    --*idx;
    break;

  default:
    //error("GenPrep: unexpected token %s\n", GetTokenName(tok));
    errorInternal(101);
  }
}

/*
;     l <[u] 0       // slt[u] w, w, 0                            "k"
      l <[u] const   // slt[u] w, w, const                        "m"
      l <[u] r       // slt[u] w, l, r                            "i"
* if (l <    0)      // bgez w, Lskip                             "f"
  if (l <[u] const)  // slt[u] w, w, const; beq w, r0, Lskip      "mc"
  if (l <[u] r)      // slt[u] w, l, r; beq w, r0, Lskip          "ic"

;     l <=[u] 0      // slt[u] w, w, 1                            "l"
      l <=[u] const  // slt[u] w, w, const + 1                    "n"
      l <=[u] r      // slt[u] w, r, l; xor w, w, 1               "js"
* if (l <=    0)     // bgtz w, Lskip                             "g"
  if (l <=[u] const) // slt[u] w, w, const + 1; beq w, r0, Lskip  "nc"
  if (l <=[u] r)     // slt[u] w, r, l; bne w, r0, Lskip          "jd"

      l >[u] 0       // slt[u] w, r0, w                           "o"
      l >[u] const   // slt[u] w, w, const + 1; xor w, w, 1       "ns"
      l >[u] r       // slt[u] w, r, l                            "j"
* if (l >    0)      // blez w, Lskip                             "h"
**if (l >u   0)      // beq w, r0, Lskip
  if (l >[u] const)  // slt[u] w, w, const + 1; bne w, r0, Lskip  "nd"
  if (l >[u] r)      // slt[u] w, r, l; beq w, r0, Lskip          "jc"

;     l >=[u] 0      // slt[u] w, w, 0; xor w, w, 1               "ks"
      l >=[u] const  // slt[u] w, w, const; xor w, w, 1           "ms"
      l >=[u] r      // slt[u] w, l, r; xor w, w, 1               "is"
* if (l >=    0)     // bltz w, Lskip                             "e"
  if (l >=[u] const) // slt[u] w, w, const; bne w, r0, Lskip      "md"
  if (l >=[u] r)     // slt[u] w, l, r; bne w, r0, Lskip          "id"

      l == 0         // sltu w, w, 1                              "q"
      l == const     // xor w, w, const; sltu w, w, 1             "tq"
      l == r         // xor w, l, r; sltu w, w, 1                 "rq"
  if (l == 0)        // bne w, r0, Lskip                          "d"
  if (l == const)    // xor w, w, const; bne w, r0, Lskip         "td"
  if (l == r)        // bne l, r, Lskip                           "b"

      l != 0         // sltu w, r0, w                             "p"
      l != const     // xor w, w, const; sltu w, r0, w            "tp"
      l != r         // xor w, l, r; sltu w, r0, w                "rp"
  if (l != 0)        // beq w, r0, Lskip                          "c"
  if (l != const)    // xor w, w, const; beq w, r0, Lskip         "tc"
  if (l != r)        // beq l, r, Lskip                           "a"
*/
char CmpBlocks[6/*op*/][2/*condbranch*/][3/*constness*/][2] =
{
  {
    { "k", "m", "i" },
    { "f", "mc", "ic" }
  },
  {
    { "l", "n", "js" },
    { "g", "nc", "jd" }
  },
  {
    { "o", "ns", "j" },
    { "h", "nd", "jc" }
  },
  {
    { "ks", "ms", "is" },
    { "e", "md", "id" }
  },
  {
    { "q", "tq", "rq" },
    { "d", "td", "b" }
  },
  {
    { "p", "tp", "rp" },
    { "c", "tc", "a" }
  }
};

STATIC
void GenCmp(int* idx, int op)
{
  // TODO: 
  /*
  For B322 (not B32P!): direct conversion from MIPS to B322 is very inefficient, these notes help:
  MIPS:
  slt reg, s < t (reg := 1, else reg := 0)

  B322 equivalent:
  bge s >= t 2
  load 1 reg
  load 0 reg
  */
  /*
  Inverses:
  BEQ a b         BNE a b
  BNE a b         BEQ a b

  BLTZ a (a < 0)  BGE a r0 (a >= 0)
  BGEZ a (a >=0)  BGT r0 a (a < 0) == (0 > a)

  BGTZ a (a > 0)  BGE r0 a (a <= 0) == (0 >= a)
  BLEZ a (a <=0)  BGT a r0 (a > 0)
  */
  // constness: 0 = zero const, 1 = non-zero const, 2 = non-const
  int constness = (stack[*idx - 1][0] == tokNumInt) ? (stack[*idx - 1][1] != 0) : 2;
  int constval = (constness == 1) ? truncInt(stack[*idx - 1][1]) : 0;
  // condbranch: 0 = no conditional branch, 1 = branch if true, 2 = branch if false
  int condbranch = (*idx + 1 < sp) ? (stack[*idx + 1][0] == tokIf) + (stack[*idx + 1][0] == tokIfNot) * 2 : 0;
  int unsign = op >> 4;
  int slt = unsign ? B32PInstrSLTU : B32PInstrSLT;

  int label = condbranch ? stack[*idx + 1][1] : 0;
  char* p;
  int i;

  op &= 0xF;
  if (constness == 2)
    GenPopReg();

  // bltz, blez, bgez, bgtz are for signed comparison with 0 only,
  // so for conditional branches on <0u, <=0u, >0u, >=0u use the general method instead
  if (condbranch && op < 4 && constness == 0 && unsign)
  {
    // Except, >0u is more optimal as !=0
    if (op == 2)
      op = 5;
    else
      constness = 1;
  }

  p = CmpBlocks[op][condbranch != 0][constness];

  for (i = 0; i < 2; i++)
  {
    switch (p[i])
    {
    case 'a':
      condbranch ^= 3;
      // fallthrough
    case 'b':
      GenPrintInstr3Operands((condbranch == 1) ? B32PInstrBNE : B32PInstrBEQ, 0,
                             GenLreg, 0,
                             GenRreg, 0,
                             B32POpConst, 2);
      GenPrintInstr1Operand(B32PInstrJump, 0,
                             B32POpNumLabel, label);
      break;
    case 'c':
      condbranch ^= 3;
      // fallthrough
    case 'd':
      GenPrintInstr3Operands((condbranch == 1) ? B32PInstrBNE : B32PInstrBEQ, 0,
                             GenWreg, 0,
                             B32POpRegZero, 0,
                             B32POpConst, 2);
      GenPrintInstr1Operand(B32PInstrJump, 0,
                             B32POpNumLabel, label);
      break;
    case 'e':
      condbranch ^= 3;
      // fallthrough
    case 'f':
      /*
        BLTZ a (a < 0)  BGE a r0 (a >= 0)
        BGEZ a (a >=0)  BGT r0 a (a < 0) == (0 > a)
      */
      if (condbranch == 1)
      {
        GenPrintInstr3Operands(B32PInstrBGE, 0,
                               GenWreg, 0,
                               B32POpRegZero, 0,
                               B32POpConst, 2);
      }
      else
      {
        GenPrintInstr3Operands(B32PInstrBGT, 0,
                               B32POpRegZero, 0,
                               GenWreg, 0,
                               B32POpConst, 2);
      }
      GenPrintInstr1Operand(B32PInstrJump, 0,
                             B32POpNumLabel, label);
      break;
    case 'g':
      condbranch ^= 3;
      // fallthrough
    case 'h':
      /*
      BGTZ a (a > 0)  BGE r0 a (a <= 0) == (0 >= a)
      BLEZ a (a <=0)  BGT a r0 (a > 0)
      */
      if (condbranch == 1)
      {
        GenPrintInstr3Operands(B32PInstrBGE, 0,
                               B32POpRegZero, 0,
                               GenWreg, 0,
                               B32POpConst, 2);
      }
      else
      {
        GenPrintInstr3Operands(B32PInstrBGT, 0,
                               GenWreg, 0,
                               B32POpRegZero, 0,
                               B32POpConst, 2);
      }
      GenPrintInstr1Operand(B32PInstrJump, 0,
                             B32POpNumLabel, label);
      break;
    case 'i':
      GenPrintInstr3Operands(slt, 0,
                             GenLreg, 0,
                             GenRreg, 0,
                             GenWreg, 0);
      break;
    case 'j':
      GenPrintInstr3Operands(slt, 0,
                             GenRreg, 0,
                             GenLreg, 0,
                             GenWreg, 0);
      break;
    case 'k':
      GenPrintInstr3Operands(slt, 0,
                             GenWreg, 0,
                             B32POpRegZero, 0,
                             GenWreg, 0);
      break;
    case 'l':
      GenPrintInstr3Operands(slt, 0,
                             GenWreg, 0,
                             B32POpConst, 1,
                             GenWreg, 0);
      break;
    case 'n':
      constval++;
      // fallthrough
    case 'm':
      if (constval < 0x8000)
      {
        GenPrintInstr3Operands(slt, 0,
                             GenWreg, 0,
                             B32POpConst, constval,
                             GenWreg, 0);
      }
      else
      {
        GenPrintInstr2Operands(B32PInstrLoad, 0,
                               B32POpConst, constval,
                               TEMP_REG_A, 0);
        GenPrintInstr3Operands(slt, 0,
                             GenWreg, 0,
                             TEMP_REG_A, 0,
                             GenWreg, 0);
      }
      
      break;
    case 'o':
      GenPrintInstr3Operands(slt, 0,
                             B32POpRegZero, 0,
                             GenWreg, 0,
                             GenWreg, 0);
      break;
    case 'p':
      GenPrintInstr3Operands(B32PInstrSLTU, 0,
                             B32POpRegZero, 0,
                             GenWreg, 0,
                             GenWreg, 0);
      break;
    case 'q':
      GenPrintInstr3Operands(B32PInstrSLTU, 0,
                             GenWreg, 0,
                             B32POpConst, 1,
                             GenWreg, 0);
      break;
    case 'r':
      GenPrintInstr3Operands(B32PInstrXOR, 0,
                             GenLreg, 0,
                             GenRreg, 0,
                             GenWreg, 0);
      break;
    case 's':
      GenPrintInstr3Operands(B32PInstrXOR, 0,
                             GenWreg, 0,
                             B32POpConst, 1,
                             GenWreg, 0);
      break;
    case 't':
      if (constval < 0x8000)
      {
        GenPrintInstr3Operands(B32PInstrXOR, 0,
                               GenWreg, 0,
                               B32POpConst, constval,
                               GenWreg, 0);
      }
      else
      {
        GenPrintInstr2Operands(B32PInstrLoad, 0,
                               B32POpConst, constval,
                               TEMP_REG_A, 0);
        GenPrintInstr3Operands(B32PInstrXOR, 0,
                               GenWreg, 0,
                               TEMP_REG_A, 0,
                               GenWreg, 0);
      }
      break;
    }
  }

  *idx += condbranch != 0;
}

STATIC
int GenIsCmp(int t)
{
  return
    t == '<' ||
    t == '>' ||
    t == tokGEQ ||
    t == tokLEQ ||
    t == tokULess ||
    t == tokUGreater ||
    t == tokUGEQ ||
    t == tokULEQ ||
    t == tokEQ ||
    t == tokNEQ;
}

// Improved register/stack-based code generator
// DONE: test 32-bit code generation
STATIC
void GenExpr0(void)
{
  int i;
  int gotUnary = 0;
  int maxCallDepth = 0;
  int callDepth = 0;
  int paramOfs = 0;
  int t = sp - 1;

  if (stack[t][0] == tokIf || stack[t][0] == tokIfNot || stack[t][0] == tokReturn)
    t--;
  GenPrep(&t);

  for (i = 0; i < sp; i++)
    if (stack[i][0] == '(')
    {
      if (++callDepth > maxCallDepth)
        maxCallDepth = callDepth;
    }
    else if (stack[i][0] == ')')
    {
      callDepth--;
    }

  CanUseTempRegs = maxCallDepth == 0;
  TempsUsed = 0;
  if (GenWreg != B32POpRegV0)
    errorInternal(102);

  for (i = 0; i < sp; i++)
  {
    int tok = stack[i][0];
    int v = stack[i][1];

#ifndef NO_ANNOTATIONS
    switch (tok)
    {
    case tokNumInt: printf2(" ; %d\n", truncInt(v)); break;
    //case tokNumUint: printf2(" ; %uu\n", truncUint(v)); break;
    case tokIdent: case tokRevIdent: printf2(" ; %s\n", IdentTable + v); break;
    case tokLocalOfs: case tokRevLocalOfs: printf2(" ; local ofs\n"); break;
    case ')': printf2(" ; ) fxn call\n"); break;
    case tokUnaryStar: printf2(" ; * (read dereference)\n"); break;
    case '=': printf2(" ; = (write dereference)\n"); break;
    case tokShortCirc: printf2(" ; short-circuit "); break;
    case tokGoto: printf2(" ; sh-circ-goto "); break;
    case tokLogAnd: printf2(" ; short-circuit && target\n"); break;
    case tokLogOr: printf2(" ; short-circuit || target\n"); break;
    case tokIf: case tokIfNot: case tokReturn: break;
    case tokNum0: printf2(" ; 0\n"); break;
    case tokAssign0:  printf2(" ; =\n"); break;
    default: printf2(" ; %s\n", GetTokenName(tok)); break;
    }
#endif

    switch (tok)
    {
    case tokNumInt:
      if (!(i + 1 < sp && ((t = stack[i + 1][0]) == '+' ||
                           t == '-' ||
                           t == '&' ||
                           t == '^' ||
                           t == '|' ||
                           t == tokLShift ||
                           t == tokRShift ||
                           t == tokURShift ||
                           GenIsCmp(t))))
      {
        if (gotUnary)
          GenPushReg();

        GenPrintInstr2Operands(B32PInstrLoad, 0,
                               B32POpConst, v,
                               GenWreg, 0);
      }
      gotUnary = 1;
      break;

    case tokIdent:
      if (gotUnary)
        GenPushReg();
      if (!(i + 1 < sp && ((t = stack[i + 1][0]) == ')' ||
                           t == tokUnaryStar ||
                           t == tokInc ||
                           t == tokDec ||
                           t == tokPostInc ||
                           t == tokPostDec)))
      {
        GenPrintInstr2Operands(B32PInstrAddr2reg, 0,
                               B32POpLabel, v,
                               GenWreg, 0);
      }
      gotUnary = 1;
      break;

    case tokLocalOfs:
      if (gotUnary)
        GenPushReg();
      if (!(i + 1 < sp && ((t = stack[i + 1][0]) == tokUnaryStar ||
                           t == tokInc ||
                           t == tokDec ||
                           t == tokPostInc ||
                           t == tokPostDec)))
      {
        GenPrintInstr3Operands(B32PInstrADD, 0,
                               B32POpRegFp, 0,
                               B32POpConst, v,
                               GenWreg, 0);
      }
      gotUnary = 1;
      break;

    case '(':
      if (gotUnary)
        GenPushReg();
      gotUnary = 0;
      if (maxCallDepth != 1 && v < 16)
        GenGrowStack(16 - v);
      paramOfs = v - 4;
      if (maxCallDepth == 1 && paramOfs >= 0 && paramOfs <= 12)
      {
        // Work directly in A0-A3 instead of working in V0 and avoid copying V0 to A0-A3
        GenWreg = B32POpRegA0 + division(paramOfs, 4); //(paramOfs >> 2); //division(paramOfs, 4);
      }
      break;

    case ',':
      if (maxCallDepth == 1)
      {
        if (paramOfs == 16)
        {
          // Got the last on-stack parameter, the rest will go in A0-A3
          GenPushReg();
          gotUnary = 0;
          GenWreg = B32POpRegA3;
        }
        if (paramOfs >= 0 && paramOfs <= 12)
        {
          // Advance to the next An reg or revert to V0
          if (paramOfs)
            GenWreg--;
          else
            GenWreg = B32POpRegV0;
          gotUnary = 0;
        }
        paramOfs -= 4;
      }
      break;

    case ')':
      GenLeaf = 0;
      if (maxCallDepth != 1)
      {
        if (v >= 4)
          GenPrintInstr2Operands(B32PInstrRead, 0,
                                 B32POpIndRegSp, 0,
                                 B32POpRegA0, 0);
        if (v >= 8)
          GenPrintInstr2Operands(B32PInstrRead, 0,
                                 B32POpIndRegSp, 4,
                                 B32POpRegA1, 0);
        if (v >= 12)
          GenPrintInstr2Operands(B32PInstrRead, 0,
                                 B32POpIndRegSp, 8,
                                 B32POpRegA2, 0);
        if (v >= 16)
          GenPrintInstr2Operands(B32PInstrRead, 0,
                                 B32POpIndRegSp, 12,
                                 B32POpRegA3, 0);
      }
      else
      {
        GenGrowStack(16);
      }
      if (stack[i - 1][0] == tokIdent)
      {
        GenPrintInstr1Operand(B32PInstrSavPC, 0,
                              B32POpRegRa, 0);
        GenPrintInstr3Operands(B32PInstrADD, 0,
                             B32POpRegRa, 0,
                             B32POpConst, 3,
                             B32POpRegRa, 0);
        GenPrintInstr1Operand(B32PInstrJump, 0,
                              B32POpLabel, stack[i - 1][1]);
      }
      else
      {
        GenPrintInstr1Operand(B32PInstrSavPC, 0,
                              B32POpRegRa, 0);
        GenPrintInstr3Operands(B32PInstrADD, 0,
                             B32POpRegRa, 0,
                             B32POpConst, 3,
                             B32POpRegRa, 0);
        GenPrintInstr2Operands(B32PInstrJumpr, 0,
                              B32POpConst, 0,
                              GenWreg, 0);
      }
      if (v < 16)
        v = 16;
      GenGrowStack(-v);
      break;

    case tokUnaryStar:
      if (stack[i - 1][0] == tokIdent)
        GenReadIdent(GenWreg, v, stack[i - 1][1]);
      else if (stack[i - 1][0] == tokLocalOfs)
        GenReadLocal(GenWreg, v, stack[i - 1][1]);
      else
        GenReadIndirect(GenWreg, GenWreg, v);
      break;

    case tokUnaryPlus:
      break;
    case '~': //nor
      GenPrintInstr3Operands(B32PInstrOR, 0,
                             GenWreg, 0,
                             GenWreg, 0,
                             GenWreg, 0);
      GenPrintInstr2Operands(B32PInstrNOT, 0,
                             GenWreg, 0,
                             GenWreg, 0);

      break;
    case tokUnaryMinus:
      GenPrintInstr3Operands(B32PInstrSUB, 0,
                             B32POpRegZero, 0,
                             GenWreg, 0,
                             GenWreg, 0);
      break;

    case '+':
    case '-':
    case '*':
    case '&':
    case '^':
    case '|':
    case tokLShift:
    case tokRShift:
    case tokURShift:
      if (stack[i - 1][0] == tokNumInt && tok != '*')
      {
        int instr = GenGetBinaryOperatorInstr(tok);
        GenPrintInstr3Operands(instr, 0,
                               GenWreg, 0,
                               B32POpConst, stack[i - 1][1],
                               GenWreg, 0);
      }
      else
      {
        int instr = GenGetBinaryOperatorInstr(tok);
        GenPopReg();
        GenPrintInstr3Operands(instr, 0,
                               GenLreg, 0,
                               GenRreg, 0,
                               GenWreg, 0);
      }
      break;

    case '/':
    case tokUDiv:
    case '%':
    case tokUMod:
      {
        printf("DIVISION/MOD is not supported!\n");
        /*
        GenPopReg();
        if (tok == '/' || tok == '%')
          GenPrintInstr3Operands(MipsInstrDiv, 0,
                                 B32POpRegZero, 0,
                                 GenLreg, 0,
                                 GenRreg, 0);
        else
          GenPrintInstr3Operands(MipsInstrDivU, 0,
                                 B32POpRegZero, 0,
                                 GenLreg, 0,
                                 GenRreg, 0);
        if (tok == '%' || tok == tokUMod)
          GenPrintInstr1Operand(MipsInstrMfHi, 0,
                                GenWreg, 0);
        else
          GenPrintInstr1Operand(MipsInstrMfLo, 0,
                                GenWreg, 0);
        */
      }
      break;

    case tokInc:
    case tokDec:
      if (stack[i - 1][0] == tokIdent)
      {
        GenIncDecIdent(GenWreg, v, stack[i - 1][1], tok);
      }
      else if (stack[i - 1][0] == tokLocalOfs)
      {
        GenIncDecLocal(GenWreg, v, stack[i - 1][1], tok);
      }
      else
      {
        GenPrintInstr3Operands(B32PInstrOR, 0,
                               B32POpRegZero, 0,
                               GenWreg, 0,
                               TEMP_REG_A, 0);
        GenIncDecIndirect(GenWreg, TEMP_REG_A, v, tok);
      }
      break;
    case tokPostInc:
    case tokPostDec:
      if (stack[i - 1][0] == tokIdent)
      {
        GenPostIncDecIdent(GenWreg, v, stack[i - 1][1], tok);
      }
      else if (stack[i - 1][0] == tokLocalOfs)
      {
        GenPostIncDecLocal(GenWreg, v, stack[i - 1][1], tok);
      }
      else
      {
        GenPrintInstr3Operands(B32PInstrOR, 0,
                               B32POpRegZero, 0,
                               GenWreg, 0,
                               TEMP_REG_A, 0);
        GenPostIncDecIndirect(GenWreg, TEMP_REG_A, v, tok);
      }
      break;

    case tokPostAdd:
    case tokPostSub:
      {
        int instr = GenGetBinaryOperatorInstr(tok);
        GenPopReg();
        if (GenWreg == GenLreg)
        {
          GenPrintInstr3Operands(B32PInstrOR, 0,
                                 B32POpRegZero, 0,
                                 GenLreg, 0,
                                 TEMP_REG_B, 0);

          GenReadIndirect(GenWreg, TEMP_REG_B, v);
          GenPrintInstr3Operands(instr, 0,
                                 GenWreg, 0,
                                 GenRreg, 0,
                                 TEMP_REG_A, 0);
          GenWriteIndirect(TEMP_REG_B, TEMP_REG_A, v);
        }
        else
        {
          // GenWreg == GenRreg here
          GenPrintInstr3Operands(B32PInstrOR, 0,
                                 B32POpRegZero, 0,
                                 GenRreg, 0,
                                 TEMP_REG_B, 0);

          GenReadIndirect(GenWreg, GenLreg, v);
          GenPrintInstr3Operands(instr, 0,
                                 GenWreg, 0,
                                 TEMP_REG_B, 0,
                                 TEMP_REG_B, 0);
          GenWriteIndirect(GenLreg, TEMP_REG_B, v);
        }
      }
      break;

    case tokAssignAdd:
    case tokAssignSub:
    case tokAssignMul:
    case tokAssignAnd:
    case tokAssignXor:
    case tokAssignOr:
    case tokAssignLSh:
    case tokAssignRSh:
    case tokAssignURSh:
      if (stack[i - 1][0] == tokRevLocalOfs || stack[i - 1][0] == tokRevIdent)
      {
        int instr = GenGetBinaryOperatorInstr(tok);

        if (stack[i - 1][0] == tokRevLocalOfs)
          GenReadLocal(TEMP_REG_B, v, stack[i - 1][1]);
        else
          GenReadIdent(TEMP_REG_B, v, stack[i - 1][1]);

        GenPrintInstr3Operands(instr, 0,
                               TEMP_REG_B, 0,
                               GenWreg, 0,
                               GenWreg, 0);

        if (stack[i - 1][0] == tokRevLocalOfs)
          GenWriteLocal(GenWreg, v, stack[i - 1][1]);
        else
          GenWriteIdent(GenWreg, v, stack[i - 1][1]);
      }
      else
      {
        int instr = GenGetBinaryOperatorInstr(tok);
        int lsaved, rsaved;
        GenPopReg();
        if (GenWreg == GenLreg)
        {
          GenPrintInstr3Operands(B32PInstrOR, 0,
                                 B32POpRegZero, 0,
                                 GenLreg, 0,
                                 TEMP_REG_B, 0);
          lsaved = TEMP_REG_B;
          rsaved = GenRreg;
        }
        else
        {
          // GenWreg == GenRreg here
          GenPrintInstr3Operands(B32PInstrOR, 0,
                                 B32POpRegZero, 0,
                                 GenRreg, 0,
                                 TEMP_REG_B, 0);
          rsaved = TEMP_REG_B;
          lsaved = GenLreg;
        }

        GenReadIndirect(GenWreg, GenLreg, v); // destroys either GenLreg or GenRreg because GenWreg coincides with one of them
        GenPrintInstr3Operands(instr, 0,
                               GenWreg, 0,
                               rsaved, 0,
                               GenWreg, 0);
        GenWriteIndirect(lsaved, GenWreg, v);
      }
      GenExtendRegIfNeeded(GenWreg, v);
      break;

    case tokAssignDiv:
    case tokAssignUDiv:
    case tokAssignMod:
    case tokAssignUMod:
      if (stack[i - 1][0] == tokRevLocalOfs || stack[i - 1][0] == tokRevIdent)
      {
        if (stack[i - 1][0] == tokRevLocalOfs)
          GenReadLocal(TEMP_REG_B, v, stack[i - 1][1]);
        else
          GenReadIdent(TEMP_REG_B, v, stack[i - 1][1]);

        if (tok == tokAssignDiv || tok == tokAssignMod)
        {
          printf("DIVISION/MOD is not supported!\n");
          /*
          GenPrintInstr3Operands(MipsInstrDiv, 0,
                                 B32POpRegZero, 0,
                                 TEMP_REG_B, 0,
                                 GenWreg, 0);*/
        }
        else
        {
          printf("DIVISION/MOD is not supported!\n");
          /*
          GenPrintInstr3Operands(MipsInstrDivU, 0,
                                 B32POpRegZero, 0,
                                 TEMP_REG_B, 0,
                                 GenWreg, 0);*/
        }
        if (tok == tokAssignMod || tok == tokAssignUMod)
        {
          printf("DIVISION/MOD is not supported!\n");
          /*
          GenPrintInstr1Operand(MipsInstrMfHi, 0,
                                GenWreg, 0);*/
        }
        else
        {
          printf("DIVISION/MOD is not supported!\n");
          /*
          GenPrintInstr1Operand(MipsInstrMfLo, 0,
                                GenWreg, 0);*/
        }

        if (stack[i - 1][0] == tokRevLocalOfs)
          GenWriteLocal(GenWreg, v, stack[i - 1][1]);
        else
          GenWriteIdent(GenWreg, v, stack[i - 1][1]);
      }
      else
      {
        int lsaved, rsaved;
        GenPopReg();
        if (GenWreg == GenLreg)
        {
          GenPrintInstr3Operands(B32PInstrOR, 0,
                                 B32POpRegZero, 0,
                                 GenLreg, 0,
                                 TEMP_REG_B, 0);
          lsaved = TEMP_REG_B;
          rsaved = GenRreg;
        }
        else
        {
          // GenWreg == GenRreg here
          GenPrintInstr3Operands(B32PInstrOR, 0,
                                 B32POpRegZero, 0,
                                 GenRreg, 0,
                                 TEMP_REG_B, 0);
          rsaved = TEMP_REG_B;
          lsaved = GenLreg;
        }

        GenReadIndirect(GenWreg, GenLreg, v); // destroys either GenLreg or GenRreg because GenWreg coincides with one of them
        if (tok == tokAssignDiv || tok == tokAssignMod)
        {
          printf("DIVISION/MOD is not supported!\n");
          /*
          GenPrintInstr3Operands(MipsInstrDiv, 0,
                                 B32POpRegZero, 0,
                                 GenWreg, 0,
                                 rsaved, 0);*/
        }
        else
        {
          printf("DIVISION/MOD is not supported!\n");
          /*
          GenPrintInstr3Operands(MipsInstrDivU, 0,
                                 B32POpRegZero, 0,
                                 GenWreg, 0,
                                 rsaved, 0);*/
        }
        if (tok == tokAssignMod || tok == tokAssignUMod)
        {
          printf("DIVISION/MOD is not supported!\n");
          /*
          GenPrintInstr1Operand(MipsInstrMfHi, 0,
                                GenWreg, 0);*/
        }
        else
        {
          printf("DIVISION/MOD is not supported!\n");
          /*
          GenPrintInstr1Operand(MipsInstrMfLo, 0,
                                GenWreg, 0);*/
        }
        GenWriteIndirect(lsaved, GenWreg, v);
      }
      GenExtendRegIfNeeded(GenWreg, v);
      break;

    case '=':
      if (stack[i - 1][0] == tokRevLocalOfs)
      {
        GenWriteLocal(GenWreg, v, stack[i - 1][1]);
      }
      else if (stack[i - 1][0] == tokRevIdent)
      {
        GenWriteIdent(GenWreg, v, stack[i - 1][1]);
      }
      else
      {
        GenPopReg();
        GenWriteIndirect(GenLreg, GenRreg, v);
        if (GenWreg != GenRreg)
          GenPrintInstr3Operands(B32PInstrOR, 0,
                                 B32POpRegZero, 0,
                                 GenRreg, 0,
                                 GenWreg, 0);
      }
      GenExtendRegIfNeeded(GenWreg, v);
      break;

    case tokAssign0: // assignment of 0, while throwing away the expression result value
      if (stack[i - 1][0] == tokRevLocalOfs)
      {
        GenWriteLocal(B32POpRegZero, v, stack[i - 1][1]);
      }
      else if (stack[i - 1][0] == tokRevIdent)
      {
        GenWriteIdent(B32POpRegZero, v, stack[i - 1][1]);
      }
      else
      {
        GenWriteIndirect(GenWreg, B32POpRegZero, v);
      }
      break;

    case '<':         GenCmp(&i, 0x00); break;
    case tokLEQ:      GenCmp(&i, 0x01); break;
    case '>':         GenCmp(&i, 0x02); break;
    case tokGEQ:      GenCmp(&i, 0x03); break;
    case tokULess:    GenCmp(&i, 0x10); break;
    case tokULEQ:     GenCmp(&i, 0x11); break;
    case tokUGreater: GenCmp(&i, 0x12); break;
    case tokUGEQ:     GenCmp(&i, 0x13); break;
    case tokEQ:       GenCmp(&i, 0x04); break;
    case tokNEQ:      GenCmp(&i, 0x05); break;

    case tok_Bool:
      /* if 0 < wreg (if wreg > 0)
          wreg = 1
         else
          wreg = 0
      GenPrintInstr3Operands(MipsInstrSLTU, 0,
                             GenWreg, 0,
                             B32POpRegZero, 0,
                             GenWreg, 0);
      */
      GenPrintInstr3Operands(B32PInstrSLTU, 0,
                             B32POpRegZero, 0,
                             GenWreg, 0,
                             GenWreg, 0);
      break;

    case tokSChar:
      /* just use as an int for now
      GenPrintInstr3Operands(B32PInstrSHIFTL, 0,
                             GenWreg, 0,
                             B32POpConst, 24,
                             GenWreg, 0);
      GenPrintInstr3Operands(B32PInstrSHIFTR, 0, //TODO this should have also replicated the sign bit (SRA)
                             GenWreg, 0,
                             B32POpConst, 24,
                             GenWreg, 0);
      */
      break;
    case tokUChar:
      /* just use as an int for now
      GenPrintInstr3Operands(B32PInstrAND, 0,
                             GenWreg, 0,
                             B32POpConst, 0xFF,
                             GenWreg, 0);
      */
      break;
    case tokShort:
      /* just use as an int for now
      GenPrintInstr3Operands(B32PInstrSHIFTL, 0,
                             GenWreg, 0,
                             B32POpConst, 16,
                             GenWreg, 0);
      GenPrintInstr3Operands(B32PInstrSHIFTR, 0, //TODO this should have also replicated the sign bit (SRA)
                             GenWreg, 0,
                             B32POpConst, 16,
                             GenWreg, 0);
      */
      break;
    case tokUShort:
      /*
      GenPrintInstr3Operands(MipsInstrAnd, 0,
                             GenWreg, 0,
                             GenWreg, 0,
                             B32POpConst, 0xFFFF);*/

      /* just use as an int for now
      GenPrintInstr3Operands(B32PInstrSHIFTL, 0,
                             GenWreg, 0,
                             B32POpConst, 16,
                             GenWreg, 0);
      GenPrintInstr3Operands(B32PInstrSHIFTR, 0,
                             GenWreg, 0,
                             B32POpConst, 16,
                             GenWreg, 0);
      */
      break;

    case tokShortCirc:
#ifndef NO_ANNOTATIONS
      if (v >= 0)
        printf2("&&\n");
      else
        printf2("||\n");
#endif
      if (v >= 0)
        GenJumpIfZero(v); // &&
      else
        GenJumpIfNotZero(-v); // ||
      gotUnary = 0;
      break;
    case tokGoto:
#ifndef NO_ANNOTATIONS
      printf2("goto\n");
#endif
      GenJumpUncond(v);
      gotUnary = 0;
      break;
    case tokLogAnd:
    case tokLogOr:
      GenNumLabel(v);
      break;

    case tokVoid:
      gotUnary = 0;
      break;

    case tokRevIdent:
    case tokRevLocalOfs:
    case tokComma:
    case tokReturn:
    case tokNum0:
      break;

    case tokIf:
      GenJumpIfNotZero(stack[i][1]);
      break;
    case tokIfNot:
      GenJumpIfZero(stack[i][1]);
      break;

    default:
      //error("Error: Internal Error: GenExpr0(): unexpected token %s\n", GetTokenName(tok));
      errorInternal(103);
      break;
    }
  }

  if (GenWreg != B32POpRegV0)
    errorInternal(104);
}

STATIC
void GenDumpChar(int ch)
{
  if (ch < 0)
  {
    if (TokenStringLen)
      printf2("\n");
    return;
  }

  if (TokenStringLen == 0)
  {
    GenStartAsciiString();
    //printf2("\"");
  }

  // Just print as ascii value
  printf2("%d ", ch);
}

STATIC
void GenExpr(void)
{
  GenExpr0();
}

STATIC
void GenFin(void)
{
  // No idea what this does (something with structs??), so I just literally converted it to B32P asm
  if (StructCpyLabel)
  {
    int lbl = LabelCnt++;

    puts2(CodeHeaderFooter[0]);

    GenNumLabel(StructCpyLabel);

    //puts2(" move r2, r6\n" //r2 := r6
    //      " move r3, r6"); //r3 := r3
    puts2(" or r0 r6 r2\n"
          " or r0 r6 r3");


    GenNumLabel(lbl);

    //puts2(" lbu r6, 0 r5\n"       // r6:=mem[r5]
    //      " addiu r5, r5, 1\n"    // r5:= r5+1
    //      " addiu r4, r4, -1\n"   // r4:= r4-1
    //      " sb r6, 0 r3\n"        // mem[r3]:=r6
    //      " addiu r3, r3, 1");    // r3:= r3+1

    puts2(" read 0 r5 r6\n"
          " add r5 1 r5\n"
          " sub r4 1 r4\n"
          " write 0 r3 r6\n"
          " add r3 1 r3");

    //printf2(" bne r4, r0, "); GenPrintNumLabel(lbl); // if r4 != 0, jump to lbl
    printf2("beq r4 r0 2\n");
    printf2("jump ");GenPrintNumLabel(lbl);


    puts2("");
    puts2(" jumpr 0 r15");

    puts2(CodeHeaderFooter[1]);
  }

  // Put all ending C specific wrapper code here
  if (compileUserBDOS)
  {
    printf2(
      ".code\n"
      "; END OF COMPILED C CODE\n"
      "\n"
      "; Interrupt handlers for BDOS user program\n"
      "; Has some administration before jumping to Label_int[ID]\n"
      "; To prevent interfering with other stacks, they have their own stack\n"
      "; Because this is a BDOS user program, the interrupts are called from the BDOS interrupt handlers\n"
      "; Therefore, it should return to that interrupt handler and not use reti\n"
      "\n"
      "Int:\n"
      "\n"
      "    load32 0x7BFFFF r13     ; initialize user int stack address\n"
      "    load32 0 r14            ; initialize base pointer address\n"
      "    addr2reg Return_Interrupt r1 ; get address of return function\n"
      "    or r0 r1 r15            ; copy return addr to r15\n"
      "    ccache                  ; clear cache\n"
      "    jump interrupt          ; jump to interrupt handler of C program\n"
      "                            ; should return to the address we just put on the stack\n"
      "    halt                    ; should not get here\n"
      "\n"
      "\n"
      "; Function that is called after any interrupt handler from C has returned\n"
      "; Rreturns to BDOS interrupt handler\n"
      "Return_Interrupt:\n"
      "\n"
      "    ; RETURN\n"
      "    pop r1\n"
      "    ccache                  ; clear cache\n"
      "    jumpr 3 r1\n"
      "\n"
      "    halt        ; should not get here\n"
    );
  }
  else
  {
    printf2(
      ".code\n"
      "; END OF COMPILED C CODE\n"
      "\n"
      "; Interrupt handlers\n"
      "; Has some administration before jumping to Label_int[ID]\n"
      "; To prevent interfering with other stacks, they have their own stack\n"
      "; Also, all registers have to be backed up and restored to hardware stack\n"
      "; A return function has to be put on the stack as wel that the C code interrupt handler\n"
      "; will jump to when it is done\n"
      "\n"
      "Int:\n"
      "    push r1\n"
      "    push r2\n"
      "    push r3\n"
      "    push r4\n"
      "    push r5\n"
      "    push r6\n"
      "    push r7\n"
      "    push r8\n"
      "    push r9\n"
      "    push r10\n"
      "    push r11\n"
      "    push r12\n"
      "    push r13\n"
      "    push r14\n"
      "    push r15\n"
      "\n"
      "    load32 0x7FFFFF r13     ; initialize (BDOS) int stack address\n"
      "    load32 0 r14            ; initialize base pointer address\n"
      "    addr2reg Return_Interrupt r1 ; get address of return function\n"
      "    or r0 r1 r15            ; copy return addr to r15\n"
      "    ccache                  ; clear cache\n"
      "    jump interrupt          ; jump to interrupt handler of C program\n"
      "                            ; should return to the address we just put on the stack\n"
      "    halt                    ; should not get here\n"
      "\n"
      "\n"
      "; Function that is called after any interrupt handler from C has returned\n"
      "; Restores all registers and issues RETI instruction to continue from original code\n"
      "Return_Interrupt:\n"
      "    pop r15\n"
      "    pop r14\n"
      "    pop r13\n"
      "    pop r12\n"
      "    pop r11\n"
      "    pop r10\n"
      "    pop r9\n"
      "    pop r8\n"
      "    pop r7\n"
      "    pop r6\n"
      "    pop r5\n"
      "    pop r4\n"
      "    pop r3\n"
      "    pop r2\n"
      "    pop r1\n"
      "    ccache                  ; clear cache\n"
      "\n"
      "    reti        ; return from interrrupt\n"
      "\n"
      "    halt        ; should not get here\n");
  }

  if (compileOS)
  {
    printf2(
      ".code\n"
      "\n; Syscall handler for OS\n"
      "; Because this is not called during an interrupt, we use a different stack\n"
      ";  located at the end of BDOS heap\n"
      "\n"
      "Syscall:\n"
      "    load32 0x3FFFFF r13     ; initialize syscall stack address\n"
      "    load32 0 r14            ; initialize base pointer address\n"
      "    addr2reg Return_Syscall r1 ; get address of return function\n"
      "    or r0 r1 r15            ; copy return addr to r15\n"
      "    ccache                  ; clear cache\n"
      "    jump syscall            ; jump to syscall handler of C program\n"
      "                            ; should return to the address we just put on the stack\n"
      "    halt                    ; should not get here\n"
      "\n"
      "Return_Syscall:\n"
      "    pop r1\n"
      "    ccache                  ; clear cache\n"
      "    jumpr 3 r1\n"
      "\n"
      "    halt        ; should not get here\n"
      );
  }

  

}
