/*
Copyright (c) 2021, bartpleiter
Copyright (c) 2012-2020, Alexey Frunze
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
/*                           BCC (B322 C Compiler)                           */
/*                                                                           */
/*                           C compiler for B322                             */
/*                 Modified version intended to run on FPGC                  */
/*                                                                           */
/*                            Based on SmallerC:                             */
/*                 A simple and small single-pass C compiler                 */
/*                                                                           */
/*                                Main file                                  */
/*                                                                           */
/*****************************************************************************/

/* PLAN:
- move through program from execution start to finish
- implement missing (mostly FS) functions on the go (skip unimportant ones first, and do them later)
- try to implement vararg printf, since it will save hours of printf replacements
- FS functions (test/create library in a seperate test program):
  - fopen: just return a pointer to (global var!) the full path of the file, init cursor (private)
  - fput/get: open the file if not already opened, use cursor to read/write
*/

// Making most functions static helps with code optimization,
// use that to further reduce compiler's code size on RetroBSD.
#define word char

#define NO_EXTRAS
#define CAN_COMPILE_32BIT
#define MIPS
#define B322
#define NO_PPACK
#define NO_TYPEDEF_ENUM
#define NO_FUNC_
#define NO_EXTRA_WARNS
#define NO_FOR_DECL
#define NO_STRUCT_BY_VAL
#define NO_FP
#define NO_WCHAR

#define NULL 0

#define size_t word //unsigned int

#define CHAR_BIT (8)
#define CHAR_MIN (-128)
#define CHAR_MAX (127)

#define INT_MAX (2147483647)
#define INT_MIN (-2147483647 - 1)

#define UINT_MAX (4294967295u)
#define UINT_MIN (0u)

#define EXIT_FAILURE 1


#define MACROTABLE_ADDR 0x440000
#define IDENTTABLE_ADDR 0x450000
#define SYNSTACK0_ADDR 0x460000
#define SYNSTACK1_ADDR 0x470000
#define INPUTBUFFER_ADDR 0x480000
#define FILENAMES_ADDR 0x490000

#include "lib/math.c"
#include "lib/sys.c"
//#include "lib/gfx.c"
#include "lib/stdlib.c"
#include "lib/fs.c"
#include "lib/stdio.c"

////////////////////////////////////////////////////////////////////////////////


// all public macros

#define MAX_IDENT_LEN         63
#define MAX_STRING_LEN        255
#define MAX_CHAR_QUEUE_LEN    (MAX_STRING_LEN + 1)
#define MAX_MACRO_TABLE_LEN   (4096+1024)
#define MAX_IDENT_TABLE_LEN   (4096+1024+512) // must be greater than MAX_IDENT_LEN
#define SYNTAX_STACK_MAX      (2048+1024)
#define MAX_FILE_NAME_LEN     95

#define MAX_INCLUDES         8
#define PREP_STACK_SIZE      8
#define MAX_SEARCH_PATH      256


/* +-~* /% &|^! << >> && || < <= > >= == !=  () *[] ++ -- = += -= ~= *= /= %= &= |= ^= <<= >>= {} ,;: -> ... */

#define tokEof        0
#define tokNumInt     1
#define tokNumUint    2
#define tokLitStr     3

#define tokLShift     4
#define tokRShift     5
#define tokLogAnd     6
#define tokLogOr      7
#define tokEQ         8
#define tokNEQ        9
#define tokLEQ        10
#define tokGEQ        11
#define tokInc        12
#define tokDec        13
#define tokArrow      14
#define tokEllipsis   15

#define tokIdent      16
#define tokVoid       17
#define tokChar       18
#define tokInt        19
#define tokReturn     20
#define tokGoto       21
#define tokIf         22
#define tokElse       23
#define tokWhile      24
#define tokCont       25
#define tokBreak      26
#define tokSizeof     27

#define tokAssignMul  'A'
#define tokAssignDiv  'B'
#define tokAssignMod  'C'
#define tokAssignAdd  'D'
#define tokAssignSub  'E'
#define tokAssignLSh  'F'
#define tokAssignRSh  'G'
#define tokAssignAnd  'H'
#define tokAssignXor  'I'
#define tokAssignOr   'J'

#define tokFloat      'a'
#define tokDouble     'b'
#define tokLong       'c'
#define tokShort      'd'
#define tokUnsigned   'e'
#define tokSigned     'f'
#define tokConst      'g'
#define tokVolatile   'h'
#define tokRestrict   'i'
#define tokStatic     'j'
#define tokInline     'k'
#define tokExtern     'l'
#define tokAuto       'm'
#define tokRegister   'n'
#define tokTypedef    'o'
#define tokEnum       'p'
#define tokStruct     'q'
#define tokUnion      'r'
#define tokDo         's'
#define tokFor        't'
#define tokSwitch     'u'
#define tokCase       'v'
#define tokDefault    'w'
#define tok_Bool      'x'
#define tok_Complex   'y'
#define tok_Imagin    'z'

#define tok_Asm       '`'

/* Pseudo-tokens (converted from others or generated) */
#define tokURShift    28
#define tokUDiv       29
#define tokUMod       30
#define tokAssignURSh 31
#define tokAssignUDiv '@'
#define tokAssignUMod 'K'
#define tokComma      '0'

#define tokIfNot      'L'

#define tokUnaryAnd   'M'
#define tokUnaryStar  'N'
#define tokUnaryPlus  'O'
#define tokUnaryMinus 'P'

#define tokPostInc    'Q'
#define tokPostDec    'R'
#define tokPostAdd    'S'
#define tokPostSub    'T'

#define tokULess      'U'
#define tokUGreater   'V'
#define tokULEQ       'W'
#define tokUGEQ       'X'

#define tokLocalOfs   'Y'
#define tokShortCirc  'Z'

#define tokSChar      0x80
#define tokUChar      0x81
#define tokUShort     0x82
#define tokULong      0x83
//#define tokLongLong   0x84
//#define tokULongLong  0x85
//#define tokLongDbl    0x86
#define tokGotoLabel  0x8F
#define tokStructPtr  0x90
#define tokTag        0x91
#define tokMemberIdent 0x92
#define tokEnumPtr    0x93
#define tokIntr       0x94
#define tokNumFloat   0x95
#define tokNumCharWide 0x96
#define tokLitStrWide 0x97

//#define FormatFlat      0
#define FormatSegmented 1
//#define FormatSegTurbo  2
#define FormatSegHuge   3
#define FormatSegUnreal 4

#define SymVoidSynPtr 0
#define SymIntSynPtr  1
#define SymUintSynPtr 2
#define SymWideCharSynPtr 3
#define SymFloatSynPtr 4
#define SymFuncPtr    5

#define STACK_SIZE 129

#define SymFxn       1
#define SymGlobalVar 2
#define SymGlobalArr 3
#define SymLocalVar  4
#define SymLocalArr  5


// all public prototypes
unsigned truncUint(unsigned);
word truncInt(word);

word GetToken(void);
char* GetTokenName(word token);


void DumpMacroTable(void);


word AddIdent(char* name);
word FindIdent(char* name);

void DumpIdentTable(void);

char* lab2str(char* p, word n);

void GenInit(void);
void GenFin(void);
void GenInitFinalize(void);
void GenStartCommentLine(void);
void GenWordAlignment(word bss);
void GenLabel(char* Label, word Static);
void GenNumLabel(word Label);
void GenZeroData(unsigned Size, word bss);
void GenIntData(word Size, word Val);
void GenStartAsciiString(void);
void GenAddrData(word Size, char* Label, word ofs);

void GenJumpUncond(word Label);
void GenJumpIfZero(word Label);
void GenJumpIfNotZero(word Label);
void GenJumpIfEqual(word val, word Label);

void GenFxnProlog(void);
void GenFxnEpilog(void);
void GenIsrProlog(void);
void GenIsrEpilog(void);

word GenMaxLocalsSize(void);

void GenDumpChar(word ch);
void GenExpr(void);

void PushSyntax(word t);
void PushSyntax2(word t, word v);


void DumpSynDecls(void);


void push2(word v, word v2);
void ins2(word pos, word v, word v2);
void ins(word pos, word v);
void del(word pos, word cnt);

word TokenStartsDeclaration(word t, word params);
word ParseDecl(word tok, unsigned structInfo[4], word cast, word label);

void ShiftChar(void);
word puts2(char*);
word printf2(char*);

void error(char* strToPrint);
void warning(char* strToPrint);
void errorFile(char* n);
void errorFileName(void);
void errorInternal(word n);
void errorChrStr(void);
void errorStrLen(void);
void errorUnexpectedToken(word tok);
void errorDirective(void);
void errorCtrlOutOfScope(void);
void errorDecl(void);
void errorVarSize(void);
void errorInit(void);
void errorUnexpectedVoid(void);
void errorOpType(void);
void errorNotLvalue(void);
void errorNotConst(void);
void errorLongExpr(void);

word FindSymbol(char* s);
word SymType(word SynPtr);
word FindTaggedDecl(char* s, word start, word* CurScope);
word GetDeclSize(word SyntaxPtr, word SizeForDeref);

word ParseExpr(word tok, word* GotUnary, word* ExprTypeSynPtr, word* ConstExpr, word* ConstVal, word option, word option2);
word GetFxnInfo(word ExprTypeSynPtr, word* MinParams, word* MaxParams, word* ReturnExprTypeSynPtr, word* FirstParamSynPtr);

// all data

word verbose = 0;
word warnings = 0;
word warnCnt = 0;
word doAnnotations = 0;

// custom compiler flags
word compileUserBDOS = 0;
word compileOS = 0;

// prep.c data

// TBD!!! get rid of TokenIdentName[] and TokenValueString[]
// and work with CharQueue[] directly
word TokenValueInt = 0;
char TokenIdentName[MAX_IDENT_LEN + 1];
word TokenIdentNameLen = 0;
char TokenValueString[MAX_STRING_LEN + 1];
unsigned TokenStringLen = 0;
unsigned TokenStringSize = 0; // TokenStringLen * sizeof(char/wchar_t)
word LineNo = 1;
word LinePos = 1;
char CharQueue[MAX_CHAR_QUEUE_LEN];
word CharQueueLen = 0;

/*
  Macro table entry format:
    idlen char:     identifier length (<= 127)
    id char[idlen]: identifier (ASCIIZ)
    exlen char:     length of what the identifier expands into (<= 127)
    ex char[exlen]: what the identifier expands into (ASCII)
*/
char *MacroTable = (char*) MACROTABLE_ADDR; //[MAX_MACRO_TABLE_LEN];
word MacroTableLen = 0;

/*
  Identifier table entry format:
    id char[idlen]: string (ASCIIZ)
    idlen char:     string length (<= 127)
*/
char *IdentTable = (char*) IDENTTABLE_ADDR; //[MAX_IDENT_TABLE_LEN];
word IdentTableLen = 0;
word DummyIdent; // corresponds to empty string

#define MAX_GOTO_LABELS 16

word gotoLabels[MAX_GOTO_LABELS][2];
// gotoLabStat[]: bit 1 = used (by "goto label;"), bit 0 = defined (with "label:")
char gotoLabStat[MAX_GOTO_LABELS];
word gotoLabCnt = 0;

#define MAX_CASES 128

word Cases[MAX_CASES][2]; // [0] is case constant, [1] is case label number
word CasesCnt = 0;

// Data structures to support #include
word FileCnt = 0;

char (*FileNames)[MAX_FILE_NAME_LEN + 1] = (char (*)[MAX_FILE_NAME_LEN + 1]) FILENAMES_ADDR;
//char FileNames[MAX_INCLUDES][MAX_FILE_NAME_LEN + 1];


word Files[MAX_INCLUDES]; // FILE
word OutFile; // FILE
char CharQueues[MAX_INCLUDES][3];
word LineNos[MAX_INCLUDES];
word LinePoss[MAX_INCLUDES];
char SysSearchPaths[MAX_SEARCH_PATH];
word SysSearchPathsLen = 0;
char SearchPaths[MAX_SEARCH_PATH];
word SearchPathsLen = 0;

// Data structures to support #ifdef/#ifndef,#else,#endif
word PrepDontSkipTokens = 1;
word PrepStack[PREP_STACK_SIZE][2];
word PrepSp = 0;


// expr.c data

word ExprLevel = 0;

// TBD??? merge expression and operator stacks into one
word stack[STACK_SIZE][2];
word sp = 0;

#define OPERATOR_STACK_SIZE STACK_SIZE
word opstack[OPERATOR_STACK_SIZE][2];
word opsp = 0;

// smc.c data

word OutputFormat = FormatSegmented;
word GenExterns = 1;
word UseBss = 1;

// Names of C functions and variables are usually prefixed with an underscore.
// One notable exception is the ELF format used by gcc in Linux.
// Global C identifiers in the ELF format should not be predixed with an underscore.
word UseLeadingUnderscores = 1;

char* FileHeader = "";
char* CodeHeaderFooter[2] = { "", "" };
char* DataHeaderFooter[2] = { "", "" };
char* RoDataHeaderFooter[2] = { "", "" };
char* BssHeaderFooter[2] = { "", "" };
char** CurHeaderFooter;

word CharIsSigned = 1;
word SizeOfWord = 2; // in chars (char can be a multiple of octets); ints and pointers are of word size
//word SizeOfWideChar = 2; // in chars/bytes, 2 or 4
//word WideCharIsSigned = 0; // 0 or 1
//word WideCharType1;
//word WideCharType2; // (un)signed counterpart of WideCharType1

// TBD??? implement a function to allocate N labels with overflow checks
word LabelCnt = 1; // label counter for jumps
word StructCpyLabel = 0; // label of the function to copy structures/unions
//word StructPushLabel = 0; // label of the function to push structures/unions onto the stack

// call stack (from higher to lower addresses):
//   arg n
//   ...
//   arg 1
//   return address
//   saved xbp register
//   local var 1
//   ...
//   local var n
word CurFxnSyntaxPtr = 0;
word CurFxnParamCntMin = 0;
word CurFxnParamCntMax = 0;
word CurFxnLocalOfs = 0; // negative
word CurFxnMinLocalOfs = 0; // negative

word CurFxnReturnExprTypeSynPtr = 0;
word CurFxnEpilogLabel = 0;

char* CurFxnName = NULL;
word IsMain; // if inside main()

word ParseLevel = 0; // Parse level/scope (file:0, fxn:1+)
word ParamLevel = 0; // 1+ if parsing params, 0 otherwise

unsigned char *SyntaxStack0 = (unsigned char*) SYNSTACK0_ADDR; //[SYNTAX_STACK_MAX];
word *SyntaxStack1 = (word*) SYNSTACK1_ADDR; //[SYNTAX_STACK_MAX];
word SyntaxStackCnt;






// all code

unsigned truncUint(unsigned n)
{
  // Truncate n to SizeOfWord * 8 bits
  if (SizeOfWord == 2)
    n &= ~(~0u << 8 << 8);
  else if (SizeOfWord == 4)
    n &= ~(~0u << 8 << 12 << 12);
  return n;
}

word truncInt(word n)
{
  // Truncate n to SizeOfWord * 8 bits and then sign-extend it
  unsigned un = n;
  if (SizeOfWord == 2)
  {
    un &= ~(~0u << 8 << 8);
    un |= (((un >> 8 >> 7) & 1) * ~0u) << 8 << 8;
  }
  else if (SizeOfWord == 4)
  {
    un &= ~(~0u << 8 << 12 << 12);
    un |= (((un >> 8 >> 12 >> 11) & 1) * ~0u) << 8 << 12 << 12;
  }
  return (word)un;
}

// prep.c code

word FindMacro(char* name)
{
  word i;

  for (i = 0; i < MacroTableLen; )
  {
    if (!strcmp(MacroTable + i + 1, name))
      return i + 1 + MacroTable[i];

    i = i + 1 + MacroTable[i]; // skip id
    i = i + 1 + MacroTable[i]; // skip ex
  }

  return -1;
}

word UndefineMacro(char* name)
{
  word i;

  for (i = 0; i < MacroTableLen; )
  {
    if (!strcmp(MacroTable + i + 1, name))
    {
      word len = 1 + MacroTable[i]; // id part len
      len = len + 1 + MacroTable[i + len]; // + ex part len

      memmove(MacroTable + i,
              MacroTable + i + len,
              MacroTableLen - i - len);
      MacroTableLen -= len;

      return 1;
    }

    i = i + 1 + MacroTable[i]; // skip id
    i = i + 1 + MacroTable[i]; // skip ex
  }

  return 0;
}

void AddMacroIdent(char* name)
{
  word l = strlen(name);

  if (l >= 127)
  {
    printf("Macro identifier too long ");
    printf(name);
    error("\n");
  }

  if (MAX_MACRO_TABLE_LEN - MacroTableLen < l + 3)
    error("Macro table exhausted\n");

  MacroTable[MacroTableLen++] = l + 1; // idlen
  strcpy(MacroTable + MacroTableLen, name);
  MacroTableLen += l + 1;

  MacroTable[MacroTableLen] = 0; // exlen
}

void AddMacroExpansionChar(char e)
{
  if (e == '\0')
  {
    // finalize macro definition entry
    // remove trailing space first
    while (MacroTable[MacroTableLen] &&
           strchr(" \t", MacroTable[MacroTableLen + MacroTable[MacroTableLen]]))
      MacroTable[MacroTableLen]--;
    MacroTableLen += 1 + MacroTable[MacroTableLen];
    return;
  }

  if (MacroTableLen + 1 + MacroTable[MacroTableLen] >= MAX_MACRO_TABLE_LEN)
    error("Macro table exhausted\n");

  if (MacroTable[MacroTableLen] >= 127)
    error("Macro definition too long\n");

  MacroTable[MacroTableLen + 1 + MacroTable[MacroTableLen]] = e;
  MacroTable[MacroTableLen]++;
}


void DumpMacroTable(void)
{
  word i, j;

  puts2("");
  GenStartCommentLine(); printf2("Macro table:\n");
  for (i = 0; i < MacroTableLen; )
  {
    GenStartCommentLine(); 
    printf2("Macro ");
    printf2(MacroTable + i + 1);
    printf2(" = ");

    i = i + 1 + MacroTable[i]; // skip id
    printf2("`");
    j = MacroTable[i++];
    while (j--)
    {
      char* c = " ";
      c[0] = MacroTable[i++];
      printf2(c);
    }
    printf2("`\n");
  }
  GenStartCommentLine(); //TODO: printf2("Bytes used: %d/%d\n\n", MacroTableLen, MAX_MACRO_TABLE_LEN);
}

word FindIdent(char* name)
{
  word i;
  for (i = IdentTableLen; i > 0; )
  {
    i -= 1 + IdentTable[i - 1];
    if (!strcmp(IdentTable + i, name))
      return i;
  }
  return -1;
}

word AddIdent(char* name)
{
  word i, len;

  if ((i = FindIdent(name)) >= 0)
    return i;

  i = IdentTableLen;
  len = strlen(name);

  if (len >= 127)
    error("Identifier too long\n");

  if (MAX_IDENT_TABLE_LEN - IdentTableLen < len + 2)
    error("Identifier table exhausted\n");

  strcpy(IdentTable + IdentTableLen, name);
  IdentTableLen += len + 1;
  IdentTable[IdentTableLen++] = len + 1;

  return i;
}

word AddNumericIdent(word n)
{
  char s[1 + (2 + CHAR_BIT * sizeof n) / 3];
  char *p = s + sizeof s;
  *--p = '\0';
  p = lab2str(p, n);
  return AddIdent(p);
}

word AddGotoLabel(char* name, word label)
{
  word i;
  for (i = 0; i < gotoLabCnt; i++)
  {
    if (!strcmp(IdentTable + gotoLabels[i][0], name))
    {
      if (gotoLabStat[i] & label)
      {
        printf("Redefinition of label ");
        printf(name);
        error("\n");
      }
      gotoLabStat[i] |= 2*!label + label;
      return gotoLabels[i][1];
    }
  }
  if (gotoLabCnt >= MAX_GOTO_LABELS)
    error("Goto table exhausted\n");
  gotoLabels[gotoLabCnt][0] = AddIdent(name);
  gotoLabels[gotoLabCnt][1] = LabelCnt++;
  gotoLabStat[gotoLabCnt] = 2*!label + label;
  return gotoLabels[gotoLabCnt++][1];
}

void UndoNonLabelIdents(word len)
{
  word i;
  IdentTableLen = len;
  for (i = 0; i < gotoLabCnt; i++)
    if (gotoLabels[i][0] >= len)
    {
      char* pfrom = IdentTable + gotoLabels[i][0];
      char* pto = IdentTable + IdentTableLen;
      word l = strlen(pfrom) + 2;
      memmove(pto, pfrom, l);
      IdentTableLen += l;
      gotoLabels[i][0] = pto - IdentTable;
    }
}

void AddCase(word val, word label)
{
  if (CasesCnt >= MAX_CASES)
    error("Case table exhausted\n");

  Cases[CasesCnt][0] = val;
  Cases[CasesCnt++][1] = label;
}

void DumpIdentTable(void)
{
  word i;
  puts2("");
  GenStartCommentLine(); printf2("Identifier table:\n");
  for (i = 0; i < IdentTableLen; )
  {
    GenStartCommentLine(); 
    printf2("Ident ");
    printf2(IdentTable + i);
    printf2("\n");
    i += strlen(IdentTable + i) + 2;
  }
  GenStartCommentLine(); //TODO: printf2("Bytes used: %d/%d\n\n", IdentTableLen, MAX_IDENT_TABLE_LEN);
}

char* rws[] =
{
  "break", "case", "char", "continue", "default", "do", "else",
  "extern", "for", "if", "int", "return", "signed", "sizeof",
  "static", "switch", "unsigned", "void", "while", "asm", "auto",
  "const", "double", "enum", "float", "goto", "inline", "long",
  "register", "restrict", "short", "struct", "typedef", "union",
  "volatile", "_Bool", "_Complex", "_Imaginary",
  "__interrupt"
};

unsigned char rwtk[] =
{
  tokBreak, tokCase, tokChar, tokCont, tokDefault, tokDo, tokElse,
  tokExtern, tokFor, tokIf, tokInt, tokReturn, tokSigned, tokSizeof,
  tokStatic, tokSwitch, tokUnsigned, tokVoid, tokWhile, tok_Asm, tokAuto,
  tokConst, tokDouble, tokEnum, tokFloat, tokGoto, tokInline, tokLong,
  tokRegister, tokRestrict, tokShort, tokStruct, tokTypedef, tokUnion,
  tokVolatile, tok_Bool, tok_Complex, tok_Imagin,
  tokIntr
};

word GetTokenByWord(char* wrd)
{
  unsigned i;

  for (i = 0; i < division(sizeof rws, sizeof rws[0]); i++)
    if (!strcmp(rws[i], wrd))
      return rwtk[i];

  return tokIdent;
}

unsigned char tktk[] =
{
  tokEof,
  // Single-character operators and punctuators:
  '+', '-', '~', '*', '/', '%', '&', '|', '^', '!',
  '<', '>', '(', ')', '[', ']',
  '{', '}', '=', ',', ';', ':', '.', '?',
  // Multi-character operators and punctuators:
  tokLShift, tokLogAnd, tokEQ, tokLEQ, tokInc, tokArrow, tokAssignMul,
  tokAssignMod, tokAssignSub, tokAssignRSh, tokAssignXor,
  tokRShift, tokLogOr, tokNEQ, tokGEQ, tokDec, tokEllipsis,
  tokAssignDiv, tokAssignAdd, tokAssignLSh, tokAssignAnd, tokAssignOr,
  // Some of the above tokens get converted into these in the process:
  tokUnaryAnd, tokUnaryPlus, tokPostInc, tokPostAdd,
  tokULess, tokULEQ, tokURShift, tokUDiv, tokUMod, tokComma,
  tokUnaryStar, tokUnaryMinus, tokPostDec, tokPostSub,
  tokUGreater, tokUGEQ, tokAssignURSh, tokAssignUDiv, tokAssignUMod,
  // Helper (pseudo-)tokens:
  tokNumInt, tokLitStr, tokLocalOfs, tokNumUint, tokIdent, tokShortCirc,
  tokSChar, tokShort, tokLong, tokUChar, tokUShort, tokULong, tokNumFloat,
  tokNumCharWide, tokLitStrWide
};

char* tks[] =
{
  "<EOF>",
  // Single-character operators and punctuators:
  "+", "-", "~", "*", "/", "%", "&", "|", "^", "!",
  "<", ">", "(", ")", "[", "]",
  "{", "}", "=", ",", ";", ":", ".", "?",
  // Multi-character operators and punctuators:
  "<<", "&&", "==", "<=", "++", "->", "*=",
  "%=", "-=", ">>=", "^=",
  ">>", "||", "!=", ">=", "--", "...",
  "/=", "+=", "<<=", "&=", "|=",
  // Some of the above tokens get converted into these in the process:
  "&u", "+u", "++p", "+=p",
  "<u", "<=u", ">>u", "/u", "%u", ",b",
  "*u", "-u", "--p", "-=p",
  ">u", ">=u", ">>=u", "/=u", "%=u",
  // Helper (pseudo-)tokens:
  "<NumInt>",  "<LitStr>", "<LocalOfs>", "<NumUint>", "<Ident>", "<ShortCirc>",
  "signed char", "short", "long", "unsigned char", "unsigned short", "unsigned long", "float",
  "<NumCharWide>", "<LitStrWide>"
};

char* GetTokenName(word token)
{
  unsigned i;

/* +-~* /% &|^! << >> && || < <= > >= == !=  () *[] ++ -- = += -= ~= *= /= %= &= |= ^= <<= >>= {} ,;: -> ... */

  // Tokens other than reserved keywords:
  for (i = 0; i < division(sizeof tktk , sizeof tktk[0]); i++)
    if (tktk[i] == token)
      return tks[i];

  // Reserved keywords:
  for (i = 0; i < division(sizeof rws , sizeof rws[0]); i++)
    if (rwtk[i] == token)
      return rws[i];

  //error("Internal Error: GetTokenName(): Invalid token %d\n", token);
  errorInternal(1);
  return "";
}

word GetNextChar(void)
{
  word ch = EOF;

  if (FileCnt && Files[FileCnt - 1])
  {
    if ((ch = fgetc(Files[FileCnt - 1])) == EOF)
    {
      fclose(Files[FileCnt - 1]);
      Files[FileCnt - 1] = NULL;

      // store the last line/pos, they may still be needed later
      LineNos[FileCnt - 1] = LineNo;
      LinePoss[FileCnt - 1] = LinePos;

      // don't drop the file record just yet
    }
  }

  return ch;
}

void ShiftChar(void)
{
  if (CharQueueLen)
    memmove(CharQueue, CharQueue + 1, --CharQueueLen);

  // make sure there always are at least 3 chars in the queue
  while (CharQueueLen < 3)
  {
    word ch = GetNextChar();
    if (ch == EOF)
      ch = '\0';
    CharQueue[CharQueueLen++] = ch;
    //BDOS_PrintcConsole(ch);
  }
}

void ShiftCharN(word n)
{
  while (n-- > 0)
  {
    ShiftChar();
    LinePos++;
  }
}

void IncludeFile(word quot)
{
  word nlen = strlen(TokenValueString);

  if (CharQueueLen != 3)
    //error("#include parsing error\n");
    errorInternal(2);

  if (FileCnt >= MAX_INCLUDES)
    error("Too many include files\n");

  // store the including file's position and buffered chars
  LineNos[FileCnt - 1] = LineNo;
  LinePoss[FileCnt - 1] = LinePos;
  memcpy(CharQueues[FileCnt - 1], CharQueue, CharQueueLen);

  // open the included file

  if (nlen > MAX_FILE_NAME_LEN)
    //error("File name too long\n");
    errorFileName();

  // DONE: differentiate between quot == '"' and quot == '<'

  // First, try opening "file" in the current directory
  // (Open Watcom C/C++ 1.9, Turbo C++ 1.01 use the current directory,
  // unlike gcc, which uses the same directory as the current file)
  if (quot == '"')
  {
    // Get path from c file to compile, so it can be appended to the include paths
    char cFileDir[255] = "";
    strcpy(cFileDir, FileNames[0]);

    word len = strlen(cFileDir);  
    while (len > 0) 
    {
       len--;
       if (cFileDir[len] == '/') 
       {
          cFileDir[len+1] = '\0'; // keep the slash
          break;
       }
    }
    if (len == 0) // remove directory if there is none
    {
      cFileDir[0] = '\0';
    }
    strcpy(FileNames[FileCnt], TokenValueString);
    strcat(cFileDir, FileNames[FileCnt]);
    Files[FileCnt] = fopen(cFileDir, 0);
  }

  // Next, iterate the search paths trying to open "file" or <file>.
  // "file" is first searched using the list provided by the -I option.
  // "file" is then searched using the list provided by the -SI option.
  // <file> is searched using the list provided by the -SI option.
  if (Files[FileCnt] == NULL)
  {
    word i;
    char *paths = SearchPaths;
    word pl = SearchPathsLen;
    for (;;)
    {
      if (quot == '<')
      {
        paths = SysSearchPaths;
        pl = SysSearchPathsLen;
      }
      for (i = 0; i < pl; )
      {
        word plen = strlen(paths + i);
        if (plen + 1 + nlen < MAX_FILE_NAME_LEN)
        {
          strcpy(FileNames[FileCnt], paths + i);
          strcpy(FileNames[FileCnt] + plen + 1, TokenValueString);
          // Use '/' as a separator, typical for Linux/Unix,
          // but also supported by file APIs in DOS/Windows just as '\\'
          FileNames[FileCnt][plen] = '/';
          if ((Files[FileCnt] = fopen(FileNames[FileCnt], 0)) != NULL)
            break;
        }
        i += plen + 1;
      }
      if (Files[FileCnt] || quot == '<')
        break;
      quot = '<';
    }
  }

  if (Files[FileCnt] == NULL)
  {
    //error("Cannot open file \"%s\"\n", TokenValueString);
    errorFile(TokenValueString);
  }

  // reset line/pos and empty the char queue
  CharQueueLen = 0;
  LineNo = LinePos = 1;
  FileCnt++;

  // fill the char queue with file data
  ShiftChar();
}

word EndOfFiles(void)
{
  // if there are no including files, we're done
  if (!--FileCnt)
    return 1;

  // restore the including file's position and buffered chars
  LineNo = LineNos[FileCnt - 1];
  LinePos = LinePoss[FileCnt - 1];
  CharQueueLen = 3;
  memcpy(CharQueue, CharQueues[FileCnt - 1], CharQueueLen);

  return 0;
}

void SkipSpace(word SkipNewLines)
{
  char* p = CharQueue;

  while (*p != '\0')
  {
    if (strchr(" \t\f\v", *p))
    {
      ShiftCharN(1);
      continue;
    }

    if (strchr("\r\n", *p))
    {
      if (!SkipNewLines)
        return;

      if (*p == '\r' && p[1] == '\n')
        ShiftChar();

      ShiftChar();
      LineNo++;
      LinePos = 1;
      continue;
    }

    if (*p == '/')
    {
      if (p[1] == '/')
      {
        // // comment
        ShiftCharN(2);
        while (!strchr("\r\n", *p))
          ShiftCharN(1);
        continue;
      }
      else if (p[1] == '*')
      {
        // /**/ comment
        ShiftCharN(2);
        while (*p != '\0' && !(*p == '*' && p[1] == '/'))
        {
          if (strchr("\r\n", *p))
          {
            if (!SkipNewLines)
              error("Invalid comment\n");

            if (*p == '\r' && p[1] == '\n')
              ShiftChar();

            ShiftChar();
            LineNo++;
            LinePos = 1;
          }
          else
          {
            ShiftCharN(1);
          }
        }
        if (*p == '\0')
          error("Invalid comment\n");
        ShiftCharN(2);
        continue;
      }
    } // endof if (*p == '/')

    break;
  } // endof while (*p != '\0')
}

void SkipLine(void)
{
  char* p = CharQueue;

  while (*p != '\0')
  {
    if (strchr("\r\n", *p))
    {
      if (*p == '\r' && p[1] == '\n')
        ShiftChar();

      ShiftChar();
      LineNo++;
      LinePos = 1;
      break;
    }
    else
    {
      ShiftCharN(1);
    }
  }
}

void GetIdent(void)
{
  char* p = CharQueue;

  if (*p != '_' && !isalpha(*p & 0xFFu))
    error("Identifier expected\n");

  TokenIdentNameLen = 0;
  TokenIdentName[TokenIdentNameLen++] = *p;
  TokenIdentName[TokenIdentNameLen] = '\0';
  ShiftCharN(1);

  while (*p == '_' || isalnum(*p & 0xFFu))
  {
    if (TokenIdentNameLen == MAX_IDENT_LEN)
    {
      printf("Identifier too long ");
      printf(TokenIdentName);
      error("\n");
    }
    TokenIdentName[TokenIdentNameLen++] = *p;
    TokenIdentName[TokenIdentNameLen] = '\0';
    ShiftCharN(1);
  }
}

unsigned GetCharValue(word wide)
{
  char* p = CharQueue;
  unsigned ch = 0;
  word cnt = 0;
  if (*p == '\\')
  {
    ShiftCharN(1);
    if (strchr("\n\r", *p))
      goto lerr;
    if (*p == 'x')
    {
      // hexadecimal character codes \xN+
      // hexadecimal escape sequence is not limited in length per se
      // (may have many leading zeroes)
      static char digs[] = "0123456789ABCDEFabcdef";
      static char vals[] =
      {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        10, 11, 12, 13, 14, 15,
        10, 11, 12, 13, 14, 15
      };
      char* pp;
      word zeroes = 0;
      ShiftCharN(1);
      if (strchr("\n\r", *p))
        goto lerr;
      if (*p == '0')
      {
        do
        {
          ShiftCharN(1);
        } while (*p == '0');
        zeroes = 1;
      }
      while (*p && (pp = strchr(digs, *p)) != NULL)
      {
        ch <<= 4;
        ch |= vals[pp - digs];
        ShiftCharN(1);
        if (++cnt > 2)
        {
          if (PrepDontSkipTokens)
            goto lerr;
        }
      }
      if (zeroes + cnt == 0)
        goto lerr;
    }
    else if (*p >= '0' && *p <= '7')
    {
      // octal character codes \N+
      // octal escape sequence is terminated after three octal digits
      do
      {
        ch <<= 3;
        ch |= *p - '0';
        ShiftCharN(1);
        ++cnt;
      } while (*p >= '0' && *p <= '7' && cnt < 3);
      if (ch >> 8)
        goto lerr;
    }
    else
    {
      switch (*p)
      {
      case 'a': ch = '\a'; ShiftCharN(1); break;
      case 'b': ch = '\b'; ShiftCharN(1); break;
      case 'f': ch = '\f'; ShiftCharN(1); break;
      case 'n': ch = '\n'; ShiftCharN(1); break;
      case 'r': ch = '\r'; ShiftCharN(1); break;
      case 't': ch = '\t'; ShiftCharN(1); break;
      case 'v': ch = '\v'; ShiftCharN(1); break;
      default:
        goto lself;
      }
    }
  }
  else
  {
lself:
    if (strchr("\n\r", *p))
    {
lerr:
      //error("Unsupported or invalid character/string constant\n");
      errorChrStr();
    }
    ch = *p & 0xFFu;
    ShiftCharN(1);
  }
  return ch;
}

void GetString(char terminator, word wide, word option)
{
  char* p = CharQueue;
  unsigned ch = '\0';
  unsigned chsz = 1;
  word i;
  char* ctmp = " ";

  TokenStringLen = 0;
  TokenStringSize = 0;
  TokenValueString[TokenStringLen] = '\0';

  ShiftCharN(1);
  while (!(*p == terminator || strchr("\n\r", *p)))
  {
    ch = GetCharValue(wide);
    switch (option)
    {
    case '#': // string literal (with file name) for #line and #include
      if (TokenStringLen == MAX_STRING_LEN)
        errorStrLen();
      TokenValueString[TokenStringLen++] = ch;
      TokenValueString[TokenStringLen] = '\0';
      TokenStringSize += chsz;
      break;
    case 'a': // string literal for asm()
      ctmp[0] = ch;
      printf2(ctmp);
      break;
    case 'd': // string literal / array of char in expression or initializer
      // Dump the char data to the appropriate data section
      for (i = 0; i < chsz; i++)
      {
        GenDumpChar(ch & 0xFFu);
        ch >>= 8;
        TokenStringLen++; // GenDumpChar() expects it to grow, doesn't know about wchar_t
      }
      TokenStringLen -= chsz;
      // fallthrough
    default: // skipped string literal (we may still need the size)
      if (TokenStringSize > UINT_MAX - chsz)
        errorStrLen();
      TokenStringSize += chsz;
      TokenStringLen++;
      break;
    } // endof switch (option)
  } // endof while (!(*p == '\0' || *p == terminator || strchr("\n\r", *p)))

  if (*p != terminator)
    //error("Unsupported or invalid character/string constant\n");
    errorChrStr();

  if (option == 'd')
    GenDumpChar(-1);

  ShiftCharN(1);

  SkipSpace(option != '#');
}

void pushPrep(word NoSkip)
{
  if (PrepSp >= PREP_STACK_SIZE)
    error("Too many #if(n)def's\n");
  PrepStack[PrepSp][0] = PrepDontSkipTokens;
  PrepStack[PrepSp++][1] = NoSkip;
  PrepDontSkipTokens &= NoSkip;
}

word popPrep(void)
{
  if (PrepSp <= 0)
    error("#else or #endif without #if(n)def\n");
  PrepDontSkipTokens = PrepStack[--PrepSp][0];
  return PrepStack[PrepSp][1];
}


word GetNumber(void)
{
  char* p = CharQueue;
  word ch = *p;
  word leadingZero = (ch == '0');
  unsigned n = 0;
  word type = 0;
  word uSuffix = 0;
  word lSuffix = 0;
  char* eTooBig = "Constant too big\n";

  // First, detect and handle hex constants. Octals can't be detected immediately
  // because floating-point constants also may begin with the digit 0.

  if (leadingZero && (p[1] == 'x' || p[1] == 'X'))
  {
    // this is a hex constant
    word cnt = 0;
    type = 'h';
    ShiftCharN(1);
    ShiftCharN(1);
    while ((ch = *p) != '\0' && (isdigit(ch & 0xFFu) || strchr("abcdefABCDEF", ch)))
    {
      if (ch >= 'a') ch -= 'a' - 10;
      else if (ch >= 'A') ch -= 'A' - 10;
      else ch -= '0';
      //if (PrepDontSkipTokens && (n * 16 / 16 != n || n * 16 + ch < n * 16))
      //  error(eTooBig);
      n = n * 16 + ch;
      ShiftCharN(1);
      cnt++;
    }
    if (!cnt)
      error("Invalid hexadecimal constant\n");
  }
  else
  {
    // handle decimal and octal integers
    word base = leadingZero ? 8 : 10;
    type = leadingZero ? 'o' : 'd';
    while ((ch = *p) >= '0' && ch < base + '0')
    {
      ch -= '0';
      if (PrepDontSkipTokens && (n * base + ch < n * base)) //n * base / base != n || 
        error(eTooBig);
      n = n * base + ch;
      ShiftCharN(1);
    }
  }

  // possible combinations of integer suffixes:
  //   none
  //   U
  //   UL
  //   L
  //   LU
  {
    if ((ch = *p) == 'u' || ch == 'U')
    {
      uSuffix = 1;
      ShiftCharN(1);
    }
    if ((ch = *p) == 'l' || ch == 'L')
    {
      lSuffix = 1;
      ShiftCharN(1);
      if (!uSuffix && ((ch = *p) == 'u' || ch == 'U'))
      {
        uSuffix = 1;
        ShiftCharN(1);
      }
    }
  }

  if (!PrepDontSkipTokens)
  {
    // Don't fail on big constants when skipping tokens under #if
    TokenValueInt = 0;
    return tokNumInt;
  }


  // Ensure the constant fits into 16(32) bits
  if (
      (SizeOfWord == 2 && n >> 8 >> 8) // equiv. to SizeOfWord == 2 && n > 0xFFFF
      || (SizeOfWord == 2 && lSuffix) // long (which must have at least 32 bits) isn't supported in 16-bit models
      || (SizeOfWord == 4 && n >> 8 >> 12 >> 12) // equiv. to SizeOfWord == 4 && n > 0xFFFFFFFF
     )
    error("Constant too big for 32-bit type\n");

  TokenValueInt = (word)n;

  // Unsuffixed (with 'u') integer constants (octal, decimal, hex)
  // fitting into 15(31) out of 16(32) bits are signed ints
  if (!uSuffix &&
      (
       (SizeOfWord == 2 && !(n >> 15)) // equiv. to SizeOfWord == 2 && n <= 0x7FFF
       || (SizeOfWord == 4 && !(n >> 8 >> 12 >> 11)) // equiv. to SizeOfWord == 4 && n <= 0x7FFFFFFF
      )
     )
    return tokNumInt;

  // Unlike octal and hex constants, decimal constants are always
  // a signed type. Error out when a decimal constant doesn't fit
  // into an int since currently there's no next bigger signed type
  // (e.g. long) to use instead of int.
  if (!uSuffix && type == 'd')
    error("Constant too big for 32-bit signed type\n");

  return tokNumUint;
}

word GetTokenInner(void)
{
  char* p = CharQueue;
  word ch = *p;
  word wide = 0;

  // these single-character tokens/operators need no further processing
  if (strchr(",;:()[]{}~?", ch))
  {
    ShiftCharN(1);
    return ch;
  }

  // parse multi-character tokens/operators

  // DONE: other assignment operators
  switch (ch)
  {
  case '+':
    if (p[1] == '+') { ShiftCharN(2); return tokInc; }
    if (p[1] == '=') { ShiftCharN(2); return tokAssignAdd; }
    ShiftCharN(1); return ch;
  case '-':
    if (p[1] == '-') { ShiftCharN(2); return tokDec; }
    if (p[1] == '=') { ShiftCharN(2); return tokAssignSub; }
    if (p[1] == '>') { ShiftCharN(2); return tokArrow; }
    ShiftCharN(1); return ch;
  case '!':
    if (p[1] == '=') { ShiftCharN(2); return tokNEQ; }
    ShiftCharN(1); return ch;
  case '=':
    if (p[1] == '=') { ShiftCharN(2); return tokEQ; }
    ShiftCharN(1); return ch;
  case '<':
    if (p[1] == '=') { ShiftCharN(2); return tokLEQ; }
    if (p[1] == '<') { ShiftCharN(2); if (p[0] != '=') return tokLShift; ShiftCharN(1); return tokAssignLSh; }
    ShiftCharN(1); return ch;
  case '>':
    if (p[1] == '=') { ShiftCharN(2); return tokGEQ; }
    if (p[1] == '>') { ShiftCharN(2); if (p[0] != '=') return tokRShift; ShiftCharN(1); return tokAssignRSh; }
    ShiftCharN(1); return ch;
  case '&':
    if (p[1] == '&') { ShiftCharN(2); return tokLogAnd; }
    if (p[1] == '=') { ShiftCharN(2); return tokAssignAnd; }
    ShiftCharN(1); return ch;
  case '|':
    if (p[1] == '|') { ShiftCharN(2); return tokLogOr; }
    if (p[1] == '=') { ShiftCharN(2); return tokAssignOr; }
    ShiftCharN(1); return ch;
  case '^':
    if (p[1] == '=') { ShiftCharN(2); return tokAssignXor; }
    ShiftCharN(1); return ch;
  case '.':
    if (p[1] == '.' && p[2] == '.') { ShiftCharN(3); return tokEllipsis; }
    ShiftCharN(1); return ch;
  case '*':
    if (p[1] == '=') { ShiftCharN(2); return tokAssignMul; }
    ShiftCharN(1); return ch;
  case '%':
    if (p[1] == '=') { ShiftCharN(2); return tokAssignMod; }
    ShiftCharN(1); return ch;
  case '/':
    if (p[1] == '=') { ShiftCharN(2); return tokAssignDiv; }
    ShiftCharN(1); return ch;
  }

  // DONE: hex and octal constants
  if (isdigit(ch & 0xFFu))
    return GetNumber();

  // parse character and string constants
  if (ch == '\'')
  {
    unsigned v = 0;
    word cnt = 0;
    word max_cnt = SizeOfWord;
    ShiftCharN(1);
    if (strchr("'\n\r", *p))
      //error("Character constant too short\n");
      errorChrStr();
    do
    {
      v <<= 8;
      v |= GetCharValue(wide);
      if (++cnt > max_cnt)
        //error("Character constant too long\n");
        errorChrStr();
    } while (!strchr("'\n\r", *p));
    if (*p != '\'')
      //error("Unsupported or invalid character/string constant\n");
      errorChrStr();
    ShiftCharN(1);
    {
      if (cnt == 1)
      {
        TokenValueInt = v;
        TokenValueInt -= (CharIsSigned && TokenValueInt >= 0x80) * 0x100;
      }
      else
      {
        TokenValueInt = v;
        TokenValueInt -= (SizeOfWord == 2 && TokenValueInt >= 0x8000) * 0x10000;
      }
      return tokNumInt;
    }
  }
  else if (ch == '"')
  {
    // The caller of GetTokenInner()/GetToken() will call GetString('"', wide, 'd')
    // to complete string literal parsing and storing as appropriate

    return tokLitStr;

  }

  return tokEof;
}


void Reserve4Expansion(char* name, word len)
{
  if (MAX_CHAR_QUEUE_LEN - CharQueueLen < len + 1)
  {
    printf("Too long expansion of macro ");
    printf(name);
    error("\n");
  }

  memmove(CharQueue + len + 1, CharQueue, CharQueueLen);

  CharQueue[len] = ' '; // space to avoid concatenation

  CharQueueLen += len + 1;
}


// TBD??? implement file I/O for input source code and output code (use fxn ptrs/wrappers to make librarization possible)
// DONE: support string literals
word GetToken(void)
{
  char* p = CharQueue;
  word ch;
  word tok;

  for (;;)
  {
/* +-~* /% &|^! << >> && || < <= > >= == !=  () *[] ++ -- = += -= ~= *= /= %= &= |= ^= <<= >>= {} ,;: -> ... */

    // skip white space and comments
    SkipSpace(1);

    if ((ch = *p) == '\0')
    {
      // done with the current file, drop its record,
      // pick up the including files (if any) or terminate
      if (EndOfFiles())
        break;
      continue;
    }

    if ((tok = GetTokenInner()) != tokEof)
    {
      if (PrepDontSkipTokens)
        return tok;
      if (tok == tokLitStr)
        GetString('"', 0, 0);

      continue;
    }

    // parse identifiers and reserved keywords
    if (ch == '_' || isalpha(ch & 0xFFu))
    {

      word midx;


      GetIdent();

      if (!PrepDontSkipTokens)
        continue;

      tok = GetTokenByWord(TokenIdentName);


      // TBD!!! think of expanding macros in the context of concatenating string literals,
      // maybe factor out this piece of code
      if (!strcmp(TokenIdentName, "__FILE__"))
      {
        char* p = FileNames[FileCnt - 1];
        word len = strlen(p);
        Reserve4Expansion(TokenIdentName, len + 2);
        *CharQueue = '"';
        memcpy(CharQueue + 1, p, len);
        CharQueue[len + 1] = '"';
        continue;
      }
      else if (!strcmp(TokenIdentName, "__LINE__"))
      {
        char s[(2 + CHAR_BIT * sizeof LineNo) / 3];
        char *p = lab2str(s + sizeof s, LineNo);
        word len = s + sizeof s - p;
        Reserve4Expansion(TokenIdentName, len);
        memcpy(CharQueue, p, len);
        continue;
      }
      else if ((midx = FindMacro(TokenIdentName)) >= 0)
      {
        // this is a macro identifier, need to expand it
        word len = MacroTable[midx];
        Reserve4Expansion(TokenIdentName, len);
        memcpy(CharQueue, MacroTable + midx + 1, len);
        continue;
      }


      // treat keywords auto, const, register, restrict and volatile as white space for now
      if ((tok == tokConst) | (tok == tokVolatile) |
          (tok == tokAuto) | (tok == tokRegister) |
          (tok == tokRestrict))
        continue;

      return tok;
    } // endof if (ch == '_' || isalpha(ch))

    // parse preprocessor directives
    if (ch == '#')
    {
      word line = 0;

      ShiftCharN(1);

      // Skip space
      SkipSpace(0);

      // Allow # not followed by a directive
      if (strchr("\r\n", *p))
        continue;

      // Get preprocessor directive
      if (isdigit(*p & 0xFFu))
      {
        // gcc-style #line directive without "line"
        line = 1;
      }
      else
      {
        GetIdent();
        if (!strcmp(TokenIdentName, "line"))
        {
          // C89-style #line directive
          SkipSpace(0);
          if (!isdigit(*p & 0xFFu))
            errorDirective();
          line = 1;
        }
      }

      if (line)
      {
        // Support for external, gcc-like, preprocessor output:
        //   # linenum filename flags
        //
        // no flags, flag = 1 -- start of a file
        //           flag = 2 -- return to a file after #include
        //        other flags -- uninteresting

        // DONE: should also support the following C89 form:
        // # line linenum filename-opt

        if (GetNumber() != tokNumInt)
          //error("Invalid line number in preprocessor output\n");
          errorDirective();
        line = TokenValueInt;

        SkipSpace(0);

        if (*p == '"' || *p == '<')
        {
          if (*p == '"')
            GetString('"', 0, '#');
          else
            GetString('>', 0, '#');

          if (strlen(TokenValueString) > MAX_FILE_NAME_LEN)
            //error("File name too long in preprocessor output\n");
            errorFileName();
          strcpy(FileNames[FileCnt - 1], TokenValueString);
        }

        // Ignore gcc-style #line's flags, if any
        while (!strchr("\r\n", *p))
          ShiftCharN(1);

        LineNo = line - 1; // "line" is the number of the next line
        LinePos = 1;

        continue;
      } // endof if (line)



      if (!strcmp(TokenIdentName, "define"))
      {
        // Skip space and get macro name
        SkipSpace(0);
        GetIdent();

        if (!PrepDontSkipTokens)
        {
          SkipSpace(0);
          while (!strchr("\r\n", *p))
            ShiftCharN(1);
          continue;
        }

        if (FindMacro(TokenIdentName) >= 0)
        {
          printf("Redefinition of macro ");
          printf(TokenIdentName);
          error("\n");
        }
        if (*p == '(')
          //error("Unsupported type of macro '%s'\n", TokenIdentName);
          errorDirective();

        AddMacroIdent(TokenIdentName);

        SkipSpace(0);

        // accumulate the macro expansion text
        while (!strchr("\r\n", *p))
        {
          AddMacroExpansionChar(*p);
          ShiftCharN(1);
          if (*p != '\0' && (strchr(" \t", *p) || (*p == '/' && (p[1] == '/' || p[1] == '*'))))
          {
            SkipSpace(0);
            AddMacroExpansionChar(' ');
          }
        }
        AddMacroExpansionChar('\0');

        continue;
      }
      else if (!strcmp(TokenIdentName, "undef"))
      {
        // Skip space and get macro name
        SkipSpace(0);
        GetIdent();

        if (PrepDontSkipTokens)
          UndefineMacro(TokenIdentName);

        SkipSpace(0);
        if (!strchr("\r\n", *p))
          //error("Invalid preprocessor directive\n");
          errorDirective();
        continue;
      }
      else if (!strcmp(TokenIdentName, "include"))
      {
        word quot;

        // Skip space and get file name
        SkipSpace(0);

        quot = *p;
        if (*p == '"')
          GetString('"', 0, '#');
        else if (*p == '<')
          GetString('>', 0, '#');
        else
          //error("Invalid file name\n");
          errorFileName();

        SkipSpace(0);
        if (!strchr("\r\n", *p))
          //error("Unsupported or invalid preprocessor directive\n");
          errorDirective();

        if (PrepDontSkipTokens)
          IncludeFile(quot);

        continue;
      }
      else if (!strcmp(TokenIdentName, "ifdef"))
      {
        word def;
        // Skip space and get macro name
        SkipSpace(0);
        GetIdent();
        def = FindMacro(TokenIdentName) >= 0;
        SkipSpace(0);
        if (!strchr("\r\n", *p))
          //error("Invalid preprocessor directive\n");
          errorDirective();
        pushPrep(def);
        continue;
      }
      else if (!strcmp(TokenIdentName, "ifndef"))
      {
        word def;
        // Skip space and get macro name
        SkipSpace(0);
        GetIdent();
        def = FindMacro(TokenIdentName) >= 0;
        SkipSpace(0);
        if (!strchr("\r\n", *p))
          //error("Invalid preprocessor directive\n");
          errorDirective();
        pushPrep(!def);
        continue;
      }
      else if (!strcmp(TokenIdentName, "else"))
      {
        word def;
        SkipSpace(0);
        if (!strchr("\r\n", *p))
          //error("Invalid preprocessor directive\n");
          errorDirective();
        def = popPrep();
        if (def >= 2)
          error("#else or #endif without #if(n)def\n");
        pushPrep(2 + !def); // #else works in opposite way to its preceding #if(n)def
        continue;
      }
      else if (!strcmp(TokenIdentName, "endif"))
      {
        SkipSpace(0);
        if (!strchr("\r\n", *p))
          //error("Invalid preprocessor directive\n");
          errorDirective();
        popPrep();
        continue;
      }

      if (!PrepDontSkipTokens)
      {
        // If skipping code and directives under #ifdef/#ifndef/#else,
        // ignore unsupported directives #if, #elif, #error (no error checking)
        if (!strcmp(TokenIdentName, "if"))
          pushPrep(0);
        else if (!strcmp(TokenIdentName, "elif"))
          popPrep(), pushPrep(0);
        SkipLine();
        continue;
      }

      //error("Unsupported or invalid preprocessor directive\n");
      errorDirective();
    } // endof if (ch == '#')

    error("Invalid or unsupported character");
    //error("Invalid or unsupported character with code 0x%02X\n", *p & 0xFFu);
  } // endof for (;;)

  return tokEof;
}

void errorRedecl(char* s)
{
  printf("Invalid or unsupported redeclaration of ");
  printf(s);
  error("\n");
}


#include "backend.c"




// expr.c code

void push2(word v, word v2)
{
  if (sp >= STACK_SIZE)
    //error("expression stack overflow!\n");
    errorLongExpr();
  stack[sp][0] = v;
  stack[sp++][1] = v2;
}

void push(word v)
{
  push2(v, 0);
}

word stacktop()
{
  if (sp == 0)
    //error("expression stack underflow!\n");
    errorInternal(3);
  return stack[sp - 1][0];
}

word pop2(word* v2)
{
  word v = stacktop();
  *v2 = stack[sp - 1][1];
  sp--;
  return v;
}

void ins2(word pos, word v, word v2)
{
  if (sp >= STACK_SIZE)
    //error("expression stack overflow!\n");
    errorLongExpr();
  memmove(&stack[pos + 1], &stack[pos], sizeof(stack[0]) * (sp - pos));
  stack[pos][0] = v;
  stack[pos][1] = v2;
  sp++;
}

void ins(word pos, word v)
{
  ins2(pos, v, 0);
}

void del(word pos, word cnt)
{
  memmove(stack[pos],
          stack[pos + cnt],
          sizeof(stack[0]) * (sp - (pos + cnt)));
  sp -= cnt;
}


void pushop2(word v, word v2)
{
  if (opsp >= OPERATOR_STACK_SIZE)
    //error("operator stack overflow!\n");
    errorLongExpr();
  opstack[opsp][0] = v;
  opstack[opsp++][1] = v2;
}

void pushop(word v)
{
  pushop2(v, 0);
}

word opstacktop()
{
  if (opsp == 0)
    //error("operator stack underflow!\n");
    errorInternal(4);
  return opstack[opsp - 1][0];
}

word popop2(word* v2)
{
  word v = opstacktop();
  *v2 = opstack[opsp - 1][1];
  opsp--;
  return v;
}

word popop()
{
  word v2;
  return popop2(&v2);
}

word isop(word tok)
{
  static unsigned char toks[] =
  {
    '!',
    '~',
    '&',
    '*',
    '/', '%',
    '+', '-',
    '|', '^',
    '<', '>',
    '=',
    tokLogOr, tokLogAnd,
    tokEQ, tokNEQ,
    tokLEQ, tokGEQ,
    tokLShift, tokRShift,
    tokInc, tokDec,
    tokSizeof,
    tokAssignMul, tokAssignDiv, tokAssignMod,
    tokAssignAdd, tokAssignSub,
    tokAssignLSh, tokAssignRSh,
    tokAssignAnd, tokAssignXor, tokAssignOr,
    tokComma,
    '?'
  };
  unsigned i;
  for (i = 0; i < division(sizeof toks , sizeof toks[0]); i++)
    if (toks[i] == tok)
      return 1;
  return 0;
}

word isunary(word tok)
{
  return (tok == '!') | (tok == '~') | (tok == tokInc) | (tok == tokDec) | (tok == tokSizeof);
}

word preced(word tok)
{
  switch (tok)
  {
  case '*': case '/': case '%': return 13;
  case '+': case '-': return 12;
  case tokLShift: case tokRShift: return 11;
  case '<': case '>': case tokLEQ: case tokGEQ: return 10;
  case tokEQ: case tokNEQ: return 9;
  case '&': return 8;
  case '^': return 7;
  case '|': return 6;
  case tokLogAnd: return 5;
  case tokLogOr: return 4;
  case '?': case ':': return 3;
  case '=':
  case tokAssignMul: case tokAssignDiv: case tokAssignMod:
  case tokAssignAdd: case tokAssignSub:
  case tokAssignLSh: case tokAssignRSh:
  case tokAssignAnd: case tokAssignXor: case tokAssignOr:
    return 2;
  case tokComma:
    return 1;
  }
  return 0;
}

word precedGEQ(word lfttok, word rhttok)
{
  // DONE: rethink the comma operator as it could be implemented similarly
  // DONE: is this correct:???
  word pl = preced(lfttok);
  word pr = preced(rhttok);
  // ternary/conditional operator ?: is right-associative
  if (pl == 3 && pr >= 3)
    pl = 0;
  // assignment is right-associative
  if (pl == 2 && pr >= 2)
    pl = 0;
  return pl >= pr;
}

word expr(word tok, word* gotUnary, word commaSeparator);

char* lab2str(char* p, word n)
{
  do
  {
    *--p = '0' + modulo(n , 10);
    n = division(n, 10);
  } while (n);

  return p;
}

word exprUnary(word tok, word* gotUnary, word commaSeparator, word argOfSizeOf)
{
  static word sizeofLevel = 0;

  word decl = 0;
  *gotUnary = 0;

  if (isop(tok) && (isunary(tok) || strchr("&*+-", tok)))
  {
    word lastTok = tok;
    word sizeofLevelInc = lastTok == tokSizeof;

    sizeofLevel += sizeofLevelInc;
    tok = exprUnary(GetToken(), gotUnary, commaSeparator, sizeofLevelInc);
    sizeofLevel -= sizeofLevelInc;

    if (!*gotUnary)
      //error("exprUnary(): primary expression expected after token %s\n", GetTokenName(lastTok));
      errorUnexpectedToken(tok);
    switch (lastTok)
    {
    // DONE: remove all collapsing of all unary operators.
    // It's wrong because type checking must occur before any optimizations.
    // WRONG: DONE: collapse alternating & and * (e.g. "*&*&x" "&*&*x")
    // WRONGISH: DONE: replace prefix ++/-- with +=1/-=1
    case '&':
      push(tokUnaryAnd);
      break;
    case '*':
      push(tokUnaryStar);
      break;
    case '+':
      push(tokUnaryPlus);
      break;
    case '-':
      push(tokUnaryMinus);
      break;
    case '!':
      // replace "!" with "== 0"
      push(tokNumInt);
      push(tokEQ);
      break;
    default:
      push(lastTok);
      break;
    }
  }
  else
  {
    word inspos = sp;

    if (tok == tokNumInt ||


        tok == tokNumUint)
    {
      push2(tok, TokenValueInt);
      *gotUnary = 1;
      tok = GetToken();
    }
    else if (tok == tokLitStr

            )
    {
      word lbl = LabelCnt++;
      word id;
      word ltok = tok;

      word wide = 0;
      unsigned chsz = 1;

      unsigned sz = chsz;

      // imitate definition: char #[len] = "...";

      if (!sizeofLevel)
      {
        if (CurHeaderFooter)
          puts2(CurHeaderFooter[1]);
        puts2(RoDataHeaderFooter[0]);


        GenNumLabel(lbl);
      }

      do
      {
        GetString('"', wide, sizeofLevel ? 0 : 'd'); // throw away string data inside sizeof, e.g. sizeof "a" or sizeof("a" + 1)
        if (sz + TokenStringSize < sz ||
            sz + TokenStringSize >= truncUint(-1))
          errorStrLen();
        sz += TokenStringSize;
        tok = GetToken();
      } while (tok == ltok); // concatenate adjacent string literals



      if (!sizeofLevel)
      {
        GenZeroData(chsz, 0);

        puts2(RoDataHeaderFooter[1]);
        if (CurHeaderFooter)
          puts2(CurHeaderFooter[0]);
      }

      // DONE: can this break incomplete yet declarations???, e.g.: int x[sizeof("az")][5];
      PushSyntax2(tokIdent, id = AddNumericIdent(lbl));
      PushSyntax('[');
      PushSyntax2(tokNumUint, division(sz, chsz));
      PushSyntax(']');

      PushSyntax(tokChar);

      push2(tokIdent, id);
      *gotUnary = 1;
    }
    else if (tok == tokIdent)
    {
      push2(tok, AddIdent(TokenIdentName));
      *gotUnary = 1;
      tok = GetToken();
    }
    else if (tok == '(')
    {
      tok = GetToken();
      decl = TokenStartsDeclaration(tok, 1);

      if (decl)
      {
        word synPtr;
        word lbl = LabelCnt++;
        char s[1 + (2 + CHAR_BIT * sizeof lbl) / 3 + sizeof "<something>" - 1];
        char *p = s + sizeof s;

        tok = ParseDecl(tok, NULL, !argOfSizeOf, 0);
        if (tok != ')')
          //error("exprUnary(): ')' expected, unexpected token %s\n", GetTokenName(tok));
          errorUnexpectedToken(tok);
        synPtr = FindSymbol("<something>");

        // Rename "<something>" to "<something#>", where # is lbl.
        // This makes the nameless declaration uniquely identifiable by name.

        *--p = '\0';
        *--p = ")>"[argOfSizeOf]; // differentiate casts (something#) from not casts <something#>

        p = lab2str(p, lbl);

        p -= sizeof "<something>" - 2 - 1;
        memcpy(p, "something", sizeof "something" - 1);

        *--p = "(<"[argOfSizeOf]; // differentiate casts (something#) from not casts <something#>

        SyntaxStack1[synPtr] = AddIdent(p);
        tok = GetToken();
        if (argOfSizeOf)
        {
          // expression: sizeof(type)
          *gotUnary = 1;
        }
        else
        {
          // unary type cast operator: (type)
          decl = 0;
          tok = exprUnary(tok, gotUnary, commaSeparator, 0);
          if (!*gotUnary)
            //error("exprUnary(): primary expression expected after '(type)'\n");
            errorUnexpectedToken(tok);
        }
        push2(tokIdent, SyntaxStack1[synPtr]);
      }
      else
      {
        tok = expr(tok, gotUnary, 0);
        if (tok != ')')
          //error("exprUnary(): ')' expected, unexpected token %s\n", GetTokenName(tok));
          errorUnexpectedToken(tok);
        if (!*gotUnary)
          //error("exprUnary(): primary expression expected in '()'\n");
          errorUnexpectedToken(tok);
        tok = GetToken();
      }
    }

    while (*gotUnary && !decl)
    {
      // DONE: f(args1)(args2) and the like: need stack order: args2, args1, f, (), ()
      // DONE: reverse the order of evaluation of groups of args in
      //       f(args1)(args2)(args3)
      // DONE: reverse the order of function argument evaluation for variadic functions
      //       we want 1st arg to be the closest to the stack top.
      // DONE: (args)[index] can be repeated interchangeably indefinitely
      // DONE: (expr)() & (expr)[]
      // DONE: [index] can be followed by ++/--, which can be followed by [index] and so on...
      // DONE: postfix ++/-- & differentiate from prefix ++/--

      if (tok == '(')
      {
        word acnt = 0;
        ins(inspos, '(');
        for (;;)
        {
          word pos2 = sp;

          tok = GetToken();
          tok = expr(tok, gotUnary, 1);

          // Reverse the order of argument evaluation, which is important for
          // variadic functions like printf():
          // we want 1st arg to be the closest to the stack top.
          // This also reverses the order of evaluation of all groups of
          // arguments.
          while (pos2 < sp)
          {
            // TBD??? not quite efficient
            word v, v2;
            v = pop2(&v2);
            ins2(inspos + 1, v, v2);
            pos2++;
          }

          if (tok == ',')
          {
            if (!*gotUnary)
              //error("exprUnary(): primary expression (fxn argument) expected before ','\n");
              errorUnexpectedToken(tok);
            acnt++;
            ins(inspos + 1, ','); // helper argument separator (hint for expression evaluator)
            continue; // off to next arg
          }
          if (tok == ')')
          {
            if (acnt && !*gotUnary)
              //error("exprUnary(): primary expression (fxn argument) expected between ',' and ')'\n");
              errorUnexpectedToken(tok);
            *gotUnary = 1; // don't fail for 0 args in ()
            break; // end of args
          }
          // DONE: think of inserting special arg pseudo tokens for verification purposes
          //error("exprUnary(): ',' or ')' expected, unexpected token %s\n", GetTokenName(tok));
          errorUnexpectedToken(tok);
        } // endof for(;;) for fxn args
        push(')');
      }
      else if (tok == '[')
      {
        tok = GetToken();
        tok = expr(tok, gotUnary, 0);
        if (!*gotUnary)
          //error("exprUnary(): primary expression expected in '[]'\n");
          errorUnexpectedToken(tok);
        if (tok != ']')
          //error("exprUnary(): ']' expected, unexpected token %s\n", GetTokenName(tok));
          errorUnexpectedToken(tok);
        // TBD??? add implicit casts to size_t of array indicies.
        // E1[E2] -> *(E1 + E2)
        // push('[');
        push('+');
        push(tokUnaryStar);
      }
      // WRONG: DONE: replace postfix ++/-- with (+=1)-1/(-=1)+1
      else if (tok == tokInc)
      {
        push(tokPostInc);
      }
      else if (tok == tokDec)
      {
        push(tokPostDec);
      }
      else if (tok == '.' || tok == tokArrow)
      {
        // transform a.b into (&a)->b
        if (tok == '.')
          push(tokUnaryAnd);
        tok = GetToken();
        if (tok != tokIdent)
          errorUnexpectedToken(tok);
        push2(tok, AddIdent(TokenIdentName));
        // "->" in "a->b" will function as "+" in "*(type_of_b*)((char*)a + offset_of_b_in_a)"
        push(tokArrow);
        push(tokUnaryStar);
      }
      else
      {
        break;
      }
      tok = GetToken();
    } // endof while (*gotUnary)
  }

  if (tok == ',' && !commaSeparator)
    tok = tokComma;

  return tok;
}

word expr(word tok, word* gotUnary, word commaSeparator)
{
  *gotUnary = 0;
  pushop(tokEof);

  tok = exprUnary(tok, gotUnary, commaSeparator, 0);

  while (tok != tokEof && strchr(",;:)]}", tok) == NULL && *gotUnary)
  {
    if (isop(tok) && !isunary(tok))
    {
      while (precedGEQ(opstacktop(), tok))
      {
        word v, v2;
        v = popop2(&v2);
        // move ?expr: as a whole to the expression stack as "expr?"
        if (v == '?')
        {
          word cnt = v2;
          while (cnt--)
          {
            v = popop2(&v2);
            push2(v, v2);
          }
          v = '?';
          v2 = 0;
        }
        push2(v, v2);
      }

      // here: preced(postacktop()) < preced(tok)

      // treat the ternary/conditional operator ?expr: as a pseudo binary operator
      if (tok == '?')
      {
        word ssp = sp;
        word cnt;

        tok = expr(GetToken(), gotUnary, 0);
        if (!*gotUnary || tok != ':')
          errorUnexpectedToken(tok);

        // move ?expr: as a whole to the operator stack
        // this is beautiful and ugly at the same time
        cnt = sp - ssp;
        while (sp > ssp)
        {
          word v, v2;
          v = pop2(&v2);
          pushop2(v, v2);
        }

        // remember the length of the expression between ? and :
        pushop2('?', cnt);
      }
      else
      {
        pushop(tok);
      }

      tok = exprUnary(GetToken(), gotUnary, commaSeparator, 0);
      // DONE: figure out a check to see if exprUnary() fails to add a rhs operand
      if (!*gotUnary)
        //error("expr(): primary expression expected after token %s\n", GetTokenName(lastTok));
        errorUnexpectedToken(tok);

      continue;
    }

    //error("expr(): Unexpected token %s\n", GetTokenName(tok));
    errorUnexpectedToken(tok);
  }

  while (opstacktop() != tokEof)
  {
    word v, v2;
    v = popop2(&v2);
    // move ?expr: as a whole to the expression stack as "expr?"
    if (v == '?')
    {
      word cnt = v2;
      while (cnt--)
      {
        v = popop2(&v2);
        push2(v, v2);
      }
      v = '?';
      v2 = 0;
    }
    push2(v, v2);
  }

  popop();
  return tok;
}

word isAnyPtr(word ExprTypeSynPtr)
{
  if (ExprTypeSynPtr < 0)
    return 1;
  switch (SyntaxStack0[ExprTypeSynPtr])
  {
  case '*':
  case '[':
  case '(':
    return 1;
  }
  return 0;
}

word derefAnyPtr(word ExprTypeSynPtr)
{
  if (ExprTypeSynPtr < 0)
    return -ExprTypeSynPtr;
  switch (SyntaxStack0[ExprTypeSynPtr])
  {
  case '*':
    return ExprTypeSynPtr + 1;
  case '[':
    return ExprTypeSynPtr + 3;
  case '(':
    return ExprTypeSynPtr;
  }
  errorInternal(22);
  return -1;
}

void decayArray(word* ExprTypeSynPtr, word arithmetic)
{
  // Dacay arrays to pointers to their first elements
  if (*ExprTypeSynPtr >= 0 && SyntaxStack0[*ExprTypeSynPtr] == '[')
  {
    (*ExprTypeSynPtr) += 3;
    // we cannot insert another '*' into the type to make it a pointer
    // to the first element, so make the index into the type negative
    *ExprTypeSynPtr = -*ExprTypeSynPtr;
  }

  // DONE: disallow arithmetic on pointers to void
  // DONE: disallow function pointers
  if (arithmetic && isAnyPtr(*ExprTypeSynPtr))
  {
    word pointee = derefAnyPtr(*ExprTypeSynPtr);
    switch (SyntaxStack0[pointee])
    {
    case tokVoid:
      //error("decayArray(): cannot do pointer arithmetic on a pointer to 'void'\n");
      errorUnexpectedVoid();
    default:
      //error("decayArray(): cannot do pointer arithmetic on a pointer to an incomplete type\n");
      if (!GetDeclSize(pointee, 0))
      // "fallthrough"
    case '(':
        //error("decayArray(): cannot do pointer arithmetic on a pointer to a function\n");
        errorOpType();
    }
  }
}

void lvalueCheck(word ExprTypeSynPtr, word pos)
{
  if (ExprTypeSynPtr >= 0 &&
      (SyntaxStack0[ExprTypeSynPtr] == '[' || SyntaxStack0[ExprTypeSynPtr] == '('))
  {
    // we can have arrays formed by dereference, e.g.
    //   char (*pac)[1]; // *pac is array of 1 char
    //                   // ++*pac or (*pac)-- are not allowed
    // and likewise functions, e.g.
    //   int (*pf)(int); // *pf is a function taking int and returning int
    //                   // *pf = 0; is not allowed
    // and that dereference shouldn't be confused for lvalue,
    // hence explicitly checking for array and function types
    //error("exprval(): lvalue expected\n");
    errorNotLvalue();
  }
  // lvalue is a dereferenced address, check for a dereference
  if (stack[pos][0] != tokUnaryStar)
    //error("exprval(): lvalue expected\n");
    errorNotLvalue();
}

void nonVoidTypeCheck(word ExprTypeSynPtr)
{
  if (ExprTypeSynPtr >= 0 && SyntaxStack0[ExprTypeSynPtr] == tokVoid)
    //error("nonVoidTypeCheck(): unexpected operand type 'void' for operator '%s'\n", GetTokenName(tok));
    errorUnexpectedVoid();
}

void scalarTypeCheck(word ExprTypeSynPtr)
{
  nonVoidTypeCheck(ExprTypeSynPtr);

  if (ExprTypeSynPtr >= 0 && SyntaxStack0[ExprTypeSynPtr] == tokStructPtr)
    errorOpType();
}

void numericTypeCheck(word ExprTypeSynPtr)
{
  if (ExprTypeSynPtr >= 0)
    switch (SyntaxStack0[ExprTypeSynPtr])
    {
    case tokChar:
    case tokSChar:
    case tokUChar:
    case tokShort:
    case tokUShort:
    case tokInt:
    case tokUnsigned:
      return;
    }
  //error("numericTypeCheck(): unexpected operand type for operator '%s', numeric type expected\n", GetTokenName(tok));
  errorOpType();
}



void anyIntTypeCheck(word ExprTypeSynPtr)
{
  // Check for any integer type
  numericTypeCheck(ExprTypeSynPtr);

}



word isUint(word ExprTypeSynPtr)
{
  return ExprTypeSynPtr >= 0 && SyntaxStack0[ExprTypeSynPtr] == tokUnsigned;
}

void compatCheck(word* ExprTypeSynPtr, word TheOtherExprTypeSynPtr, word ConstExpr[2], word lidx, word ridx)
{
  word exprTypeSynPtr = *ExprTypeSynPtr;
  word c = 0;
  word lptr, rptr, lnum, rnum;

  // (un)decay/convert functions to pointers to functions
  // and to simplify matters convert all '*' pointers to negative type indices
  if (exprTypeSynPtr >= 0)
  {
    switch (SyntaxStack0[exprTypeSynPtr])
    {
    case '*':
      exprTypeSynPtr++;
      // fallthrough
    case '(':
      exprTypeSynPtr = -exprTypeSynPtr;
    }
    *ExprTypeSynPtr = exprTypeSynPtr;
  }
  if (TheOtherExprTypeSynPtr >= 0)
  {
    switch (SyntaxStack0[TheOtherExprTypeSynPtr])
    {
    case '*':
      TheOtherExprTypeSynPtr++;
      // fallthrough
    case '(':
      TheOtherExprTypeSynPtr = -TheOtherExprTypeSynPtr;
    }
  }
  lptr = exprTypeSynPtr < 0;
  rptr = TheOtherExprTypeSynPtr < 0;
  lnum = !lptr && (SyntaxStack0[exprTypeSynPtr] == tokInt ||

                   SyntaxStack0[exprTypeSynPtr] == tokUnsigned);
  rnum = !rptr && (SyntaxStack0[TheOtherExprTypeSynPtr] == tokInt ||

                   SyntaxStack0[TheOtherExprTypeSynPtr] == tokUnsigned);

  // both operands have arithmetic type
  // (arithmetic operands have been already promoted):
  if (lnum && rnum)
    return;

  // both operands have void type:
  if (!lptr && SyntaxStack0[exprTypeSynPtr] == tokVoid &&
      !rptr && SyntaxStack0[TheOtherExprTypeSynPtr] == tokVoid)
    return;

  // one operand is a pointer and the other is NULL constant
  // ((void*)0 is also a valid null pointer constant),
  // the type of the expression is that of the pointer:
  if (lptr &&
      ((rnum && ConstExpr[1] && truncInt(stack[ridx][1]) == 0) ||
       (rptr && SyntaxStack0[-TheOtherExprTypeSynPtr] == tokVoid &&
        (stack[ridx][0] == tokNumInt || stack[ridx][0] == tokNumUint) &&
        truncInt(stack[ridx][1]) == 0)))
    return;
  if (rptr &&
      ((lnum && ConstExpr[0] && truncInt(stack[lidx][1]) == 0) ||
       (lptr && SyntaxStack0[-exprTypeSynPtr] == tokVoid &&
        (stack[lidx][0] == tokNumInt || stack[lidx][0] == tokNumUint) &&
        truncInt(stack[lidx][1]) == 0)))
  {
    *ExprTypeSynPtr = TheOtherExprTypeSynPtr;
    return;
  }

  // not expecting non-pointers beyond this point
  if (!(lptr && rptr))
    errorOpType();

  // one operand is a pointer and the other is a pointer to void
  // (except (void*)0 (AKA NULL), which is different from other pointers to void),
  // the type of the expression is pointer to void:
  if (SyntaxStack0[-exprTypeSynPtr] == tokVoid)
    return;
  if (SyntaxStack0[-TheOtherExprTypeSynPtr] == tokVoid)
  {
    *ExprTypeSynPtr = TheOtherExprTypeSynPtr;
    return;
  }

  // both operands are pointers to compatible types:

  if (exprTypeSynPtr == TheOtherExprTypeSynPtr)
    return;

  exprTypeSynPtr = -exprTypeSynPtr;
  TheOtherExprTypeSynPtr = -TheOtherExprTypeSynPtr;

  for (;;)
  {
    word tok = SyntaxStack0[exprTypeSynPtr];
    if (tok != SyntaxStack0[TheOtherExprTypeSynPtr])
      errorOpType();

    if (tok != tokIdent &&
        SyntaxStack1[exprTypeSynPtr] != SyntaxStack1[TheOtherExprTypeSynPtr])
      errorOpType();

    c += (tok == '(') - (tok == ')') + (tok == '[') - (tok == ']');

    if (!c)
    {
      switch (tok)
      {
      case tokVoid:
      case tokChar: case tokSChar: case tokUChar:

      case tokShort: case tokUShort:

      case tokInt: case tokUnsigned:

      case tokStructPtr:
        return;
      }
    }

    exprTypeSynPtr++;
    TheOtherExprTypeSynPtr++;
  }
}

void shiftCountCheck(word *psr, word idx, word ExprTypeSynPtr)
{
  word sr = *psr;
  // can't shift by a negative count and by a count exceeding
  // the number of bits in int
  if ((SyntaxStack0[ExprTypeSynPtr] != tokUnsigned && sr < 0) ||
      (unsigned)sr >= CHAR_BIT * sizeof(word) ||
      (unsigned)sr >= 8u * SizeOfWord)
  {
    //error("exprval(): Invalid shift count\n");
    warning("Shift count out of range\n");
    // truncate the count, so the assembler doesn't get an invalid count
    sr &= SizeOfWord * 8 - 1;
    *psr = sr;
    stack[idx][1] = sr;
  }
}

word divCheckAndCalc(word tok, word* psl, word sr, word Unsigned, word ConstExpr[2])
{
  word div0 = 0;
  word sl = *psl;
  word int_min = INT_MIN;

  if (!ConstExpr[1])
    return !div0;

  if (Unsigned)
  {
    sl = (word)truncUint(sl);
    sr = (word)truncUint(sr);
  }
  else
  {
    sl = truncInt(sl);
    sr = truncInt(sr);
  }

  if (sr == 0)
  {
    div0 = 1;
  }
  else if (!ConstExpr[0])
  {
    return !div0;
  }
  else if (!Unsigned && ((sl == int_min && sr == -1) || division(sl , sr) != truncInt(division(sl , sr))))
  {
    div0 = 1;
  }
  else
  {
    if (Unsigned)
    {
      if (tok == '/')
        sl = (word)((unsigned)division(sl , sr));
      else
        sl = (word)((unsigned)modulo(sl , sr));
    }
    else
    {
      // TBD!!! C89 gives freedom in how exactly division of negative integers
      // can be implemented w.r.t. rounding and w.r.t. the sign of the remainder.
      // A stricter, C99-conforming implementation, non-dependent on the
      // compiler used to compile Smaller C is needed.
      if (tok == '/')
        sl = division(sl, sr);
      else
        sl = modulo(sl, sr);
    }
    *psl = sl;
  }

  if (div0)
    warning("Division by 0 or division overflow\n");

  return !div0;
}

void promoteType(word* ExprTypeSynPtr, word* TheOtherExprTypeSynPtr)
{
  // Integer promotion to signed int or unsigned int from smaller types
  // (all kinds of char and short). Promotion to unsigned int occurs
  // only if the other operand (of a binary operator) is already an
  // unsigned int. Effectively, this promotion to unsigned int performs
  // usual arithmetic conversion for integers.
  if (*ExprTypeSynPtr >= 0)
  {
    // chars must be promoted to ints in expressions as the very first thing
    switch (SyntaxStack0[*ExprTypeSynPtr])
    {
    case tokChar:
    case tokShort:
    case tokUShort:
    case tokSChar:
    case tokUChar:
      *ExprTypeSynPtr = SymIntSynPtr;
    }
    if (*TheOtherExprTypeSynPtr >= 0)
    {
      // ints must be converted to unsigned ints if they are used in binary
      // operators whose other operand is unsigned int (except <<,>>,<<=,>>=)
      if (SyntaxStack0[*ExprTypeSynPtr] == tokInt &&
          SyntaxStack0[*TheOtherExprTypeSynPtr] == tokUnsigned)
        *ExprTypeSynPtr = SymUintSynPtr;
    }
  }
}

word GetFxnInfo(word ExprTypeSynPtr, word* MinParams, word* MaxParams, word* ReturnExprTypeSynPtr, word* FirstParamSynPtr)
{
  *MaxParams = *MinParams = 0;

  if (ExprTypeSynPtr < 0)
  {
    ExprTypeSynPtr = -ExprTypeSynPtr;
  }
  else
  {
    while (SyntaxStack0[ExprTypeSynPtr] == tokIdent || SyntaxStack0[ExprTypeSynPtr] == tokLocalOfs)
      ExprTypeSynPtr++;
    if (SyntaxStack0[ExprTypeSynPtr] == '*')
      ExprTypeSynPtr++;
  }

  if (SyntaxStack0[ExprTypeSynPtr] != '(')
    return 0;

  // DONE: return syntax pointer to the function's return type

  // Count params

  ExprTypeSynPtr++;

  if (FirstParamSynPtr)
    *FirstParamSynPtr = ExprTypeSynPtr;

  if (SyntaxStack0[ExprTypeSynPtr] == ')')
  {
    // "fxn()": unspecified parameters, so, there can be any number of them
    *MaxParams = 32767; // INT_MAX;
    *ReturnExprTypeSynPtr = ExprTypeSynPtr + 1;
    return 1;
  }

  if (SyntaxStack0[ExprTypeSynPtr + 1] == tokVoid)
  {
    // "fxn(void)": 0 parameters
    *ReturnExprTypeSynPtr = ExprTypeSynPtr + 3;
    return 1;
  }

  for (;;)
  {
    word tok = SyntaxStack0[ExprTypeSynPtr];

    if (tok == tokIdent)
    {
      if (SyntaxStack0[ExprTypeSynPtr + 1] != tokEllipsis)
      {
        ++*MinParams;
        ++*MaxParams;
      }
      else
      {
        *MaxParams = 32767; // INT_MAX;
      }
    }
    else if (tok == '(')
    {
      // skip parameters in parameters
      word c = 1;
      while (c && ExprTypeSynPtr < SyntaxStackCnt)
      {
        tok = SyntaxStack0[++ExprTypeSynPtr];
        c += (tok == '(') - (tok == ')');
      }
    }
    else if (tok == ')')
    {
      ExprTypeSynPtr++;
      break;
    }

    ExprTypeSynPtr++;
  }

  // get the function's return type
  *ReturnExprTypeSynPtr = ExprTypeSynPtr;

  return 1;
}

void simplifyConstExpr(word val, word isConst, word* ExprTypeSynPtr, word top, word bottom)
{
  // If non-const, nothing to do.
  // If const and already a number behind the scenes, nothing to do
  // (val must not differ from the number!).
  if (!isConst || stack[top][0] == tokNumInt || stack[top][0] == tokNumUint)
    return;

  // Const, but not a number yet. Reduce to a number equal val.
  if (SyntaxStack0[*ExprTypeSynPtr] == tokUnsigned)
    stack[top][0] = tokNumUint;
  else
    stack[top][0] = tokNumInt;
  stack[top][1] = val;

  del(bottom, top - bottom);
}

word AllocLocal(unsigned size)
{
  // Let's calculate variable's relative on-stack location
  word oldOfs = CurFxnLocalOfs;

  // Note: local vars are word-aligned on the stack
  CurFxnLocalOfs = (word)((CurFxnLocalOfs - size) & ~(SizeOfWord - 1u));
  if (CurFxnLocalOfs >= oldOfs ||
      CurFxnLocalOfs != truncInt(CurFxnLocalOfs) ||
      (unsigned int)CurFxnLocalOfs < (unsigned int) (-GenMaxLocalsSize()))
    //error("AllocLocal(): Local variables take too much space\n");
  {
    //printf("a");
    errorVarSize();
  }
    

  if (CurFxnMinLocalOfs > CurFxnLocalOfs)
    CurFxnMinLocalOfs = CurFxnLocalOfs;

  return CurFxnLocalOfs;
}

// DONE: sizeof(type)
// DONE: "sizeof expr"
// DONE: constant expressions
// DONE: collapse constant subexpressions into constants
word exprval(word* idx, word* ExprTypeSynPtr, word* ConstExpr)
{
  word tok;
  word s;
  word RightExprTypeSynPtr;
  word oldIdxRight;
  word oldSpRight;
  word constExpr[3];

  if (*idx < 0)
    //error("exprval(): idx < 0\n");
    errorInternal(5);

  tok = stack[*idx][0];
  s = stack[*idx][1];

  --*idx;

  oldIdxRight = *idx;
  oldSpRight = sp;

  switch (tok)
  {
  // Constants
  case tokNumInt:
    // return the constant's type: int
    *ExprTypeSynPtr = SymIntSynPtr;
    *ConstExpr = 1;
    break;
  case tokNumUint:
    // return the constant's type: unsigned int
    *ExprTypeSynPtr = SymUintSynPtr;
    *ConstExpr = 1;
    break;



  // Identifiers
  case tokIdent:
    {
      // DONE: support __func__
      char* ident = IdentTable + s;
      word synPtr, type;

      {
        synPtr = FindSymbol(ident);
        // "Rename" static vars in function scope
        if (synPtr >= 0 && synPtr + 1 < SyntaxStackCnt && SyntaxStack0[synPtr + 1] == tokIdent)
        {
          s = stack[*idx + 1][1] = SyntaxStack1[++synPtr];
          ident = IdentTable + s;
        }
      }

      if (synPtr < 0)
      {
        if ((*idx + 2 >= sp) || stack[*idx + 2][0] != ')')
        {
          printf("Undeclared identifier ");
          error(ident);
        }
        else
        {
          // silent this warning for now, since it seems to trigger false positives
          //printf("Call to undeclared function ");
          //warning(ident);
          // Implicitly declare "extern int ident();"
          PushSyntax2(tokIdent, s);
          PushSyntax('(');
          PushSyntax(')');
          PushSyntax(tokInt);
          synPtr = FindSymbol(ident);
        }
      }



      // DONE: this declaration is actually a type cast
      if (!strncmp(IdentTable + SyntaxStack1[synPtr], "(something", sizeof "(something)" - 1 - 1))
      {

        word castSize;

        if (SyntaxStack0[++synPtr] == tokLocalOfs) // TBD!!! is this really needed???
          synPtr++;

        s = exprval(idx, ExprTypeSynPtr, ConstExpr);

        // can't cast void or structure/union to anything (except void)
        if (*ExprTypeSynPtr >= 0 &&
            (SyntaxStack0[*ExprTypeSynPtr] == tokVoid ||
             SyntaxStack0[*ExprTypeSynPtr] == tokStructPtr) &&
            SyntaxStack0[synPtr] != tokVoid)
          errorOpType();

        // can't cast to function, array or structure/union
        if (SyntaxStack0[synPtr] == '(' ||
            SyntaxStack0[synPtr] == '[' ||
            SyntaxStack0[synPtr] == tokStructPtr)
          errorOpType();



        // will try to propagate constants through casts
        if (!*ConstExpr &&
            (stack[oldIdxRight - (oldSpRight - sp)][0] == tokNumInt ||
             stack[oldIdxRight - (oldSpRight - sp)][0] == tokNumUint))
        {
          s = stack[oldIdxRight - (oldSpRight - sp)][1];
          *ConstExpr = 1;
        }

        castSize = GetDeclSize(synPtr, 1); // 0 for cast to void



        // insertion of tokUChar, tokSChar and tokUnaryPlus transforms
        // lvalues (values formed by dereferences) into rvalues
        // (by hiding the dereferences), just as casts should do
        switch (castSize)
        {
        case 1:
          // cast to unsigned char
          stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokUChar;
          s &= 0xFFu;
          break;
        case -1:
          // cast to signed char
          stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokSChar;
          if ((s &= 0xFFu) >= 0x80)
            s -= 0x100;
          break;
        default:

          if (castSize && castSize != SizeOfWord)
          {
            // cast not to void and not to word-sized type (int/unsigned/pointer/float)
            if (castSize == 2)
            {
              // cast to unsigned short
              stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokUShort;
              s &= 0xFFFFu;
            }
            else
            {
              // cast to signed short
              stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokShort;
              if ((s &= 0xFFFFu) >= 0x8000)
                s -= 0x10000;
            }
          }
          else // fallthrough

          {
            // cast to void or word-sized type (int/unsigned/pointer/float)

            if (stack[oldIdxRight - (oldSpRight - sp)][0] == tokUnaryStar)
              // hide the dereference
              stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokUnaryPlus;
          }
          break;
        }

        if (*ConstExpr)
          stack[oldIdxRight - (oldSpRight - sp)][1] = s;

        *ExprTypeSynPtr = synPtr;
        simplifyConstExpr(s, *ConstExpr, ExprTypeSynPtr, oldIdxRight + 1 - (oldSpRight - sp), *idx + 1);

        if (!*ConstExpr && stack[oldIdxRight + 1 - (oldSpRight - sp)][0] == tokIdent)
          // nothing to hide, remove the cast
          del(oldIdxRight + 1 - (oldSpRight - sp), 1);

        switch (SyntaxStack0[synPtr])
        {
        case tokChar:
        case tokSChar:
        case tokUChar:
        case tokShort:
        case tokUShort:

        case tokInt:
        case tokUnsigned:

          break;
        default:
          // only numeric types can be const
          *ConstExpr = 0;
          break;
        }

        break;
      }

      // Finally, not __func__, not enum, not cast.

      type = SymType(synPtr);

      if (type == SymLocalVar || type == SymLocalArr)
      {
        // replace local variables/arrays with their local addresses
        // (global variables/arrays' addresses are their names)
        stack[*idx + 1][0] = tokLocalOfs;
        stack[*idx + 1][1] = SyntaxStack1[synPtr + 1];
      }
      if (type == SymLocalVar || type == SymGlobalVar)
      {
        // add implicit dereferences for local/global variables
        ins2(*idx + 2, tokUnaryStar, GetDeclSize(synPtr, 1));
      }

      // return the identifier's type
      while (SyntaxStack0[synPtr] == tokIdent || SyntaxStack0[synPtr] == tokLocalOfs)
        synPtr++;
      *ExprTypeSynPtr = synPtr;
    }
    *ConstExpr = 0;
    break;

  // sizeof operator
  case tokSizeof:
    exprval(idx, ExprTypeSynPtr, ConstExpr);
    s = GetDeclSize(*ExprTypeSynPtr, 0);
    if (s == 0)
      error("sizeof of incomplete type\n");

    // replace sizeof with its numeric value
    stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokNumUint;
    stack[oldIdxRight + 1 - (oldSpRight - sp)][1] = s;

    // remove the sizeof's subexpression
    del(*idx + 1, oldIdxRight - (oldSpRight - sp) - *idx);

    *ExprTypeSynPtr = SymUintSynPtr;
    *ConstExpr = 1;
    break;

  // Address unary operator
  case tokUnaryAnd:
    exprval(idx, ExprTypeSynPtr, ConstExpr);

    if (*ExprTypeSynPtr >= 0 &&
        (SyntaxStack0[*ExprTypeSynPtr] == '[' || SyntaxStack0[*ExprTypeSynPtr] == '('))
    {
      // convert an array into a pointer to the array,
      // convert a function into a pointer to the function,
      // remove the reference
      del(oldIdxRight + 1 - (oldSpRight - sp), 1);
    }
    else if (*ExprTypeSynPtr >= 0 &&
             stack[oldIdxRight - (oldSpRight - sp)][0] == tokUnaryStar)
    {
      // it's an lvalue (with implicit or explicit dereference),
      // convert it into its address by
      // collapsing/removing the reference and the dereference
      del(oldIdxRight - (oldSpRight - sp), 2);
    }
    else
      //error("exprval(): lvalue expected after '&'\n");
      errorNotLvalue();

    // we cannot insert another '*' into the type to make it a pointer
    // to an array/function/etc, so make the index into the type negative
    *ExprTypeSynPtr = -*ExprTypeSynPtr;

    *ConstExpr = 0;
    break;

  // Indirection unary operator
  case tokUnaryStar:
    exprval(idx, ExprTypeSynPtr, ConstExpr);

    if (*ExprTypeSynPtr < 0 || SyntaxStack0[*ExprTypeSynPtr] == '*')
    {
      // type is a pointer to something,
      // transform it into that something
      if (*ExprTypeSynPtr < 0)
        *ExprTypeSynPtr = -*ExprTypeSynPtr;
      else
        ++*ExprTypeSynPtr;
      nonVoidTypeCheck(*ExprTypeSynPtr);
      if (SyntaxStack0[*ExprTypeSynPtr] == tokStructPtr && !GetDeclSize(*ExprTypeSynPtr, 0))
        // incomplete structure/union type
        errorOpType();
      // remove the dereference if that something is an array or a function
      if (SyntaxStack0[*ExprTypeSynPtr] == '[' ||
          SyntaxStack0[*ExprTypeSynPtr] == '(')
        del(oldIdxRight + 1 - (oldSpRight - sp), 1);
      // else attach dereference size in bytes
      else
        stack[oldIdxRight + 1 - (oldSpRight - sp)][1] = GetDeclSize(*ExprTypeSynPtr, 1);
    }
    else if (SyntaxStack0[*ExprTypeSynPtr] == '[')
    {
      // type is an array,
      // transform it into the array's first element
      // (a subarray, if type is a multidimensional array)
      (*ExprTypeSynPtr) += 3;
      // remove the dereference if that element is an array
      if (SyntaxStack0[*ExprTypeSynPtr] == '[')
        del(oldIdxRight + 1 - (oldSpRight - sp), 1);
      // else attach dereference size in bytes
      else
        stack[oldIdxRight + 1 - (oldSpRight - sp)][1] = GetDeclSize(*ExprTypeSynPtr, 1);
    }
    else
      //error("exprval(): pointer/array expected after '*' / before '[]'\n");
      errorOpType();

    *ConstExpr = 0;
    break;

  // Additive binary operators
  case '+':
  case '-':
  // WRONGISH: DONE: replace prefix ++/-- with +=1/-=1
  // WRONG: DONE: replace postfix ++/-- with (+=1)-1/(-=1)+1
    {
      word ptrmask;

      word oldIdxLeft, oldSpLeft;
      word sl, sr;
      word incSize;
      sr = exprval(idx, &RightExprTypeSynPtr, &constExpr[1]);
      oldIdxLeft = *idx;
      oldSpLeft = sp;
      sl = exprval(idx, ExprTypeSynPtr, &constExpr[0]);

      // Decay arrays to pointers to their first elements
      // and ensure that the pointers are suitable for pointer arithmetic
      // (not pointers to functions, sizes of pointees are known and non-zero)
      decayArray(&RightExprTypeSynPtr, 1);
      decayArray(ExprTypeSynPtr, 1);

      // Bar void and struct/union
      scalarTypeCheck(RightExprTypeSynPtr);
      scalarTypeCheck(*ExprTypeSynPtr);

      ptrmask = isAnyPtr(RightExprTypeSynPtr) + isAnyPtr(*ExprTypeSynPtr) * 2;


      // DONE: index/subscript scaling
      if (ptrmask == 1 && tok == '+') // pointer in right-hand expression
      {
        incSize = GetDeclSize(derefAnyPtr(RightExprTypeSynPtr), 0);

        if (constExpr[0]) // integer constant in left-hand expression
        {
          s = (word)((unsigned)sl * incSize);
          stack[oldIdxLeft - (oldSpLeft - sp)][1] = s;
          // optimize a little if possible
          {
            word i = oldIdxRight - (oldSpRight - sp);
            // Skip any type cast markers
            while (stack[i][0] == tokUnaryPlus || stack[i][0] == '+')
              i--;
            // See if the pointer is an integer constant or a local variable offset
            // and if it is, adjust it here instead of generating code for
            // addition/subtraction
            if (stack[i][0] == tokNumInt || stack[i][0] == tokNumUint || stack[i][0] == tokLocalOfs)
            {
              s = (word)((unsigned)stack[i][1] + s);
              stack[i][1] = s; // TBD!!! need extra truncation?
              del(oldIdxLeft - (oldSpLeft - sp), 1);
              del(oldIdxRight - (oldSpRight - sp) + 1, 1);
            }
          }
        }
        else if (incSize != 1)
        {
          ins2(oldIdxLeft + 1 - (oldSpLeft - sp), tokNumInt, incSize);
          ins(oldIdxLeft + 1 - (oldSpLeft - sp), '*');
        }

        *ExprTypeSynPtr = RightExprTypeSynPtr;
      }
      else if (ptrmask == 2) // pointer in left-hand expression
      {
        incSize = GetDeclSize(derefAnyPtr(*ExprTypeSynPtr), 0);
        if (constExpr[1]) // integer constant in right-hand expression
        {
          s = (word)((unsigned)sr * incSize);
          stack[oldIdxRight - (oldSpRight - sp)][1] = s;
          // optimize a little if possible
          {
            word i = oldIdxLeft - (oldSpLeft - sp);
            // Skip any type cast markers
            while (stack[i][0] == tokUnaryPlus || stack[i][0] == '+')
              i--;
            // See if the pointer is an integer constant or a local variable offset
            // and if it is, adjust it here instead of generating code for
            // addition/subtraction
            if (stack[i][0] == tokNumInt || stack[i][0] == tokNumUint || stack[i][0] == tokLocalOfs)
            {
              if (tok == '-')
                s = (word)~(s - 1u);
              s = (word)((unsigned)stack[i][1] + s);
              stack[i][1] = s; // TBD!!! need extra truncation?
              del(oldIdxRight - (oldSpRight - sp), 2);
            }
          }
        }
        else if (incSize != 1)
        {
          ins2(oldIdxRight + 1 - (oldSpRight - sp), tokNumInt, incSize);
          ins(oldIdxRight + 1 - (oldSpRight - sp), '*');
        }
      }
      else if (ptrmask == 3 && tok == '-') // pointers in both expressions
      {
        incSize = GetDeclSize(derefAnyPtr(*ExprTypeSynPtr), 0);
        // TBD!!! "ptr1-ptr2": better pointer type compatibility test needed, like compatCheck()?
        if (incSize != GetDeclSize(derefAnyPtr(RightExprTypeSynPtr), 0))
          //error("exprval(): incompatible pointers\n");
          errorOpType();
        if (incSize != 1)
        {
          ins2(oldIdxRight + 2 - (oldSpRight - sp), tokNumInt, incSize);
          ins(oldIdxRight + 2 - (oldSpRight - sp), '/');
        }
        *ExprTypeSynPtr = SymIntSynPtr;
      }
      else if (ptrmask)
        //error("exprval(): invalid combination of operands for '+' or '-'\n");
        errorOpType();

      // Promote the result from char to int (and from int to unsigned) if necessary
      promoteType(ExprTypeSynPtr, &RightExprTypeSynPtr);

      *ConstExpr = constExpr[0] && constExpr[1];


      {
        if (tok == '+')
          s = (word)((unsigned)sl + sr);
        else
          s = (word)((unsigned)sl - sr);
      }

      simplifyConstExpr(s, *ConstExpr, ExprTypeSynPtr, oldIdxRight + 1 - (oldSpRight - sp), *idx + 1);
    }
    break;

  // Prefix/postfix increment/decrement unary operators
  case tokInc:
  case tokDec:
  case tokPostInc:
  case tokPostDec:
    {
      word incSize = 1;
      word inc = tok == tokInc || tok == tokPostInc;
      word post = tok == tokPostInc || tok == tokPostDec;
      word opSize;

      exprval(idx, ExprTypeSynPtr, ConstExpr);

      lvalueCheck(*ExprTypeSynPtr, oldIdxRight - (oldSpRight - sp));

      // if it's a pointer, ensure that it's suitable for pointer arithmetic
      // (not pointer to function, pointee size is known and non-zero)
      decayArray(ExprTypeSynPtr, 1); // no actual decay here, just a type check

      // Bar void and struct/union
      scalarTypeCheck(*ExprTypeSynPtr);



      // "remove" the lvalue dereference as we don't need
      // to read the value while forgetting its location.
      // We need to keep the lvalue location.
      // Remember the operand size.
      opSize = stack[oldIdxRight - (oldSpRight - sp)][1];
      del(oldIdxRight - (oldSpRight - sp), 1);

      if (isAnyPtr(*ExprTypeSynPtr))
        incSize = GetDeclSize(derefAnyPtr(*ExprTypeSynPtr), 0);

      if (incSize == 1)
      {
        // store the operand size in the operator
        stack[oldIdxRight + 1 - (oldSpRight - sp)][1] = opSize;
      }
      else
      {
        // replace ++/-- with "postfix" +=/-= incSize when incSize != 1
        stack[oldIdxRight + 1 - (oldSpRight - sp)][0] =
          inc ? (post ? tokPostAdd : tokAssignAdd) :
                (post ? tokPostSub : tokAssignSub);
        // store the operand size in the operator
        stack[oldIdxRight + 1 - (oldSpRight - sp)][1] = opSize;
        ins2(oldIdxRight + 1 - (oldSpRight - sp), tokNumInt, incSize);
      }

      *ConstExpr = 0;
    }
    break;

  // Simple assignment binary operator
  case '=':
    {
      word oldIdxLeft, oldSpLeft;
      word opSize;
      word structs;

        exprval(idx, &RightExprTypeSynPtr, &constExpr[1]);
      oldIdxLeft = *idx;
      oldSpLeft = sp;
      exprval(idx, ExprTypeSynPtr, &constExpr[0]);

      lvalueCheck(*ExprTypeSynPtr, oldIdxLeft - (oldSpLeft - sp));

      nonVoidTypeCheck(RightExprTypeSynPtr);
      nonVoidTypeCheck(*ExprTypeSynPtr);

      structs = (*ExprTypeSynPtr >= 0 && SyntaxStack0[*ExprTypeSynPtr] == tokStructPtr) * 2 +
                (RightExprTypeSynPtr >= 0 && SyntaxStack0[RightExprTypeSynPtr] == tokStructPtr);


      if (structs)
      {
        word sz;

        if (structs != 3 ||
            SyntaxStack1[RightExprTypeSynPtr] != SyntaxStack1[*ExprTypeSynPtr])
          errorOpType();

        // TBD??? (a = b) should be an rvalue and so &(a = b) and (&(a = b))->c shouldn't be
        // allowed, while (a = b).c should be allowed.

        // transform "*psleft = *psright" into "*fxn(sizeof *psright, psright, psleft)"
/*
        if (stack[oldIdxLeft - (oldSpLeft - sp)][0] != tokUnaryStar ||
            stack[oldIdxRight - (oldSpRight - sp)][0] != tokUnaryStar)
          errorInternal(18);
*/
        stack[oldIdxLeft - (oldSpLeft - sp)][0] = ','; // replace '*' with ','
        stack[oldIdxRight - (oldSpRight - sp)][0] = ','; // replace '*' with ','

        sz = GetDeclSize(RightExprTypeSynPtr, 0);

        stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokNumUint; // replace '=' with "sizeof *psright"
        stack[oldIdxRight + 1 - (oldSpRight - sp)][1] = sz;

        ins(oldIdxRight + 2 - (oldSpRight - sp), ',');

        if (!StructCpyLabel)
          StructCpyLabel = LabelCnt++;
        ins2(oldIdxRight + 2 - (oldSpRight - sp), tokIdent, AddNumericIdent(StructCpyLabel));

        ins2(oldIdxRight + 2 - (oldSpRight - sp), ')', SizeOfWord * 3);
        ins2(oldIdxRight + 2 - (oldSpRight - sp), tokUnaryStar, 0); // use 0 deref size to drop meaningless dereferences

        ins2(*idx + 1, '(', SizeOfWord * 3);
      }
      else
      {

        // "remove" the lvalue dereference as we don't need
        // to read the value while forgetting its location.
        // We need to keep the lvalue location.
        // Remember the operand size.
        opSize = stack[oldIdxLeft - (oldSpLeft - sp)][1];
        // store the operand size in the operator
        stack[oldIdxRight + 1 - (oldSpRight - sp)][1] = opSize;
        del(oldIdxLeft - (oldSpLeft - sp), 1);
      }

      *ConstExpr = 0;
    }
    break;

  // DONE: other assignment operators

  // Arithmetic and bitwise unary operators
  case '~':
  case tokUnaryPlus:
  case tokUnaryMinus:
    s = exprval(idx, ExprTypeSynPtr, ConstExpr);
    numericTypeCheck(*ExprTypeSynPtr);
    promoteType(ExprTypeSynPtr, ExprTypeSynPtr);
    switch (tok)
    {
    case '~':

      s = ~s;
      break;
    case tokUnaryPlus:
      break;
    case tokUnaryMinus:

      {
        s = (word)~(s - 1u);
      }
      break;
    }
    simplifyConstExpr(s, *ConstExpr, ExprTypeSynPtr, oldIdxRight + 1 - (oldSpRight - sp), *idx + 1);
    break;

  // Arithmetic and bitwise binary operators
  case '*':
  case '/':
  case '%':
  case tokLShift:
  case tokRShift:
  case '&':
  case '^':
  case '|':
    {

      word sr, sl;
      word Unsigned;
      sr = exprval(idx, &RightExprTypeSynPtr, &constExpr[1]);

      sl = exprval(idx, ExprTypeSynPtr, &constExpr[0]);

      numericTypeCheck(RightExprTypeSynPtr);
      numericTypeCheck(*ExprTypeSynPtr);


      *ConstExpr = constExpr[0] && constExpr[1];

      Unsigned = SyntaxStack0[*ExprTypeSynPtr] == tokUnsigned || SyntaxStack0[RightExprTypeSynPtr] == tokUnsigned;


      {
        switch (tok)
        {
        // DONE: check for division overflows
        case '/':
        case '%':
          *ConstExpr &= divCheckAndCalc(tok, &sl, sr, Unsigned, constExpr);

          if (Unsigned)
          {
            if (tok == '/')
              stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokUDiv;
            else
              stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokUMod;
          }
          break;

        case '*':
          sl = (word)((unsigned)sl * sr);
          break;

        case tokLShift:
        case tokRShift:
          if (constExpr[1])
          {
            if (SyntaxStack0[RightExprTypeSynPtr] != tokUnsigned)
              sr = truncInt(sr);
            else
              sr = (word)truncUint(sr);
            shiftCountCheck(&sr, oldIdxRight - (oldSpRight - sp), RightExprTypeSynPtr);
          }
          if (*ConstExpr)
          {
            if (tok == tokLShift)
            {
              // left shift is the same for signed and unsigned ints
              sl = (word)((unsigned)sl << sr);
            }
            else
            {
              if (SyntaxStack0[*ExprTypeSynPtr] == tokUnsigned)
              {
                // right shift for unsigned ints
                sl = (word)(truncUint(sl) >> sr);
              }
              else if (sr)
              {
                // right shift for signed ints is arithmetic, sign-bit-preserving
                // don't depend on the compiler's implementation, do it "manually"
                sl = truncInt(sl);
                sl = (word)((truncUint(sl) >> sr) |
                           ((sl < 0) * (~0u << (8 * SizeOfWord - sr))));
              }
            }
          }

          if (SyntaxStack0[*ExprTypeSynPtr] == tokUnsigned && tok == tokRShift)
            stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokURShift;

          // ignore RightExprTypeSynPtr for the purpose of promotion/conversion of the result of <</>>
          RightExprTypeSynPtr = SymIntSynPtr;
          break;

        case '&': sl &= sr; break;
        case '^': sl ^= sr; break;
        case '|': sl |= sr; break;
        }
        s = sl;
      }
      promoteType(ExprTypeSynPtr, &RightExprTypeSynPtr);
      simplifyConstExpr(s, *ConstExpr, ExprTypeSynPtr, oldIdxRight + 1 - (oldSpRight - sp), *idx + 1);
    }
    break;

  // Relational and equality binary operators
  // DONE: add (sub)tokens for unsigned >, >=, <, <= for pointers
  case '<':
  case '>':
  case tokLEQ:
  case tokGEQ:
  case tokEQ:
  case tokNEQ:
    {
      word ptrmask;

      word sr, sl;
      sr = exprval(idx, &RightExprTypeSynPtr, &constExpr[1]);

      sl = exprval(idx, ExprTypeSynPtr, &constExpr[0]);

      // Bar void and struct/union
      scalarTypeCheck(RightExprTypeSynPtr);
      scalarTypeCheck(*ExprTypeSynPtr);

      ptrmask = isAnyPtr(RightExprTypeSynPtr) + isAnyPtr(*ExprTypeSynPtr) * 2;


      // TBD??? stricter type checks???
      if (tok != tokEQ && tok != tokNEQ)
      {
        // Disallow >, <, >=, <= between a pointer and a number
        if (ptrmask == 1 || ptrmask == 2)
          //error("exprval(): Invalid/unsupported combination of compared operands\n");
          errorOpType();
        // Disallow >, <, >=, <= with pointers to functions
        if (((ptrmask & 1) && SyntaxStack0[derefAnyPtr(RightExprTypeSynPtr)] == '(') ||
            ((ptrmask & 2) && SyntaxStack0[derefAnyPtr(*ExprTypeSynPtr)] == '('))
          errorOpType();
      }
      else
      {
        // Disallow == and != between a pointer and a number other than constant 0 (AKA NULL)
        if ((ptrmask == 1 && !(constExpr[0] && !truncInt(sl))) ||
            (ptrmask == 2 && !(constExpr[1] && !truncInt(sr))))
          errorOpType();
      }

      *ConstExpr = constExpr[0] && constExpr[1];


      {
        word Unsigned = isUint(*ExprTypeSynPtr) || isUint(RightExprTypeSynPtr);

        if (*ConstExpr)
        {
          if (!Unsigned)
          {
            sl = truncInt(sl);
            sr = truncInt(sr);
            switch (tok)
            {
            case '<':    sl = sl <  sr; break;
            case '>':    sl = sl >  sr; break;
            case tokLEQ: sl = sl <= sr; break;
            case tokGEQ: sl = sl >= sr; break;
            case tokEQ:  sl = sl == sr; break;
            case tokNEQ: sl = sl != sr; break;
            }
          }
          else
          {
            sl = (word)truncUint(sl);
            sr = (word)truncUint(sr);
            switch (tok)
            {
            case '<':    sl = (unsigned)sl <  (unsigned)sr; break;
            case '>':    sl = (unsigned)sl >  (unsigned)sr; break;
            case tokLEQ: sl = (unsigned)sl <= (unsigned)sr; break;
            case tokGEQ: sl = (unsigned)sl >= (unsigned)sr; break;
            case tokEQ:  sl = sl == sr; break;
            case tokNEQ: sl = sl != sr; break;
            }
          }
        }

        if (ptrmask || Unsigned)
        {
          // Pointer comparison should be unsigned
          word t = tok;
          switch (tok)
          {
          case '<': t = tokULess; break;
          case '>': t = tokUGreater; break;
          case tokLEQ: t = tokULEQ; break;
          case tokGEQ: t = tokUGEQ; break;
          }
          stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = t;
        }
      }

      s = sl;
      *ExprTypeSynPtr = SymIntSynPtr;
      simplifyConstExpr(s, *ConstExpr, ExprTypeSynPtr, oldIdxRight + 1 - (oldSpRight - sp), *idx + 1);
    }
    break;

  // implicit pseudo-conversion to _Bool of operands of && and ||
  case tok_Bool:
    s = exprval(idx, ExprTypeSynPtr, ConstExpr);
    // Bar void and struct/union
    scalarTypeCheck(*ExprTypeSynPtr);

    {
      s = truncInt(s) != 0;
    }
    *ExprTypeSynPtr = SymIntSynPtr;
    simplifyConstExpr(s, *ConstExpr, ExprTypeSynPtr, oldIdxRight + 1 - (oldSpRight - sp), *idx + 1);
    break;

  // Logical binary operators
  case tokLogAnd: // DONE: short-circuit
  case tokLogOr: // DONE: short-circuit
    {
      word sr, sl;

      // DONE: think of pushing a special short-circuit (jump-to) token
      // to skip the rhs operand evaluation in && and ||
      // DONE: add implicit "casts to _Bool" of && and || operands,
      // do the same for control statements of if() while() and for(;;).

      word sc = LabelCnt++;
      // tag the logical operator as a numbered short-circuit jump target
      stack[*idx + 1][1] = sc;

      // insert "!= 0" for right-hand operand
      switch (stack[*idx][0])
      {
      case '<':
      case tokULess:
      case '>':
      case tokUGreater:
      case tokLEQ:
      case tokULEQ:
      case tokGEQ:
      case tokUGEQ:
      case tokEQ:
      case tokNEQ:
        break;
      default:
        ins(++*idx, tok_Bool);
        break;
      }

      sr = exprval(idx, &RightExprTypeSynPtr, &constExpr[1]);

      // insert a reference to the short-circuit jump target
      if (tok == tokLogAnd)
        ins2(++*idx, tokShortCirc, sc);
      else
        ins2(++*idx, tokShortCirc, -sc);
      // insert "!= 0" for left-hand operand
      switch (stack[*idx - 1][0])
      {
      case '<':
      case tokULess:
      case '>':
      case tokUGreater:
      case tokLEQ:
      case tokULEQ:
      case tokGEQ:
      case tokUGEQ:
      case tokEQ:
      case tokNEQ:
        --*idx;
        break;
      default:
        ins(*idx, tok_Bool);
        break;
      }

      sl = exprval(idx, ExprTypeSynPtr, &constExpr[0]);

      if (tok == tokLogAnd)
        s = sl && sr;
      else
        s = sl || sr;

      *ExprTypeSynPtr = SymIntSynPtr;
      *ConstExpr = constExpr[0] && constExpr[1];
      if (constExpr[0])
      {
        if (tok == tokLogAnd)
        {
          if (!sl)
            *ConstExpr = 1, s = 0;
          // TBD??? else can drop LHS expression
        }
        else
        {
          if (sl)
            *ConstExpr = s = 1;
          // TBD??? else can drop LHS expression
        }
      }
      simplifyConstExpr(s, *ConstExpr, ExprTypeSynPtr, oldIdxRight + 1 - (oldSpRight - sp), *idx + 1);
    }
    break;

  // Function call
  case ')':
    {
      word tmpSynPtr, c;
      word minParams, maxParams;
      word firstParamSynPtr;
      word oldIdx, oldSp;

      exprval(idx, ExprTypeSynPtr, ConstExpr);

      if (!GetFxnInfo(*ExprTypeSynPtr, &minParams, &maxParams, ExprTypeSynPtr, &firstParamSynPtr))
        //error("exprval(): function or function pointer expected\n");
        errorOpType();

      // DONE: validate the number of function arguments
      // DONE: warnings on int<->pointer substitution in params/args



      // evaluate function arguments
      c = 0;
      while (stack[*idx][0] != '(')
      {

        word ptrmask;


        // add a comma after the first (last to be pushed) argument,
        // so all arguments can be pushed whenever a comma is encountered
        if (!c)
          ins(*idx + 1, ',');

        oldIdx = *idx;
        oldSp = sp;
        (void)oldIdx;
        (void)oldSp;

        exprval(idx, &tmpSynPtr, ConstExpr);
        //error("exprval(): function arguments cannot be of type 'void'\n");

        if (c >= maxParams)
          error("Too many function arguments\n");

        // Find the type of the formal parameter in the function declaration
        if (c < minParams)
        {
          word t;
          while ((t = SyntaxStack0[firstParamSynPtr]) != tokIdent)
          {
            if (t == '(')
            {
              // skip parameters in parameters
              word c = 1;
              while (c)
              {
                t = SyntaxStack0[++firstParamSynPtr];
                c += (t == '(') - (t == ')');
              }
            }
            firstParamSynPtr++;
          }
          firstParamSynPtr++;
        }
        else
        {
          firstParamSynPtr = SymVoidSynPtr;
        }

        ptrmask = isAnyPtr(firstParamSynPtr) * 2 + isAnyPtr(tmpSynPtr);
        (void)ptrmask;

        // Bar void and struct/union
        scalarTypeCheck(tmpSynPtr);

        // if there's a formal parameter for this argument, check the types
        if (c < minParams)
        {



        }





        c++;

        if (stack[*idx][0] == ',')
          --*idx;
      }
      --*idx;

      if (c < minParams)
        error("Too few function arguments\n");

      // store the cumulative argument size in the function call operators
      {
        word i = oldIdxRight + 1 - (oldSpRight - sp);

        stack[1 + *idx][1] = stack[i][1] = c * SizeOfWord;

      }

      *ConstExpr = 0;
    }
    break;

  // Binary comma operator
  case tokComma:
    {
      word oldIdxLeft, oldSpLeft;
      word retStruct = 0;
      s = exprval(idx, &RightExprTypeSynPtr, &constExpr[1]);
      oldIdxLeft = *idx;
      oldSpLeft = sp;

      // Signify uselessness of the result of the left operand's value
      ins(*idx + 1, tokVoid);

      exprval(idx, ExprTypeSynPtr, &constExpr[0]);
      *ConstExpr = constExpr[0] && constExpr[1];
      *ExprTypeSynPtr = RightExprTypeSynPtr;
      retStruct = RightExprTypeSynPtr >= 0 && SyntaxStack0[RightExprTypeSynPtr] == tokStructPtr;

      if (*ConstExpr)
      {
        // both subexprs are const, remove both and comma
        simplifyConstExpr(s, *ConstExpr, ExprTypeSynPtr, oldIdxRight + 1 - (oldSpRight - sp), *idx + 1);
      }
      else if (constExpr[0])
      {
        // only left subexpr is const, remove it
        del(*idx + 1, oldIdxLeft - (oldSpLeft - sp) - *idx);

        if (!retStruct)
          // Ensure non-lvalue-ness of the result by changing comma to unary plus
          // and thus hiding dereference, if any
          stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokUnaryPlus;
        else
          // However, (something, struct).member should still be allowed,
          // so, comma needs to produce lvalue
          del(oldIdxRight + 1 - (oldSpRight - sp), 1);
      }
      else if (retStruct)
      {
        // However, (something, struct).member should still be allowed,
        // so, comma needs to produce lvalue. Swap comma and structure dereference.
        word i = oldIdxRight + 1 - (oldSpRight - sp);
        stack[i][0] = tokUnaryStar;
        stack[i][1] = stack[i - 1][1];
        stack[i - 1][0] = tokComma;
      }
    }
    break;

  // Compound assignment operators
  case tokAssignMul: case tokAssignDiv: case tokAssignMod:
  case tokAssignAdd: case tokAssignSub:
  case tokAssignLSh: case tokAssignRSh:
  case tokAssignAnd: case tokAssignXor: case tokAssignOr:
    {
      word ptrmask;
      word oldIdxLeft, oldSpLeft;
      word incSize;
      word opSize;
      word Unsigned;
      word sr = exprval(idx, &RightExprTypeSynPtr, &constExpr[1]);
      oldIdxLeft = *idx;
      oldSpLeft = sp;
      exprval(idx, ExprTypeSynPtr, &constExpr[0]);

      lvalueCheck(*ExprTypeSynPtr, oldIdxLeft - (oldSpLeft - sp));

      // if it's a pointer, ensure that it's suitable for pointer arithmetic
      // (not pointer to function, pointee size is known and non-zero)
      decayArray(ExprTypeSynPtr, 1); // no actual decay here, just a type check

      // Bar void and struct/union
      scalarTypeCheck(RightExprTypeSynPtr);
      scalarTypeCheck(*ExprTypeSynPtr);



      // "remove" the lvalue dereference as we don't need
      // to read the value while forgetting its location.
      // We need to keep the lvalue location.
      // Remember the operand size.
      opSize = stack[oldIdxLeft - (oldSpLeft - sp)][1];
      // store the operand size in the operator
      stack[oldIdxRight + 1 - (oldSpRight - sp)][1] = opSize;
      del(oldIdxLeft - (oldSpLeft - sp), 1);

      ptrmask = isAnyPtr(*ExprTypeSynPtr) * 2 + isAnyPtr(RightExprTypeSynPtr);

      Unsigned = isUint(*ExprTypeSynPtr) * 2 + isUint(RightExprTypeSynPtr);

      if (tok != tokAssignAdd && tok != tokAssignSub)
      {
        if (ptrmask)
          //error("exprval(): invalid combination of operands for %s\n", GetTokenName(tok));
          errorOpType();
      }
      else
      {
        // No pointer to the right of += and -=
        if (ptrmask & 1)
          //error("exprval(): invalid combination of operands for %s\n", GetTokenName(tok));
          errorOpType();
      }

      if (tok == tokAssignLSh || tok == tokAssignRSh)
      {
        if (constExpr[1])
        {
          if (Unsigned & 1)
            sr = (word)truncUint(sr);
          else
            sr = truncInt(sr);
          shiftCountCheck(&sr, oldIdxRight - (oldSpRight - sp), RightExprTypeSynPtr);
        }
      }

      if (tok == tokAssignDiv || tok == tokAssignMod)
      {
        word t, sl = 0;
        if (tok == tokAssignDiv)
          t = '/';
        else
          t = '%';
        divCheckAndCalc(t, &sl, sr, 1, constExpr);
      }

      // TBD??? replace +=/-= with prefix ++/-- if incSize == 1
      if (ptrmask == 2) // left-hand expression
      {
        incSize = GetDeclSize(derefAnyPtr(*ExprTypeSynPtr), 0);
        if (constExpr[1])
        {
          word t = (word)(stack[oldIdxRight - (oldSpRight - sp)][1] * (unsigned)incSize);
          stack[oldIdxRight - (oldSpRight - sp)][1] = t;
        }
        else if (incSize != 1)
        {
          ins2(oldIdxRight + 1 - (oldSpRight - sp), tokNumInt, incSize);
          ins(oldIdxRight + 1 - (oldSpRight - sp), '*');
        }
      }
      else if (Unsigned)
      {
        word t = tok;
        switch (tok)
        {
        case tokAssignDiv: t = tokAssignUDiv; break;
        case tokAssignMod: t = tokAssignUMod; break;
        case tokAssignRSh:
          if (Unsigned & 2)
            t = tokAssignURSh;
          break;
        }
        stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = t;
      }

      *ConstExpr = 0;
    }
    break;

  // Ternary/conditional operator
  case '?':
    {
      word oldIdxLeft, oldSpLeft;
      word oldIdxCond, oldSpCond;
      word sr, sl, smid;
      word condTypeSynPtr;
      word sc = (LabelCnt += 2) - 2;
      word structs;

      // "exprL ? exprMID : exprR" appears on the stack as
      // "exprL exprR exprMID ?"

      // label at the end of ?:
      stack[*idx + 1][0] = tokLogAnd; // piggyback on && for CG (ugly, but simple)
      stack[*idx + 1][1] = sc + 1;

      smid = exprval(idx, ExprTypeSynPtr, &constExpr[1]);

      oldIdxLeft = *idx;
      oldSpLeft = sp;
      sr = exprval(idx, &RightExprTypeSynPtr, &constExpr[2]);

      decayArray(&RightExprTypeSynPtr, 0);
      decayArray(ExprTypeSynPtr, 0);
      promoteType(&RightExprTypeSynPtr, ExprTypeSynPtr);
      promoteType(ExprTypeSynPtr, &RightExprTypeSynPtr);

      structs = (*ExprTypeSynPtr >= 0 && SyntaxStack0[*ExprTypeSynPtr] == tokStructPtr) * 2 +
                (RightExprTypeSynPtr >= 0 && SyntaxStack0[RightExprTypeSynPtr] == tokStructPtr);


      // TBD??? move struct/union-related checks into compatChecks()

      if (structs)
      {
        if (structs != 3 ||
            SyntaxStack1[RightExprTypeSynPtr] != SyntaxStack1[*ExprTypeSynPtr])
          errorOpType();

        // transform "cond ? a : b" into "*(cond ? &a : &b)"
/*
        if (stack[oldIdxLeft - (oldSpLeft - sp)][0] != tokUnaryStar ||
            stack[oldIdxRight - (oldSpRight - sp)][0] != tokUnaryStar)
          errorInternal(19);
*/
        del(oldIdxLeft - (oldSpLeft - sp), 1); // delete '*'
        del(oldIdxRight - (oldSpRight - sp), 1); // delete '*'
        oldSpLeft--;
        // '*' will be inserted at the end
      }
      else
      {
        compatCheck(ExprTypeSynPtr,
                    RightExprTypeSynPtr,
                    &constExpr[1],
                    oldIdxRight - (oldSpRight - sp),
                    oldIdxLeft - (oldSpLeft - sp));
      }

      // label at the start of exprMID
      ins2(oldIdxLeft + 1 - (oldSpLeft - sp), tokLogAnd, sc); // piggyback on && for CG (ugly, but simple)
      // jump from the end of exprR over exprMID to the end of ?:
      ins2(oldIdxLeft - (oldSpLeft - sp), tokGoto, sc + 1);
      // jump to exprMID if exprL is non-zero
      ins2(*idx + 1, tokShortCirc, -sc);

      oldIdxCond = *idx;
      oldSpCond = sp;
      sl = exprval(idx, &condTypeSynPtr, &constExpr[0]);

      // Bar void and struct/union
      scalarTypeCheck(condTypeSynPtr);



      *ConstExpr = s = 0;

      if (constExpr[0])
      {
        word c1 = 0, c2 = 0;
        // Stack now: exprL tokShortCirc exprR tokGoto tokLogAnd exprMID ?/tokLogAnd
        if (

              (truncUint(sl) != 0))
        {
          if (constExpr[1])
          {
            *ConstExpr = 1, s = smid;
          }
          else
          {
            // Drop exprL and exprR subexpressions
            c1 = oldIdxLeft - (oldSpLeft - sp) - *idx; // includes tokShortCirc, tokGoto, tokLogAnd
            c2 = 1; // include '?'/tokLogAnd
          }
        }
        else
        {
          if (constExpr[2])
          {
            *ConstExpr = 1, s = sr;
          }
          else
          {
            // Drop exprL and exprMID subexpressions
            c1 = oldIdxCond - (oldSpCond - sp) - *idx + 1; // includes tokShortCirc
            c2 = (oldIdxRight - (oldSpRight - sp)) -
                 (oldIdxLeft - (oldSpLeft - sp)) + 3; // includes tokGoto, tokLogAnd, '?'/tokLogAnd
          }
        }
        if (c1)
        {
          word pos = oldIdxRight - (oldSpRight - sp) + 2 - c2;
          if (!structs && stack[pos - 1][0] == tokUnaryStar)
            stack[pos++][0] = tokUnaryPlus, c2--; // ensure non-lvalue-ness by hiding the dereference
          del(pos, c2);
          del(*idx + 1, c1);
        }
      }
      // finish transforming "cond ? a : b" into "*(cond ? &a : &b)", insert '*'
      if (structs)
        ins2(oldIdxRight + 2 - (oldSpRight - sp), tokUnaryStar, 0); // use 0 deref size to drop meaningless dereferences
      simplifyConstExpr(s, *ConstExpr, ExprTypeSynPtr, oldIdxRight + 1 - (oldSpRight - sp), *idx + 1);
    }
    break;

  // Postfix indirect structure/union member selection operator
  case tokArrow:
    {
      word member, i = 0, j = 0, c = 1, ofs = 0;

      stack[*idx + 1][0] = '+'; // replace -> with +
      member = stack[*idx][1]; // keep the member name, it will be replaced with member offset
      stack[*idx][0] = tokNumInt;

      --*idx;
      exprval(idx, ExprTypeSynPtr, ConstExpr);

      if (!isAnyPtr(*ExprTypeSynPtr) ||
          SyntaxStack0[i = derefAnyPtr(*ExprTypeSynPtr)] != tokStructPtr)
        error("Pointer to or structure or union expected\n");

      i = SyntaxStack1[i];
      if (i + 2 > SyntaxStackCnt ||
          (SyntaxStack0[i] != tokStruct && SyntaxStack0[i] != tokUnion) ||
          SyntaxStack0[i + 1] != tokTag)
        errorInternal(20);

      if (!GetDeclSize(i, 0))
        // incomplete structure/union type
        errorOpType();

      i += 5; // step inside the {} body of the struct/union
      while (c)
      {
        word t = SyntaxStack0[i];
        c += (t == '(') - (t == ')') + (t == '{') - (t == '}');
        if (c == 1 &&
            t == tokMemberIdent && SyntaxStack1[i] == member &&
            SyntaxStack0[i + 1] == tokLocalOfs)
        {
          j = i;
          ofs = SyntaxStack1[i + 1];
          break;
        }
        i++;
      }
      if (!j)
      {
        printf("Undefined structure or union member ");
        error(IdentTable + member);
      }

      j += 2;
      // we cannot insert another '*' into the type to make it a pointer,
      // so make the index into the type negative
      *ExprTypeSynPtr = -j; // type: pointer to member's type

      stack[oldIdxRight - (oldSpRight - sp)][1] = ofs; // member offset within structure/union

      // optimize a little, if possible
      {
        word i = oldIdxRight - (oldSpRight - sp) - 1;
        // Skip any type cast markers
        while (stack[i][0] == tokUnaryPlus)
          i--;
        // See if the pointer is an integer constant or a local variable offset
        // and if it is, adjust it here instead of generating code for
        // addition/subtraction
        if (stack[i][0] == tokNumInt || stack[i][0] == tokNumUint || stack[i][0] == tokLocalOfs)
        {
          stack[i][1] = (word)((unsigned)stack[i][1] + ofs); // TBD!!! need extra truncation?
          del(oldIdxRight - (oldSpRight - sp), 2);
        }
      }

      *ConstExpr = 0;
    }
    break;

  default:
    //error("exprval(): Unexpected token %s\n", GetTokenName(tok));
    errorInternal(21);
  }

  return s;
}







word ParseExpr(word tok, word* GotUnary, word* ExprTypeSynPtr, word* ConstExpr, word* ConstVal, word option, word option2)
{
  word identFirst = tok == tokIdent;

  *ConstVal = *ConstExpr = 0;
  *ExprTypeSynPtr = SymVoidSynPtr;

  if (!ExprLevel++)
  {
    opsp = sp = 0;
  }

  if (option == '=')
    push2(tokIdent, option2);

  tok = expr(tok, GotUnary, option == ',' || option == '=');

  if (tok == tokEof || strchr(",;:)]}", tok) == NULL)
    //error("ParseExpr(): Unexpected token %s\n", GetTokenName(tok));
    errorUnexpectedToken(tok);

  if (option == '=')
  {
    push('=');
  }
  else if (option == tokGotoLabel && identFirst && tok == ':' && *GotUnary && sp == 1 && stack[sp - 1][0] == tokIdent)
  {
    // This is a label.
    ExprLevel--;
    return tokGotoLabel;
  }

  if (*GotUnary)
  {
    word j;
    // Do this twice so we can see the stack before
    // and after manipulations
    for (j = 0; j < 2; j++)
    {
      if (doAnnotations)
      {
        word i;
        GenStartCommentLine();
        if (j) printf2("Expanded");
        else printf2("RPN'ized");
        printf2(" expression: \"");
        for (i = 0; i < sp; i++)
        {
          word tok = stack[i][0];
          switch (tok)
          {
          case tokNumInt:
            printd2(truncInt(stack[i][1]));
            break;
          case tokNumUint:
            printd2(truncUint(stack[i][1]));
            break;

          case tokIdent:
            {
              char* p = IdentTable + stack[i][1];
              if (isdigit(*p))
                printf2("L");
              printf2(p);
            }
            break;
          case tokShortCirc:
            if (stack[i][1] >= 0)
            {
              printf2("[sh&&->");
              printd2(stack[i][1]);
              printf2("]");
            }
            else
            {
              printf2("[sh||->");
              printd2(-stack[i][1]);
              printf2("]");
            }
            break;
          case tokLocalOfs:
            printf2("(@");
            printd2(truncInt(stack[i][1]));
            printf2(")");
            break;
          case tokUnaryStar:
            if (j) 
            {
              printf2("*(");
              printd2(stack[i][1]);
              printf2(")");
            }
            else printf2("*u");
            break;
          case '(': case ',':
            if (!j)
            {
              char* ctmp = " ";
              ctmp[0] = tok;
              printf2(ctmp);
            } 
              
            // else printf2("\b");
            break;
          case ')':
            if (j) printf2("(");

            char* ctmp = " ";
            ctmp[0] = tok;
            printf2(ctmp);

            if (j) printd2(stack[i][1]);
            break;
          default:
            printf2(GetTokenName(tok));
            if (j)
            {
              switch (tok)
              {
              case tokLogOr: case tokLogAnd:
                printf2("[");
                printd2(stack[i][1]);
                printf2("]");
                break;
              case '=':
              case tokInc: case tokDec:
              case tokPostInc: case tokPostDec:
              case tokAssignAdd: case tokAssignSub:
              case tokPostAdd: case tokPostSub:
              case tokAssignMul:
              case tokAssignDiv: case tokAssignMod:
              case tokAssignUDiv: case tokAssignUMod:
              case tokAssignLSh: case tokAssignRSh: case tokAssignURSh:
              case tokAssignAnd: case tokAssignXor: case tokAssignOr:
                printf2("(");
                printd2(stack[i][1]);
                printf2(")");
                break;
              }
            }
            break;
          }
          printf2(" ");
        }
        printf2("\"\n");
      }

      if (!j)
      {
        word idx = sp - 1;
        *ConstVal = exprval(&idx, ExprTypeSynPtr, ConstExpr);
        // remove the unneeded unary +'s that have served their cast-substitute purpose
        // also remove dereferences of size 0 (dereferences of pointers to structures)
        for (idx = sp - 1; idx >= 0; idx--)
          if (stack[idx][0] == tokUnaryPlus ||
              (stack[idx][0] == tokUnaryStar && !stack[idx][1]))
            del(idx, 1);
      }
      else if (*ConstExpr)
      {
        if (doAnnotations)
        {
          GenStartCommentLine();

          switch (SyntaxStack0[*ExprTypeSynPtr])
          {
          case tokChar:
          case tokSChar:
          case tokUChar:
          case tokShort:
          case tokUShort:
          case tokInt:
            printf2("Expression value: ");
            printd2(truncInt(*ConstVal));
            printf2("\n");
            break;

          default:
          case tokUnsigned:
            printf2("Expression value: ");
            printd2(truncUint(*ConstVal));
            printf2("\n");
            break;
          }
        }
      }
    }
  }

  ExprLevel--;


  return tok;
}

// smc.c code


// Equivalent to puts() but outputs to OutFile.
word puts2(char* s)
{
  word res;
  if (!OutFile)
    return 0;
  // Turbo C++ 1.01's fputs() returns EOF if s is empty, which is wrong.
  // Hence the workaround.
  fputs(OutFile, s);
  res = fputc(OutFile, '\n');
  return res;
}


// Print string to outfile
word printf2(char* sToWrite)
{
  if (!OutFile)
  {
    printf("COULD NOT WRITE TO OUT!");
    return 0;
  }

  // TODO: escape handling

  word res = fputs(OutFile, sToWrite);


  return res;
}


// Print decimal to outfile
word printd2(word dToWrite)
{
  if (!OutFile)
    return 0;

  char buf[32];

  // handle negative numbers
  if (dToWrite < 0)
  {
    fputc(OutFile, '-');
    dToWrite = -dToWrite;
  }
  itoa(dToWrite, buf);

  word res = fputs(OutFile, buf);

  return res;
}

void error(char* strToPrint)
{
  word i, fidx = FileCnt - 1 + !FileCnt;

  for (i = 0; i < FileCnt; i++)
    if (Files[i])
      fclose(Files[i]);

  /*
  puts2("");

  DumpSynDecls();
  DumpMacroTable();
  DumpIdentTable();
  */

  // using stdout implicitly instead of stderr explicitly because:
  // - stderr can be a macro and it's unknown if standard headers
  //   aren't included (which is the case when SmallerC is compiled
  //   with itself and linked with some other compiler's standard
  //   libraries)
  // - output to stderr can interfere/overlap with buffered
  //   output to stdout and the result may literally look ugly

  //GenStartCommentLine(); printf2("Compilation failed.\n");

  if (OutFile)
    fclose(OutFile);

  printf("Error in ");
  printf(FileNames[fidx]);
  printf(" at ");
  printd(LineNo);
  printf(":");
  printd(LinePos);
  printf("\n");

  printf(strToPrint);
  printf("\n");

  exit(EXIT_FAILURE);
}

void warning(char* strToPrint)
{
  word fidx = FileCnt - 1 + !FileCnt;

  warnCnt++;

  if (!warnings)
    return;

  //printf("Warning in \"%s\" (%d:%d)\n", FileNames[fidx], LineNo, LinePos);
  printf("warning.");

  printf(strToPrint);

}

void errorFile(char* n)
{
  printf("Unable to open, read, write or close file ");
  error(n);
}

void errorFileName(void)
{
  error("Invalid or too long file name or path name\n");
}

void errorInternal(word n)
{
  printd(n);
  error(" internal error\n");
}

void errorChrStr(void)
{
  error("Invalid or unsupported character constant or string literal\n");
}


void errorStrLen(void)
{
  error("String literal too long\n");
}

void errorUnexpectedToken(word tok)
{
  printf("Unexpected token ");
  error((tok == tokIdent) ? TokenIdentName : GetTokenName(tok));
}

void errorDirective(void)
{
  error("Invalid or unsupported preprocessor directive\n");
}

void errorCtrlOutOfScope(void)
{
  error("break, continue, case or default in wrong scope\n");
}

void errorDecl(void)
{
  error("Invalid or unsupported declaration\n");
}

void errorTagRedef(word ident)
{
  printf("Redefinition of type tagged ");
  printd(IdentTable + ident);
  error("\n");
}

void errorVarSize(void)
{
  error("Variable(s) take(s) too much space\n");
}

void errorInit(void)
{
  error("Invalid or unsupported initialization\n");
}

void errorUnexpectedVoid(void)
{
  error("Unexpected declaration or expression of type void\n");
}

void errorOpType(void)
{
  error("Unexpected operand type\n");
}

void errorNotLvalue(void)
{
  error("lvalue expected\n");
}

void errorNotConst(void)
{
  error("Non-constant expression\n");
}

void errorLongExpr(void)
{
  error("Expression too long\n");
}


word tsd[] =
{
  tokVoid, tokChar, tokInt,
  tokSigned, tokUnsigned, tokShort,
  tokStruct, tokUnion,
};

word TokenStartsDeclaration(word t, word params)
{
  unsigned i;

  for (i = 0; i < division(sizeof tsd, sizeof tsd[0]); i++)
    if (tsd[i] == t)
      return 1;

  return
         (SizeOfWord != 2 && t == tokLong) ||
         (!params && (t == tokExtern ||
                      t == tokStatic));
}

void PushSyntax2(word t, word v)
{
  if (SyntaxStackCnt >= SYNTAX_STACK_MAX)
    error("Symbol table exhausted\n");
  SyntaxStack0[SyntaxStackCnt] = t;
  SyntaxStack1[SyntaxStackCnt++] = v;
}

void PushSyntax(word t)
{
  PushSyntax2(t, 0);
}

void InsertSyntax2(word pos, word t, word v)
{
  if (SyntaxStackCnt >= SYNTAX_STACK_MAX)
    error("Symbol table exhausted\n");
  memmove(&SyntaxStack0[pos + 1],
          &SyntaxStack0[pos],
          sizeof(SyntaxStack0[0]) * (SyntaxStackCnt - pos));
  memmove(&SyntaxStack1[pos + 1],
          &SyntaxStack1[pos],
          sizeof(SyntaxStack1[0]) * (SyntaxStackCnt - pos));
  SyntaxStack0[pos] = t;
  SyntaxStack1[pos] = v;
  SyntaxStackCnt++;
}

void InsertSyntax(word pos, word t)
{
  InsertSyntax2(pos, t, 0);
}

void DeleteSyntax(word pos, word cnt)
{
  memmove(&SyntaxStack0[pos],
          &SyntaxStack0[pos + cnt],
          sizeof(SyntaxStack0[0]) * (SyntaxStackCnt - (pos + cnt)));
  memmove(&SyntaxStack1[pos],
          &SyntaxStack1[pos + cnt],
          sizeof(SyntaxStack1[0]) * (SyntaxStackCnt - (pos + cnt)));
  SyntaxStackCnt -= cnt;
}

word FindSymbol(char* s)
{
  word i;

  // TBD!!! return declaration scope number so
  // redeclarations can be reported if occur in the same scope.

  // TBD??? Also, I could first use FindIdent() and then just look for the
  // index into IdentTable[] instead of doing strcmp()

  for (i = SyntaxStackCnt - 1; i >= 0; i--)
  {
    word t = SyntaxStack0[i];
    if (t == tokIdent &&
        !strcmp(IdentTable + SyntaxStack1[i], s))
    {
      return i;
    }

    if (t == ')')
    {
      // Skip over the function params
      word c = -1;
      while (c)
      {
        t = SyntaxStack0[--i];
        c += (t == '(') - (t == ')');
      }
    }
  }

  return -1;
}

word SymType(word SynPtr)
{
  word local = 0;

  if (SyntaxStack0[SynPtr] == tokIdent)
    SynPtr++;

  if ((local = SyntaxStack0[SynPtr] == tokLocalOfs) != 0)
    SynPtr++;

  switch (SyntaxStack0[SynPtr])
  {
  case '(':
    return SymFxn;

  case '[':
    if (local)
      return SymLocalArr;
    return SymGlobalArr;

  default:
    if (local)
      return SymLocalVar;
    return SymGlobalVar;
  }
}

word FindTaggedDecl(char* s, word start, word* CurScope)
{
  word i;

  *CurScope = 1;

  for (i = start; i >= 0; i--)
  {
    word t = SyntaxStack0[i];
    if (t == tokTag &&
        !strcmp(IdentTable + SyntaxStack1[i], s))
    {
      return i - 1;
    }
    else if (t == ')')
    {
      // Skip over the function params
      word c = -1;
      while (c)
      {
        t = SyntaxStack0[--i];
        c += (t == '(') - (t == ')');
      }
    }
    else if (t == '#')
    {
      // the scope has changed to the outer scope
      *CurScope = 0;
    }
  }

  return -1;
}


word GetDeclSize(word SyntaxPtr, word SizeForDeref)
{
  word i;
  unsigned size = 1;
  word arr = 0;

  if (SyntaxPtr < 0) // pointer?
    return SizeOfWord;

  for (i = SyntaxPtr; i < SyntaxStackCnt; i++)
  {
    word tok = SyntaxStack0[i];
    switch (tok)
    {
    case tokIdent: // skip leading identifiers, if any
    case tokLocalOfs: // skip local var offset, too
      break;
    case tokChar:
    case tokSChar:
      if (!arr && ((tok == tokSChar) || CharIsSigned) && SizeForDeref)
        return -1; // 1 byte, needing sign extension when converted to int/unsigned int
      // fallthrough
    case tokUChar:
      return (word)size;
    case tokShort:
      if (!arr && SizeForDeref)
        return -2; // 2 bytes, needing sign extension when converted to int/unsigned int
      // fallthrough
    case tokUShort:
      //if (size * 2 / 2 != size)
        //error("Variable too big\n");
      //  errorVarSize();
      size *= 2;
      if (size != truncUint(size))
        //error("Variable too big\n");
      {
        //printf("b");
        errorVarSize();
      }
        
      return (word)size;
    case tokInt:
    case tokUnsigned:
    case '*':
    case '(': // size of fxn = size of ptr for now
      //if (size * SizeOfWord / SizeOfWord != size)
        //error("Variable too big\n");
        //errorVarSize();
      size *= SizeOfWord;
      if (size != truncUint(size))
        //error("Variable too big\n");
      {
        //printf("c");
        errorVarSize();
      }
      return (word)size;
    case '[':
      if (SyntaxStack0[i + 1] != tokNumInt && SyntaxStack0[i + 1] != tokNumUint)
        errorInternal(11);
      //if (SyntaxStack1[i + 1] &&
          //size * SyntaxStack1[i + 1] / SyntaxStack1[i + 1] != size)
        //error("Variable too big\n");
        //errorVarSize();
      size *= SyntaxStack1[i + 1];
      if (size != truncUint(size))
        //error("Variable too big\n");
      {
        //printf("d");
        errorVarSize();
      }
      i += 2;
      arr = 1;
      break;
    case tokStructPtr:
      // follow the "type pointer"
      i = SyntaxStack1[i] - 1;
      break;
    case tokStruct:
    case tokUnion:
      if (i + 2 < SyntaxStackCnt && SyntaxStack0[i + 2] == tokSizeof && !SizeForDeref)
      {
        unsigned s = SyntaxStack1[i + 2];
        //if (s && size * s / s != size)
        //  errorVarSize();
        size *= s;
        if (size != truncUint(size))
        {
          //printf("d");
          errorVarSize();
        }
        return (word)size;
      }
      return 0;
    case tokVoid:
      return 0;
    default:
      errorInternal(12);
    }
  }

  errorInternal(13);
  return 0;
}

word GetDeclAlignment(word SyntaxPtr)
{
  word i;

  if (SyntaxPtr < 0) // pointer?
    return SizeOfWord;

  for (i = SyntaxPtr; i < SyntaxStackCnt; i++)
  {
    word tok = SyntaxStack0[i];
    switch (tok)
    {
    case tokIdent: // skip leading identifiers, if any
    case tokLocalOfs: // skip local var offset, too
      break;
    case tokChar:
    case tokSChar:
    case tokUChar:
      return 1;

    case tokShort:
    case tokUShort:
      return 2;

    case tokInt:
    case tokUnsigned:
    case '*':
    case '(':
      return SizeOfWord;
    case '[':
      if (SyntaxStack0[i + 1] != tokNumInt && SyntaxStack0[i + 1] != tokNumUint)
        errorInternal(15);
      i += 2;
      break;
    case tokStructPtr:
      // follow the "type pointer"
      i = SyntaxStack1[i] - 1;
      break;
    case tokStruct:
    case tokUnion:
      if (i + 3 < SyntaxStackCnt && SyntaxStack0[i + 2] == tokSizeof)
      {
        return SyntaxStack1[i + 3];
      }
      return 1;
    case tokVoid:
      return 1;
    default:
      errorInternal(16);
    }
  }

  errorInternal(17);
  return 0;
}


void DumpDecl(word SyntaxPtr, word IsParam)
{
  word i;
  word icnt = 0;

  if (SyntaxPtr < 0)
    return;

  for (i = SyntaxPtr; i < SyntaxStackCnt; i++)
  {
    word tok = SyntaxStack0[i];
    word v = SyntaxStack1[i];
    switch (tok)
    {
    case tokLocalOfs:
      printf2("(@");
      printd2(truncInt(v));
      printf2(") : ");
      break;

    case tokIdent:
      if (++icnt > 1 && !IsParam) // show at most one declaration, except params
        return;

      GenStartCommentLine();

      if (ParseLevel == 0)
        printf2("glb ");
      else if (IsParam)
        printf2("prm ");
      else
        printf2("loc ");

      {
        word j;
        for (j = 0; j < ParseLevel * 4; j++)
          printf2(" ");
      }

      if (IsParam && !strcmp(IdentTable + v, "<something>") && (i + 1 < SyntaxStackCnt))
      {
        if (SyntaxStack0[i + 1] == tokEllipsis)
          continue;
      }

      printf2(IdentTable + v);
      printf2(" : ");
      if (!IsParam && (i + 1 < SyntaxStackCnt) && SyntaxStack0[i + 1] == tokIdent)
      {
        // renamed local static variable
        GenPrintLabel(IdentTable + SyntaxStack1[++i]);
        printf2(" : ");
      }
      break;

    case '[':
      printf2("[");
      break;

    case tokNumInt:
      printd2(truncInt(v));
      break;
    case tokNumUint:
      printd2(truncUint(v));
      break;

    case ']':
      printf2("] ");
      break;

    case '(':
      {
        word noparams;
        // Skip over the params to the base type
        word j = ++i, c = 1;
        while (c)
        {
          word t = SyntaxStack0[j++];
          c += (t == '(') - (t == ')');
        }

        noparams = (i + 1 == j) || (SyntaxStack0[i + 1] == tokVoid);

        printf2("(");

        // Print the params (recursively)
        if (noparams)
        {
          // Don't recurse if it's "fxn()" or "fxn(void)"
          if (i + 1 != j)
            printf2("void");
        }
        else
        {
          puts2("");
          ParseLevel++;
          DumpDecl(i, 1);
          ParseLevel--;
        }

        // Continue normally
        i = j - 1;
        if (!noparams)
        {
          GenStartCommentLine();
          printf2("    ");
          {
            word j;
            for (j = 0; j < ParseLevel * 4; j++)
              printf2(" ");
          }
        }

        printf2(") ");
      }
      break;

    case ')': // end of param list
      return;

    case tokStructPtr:
      DumpDecl(v, 0);
      break;

    default:
      switch (tok)
      {
      case tokVoid:
      case tokChar:
      case tokSChar:
      case tokUChar:
      case tokShort:
      case tokUShort:
      case tokInt:
      case tokUnsigned:
      case tokEllipsis:
        printf2(GetTokenName(tok));
        printf2("\n");
        break;
      default:
        printf2(GetTokenName(tok));
        printf2(" ");
        break;
      case tokTag:
        printf2(IdentTable + v);
        printf2("\n");
        return;
      }
      break;
    }
  }
}

void DumpSynDecls(void)
{
  word used = SyntaxStackCnt * (sizeof SyntaxStack0[0] + sizeof SyntaxStack1[0]);
  word total = SYNTAX_STACK_MAX * (sizeof SyntaxStack0[0] + sizeof SyntaxStack1[0]);
  puts2("");
  GenStartCommentLine(); printf2("Syntax/declaration table/stack:\n");
  GenStartCommentLine(); 
  printf2("Bytes used: ");
  printd2(used);
  printf2("/");
  printd2(total);
  printf2("\n\n");
}


word ParseArrayDimension(word AllowEmptyDimension)
{
  word tok;
  word gotUnary, synPtr, constExpr, exprVal;
  unsigned exprValU;
  word oldssp, oldesp, undoIdents;

  tok = GetToken();
  // DONE: support arbitrary constant expressions
  oldssp = SyntaxStackCnt;
  oldesp = sp;
  undoIdents = IdentTableLen;
  tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0);
  IdentTableLen = undoIdents; // remove all temporary identifier names from e.g. "sizeof"
  SyntaxStackCnt = oldssp; // undo any temporary declarations from e.g. "sizeof" in the expression
  sp = oldesp;

  if (tok != ']')
    //error("ParseArrayDimension(): Unsupported or invalid array dimension (token %s)\n", GetTokenName(tok));
    errorUnexpectedToken(tok);

  if (!gotUnary)
  {
    if (!AllowEmptyDimension)
      //error("ParseArrayDimension(): missing array dimension\n");
      errorUnexpectedToken(tok);
    // Empty dimension is dimension of 0
    exprVal = 0;
  }
  else
  {
    if (!constExpr)
      //error("ParseArrayDimension(): non-constant array dimension\n");
      errorNotConst();

    exprValU = truncUint(exprVal);
    exprVal = truncInt(exprVal);

    promoteType(&synPtr, &synPtr);
    anyIntTypeCheck(synPtr);
    if ((SyntaxStack0[synPtr] == tokInt && exprVal < 1) || (SyntaxStack0[synPtr] == tokUnsigned && exprValU < 1))
      error("Array dimension less than 1\n");

    exprVal = (word)exprValU;
  }

  PushSyntax2(tokNumUint, exprVal);
  return tok;
}

void ParseFxnParams(word tok);
static word BrkCntTargetFxn[2];
word ParseBlock(word BrkCntTarget[2], word casesIdx);
void AddFxnParamSymbols(word SyntaxPtr);
void CheckRedecl(word lastSyntaxPtr);

word ParseBase(word tok, word base[2])
{
  word valid = 1;
  base[1] = 0;

  switch (tok)
  {
  case tokVoid:
    *base = tok;
    tok = GetToken();
    break;

  case tokChar:
  case tokInt:
  case tokShort:
  case tokLong:
  case tokSigned:
  case tokUnsigned:
  {
    word allowedMask = 0x7F; // double:0x40 unsigned:0x20 signed:0x10 long:0x08 int:0x04 short:0x02 char:0x01
    word typeMask = 0;
    word tokMask, disallowedMask;

lcont:
    switch (tok)
    {
    case tokChar:
      tokMask = 0x01; disallowedMask = 0x4E; break; // disallows double, long, int, short
    case tokShort:
      tokMask = 0x02; disallowedMask = 0x49; break; // disallows double, long, char
    case tokInt:
      tokMask = 0x04; disallowedMask = 0x41; break; // disallows double, char
    case tokLong:
      tokMask = 0x08; disallowedMask = 0x03; break; // disallows short, char
    case tokSigned:
      tokMask = 0x10; disallowedMask = 0x60; break; // disallows double, unsigned
    case tokUnsigned:
      tokMask = 0x20; disallowedMask = 0x50; break; // disallows double, signed
    default:
      tokMask = disallowedMask = 0; break;
    }

    if (allowedMask & tokMask)
    {
      typeMask |= tokMask;
      allowedMask &= ~(disallowedMask | tokMask);
      tok = GetToken();
      goto lcont;
    }

    switch (typeMask)
    {
    case 0x01: typeMask = tokChar; break;
    case 0x11: typeMask = tokSChar; break;
    case 0x21: typeMask = tokUChar; break;
    case 0x02: case 0x12: case 0x06: case 0x16: typeMask = tokShort; break;
    case 0x22: case 0x26: typeMask = tokUShort; break;
    case 0x04: case 0x10: case 0x14: typeMask = tokInt; break;
    case 0x20: case 0x24: typeMask = tokUnsigned; break;
    case 0x08: case 0x18: case 0x0C: case 0x1C: typeMask = tokLong; break;
    case 0x28: case 0x2C: typeMask = tokULong; break;

    default:
      errorDecl();
    }
    *base = typeMask;
  }
    break;

  case tokStruct:
  case tokUnion:
  {
    word structType = tok;
    word empty = 1;
    word typePtr = SyntaxStackCnt;
    word gotTag = 0, tagIdent = 0, declPtr = -1, curScope = 0;

    tok = GetToken();

    if (tok == tokIdent)
    {
      // this is a structure/union/enum tag
      gotTag = 1;
      declPtr = FindTaggedDecl(TokenIdentName, SyntaxStackCnt - 1, &curScope);
      tagIdent = AddIdent(TokenIdentName);

      if (declPtr >= 0)
      {
        // Within the same scope we can't declare more than one union, structure or enum
        // with the same tag.
        // There's one common tag namespace for structures, unions and enumerations.
        if (curScope && SyntaxStack0[declPtr] != structType)
          errorTagRedef(tagIdent);
      }
      else if (ParamLevel)
      {
        // new structure/union/enum declarations aren't supported in function parameters
        errorDecl();
      }

      tok = GetToken();
    }
    else
    {
      // structure/union/enum declarations aren't supported in expressions
      if (ExprLevel)
        errorDecl();
      PushSyntax(structType);
      PushSyntax2(tokTag, AddIdent("<something>"));
    }

    if (tok == '{')
    {
      unsigned structInfo[4], sz, alignment, tmp;

      // new structure/union/enum declarations aren't supported in expressions and function parameters
      if (ExprLevel || ParamLevel)
        errorDecl();

      if (gotTag)
      {
        // Cannot redefine a tagged structure/union/enum within the same scope
        if (declPtr >= 0 &&
            curScope &&
            ((declPtr + 2 < SyntaxStackCnt && SyntaxStack0[declPtr + 2] == tokSizeof)

            ))
          errorTagRedef(tagIdent);

        PushSyntax(structType);
        PushSyntax2(tokTag, tagIdent);
      }


      {
        structInfo[0] = structType;
        structInfo[1] = 1; // initial member alignment
        structInfo[2] = 0; // initial member offset
        structInfo[3] = 0; // initial max member size (for unions)

        PushSyntax(tokSizeof); // 0 = initial structure/union size, to be updated
        PushSyntax2(tokSizeof, 1); // 1 = initial structure/union alignment, to be updated

        PushSyntax('{');

        tok = GetToken();
        while (tok != '}')
        {
          if (!TokenStartsDeclaration(tok, 1))
            errorUnexpectedToken(tok);
          tok = ParseDecl(tok, structInfo, 0, 0);
          empty = 0;
        }

        if (empty)
          errorUnexpectedToken('}');

        PushSyntax('}');

        // Update structure/union alignment
        alignment = structInfo[1];
        SyntaxStack1[typePtr + 3] = alignment;

        // Update structure/union size and include trailing padding if needed
        sz = structInfo[2] + structInfo[3];
        tmp = sz;
        sz = (sz + alignment - 1) & ~(alignment - 1);
        if (sz < tmp || sz != truncUint(sz))
        {
          //printf("e");
          errorVarSize();
        }
        SyntaxStack1[typePtr + 2] = (word)sz;

        tok = GetToken();
      }
    }
    else
    {


      if (gotTag)
      {
        if (declPtr >= 0 &&
            SyntaxStack0[declPtr] == structType)
        {
          base[0] = tokStructPtr;
          base[1] = declPtr;
          return tok;
        }

        PushSyntax(structType);
        PushSyntax2(tokTag, tagIdent);

        empty = 0;
      }
    }

    if (empty)
      errorDecl();

    base[0] = tokStructPtr;
    base[1] = typePtr;

    // If we've just defined a structure/union and there are
    // preceding references to this tag within this scope,
    // IOW references to an incomplete type, complete the
    // type in the references
    if (gotTag && SyntaxStack0[SyntaxStackCnt - 1] == '}')
    {
      word i;
      for (i = SyntaxStackCnt - 1; i >= 0; i--)
        if (SyntaxStack0[i] == tokStructPtr)
        {
          word j = SyntaxStack1[i];
          if (SyntaxStack1[j + 1] == tagIdent && !GetDeclSize(i, 0))
            SyntaxStack1[i] = typePtr;
        }
        else if (SyntaxStack0[i] == '#')
        {
          // reached the beginning of the current scope
          break;
        }
    }
  }
    break;



  default:
    valid = 0;
    break;
  }


  if (SizeOfWord == 2 &&
      (*base == tokLong || *base == tokULong))
    valid = 0;
  // to simplify matters, treat long and unsigned long as aliases for int and unsigned int
  // in 32-bit and huge mode(l)s
  if (*base == tokLong)
    *base = tokInt;
  if (*base == tokULong)
    *base = tokUnsigned;


  if (SizeOfWord == 2)
  {
    // to simplify matters, treat short and unsigned short as aliases for int and unsigned int
    // in 16-bit mode
    if (*base == tokShort)
      *base = tokInt;
    if (*base == tokUShort)
      *base = tokUnsigned;
  }

  // TBD!!! review/test this fxn
//  if (!valid || !tok || !(strchr("*([,)", tok) || tok == tokIdent))
  if (!valid | !tok)
    //error("ParseBase(): Invalid or unsupported type\n");
    error("Invalid or unsupported type\n");

  return tok;
}

/*
  base * name []  ->  name : [] * base
  base *2 (*1 name []1) []2  ->  name : []1 *1 []2 *2 base
  base *3 (*2 (*1 name []1) []2) []3  ->  name : []1 *1 []2 *2 []3 *3 base
*/

word ParseDerived(word tok)
{
  word stars = 0;
  word params = 0;

  while (tok == '*')
  {
    stars++;
    tok = GetToken();
  }



  if (tok == '(')
  {
    tok = GetToken();
    if (tok != ')' && !TokenStartsDeclaration(tok, 1))
    {
      tok = ParseDerived(tok);
      if (tok != ')')
        //error("ParseDerived(): ')' expected\n");
        errorUnexpectedToken(tok);
      tok = GetToken();
    }
    else
    {
      params = 1;
    }
  }
  else if (tok == tokIdent)
  {
    PushSyntax2(tok, AddIdent(TokenIdentName));
    tok = GetToken();
  }
  else
  {
    PushSyntax2(tokIdent, AddIdent("<something>"));
  }

  if (params | (tok == '('))
  {
    word t = SyntaxStack0[SyntaxStackCnt - 1];
    if ((t == ')') | (t == ']'))
      errorUnexpectedToken('('); // array of functions or function returning function
    if (!params)
      tok = GetToken();
    else
      PushSyntax2(tokIdent, AddIdent("<something>"));

    PushSyntax('(');

    ParseLevel++;
    ParamLevel++;
    ParseFxnParams(tok);
    ParamLevel--;
    ParseLevel--;
    PushSyntax(')');
    tok = GetToken();
  }
  else if (tok == '[')
  {
    // DONE!!! allow the first [] without the dimension in function parameters
    word allowEmptyDimension = 1;
    if (SyntaxStack0[SyntaxStackCnt - 1] == ')')
      errorUnexpectedToken('['); // function returning array
    while (tok == '[')
    {
      word oldsp = SyntaxStackCnt;
      PushSyntax(tokVoid); // prevent cases like "int arr[arr];" and "int arr[arr[0]];"
      PushSyntax(tok);
      tok = ParseArrayDimension(allowEmptyDimension);
      if (tok != ']')
        //error("ParseDerived(): ']' expected\n");
        errorUnexpectedToken(tok);
      PushSyntax(']');
      tok = GetToken();
      DeleteSyntax(oldsp, 1);
      allowEmptyDimension = 0;
    }
  }

  while (stars--)
    PushSyntax('*');

  if (!tok || !strchr(",;{=)", tok))
    //error("ParseDerived(): unexpected token %s\n", GetTokenName(tok));
    errorUnexpectedToken(tok);

  return tok;
}

void PushBase(word base[2])
{
  {
    PushSyntax2(base[0], base[1]);
  }

  // Cannot have array of void
  if (SyntaxStack0[SyntaxStackCnt - 1] == tokVoid &&
      SyntaxStack0[SyntaxStackCnt - 2] == ']')
    errorUnexpectedVoid();
}

word InitScalar(word synPtr, word tok);
word InitArray(word synPtr, word tok);
word InitStruct(word synPtr, word tok);

word InitVar(word synPtr, word tok)
{
  word p = synPtr, t;
  word undoIdents = IdentTableLen;

  while ((t = SyntaxStack0[p]), (t == tokIdent) | (t == tokLocalOfs))
    p++;

  switch (t)
  {
  case '[':
    // Initializers for aggregates must be enclosed in braces,
    // except for arrays of char initialized with string literals,
    // in which case braces are optional
    if (tok != '{')
    {
      t = SyntaxStack0[p + 3];
      if (((tok != tokLitStr) | ((t != tokChar) & (t != tokUChar) & (t != tokSChar)))
         )
        errorUnexpectedToken(tok);
    }
    tok = InitArray(p, tok);
    break;
  case tokStructPtr:
    // Initializers for aggregates must be enclosed in braces
    if (tok != '{')
      errorUnexpectedToken(tok);
    tok = InitStruct(p, tok);
    break;
  default:
    tok = InitScalar(p, tok);
    break;
  }

  if (!strchr(",;", tok))
    errorUnexpectedToken(tok);

  IdentTableLen = undoIdents; // remove all temporary identifier names from e.g. "sizeof" or "str"

  return tok;
}

word InitScalar(word synPtr, word tok)
{
  unsigned elementSz = GetDeclSize(synPtr, 0);
  word gotUnary, synPtr2, constExpr, exprVal;
  word oldssp = SyntaxStackCnt;
  word undoIdents = IdentTableLen;
  word ttop;
  word braces = 0;

  // Initializers for scalars can be optionally enclosed in braces
  if (tok == '{')
  {
    braces = 1;
    tok = GetToken();
  }

  tok = ParseExpr(tok, &gotUnary, &synPtr2, &constExpr, &exprVal, ',', 0);

  if (!gotUnary)
    errorUnexpectedToken(tok);

  if (braces)
  {
    if (tok != '}')
      errorUnexpectedToken(tok);
    tok = GetToken();
  }

  // Bar void and struct/union
  scalarTypeCheck(synPtr2);


  ttop = stack[sp - 1][0];
  if (ttop == tokNumInt || ttop == tokNumUint)
  {
    word val = stack[sp - 1][1];
    // TBD??? truncate values for types smaller than int (e.g. char and short),
    // so they are always in range for the assembler?
    GenIntData(elementSz, val);
  }
  else if (elementSz == (unsigned)SizeOfWord)
  {
    if (ttop == tokIdent)
    {
      GenAddrData(elementSz, IdentTable + stack[sp - 1][1], 0);
    }
    else if (ttop == '+' || ttop == '-')
    {
      word tleft = stack[sp - 3][0];
      word tright = stack[sp - 2][0];
      if (tleft == tokIdent &&
          (tright == tokNumInt || tright == tokNumUint))
      {
        GenAddrData(elementSz, IdentTable + stack[sp - 3][1], (ttop == '+') ? stack[sp - 2][1] : -stack[sp - 2][1]);
      }
      else if (ttop == '+' &&
               tright == tokIdent &&
               (tleft == tokNumInt || tleft == tokNumUint))
      {
        GenAddrData(elementSz, IdentTable + stack[sp - 2][1], stack[sp - 3][1]);
      }
      else
        errorNotConst();
    }
    else
      errorNotConst();
    // Defer storage of string literal data (if any) until the end.
    // This will let us generate the contiguous array of pointers to
    // string literals unperturbed by the string literal data
    // (e.g. "char* colors[] = { "red", "green", "blue" };").
  }
  else
    //error("ParseDecl(): cannot initialize a global variable with a non-constant expression\n");
    errorNotConst();

  IdentTableLen = undoIdents; // remove all temporary identifier names from e.g. "sizeof" or "str"
  SyntaxStackCnt = oldssp; // undo any temporary declarations from e.g. "sizeof" or "str" in the expression
  return tok;
}

word InitArray(word synPtr, word tok)
{
  word elementTypePtr = synPtr + 3;
  word elementType = SyntaxStack0[elementTypePtr];
  unsigned elementSz = GetDeclSize(elementTypePtr, 0);
  word braces = 0;
  unsigned elementCnt = 0;
  unsigned elementsRequired = SyntaxStack1[synPtr + 1];
  word arrOfChar = (elementType == tokChar) | (elementType == tokUChar) | (elementType == tokSChar);

  if (tok == '{')
  {
    braces = 1;
    tok = GetToken();
  }

  if ((arrOfChar & (tok == tokLitStr))
     )
  {
    word ltok = tok;
    unsigned sz = 0;
    // this is 'someArray[someCountIfAny] = "some string"' or
    // 'someArray[someCountIfAny] = { "some string" }'
    do
    {
      GetString('"', 0, 'd');
      if (sz + TokenStringSize < sz ||
          sz + TokenStringSize >= truncUint(-1))
        errorStrLen();
      sz += TokenStringSize;
      elementCnt += TokenStringLen;
      tok = GetToken();
    } while (tok == ltok); // concatenate adjacent string literals


    if (elementsRequired && elementCnt > elementsRequired)
      errorStrLen();

    if (elementCnt < elementsRequired)
      GenZeroData(elementsRequired - elementCnt, 0);

    if (!elementsRequired)
      GenZeroData(elementSz, 0), elementCnt++;

    if (braces)
    {
      if (tok != '}')
        errorUnexpectedToken(tok);
      tok = GetToken();
    }
  }
  else
  {
    while (tok != '}')
    {
      if (elementType == '[')
      {
        tok = InitArray(elementTypePtr, tok);
      }
      else if (elementType == tokStructPtr)
      {
        tok = InitStruct(elementTypePtr, tok);
      }
      else
      {
        tok = InitScalar(elementTypePtr, tok);
      }

      // Last element?
      if (++elementCnt >= elementsRequired && elementsRequired)
      {
        if (braces & (tok == ','))
          tok = GetToken();
        break;
      }

      if (tok == ',')
        tok = GetToken();
      else if (tok != '}')
        errorUnexpectedToken(tok);
    }

    if (braces)
    {
      if ((!elementCnt) | (tok != '}'))
        errorUnexpectedToken(tok);
      tok = GetToken();
    }

    if (elementCnt < elementsRequired)
      GenZeroData((elementsRequired - elementCnt) * elementSz, 0);
  }

  // Store the element count if it's an incomplete array
  if (!elementsRequired)
    SyntaxStack1[synPtr + 1] = elementCnt;

  return tok;
}

word InitStruct(word synPtr, word tok)
{
  word isUnion;
  unsigned size, ofs = 0;
  word braces = 0;
  word c = 1;

  synPtr = SyntaxStack1[synPtr];
  isUnion = SyntaxStack0[synPtr++] == tokUnion;
  size = SyntaxStack1[++synPtr];
  synPtr += 3; // step inside the {} body of the struct/union

  if (tok == '{')
  {
    braces = 1;
    tok = GetToken();
  }

  // Find the first member
  while (c)
  {
    word t = SyntaxStack0[synPtr];
    c += (t == '(') - (t == ')') + (t == '{') - (t == '}');
    if (c == 1 && t == tokMemberIdent)
      break;
    synPtr++;
  }

  while (tok != '}')
  {
    word c = 1;
    word elementTypePtr, elementType;
    unsigned elementOfs, elementSz;

    elementOfs = SyntaxStack1[++synPtr];
    elementTypePtr = ++synPtr;
    elementType = SyntaxStack0[elementTypePtr];
    elementSz = GetDeclSize(elementTypePtr, 0);

    // Alignment
    if (ofs < elementOfs)
      GenZeroData(elementOfs - ofs, 0);

    if (elementType == '[')
    {
      tok = InitArray(elementTypePtr, tok);
    }
    else if (elementType == tokStructPtr)
    {
      tok = InitStruct(elementTypePtr, tok);
    }
    else
    {
      tok = InitScalar(elementTypePtr, tok);
    }

    ofs = elementOfs + elementSz;

    // Find the next member or the closing brace
    while (c)
    {
      word t = SyntaxStack0[synPtr];
      c += (t == '(') - (t == ')') + (t == '{') - (t == '}');
      if (c == 1 && t == tokMemberIdent)
        break;
      synPtr++;
    }

    // Last member?
    // Only one member (first) is initialized in unions explicitly
    if ((!c) | isUnion)
    {
      if (braces & (tok == ','))
        tok = GetToken();
      break;
    }

    if (tok == ',')
      tok = GetToken();
    else if (tok != '}')
      errorUnexpectedToken(tok);
  }

  if (braces)
  {
    if ((!ofs) | (tok != '}'))
      errorUnexpectedToken(tok);
    tok = GetToken();
  }

  // Implicit initialization of the rest and trailing padding
  if (ofs < size)
    GenZeroData(size - ofs, 0);

  return tok;
}

word compatCheck2(word lastSyntaxPtr, word i)
{
  word res = 0;
  word c = 0;
  word t;
  for (;;)
  {
    t = SyntaxStack0[lastSyntaxPtr];
    if (t != SyntaxStack0[i])
    {
      if (SyntaxStack0[i] == ')' && SyntaxStack0[i - 1] == '(')
      {
        // Complete a previously incomplete parameter specification
        word c1 = 1;
        // Skip over the function params
        do
        {
          t = SyntaxStack0[lastSyntaxPtr++];
          c1 += (t == '(') - (t == ')');
        } while (c1);
        lastSyntaxPtr--;
      }
      else if (t == ')' &&
               SyntaxStack0[i - 1] == '(' && SyntaxStack0[i] == tokIdent &&
               SyntaxStack0[i + 1] == tokVoid && SyntaxStack0[i + 2] == ')')
      {
        // As an exception allow foo(void) to be redeclared as foo()
        // since this happens very often in code.
        // This weakens our redeclaration checks, however. Warn about it.
        i += 2;
        warning("Redeclaration from no parameters to unspecified parameters.\n");
      }
      else
        goto lend;
    }

    if (t != tokIdent &&
        SyntaxStack1[lastSyntaxPtr] != SyntaxStack1[i])
    {
      if (SyntaxStack0[lastSyntaxPtr - 1] == '[')
      {
        // Complete an incomplete array dimension or check for dimension mismatch
        if (SyntaxStack1[lastSyntaxPtr] == 0)
          SyntaxStack1[lastSyntaxPtr] = SyntaxStack1[i];
        else if (SyntaxStack1[i])
          goto lend;
      }
      else
        goto lend;
    }

    c += (t == '(') - (t == ')') + (t == '[') - (t == ']');

    if (!c)
    {
      switch (t)
      {
      case tokVoid:
      case tokChar: case tokSChar: case tokUChar:
      case tokShort: case tokUShort:
      case tokInt: case tokUnsigned:
      case tokStructPtr:
        goto lok;
      }
    }

    lastSyntaxPtr++;
    i++;
  }

lok:
  res = 1;

lend:
  return res;
}

void CheckRedecl(word lastSyntaxPtr)
{
  word tid, id, external = 0;
  word i;
  word curScopeOnly;
  word level = ParseLevel;

  tid = SyntaxStack0[lastSyntaxPtr];
  id = SyntaxStack1[lastSyntaxPtr];
  switch (tid)
  {
  case tokIdent:
    switch (SyntaxStack0[lastSyntaxPtr + 1])
    {

    default:
      external = 1;
      // fallthrough
    case tokLocalOfs: // block-scope auto
    case tokIdent: // block-scope static
      ;
    }
    // fallthrough

  case tokTypedef:
    break;

  case tokMemberIdent:
    {
      word c = 1;
      i = lastSyntaxPtr - 1;
      do
      {
        word t = SyntaxStack0[i];
        c -= (t == '(') - (t == ')') + (t == '{') - (t == '}');
        if (c == 1 &&
            t == tokMemberIdent && SyntaxStack1[i] == id &&
            SyntaxStack0[i + 1] == tokLocalOfs)
          errorRedecl(IdentTable + id);
        i--;
      } while (c);
    }
    return;

  default:
    errorInternal(23);
  }

  // limit search to current scope for typedef and enum,
  // ditto for non-external declarations
  curScopeOnly = tid != tokIdent || !external;

  for (i = lastSyntaxPtr - 1; i >= 0; i--)
  {
    word t = SyntaxStack0[i];

    switch (t)
    {
    case ')':
      {
        // Skip over the function params
        word c = -1;
        while (c)
        {
          t = SyntaxStack0[--i];
          c += (t == '(') - (t == ')');
        }
      }
      break;

    case '#':
      // the scope has changed to the outer scope
      if (curScopeOnly)
        return;
      level--;
      break;

    case tokTypedef:
    case tokIdent:
      if (SyntaxStack1[i] == id)
      {
        if (level == ParseLevel)
        {
          // block scope:
          //   can differentiate between auto(tokLocalOfs), static(tokIdent),
          //   extern/proto(nothing) in SyntaxStack*[], hence dup checks and
          //   type match checks needed here
          // file scope:
          //   can't differentiate between static(nothing), extern(nothing),
          //   neither(nothing) in SyntaxStack*[], but duplicate definitions
          //   are taken care of (in CG), hence only type match checks needed
          //   here
          if (level) // block scope: check for bad dups
          {
            switch (SyntaxStack0[i + 1])
            {
            case tokLocalOfs: // block-scope auto
            case tokIdent: // block-scope static
              // auto and static can't be redefined in block scope
              errorRedecl(IdentTable + id);
            default:
              // extern can't be redefined as non-extern in block scope
              if (!external)
                errorRedecl(IdentTable + id);
            }
            // extern/proto type match check follows
          }

          if (compatCheck2(lastSyntaxPtr, i))
            return;
          errorRedecl(IdentTable + id);
        }
        else // elseof if (level == ParseLevel)
        {
          // The new decl is extern/proto.
          // Ignore typedef and enum
          if (t == tokIdent)
          {
            switch (SyntaxStack0[i + 1])
            {
            case tokLocalOfs: // block-scope auto
            case tokIdent: // block-scope static
              // Ignore auto and static
              break;
            default:
              // extern/proto
              if (compatCheck2(lastSyntaxPtr, i))
                return;
              errorRedecl(IdentTable + id);
            }
          }
        }
      } // endof if (SyntaxStack1[i] == id)
      break;
    } // endof switch (t)
  } // endof for (i = lastSyntaxPtr - 1; i >= 0; i--)
}

// DONE: support extern
// DONE: support static
// DONE: support basic initialization
// DONE: support simple non-array initializations with string literals
// DONE: support basic 1-d array initialization
// DONE: global/static data allocations
word ParseDecl(word tok, unsigned structInfo[4], word cast, word label)
{
  word base[2];
  word lastSyntaxPtr;
  word external = tok == tokExtern;
  word Static = tok == tokStatic;
  (void)label;

  if (external |
      Static)
  {
    tok = GetToken();
    if (!TokenStartsDeclaration(tok, 1))
      //error("ParseDecl(): unexpected token %s\n", GetTokenName(tok));
      // Implicit int (as in "extern x; static y;") isn't supported
      errorUnexpectedToken(tok);
  }
  tok = ParseBase(tok, base);


  for (;;)
  {
    lastSyntaxPtr = SyntaxStackCnt;

    /* derived type */
    tok = ParseDerived(tok);

    /* base type */
    PushBase(base);

    if ((tok && strchr(",;{=", tok)) || (tok == ')' && ExprLevel))
    {
      word isLocal = 0, isGlobal = 0, isFxn, isStruct, isArray, isIncompleteArr;
      unsigned alignment = 0;
      word staticLabel = 0;

      // Disallow void variables
      if (SyntaxStack0[SyntaxStackCnt - 1] == tokVoid)
      {
        if (SyntaxStack0[SyntaxStackCnt - 2] == tokIdent &&
            !(cast
             ))
          //error("ParseDecl(): Cannot declare a variable ('%s') of type 'void'\n", IdentTable + SyntaxStack1[lastSyntaxPtr]);
          errorUnexpectedVoid();
      }

      isFxn = SyntaxStack0[lastSyntaxPtr + 1] == '(';


      isArray = SyntaxStack0[lastSyntaxPtr + 1] == '[';
      isIncompleteArr = isArray && SyntaxStack1[lastSyntaxPtr + 2] == 0;

      isStruct = SyntaxStack0[lastSyntaxPtr + 1] == tokStructPtr;

      if (!(ExprLevel || structInfo) &&
          !(external |
            Static) &&
          !strcmp(IdentTable + SyntaxStack1[lastSyntaxPtr], "<something>") &&
          tok == ';')
      {
        if (isStruct)
        {
          // This is either an incomplete tagged structure/union declaration, e.g. "struct sometag;",
          // or a tagged complete structure/union declaration, e.g. "struct sometag { ... };", without an instance variable,
          // or an untagged complete structure/union declaration, e.g. "struct { ... };", without an instance variable
          word declPtr, curScope;
          word j = SyntaxStack1[lastSyntaxPtr + 1];

          if (j + 2 < SyntaxStackCnt &&
              IdentTable[SyntaxStack1[j + 1]] == '<' && // without tag
              SyntaxStack0[j + 2] == tokSizeof) // but with the {} "body"
            errorDecl();

          // If a structure/union with this tag has been declared in an outer scope,
          // this new declaration should override it
          declPtr = FindTaggedDecl(IdentTable + SyntaxStack1[j + 1], lastSyntaxPtr - 1, &curScope);
          if (declPtr >= 0 && !curScope)
          {
            // If that's the case, unbind this declaration from the old declaration
            // and make it a new incomplete declaration
            PushSyntax(SyntaxStack0[j]); // tokStruct or tokUnion
            PushSyntax2(tokTag, SyntaxStack1[j + 1]);
            SyntaxStack1[lastSyntaxPtr + 1] = SyntaxStackCnt - 2;
          }
          return GetToken();
        }
      }


      // Structure/union members can't be initialized nor be functions nor
      // be incompletely typed arrays inside structure/union declarations
      if (structInfo && ((tok == '=') | isFxn | (tok == '{') | isIncompleteArr))
        errorDecl();


      // Error conditions in declarations(/definitions/initializations):
      // Legend:
      //   +  error
      //   -  no error
      //
      // file scope          fxn   fxn {}  var   arr[]   arr[]...[]   arr[incomplete]   arr[incomplete]...[]
      //                     -     -       -     -       -            +                 +
      // file scope          fxn=          var=  arr[]=  arr[]...[]=  arr[incomplete]=  arr[incomplete]...[]=
      //                     +             -     -       +            -                 +
      // file scope  extern  fxn   fxn {}  var   arr[]   arr[]...[]   arr[incomplete]   arr[incomplete]...[]
      //                     -     -       -     -       -            -                 -
      // file scope  extern  fxn=          var=  arr[]=  arr[]...[]=  arr[incomplete]=  arr[incomplete]...[]=
      //                     +             +     +       +            +                 +
      // file scope  static  fxn   fxn {}  var   arr[]   arr[]...[]   arr[incomplete]   arr[incomplete]...[]
      //                     -     -       -     -       -            +                 +
      // file scope  static  fxn=          var=  arr[]=  arr[]...[]=  arr[incomplete]=  arr[incomplete]...[]=
      //                     +             -     -       +            -                 +
      // fxn scope           fxn   fxn {}  var   arr[]   arr[]...[]   arr[incomplete]   arr[incomplete]...[]
      //                     -     +       -     -       -            +                 +
      // fxn scope           fxn=          var=  arr[]=  arr[]...[]=  arr[incomplete]=  arr[incomplete]...[]=
      //                     +             -     +       +            +                 +
      // fxn scope   extern  fxn   fxn {}  var   arr[]   arr[]...[]   arr[incomplete]   arr[incomplete]...[]
      //                     -     +       -     -       -            -                 -
      // fxn scope   extern  fxn=          var=  arr[]=  arr[]...[]=  arr[incomplete]=  arr[incomplete]...[]=
      //                     +             +     +       +            +                 +
      // fxn scope   static  fxn   fxn {}  var   arr[]   arr[]...[]   arr[incomplete]   arr[incomplete]...[]
      //                     +     +       +     +       +            +                 +
      // fxn scope   static  fxn=          var=  arr[]=  arr[]...[]=  arr[incomplete]=  arr[incomplete]...[]=
      //                     +             +     +       +            +                 +

      if (isFxn & (tok == '='))
        //error("ParseDecl(): cannot initialize a function\n");
        errorInit();

      if ((isFxn & (tok == '{')) && ParseLevel)
        //error("ParseDecl(): cannot define a nested function\n");
        errorDecl();

      if ((isFxn & Static) && ParseLevel)
        //error("ParseDecl(): cannot declare a static function in this scope\n");
        errorDecl();

      if (external & (tok == '='))
        //error("ParseDecl(): cannot initialize an external variable\n");
        errorInit();

      if (isIncompleteArr & !(external |
                              (tok == '=')))
        //error("ParseDecl(): cannot define an array of incomplete type\n");
        errorDecl();

      // TBD!!! de-uglify
      if (!strcmp(IdentTable + SyntaxStack1[lastSyntaxPtr], "<something>"))
      {
        // Disallow nameless variables, prototypes, structure/union members and typedefs.
        if (structInfo ||
            !ExprLevel)
          error("Identifier expected in declaration\n");
      }
      else
      {
        // Disallow named variables and prototypes in sizeof(typedecl) and (typedecl).
        if (ExprLevel && !structInfo)
          error("Identifier unexpected in declaration\n");
      }

      if (!isFxn
         )
      {
        // This is a variable or a variable (member) in a struct/union declaration
        word sz = GetDeclSize(lastSyntaxPtr, 0);

        if (!((sz | isIncompleteArr) || ExprLevel)) // incomplete type
          errorDecl(); // TBD!!! different error when struct/union tag is not found

        if (isArray && !GetDeclSize(lastSyntaxPtr + 4, 0))
          // incomplete type of array element (e.g. struct/union)
          errorDecl();

        alignment = GetDeclAlignment(lastSyntaxPtr);

        if (structInfo)
        {
          // It's a variable (member) in a struct/union declaration
          unsigned tmp;
          unsigned newAlignment = alignment;
          // Update structure/union alignment
          if (structInfo[1] < newAlignment)
            structInfo[1] = newAlignment;
          // Align structure member
          tmp = structInfo[2];
          structInfo[2] = (structInfo[2] + newAlignment - 1) & ~(newAlignment - 1);
          if (structInfo[2] < tmp || structInfo[2] != truncUint(structInfo[2]))
          {
            //printf("f");
            errorVarSize();
          }
          // Change tokIdent to tokMemberIdent and insert a local var offset token
          SyntaxStack0[lastSyntaxPtr] = tokMemberIdent;
          InsertSyntax2(lastSyntaxPtr + 1, tokLocalOfs, (word)structInfo[2]);

          // Advance member offset for structures, keep it zero for unions
          if (structInfo[0] == tokStruct)
          {
            tmp = structInfo[2];
            structInfo[2] += sz;
            if (structInfo[2] < tmp || structInfo[2] != truncUint(structInfo[2]))
            {
              //printf("g");
              errorVarSize();
            }
          }
          // Update max member size for unions
          else if (structInfo[3] < (unsigned)sz)
          {
            structInfo[3] = sz;
          }
        }
        else if (ParseLevel && !((external | Static) || ExprLevel))
        {
          // It's a local variable
          isLocal = 1;
          // Defer size calculation until initialization
          // Insert a local var offset token, the offset is to be updated
          InsertSyntax2(lastSyntaxPtr + 1, tokLocalOfs, 0);
        }
        else if (!ExprLevel)
        {
          // It's a global variable (external, static or neither)
          isGlobal = 1;
          if (Static && ParseLevel)
          {
            // It's a static variable in function scope, "rename" it by providing
            // an alternative unique numeric identifier right next to it and use it
            staticLabel = LabelCnt++;
            InsertSyntax2(lastSyntaxPtr + 1, tokIdent, AddNumericIdent(staticLabel));
          }
        }
      }

      // If it's a type declaration in a sizeof(typedecl) expression or
      // in an expression with a cast, e.g. (typedecl)expr, we're done
      if (ExprLevel && !structInfo)
      {
        if (doAnnotations)
        {
          DumpDecl(lastSyntaxPtr, 0);
        }
        return tok;
      }

      if (isLocal | isGlobal)
      {
        word hasInit = tok == '=';
        word needsGlobalInit = isGlobal & !external;
        word sz = GetDeclSize(lastSyntaxPtr, 0);
        word initLabel = 0;
        word bss = (!hasInit) & UseBss;

        if (doAnnotations)
        {
          if (isGlobal)
            DumpDecl(lastSyntaxPtr, 0);
        }

        if (hasInit)
        {
          tok = GetToken();
        }

        if (isLocal & hasInit)
          needsGlobalInit = isArray | (isStruct & (tok == '{'));

        if (needsGlobalInit)
        {
          char** oldHeaderFooter = CurHeaderFooter;
          if (oldHeaderFooter)
            puts2(oldHeaderFooter[1]);
          CurHeaderFooter = bss ? BssHeaderFooter : DataHeaderFooter;
          puts2(CurHeaderFooter[0]);

          // DONE: imperfect condition for alignment
          if (alignment != 1)
            GenWordAlignment(bss);

          if (isGlobal)
          {
            if (Static && ParseLevel)
              GenNumLabel(staticLabel);
            else
              GenLabel(IdentTable + SyntaxStack1[lastSyntaxPtr], Static);
          }
          else
          {
            // Generate numeric labels for global initializers of local vars
            GenNumLabel(initLabel = LabelCnt++);
          }

          // Generate global initializers
          if (hasInit)
          {
            if (doAnnotations)
            {
              if (isGlobal)
              {
                GenStartCommentLine(); printf2("=\n");
              }
            }
            tok = InitVar(lastSyntaxPtr, tok);
            // Update the size in case it's an incomplete array
            sz = GetDeclSize(lastSyntaxPtr, 0);
          }
          else
          {
            GenZeroData(sz, bss);
          }

          puts2(CurHeaderFooter[1]);
          if (oldHeaderFooter)
            puts2(oldHeaderFooter[0]);
          CurHeaderFooter = oldHeaderFooter;
        }

        if (isLocal)
        {
          // Now that the size of the local is certainly known,
          // update its offset in the offset token
          SyntaxStack1[lastSyntaxPtr + 1] = AllocLocal(sz);

          if (doAnnotations)
          {
            DumpDecl(lastSyntaxPtr, 0);
          }
        }

        // Copy global initializers into local vars
        if (isLocal & needsGlobalInit)
        {
          if (doAnnotations)
          {
            GenStartCommentLine(); printf2("=\n");
          }
          if (!StructCpyLabel)
            StructCpyLabel = LabelCnt++;

          sp = 0;

          push2('(', SizeOfWord * 3);
          push2(tokLocalOfs, SyntaxStack1[lastSyntaxPtr + 1]);
          push(',');
          push2(tokIdent, AddNumericIdent(initLabel));
          push(',');
          push2(tokNumUint, sz);
          push(',');
          push2(tokIdent, AddNumericIdent(StructCpyLabel));
          push2(')', SizeOfWord * 3);

          GenExpr();
        }
        // Initialize local vars with expressions
        else if (hasInit & !needsGlobalInit)
        {
          word gotUnary, synPtr, constExpr, exprVal;
          word brace = 0;

          // Initializers for scalars can be optionally enclosed in braces
          if ((!isStruct) & (tok == '{'))
          {
            brace = 1;
            tok = GetToken();
          }

          // ParseExpr() will transform the initializer expression into an assignment expression here
          tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, '=', SyntaxStack1[lastSyntaxPtr]);

          if (!gotUnary)
            errorUnexpectedToken(tok);

          if (brace)
          {
            if (tok != '}')
              errorUnexpectedToken(tok);
            tok = GetToken();
          }

          if (!isStruct)
          {
            // This is a special case for initialization of integers smaller than int.
            // Since a local integer variable always takes as much space as a whole int,
            // we can optimize code generation a bit by storing the initializer as an int.
            // This is an old accidental optimization and I preserve it for now.
            // Note, this implies a little-endian CPU.
            stack[sp - 1][1] = SizeOfWord;
          }

          // Storage of string literal data from the initializing expression
          // occurs here.
          GenExpr();
        }
      }
      else if (tok == '{')
      {
        // It's a function body. Let's add function parameters as
        // local variables to the symbol table and parse the body.
        word undoSymbolsPtr = SyntaxStackCnt;
        word undoIdents = IdentTableLen;
        word i;
        word endLabel = 0;

        if (doAnnotations)
        {
          DumpDecl(lastSyntaxPtr, 0);
        }

        CurFxnName = IdentTable + SyntaxStack1[lastSyntaxPtr];
        IsMain = !strcmp(CurFxnName, "main");

        gotoLabCnt = 0;

        if (verbose)
        {
          printf(CurFxnName);
          printf("()\n");
        }

        ParseLevel++;
        GetFxnInfo(lastSyntaxPtr, &CurFxnParamCntMin, &CurFxnParamCntMax, &CurFxnReturnExprTypeSynPtr, NULL); // get return type



        CurHeaderFooter = CodeHeaderFooter;
        puts2(CurHeaderFooter[0]);

        //BDOS_PrintlnConsole(CurFxnName);
        //printf2("; put the function label here:\n");
        GenLabel(CurFxnName, Static);


        GenFxnProlog();
        CurFxnEpilogLabel = LabelCnt++;

        // A new scope begins before the function parameters
        PushSyntax('#');
        AddFxnParamSymbols(lastSyntaxPtr);


        // The block doesn't begin yet another new scope.
        // This is to catch redeclarations of the function parameters.
        tok = ParseBlock(BrkCntTargetFxn, 0);
        ParseLevel--;
        if (tok != '}')
          //error("ParseDecl(): '}' expected\n");
          errorUnexpectedToken(tok);

        for (i = 0; i < gotoLabCnt; i++)
          if (gotoLabStat[i] == 2)
          {
            printf("Undeclared label ");
            error(IdentTable + gotoLabels[i][0]);
          }

        // DONE: if execution of main() reaches here, before the epilog (i.e. without using return),
        // main() should return 0.
        if (IsMain)
        {
          sp = 0;
          push(tokNumInt);
          push(tokReturn); // value produced by generated code is used
          GenExpr();
        }

        GenNumLabel(CurFxnEpilogLabel);


        GenFxnEpilog();

        if (GenFxnSizeNeeded())
          GenNumLabel(endLabel = LabelCnt++);

        puts2(CurHeaderFooter[1]);
        CurHeaderFooter = NULL;

        if (GenFxnSizeNeeded())
          GenRecordFxnSize(CurFxnName, endLabel);



        CurFxnName = NULL;
        IdentTableLen = undoIdents; // remove all identifier names
        SyntaxStackCnt = undoSymbolsPtr; // remove all params and locals
        SyntaxStack1[SymFuncPtr] = DummyIdent;
      }

      else if (isFxn)
      {
        if (doAnnotations)
        {
          // function prototype
          DumpDecl(lastSyntaxPtr, 0);
        }
      }

      CheckRedecl(lastSyntaxPtr);

      if ((tok == ';') | (tok == '}'))
        break;

      tok = GetToken();
      continue;
    }

    //error("ParseDecl(): unexpected token %s\n", GetTokenName(tok));
    errorUnexpectedToken(tok);
  }

  tok = GetToken();
  return tok;
}

void ParseFxnParams(word tok)
{
  word base[2];
  word lastSyntaxPtr;
  word cnt = 0;
  word ellCnt = 0;

  for (;;)
  {
    lastSyntaxPtr = SyntaxStackCnt;

    if (tok == ')') /* unspecified params */
      break;

    if (!TokenStartsDeclaration(tok, 1))
    {
      if (tok == tokEllipsis)
      {
        // "..." cannot be the first parameter and
        // it can be only one
        if (!cnt || ellCnt)
          //error("ParseFxnParams(): '...' unexpected here\n");
          errorUnexpectedToken(tok);
        ellCnt++;
      }
      else
        //error("ParseFxnParams(): Unexpected token %s\n", GetTokenName(tok));
        errorUnexpectedToken(tok);
      base[0] = tok; // "..."
      base[1] = 0;
      PushSyntax2(tokIdent, AddIdent("<something>"));
      tok = GetToken();
    }
    else
    {
      if (ellCnt)
        //error("ParseFxnParams(): '...' must be the last in the parameter list\n");
        errorUnexpectedToken(tok);

      /* base type */
      tok = ParseBase(tok, base);

      /* derived type */
      tok = ParseDerived(tok);
    }

    /* base type */
    PushBase(base);



    /* Decay arrays to pointers */
    lastSyntaxPtr++; /* skip name */
    if (SyntaxStack0[lastSyntaxPtr] == '[')
    {
      word t;
      DeleteSyntax(lastSyntaxPtr, 1);
      t = SyntaxStack0[lastSyntaxPtr];
      if (t == tokNumInt || t == tokNumUint)
        DeleteSyntax(lastSyntaxPtr, 1);
      SyntaxStack0[lastSyntaxPtr] = '*';
    }
    /* "(Un)decay" functions to function pointers */
    else if (SyntaxStack0[lastSyntaxPtr] == '(')
    {
      InsertSyntax(lastSyntaxPtr, '*');
    }
    lastSyntaxPtr--; /* "unskip" name */

    cnt++;

    if (tok == ')' || tok == ',')
    {
      word t = SyntaxStack0[SyntaxStackCnt - 2];
      if (SyntaxStack0[SyntaxStackCnt - 1] == tokVoid)
      {
        // Disallow void variables. TBD!!! de-uglify
        if (t == tokIdent &&
            !(!strcmp(IdentTable + SyntaxStack1[SyntaxStackCnt - 2], "<something>") &&
              cnt == 1 && tok == ')'))
          //error("ParseFxnParams(): Cannot declare a variable ('%s') of type 'void'\n", IdentTable + SyntaxStack1[lastSyntaxPtr]);
          errorUnexpectedVoid();
      }



      if (tok == ')')
        break;

      tok = GetToken();
      continue;
    }

    //error("ParseFxnParams(): Unexpected token %s\n", GetTokenName(tok));
    errorUnexpectedToken(tok);
  }
}

void AddFxnParamSymbols(word SyntaxPtr)
{
  word i;
  unsigned paramOfs = 2 * SizeOfWord; // ret addr, xbp

  if (SyntaxPtr < 0 ||
      SyntaxPtr > SyntaxStackCnt - 3 ||
      SyntaxStack0[SyntaxPtr] != tokIdent ||
      SyntaxStack0[SyntaxPtr + 1] != '(')
    //error("Internal error: AddFxnParamSymbols(): Invalid input\n");
    errorInternal(6);

  CurFxnSyntaxPtr = SyntaxPtr;
  CurFxnLocalOfs = 0;
  CurFxnMinLocalOfs = 0;



  SyntaxPtr += 2; // skip "ident("

  for (i = SyntaxPtr; i < SyntaxStackCnt; i++)
  {
    word tok = SyntaxStack0[i];

    if (tok == tokIdent)
    {
      unsigned sz;
      word paramPtr;

      if (i + 1 >= SyntaxStackCnt)
        //error("Internal error: AddFxnParamSymbols(): Invalid input\n");
        errorInternal(7);

      if (SyntaxStack0[i + 1] == tokVoid) // "ident(void)" = no params
        break;
      if (SyntaxStack0[i + 1] == tokEllipsis) // "ident(something,...)" = no more params
        break;

      // Make sure the parameter is not an incomplete structure
      sz = GetDeclSize(i, 0);
      if (sz == 0)
        //error("Internal error: AddFxnParamSymbols(): GetDeclSize() = 0\n");
        //errorInternal(8);
        errorDecl();

      // Let's calculate this parameter's relative on-stack location
      paramPtr = SyntaxStackCnt;
      PushSyntax2(SyntaxStack0[i], SyntaxStack1[i]);
      PushSyntax2(tokLocalOfs, paramOfs);

      if (sz + SizeOfWord - 1 < sz)
      {
        //printf("h");
        errorVarSize();
      }
      sz = (sz + SizeOfWord - 1) & ~(SizeOfWord - 1u);
      if (paramOfs + sz < paramOfs)
      {
        //printf("i");
        errorVarSize();
      }
      paramOfs += sz;
      if (paramOfs > (unsigned)GenMaxLocalsSize())
      {
        //printf("j");
        errorVarSize();
      }

      // Duplicate this parameter in the symbol table
      i++;
      while (i < SyntaxStackCnt)
      {
        tok = SyntaxStack0[i];
        if (tok == tokIdent || tok == ')')
        {
          if (doAnnotations)
          {
            DumpDecl(paramPtr, 0);
          }
          if (IdentTable[SyntaxStack1[paramPtr]] == '<')
            error("Parameter name expected\n");
          CheckRedecl(paramPtr);
          i--;
          break;
        }
        else if (tok == '(')
        {
          word c = 1;
          i++;
          PushSyntax(tok);
          while (c && i < SyntaxStackCnt)
          {
            tok = SyntaxStack0[i];
            c += (tok == '(') - (tok == ')');
            PushSyntax2(SyntaxStack0[i], SyntaxStack1[i]);
            i++;
          }
        }
        else
        {
          PushSyntax2(SyntaxStack0[i], SyntaxStack1[i]);
          i++;
        }
      }
    }
    else if (tok == ')') // endof "ident(" ... ")"
      break;
    else
      //error("Internal error: AddFxnParamSymbols(): Unexpected token %s\n", GetTokenName(tok));
      errorInternal(9);
  }
}

word ParseStatement(word tok, word BrkCntTarget[2], word casesIdx)
{
/*
  labeled statements:
  + ident : statement
  + case const-expr : statement
  + default : statement

  compound statement:
  + { declaration(s)/statement(s)-opt }

  expression statement:
  + expression-opt ;

  selection statements:
  + if ( expression ) statement
  + if ( expression ) statement else statement
  + switch ( expression ) { statement(s)-opt }

  iteration statements:
  + while ( expression ) statement
  + do statement while ( expression ) ;
  + for ( expression-opt ; expression-opt ; expression-opt ) statement

  jump statements:
  + goto ident ;
  + continue ;
  + break ;
  + return expression-opt ;
*/
  word gotUnary, synPtr,  constExpr, exprVal;
  word brkCntTarget[2];
  word statementNeeded;

  do
  {
    statementNeeded = 0;

    if (tok == ';')
    {
      tok = GetToken();
    }
    else if (tok == '{')
    {
      // A new {} block begins in the function body
      word undoSymbolsPtr = SyntaxStackCnt;
      word undoLocalOfs = CurFxnLocalOfs;
      word undoIdents = IdentTableLen;
      if (doAnnotations)
      {
        GenStartCommentLine(); printf2("{\n");
      }
      ParseLevel++;
      tok = ParseBlock(BrkCntTarget, casesIdx);
      ParseLevel--;
      if (tok != '}')
        //error("ParseStatement(): '}' expected. Unexpected token %s\n", GetTokenName(tok));
        errorUnexpectedToken(tok);
      UndoNonLabelIdents(undoIdents); // remove all identifier names, except those of labels
      SyntaxStackCnt = undoSymbolsPtr; // remove all params and locals
      CurFxnLocalOfs = undoLocalOfs; // destroy on-stack local variables
      if (doAnnotations)
      {
        GenStartCommentLine(); printf2("}\n");
      }
      tok = GetToken();
    }
    else if (tok == tokReturn)
    {
      // DONE: functions returning void vs non-void
      word retVoid = CurFxnReturnExprTypeSynPtr >= 0 &&
                    SyntaxStack0[CurFxnReturnExprTypeSynPtr] == tokVoid;
      if (doAnnotations)
      {
        GenStartCommentLine(); printf2("return\n");
      }
      tok = GetToken();
      if (tok == ';')
      {
        gotUnary = 0;
        if (!retVoid)
          //error("ParseStatement(): missing return value\n");
          errorUnexpectedToken(tok);
      }
      else
      {
        if (retVoid)
          //error("Error: ParseStatement(): cannot return a value from a function returning 'void'\n");
          errorUnexpectedToken(tok);
        if ((tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0)) != ';')
          //error("ParseStatement(): ';' expected\n");
          errorUnexpectedToken(tok);
        if (gotUnary)
          //error("ParseStatement(): cannot return a value of type 'void'\n");

          // Bar void
          nonVoidTypeCheck(synPtr);
      }
      if (gotUnary)
      {


        decayArray(&synPtr, 0);


        {
          word castSize = GetDeclSize(CurFxnReturnExprTypeSynPtr, 1);


          // If return value (per function declaration) is a scalar type smaller than machine word,
          // properly zero- or sign-extend the returned value to machine word size.
          // TBD??? Move this cast to the caller?
          if (castSize != SizeOfWord && castSize != GetDeclSize(synPtr, 1))
          {
            if (constExpr)
            {
              switch (castSize)
              {
              case 1:
                exprVal &= 0xFFu;
                break;
              case -1:
                if ((exprVal &= 0xFFu) >= 0x80)
                  exprVal -= 0x100;
                break;
              case 2:
                exprVal &= 0xFFFFu;
                break;
              case -2:
                if ((exprVal &= 0xFFFFu) >= 0x8000)
                  exprVal -= 0x10000;
                break;
              }
            }
            else
            {
              switch (castSize)
              {
              case 1:
                push(tokUChar);
                break;
              case -1:
                push(tokSChar);
                break;
              case 2:
                push(tokUShort);
                break;
              case -2:
                push(tokShort);
                break;
              }
            }
          }
        }
        if (constExpr)
          stack[0][1] = exprVal;
        push(tokReturn); // value produced by generated code is used
        GenExpr();
      }
      tok = GetToken();
      // If this return is the last statement in the function, the epilogue immediately
      // follows and there's no need to jump to it.
      if (!(tok == '}' && ParseLevel == 1 && !IsMain))
        GenJumpUncond(CurFxnEpilogLabel);
    }
    else if (tok == tokWhile)
    {
      word labelBefore = LabelCnt++;
      word labelAfter = LabelCnt++;
      word forever = 0;
      if (doAnnotations)
      {
        GenStartCommentLine(); printf2("while\n");
      }

      tok = GetToken();
      if (tok != '(')
        //error("ParseStatement(): '(' expected after 'while'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();
      if ((tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0)) != ')')
        //error("ParseStatement(): ')' expected after 'while ( expression'\n");
        errorUnexpectedToken(tok);

      if (!gotUnary)
        //error("ParseStatement(): expression expected in 'while ( expression )'\n");
        errorUnexpectedToken(tok);

      // DONE: void control expressions
      //error("ParseStatement(): unexpected 'void' expression in 'while ( expression )'\n");
      // Bar void and struct/union
      scalarTypeCheck(synPtr);

      GenNumLabel(labelBefore);

      if (constExpr)
      {

        // Special cases for while(0) and while(1)
        if (!(forever = truncInt(exprVal)))
          GenJumpUncond(labelAfter);
      }
      else
      {
        switch (stack[sp - 1][0])
        {
        case '<':
        case '>':
        case tokEQ:
        case tokNEQ:
        case tokLEQ:
        case tokGEQ:
        case tokULess:
        case tokUGreater:
        case tokULEQ:
        case tokUGEQ:
          push2(tokIfNot, labelAfter);
          GenExpr();
          break;
        default:

          push(tokReturn); // value produced by generated code is used
          GenExpr();
          GenJumpIfZero(labelAfter);
          break;
        }
      }

      tok = GetToken();
      brkCntTarget[0] = labelAfter; // break target
      brkCntTarget[1] = labelBefore; // continue target
      tok = ParseStatement(tok, brkCntTarget, casesIdx);

      // Special case for while(0)
      if (!(constExpr && !forever))
        GenJumpUncond(labelBefore);
      GenNumLabel(labelAfter);
    }
    else if (tok == tokDo)
    {
      word labelBefore = LabelCnt++;
      word labelWhile = LabelCnt++;
      word labelAfter = LabelCnt++;
      if (doAnnotations)
      {
        GenStartCommentLine(); printf2("do\n");
      }
      GenNumLabel(labelBefore);

      tok = GetToken();
      brkCntTarget[0] = labelAfter; // break target
      brkCntTarget[1] = labelWhile; // continue target
      tok = ParseStatement(tok, brkCntTarget, casesIdx);
      if (tok != tokWhile)
        //error("ParseStatement(): 'while' expected after 'do statement'\n");
        errorUnexpectedToken(tok);

      if (doAnnotations)
      {
        GenStartCommentLine(); printf2("while\n");
      }
      tok = GetToken();
      if (tok != '(')
        //error("ParseStatement(): '(' expected after 'while'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();
      if ((tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0)) != ')')
        //error("ParseStatement(): ')' expected after 'while ( expression'\n");
        errorUnexpectedToken(tok);

      if (!gotUnary)
        //error("ParseStatement(): expression expected in 'while ( expression )'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();
      if (tok != ';')
        //error("ParseStatement(): ';' expected after 'do statement while ( expression )'\n");
        errorUnexpectedToken(tok);

      // DONE: void control expressions
      //error("ParseStatement(): unexpected 'void' expression in 'while ( expression )'\n");
      // Bar void and struct/union
      scalarTypeCheck(synPtr);

      GenNumLabel(labelWhile);

      if (constExpr)
      {

        // Special cases for while(0) and while(1)
        if (truncInt(exprVal))
          GenJumpUncond(labelBefore);
      }
      else
      {
        switch (stack[sp - 1][0])
        {
        case '<':
        case '>':
        case tokEQ:
        case tokNEQ:
        case tokLEQ:
        case tokGEQ:
        case tokULess:
        case tokUGreater:
        case tokULEQ:
        case tokUGEQ:
          push2(tokIf, labelBefore);
          GenExpr();
          break;
        default:

          push(tokReturn); // value produced by generated code is used
          GenExpr();
          GenJumpIfNotZero(labelBefore);
          break;
        }
      }

      GenNumLabel(labelAfter);

      tok = GetToken();
    }
    else if (tok == tokIf)
    {
      word labelAfterIf = LabelCnt++;
      word labelAfterElse = LabelCnt++;
      if (doAnnotations)
      {
        GenStartCommentLine(); printf2("if\n");
      }

      tok = GetToken();
      if (tok != '(')
        //error("ParseStatement(): '(' expected after 'if'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();
      if ((tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0)) != ')')
        //error("ParseStatement(): ')' expected after 'if ( expression'\n");
        errorUnexpectedToken(tok);

      if (!gotUnary)
        //error("ParseStatement(): expression expected in 'if ( expression )'\n");
        errorUnexpectedToken(tok);

      // DONE: void control expressions
      //error("ParseStatement(): unexpected 'void' expression in 'if ( expression )'\n");
      // Bar void and struct/union
      scalarTypeCheck(synPtr);

      if (constExpr)
      {

        // Special cases for if(0) and if(1)
        if (!truncInt(exprVal))
          GenJumpUncond(labelAfterIf);
      }
      else
      {
        switch (stack[sp - 1][0])
        {
        case '<':
        case '>':
        case tokEQ:
        case tokNEQ:
        case tokLEQ:
        case tokGEQ:
        case tokULess:
        case tokUGreater:
        case tokULEQ:
        case tokUGEQ:
          push2(tokIfNot, labelAfterIf);
          GenExpr();
          break;
        default:

          push(tokReturn); // value produced by generated code is used
          GenExpr();
          GenJumpIfZero(labelAfterIf);
          break;
        }
      }

      tok = GetToken();
      tok = ParseStatement(tok, BrkCntTarget, casesIdx);

      // DONE: else
      if (tok == tokElse)
      {
        GenJumpUncond(labelAfterElse);
        GenNumLabel(labelAfterIf);
        if (doAnnotations)
        {
          GenStartCommentLine(); printf2("else\n");
        }
        tok = GetToken();
        tok = ParseStatement(tok, BrkCntTarget, casesIdx);
        GenNumLabel(labelAfterElse);
      }
      else
      {
        GenNumLabel(labelAfterIf);
      }
    }
    else if (tok == tokFor)
    {
      word labelBefore = LabelCnt++;
      word labelExpr3 = LabelCnt++;
      word labelBody = LabelCnt++;
      word labelAfter = LabelCnt++;
      word cond = -1;
      static word expr3Stack[STACK_SIZE >> 1][2];
      static word expr3Sp;

      if (doAnnotations)
      {
        GenStartCommentLine(); printf2("for\n");
      }
      tok = GetToken();
      if (tok != '(')
        //error("ParseStatement(): '(' expected after 'for'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();

      {
        if ((tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0)) != ';')
          //error("ParseStatement(): ';' expected after 'for ( expression'\n");
          errorUnexpectedToken(tok);
        if (gotUnary)
        {
          GenExpr();
        }
        tok = GetToken();
      }

      GenNumLabel(labelBefore);
      if ((tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0)) != ';')
        //error("ParseStatement(): ';' expected after 'for ( expression ; expression'\n");
        errorUnexpectedToken(tok);
      if (gotUnary)
      {
        // DONE: void control expressions
        //error("ParseStatement(): unexpected 'void' expression in 'for ( ; expression ; )'\n");
        // Bar void and struct/union
        scalarTypeCheck(synPtr);

        if (constExpr)
        {

          // Special cases for for(...; 0; ...) and for(...; 1; ...)
          cond = truncInt(exprVal) != 0;
        }
        else
        {
          switch (stack[sp - 1][0])
          {
          case '<':
          case '>':
          case tokEQ:
          case tokNEQ:
          case tokLEQ:
          case tokGEQ:
          case tokULess:
          case tokUGreater:
          case tokULEQ:
          case tokUGEQ:
            push2(tokIfNot, labelAfter);
            GenExpr();
            break;
          default:

            push(tokReturn); // value produced by generated code is used
            GenExpr();
            GenJumpIfZero(labelAfter);
            break;
          }
        }
      }
      else
      {
        // Special case for for(...; ; ...)
        cond = 1;
      }
      if (!cond)
        // Special case for for(...; 0; ...)
        GenJumpUncond(labelAfter);

      tok = GetToken();
      if ((tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0)) != ')')
        //error("ParseStatement(): ')' expected after 'for ( expression ; expression ; expression'\n");
        errorUnexpectedToken(tok);

      // Try to reorder expr3 with body to reduce the number of jumps, favor small expr3's
      if (gotUnary && sp <= 16 && (unsigned)sp <= division(sizeof expr3Stack , sizeof expr3Stack[0]) - expr3Sp)
      {
        word cnt = sp;
        // Stash the stack containing expr3
        memcpy(expr3Stack + expr3Sp, stack, cnt * sizeof stack[0]);
        expr3Sp += cnt;

        // Body
        tok = GetToken();
        brkCntTarget[0] = labelAfter; // break target
        brkCntTarget[1] = labelExpr3; // continue target
        tok = ParseStatement(tok, brkCntTarget, casesIdx);

        // Unstash expr3 and generate code for it
        expr3Sp -= cnt;
        memcpy(stack, expr3Stack + expr3Sp, cnt * sizeof stack[0]);
        sp = cnt;
        GenNumLabel(labelExpr3);
        GenExpr();

        // Special case for for(...; 0; ...)
        if (cond)
          GenJumpUncond(labelBefore);
      }
      else
      {
        if (gotUnary)
        {
          GenJumpUncond(labelBody);
          // expr3
          GenNumLabel(labelExpr3);
          GenExpr();
          GenJumpUncond(labelBefore);
          GenNumLabel(labelBody);
        }

        // Body
        tok = GetToken();
        brkCntTarget[0] = labelAfter; // break target
        brkCntTarget[1] = gotUnary ? labelExpr3 : (cond ? labelBefore : labelAfter); // continue target
        tok = ParseStatement(tok, brkCntTarget, casesIdx);

        // Special case for for(...; 0; ...)
        if (brkCntTarget[1] != labelAfter)
          GenJumpUncond(brkCntTarget[1]);
      }

      GenNumLabel(labelAfter);


    }
    else if (tok == tokBreak)
    {
      if (doAnnotations)
      {
        GenStartCommentLine(); printf2("break\n");
      }
      if ((tok = GetToken()) != ';')
        //error("ParseStatement(): ';' expected\n");
        errorUnexpectedToken(tok);
      tok = GetToken();
      if (BrkCntTarget == NULL)
        //error("ParseStatement(): 'break' must be within 'while', 'for' or 'switch' statement\n");
        errorCtrlOutOfScope();
      GenJumpUncond(BrkCntTarget[0]);
    }
    else if (tok == tokCont)
    {
      if (doAnnotations)
      {
        GenStartCommentLine(); printf2("continue\n");
      }
      if ((tok = GetToken()) != ';')
        //error("ParseStatement(): ';' expected\n");
        errorUnexpectedToken(tok);
      tok = GetToken();
      if (BrkCntTarget == NULL || BrkCntTarget[1] == 0)
        //error("ParseStatement(): 'continue' must be within 'while' or 'for' statement\n");
        errorCtrlOutOfScope();
      GenJumpUncond(BrkCntTarget[1]);
    }
    else if (tok == tokSwitch)
    {
      word undoCases = CasesCnt;
      word brkLabel = LabelCnt++;
      word lbl = LabelCnt++;
      word i;
      if (doAnnotations)
      {
        GenStartCommentLine(); printf2("switch\n");
      }

      tok = GetToken();
      if (tok != '(')
        //error("ParseStatement(): '(' expected after 'switch'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();
      if ((tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0)) != ')')
        //error("ParseStatement(): ')' expected after 'switch ( expression'\n");
        errorUnexpectedToken(tok);

      if (!gotUnary)
        //error("ParseStatement(): expression expected in 'switch ( expression )'\n");
        errorUnexpectedToken(tok);

      // DONE: void control expressions
      //error("ParseStatement(): unexpected 'void' expression in 'switch ( expression )'\n");
      anyIntTypeCheck(synPtr);

      push(tokReturn); // value produced by generated code is used
      GenExpr();

      tok = GetToken();

      // Skip the code for the cases
      GenJumpUncond(lbl);

      brkCntTarget[0] = brkLabel; // break target
      brkCntTarget[1] = 0; // continue target
      if (BrkCntTarget)
      {
        // Preserve the continue target
        brkCntTarget[1] = BrkCntTarget[1]; // continue target
      }

      // Reserve a slot in the case table for the default label
      AddCase(0, 0);

      tok = ParseStatement(tok, brkCntTarget, CasesCnt);

      // If there's no default target, will use the break target as default
      if (!Cases[undoCases][1])
        Cases[undoCases][1] = brkLabel;

      // End of switch reached (not via break), skip conditional jumps
      GenJumpUncond(brkLabel);
      // Generate conditional jumps
      GenNumLabel(lbl);
      for (i = undoCases + 1; i < CasesCnt; i++)
      {
        GenJumpIfEqual(Cases[i][0], Cases[i][1]);
      }
      // If none of the cases matches, take the default case
      if (Cases[undoCases][1] != brkLabel)
        GenJumpUncond(Cases[undoCases][1]);
      GenNumLabel(brkLabel); // break label

      CasesCnt = undoCases;
    }
    else if (tok == tokCase)
    {
      word i;
      if (doAnnotations)
      {
        GenStartCommentLine(); printf2("case\n");
      }

      if (!casesIdx)
        //error("ParseStatement(): 'case' must be within 'switch' statement\n");
        errorCtrlOutOfScope();

      tok = GetToken();
      if ((tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0)) != ':')
        //error("ParseStatement(): ':' expected after 'case expression'\n");
        errorUnexpectedToken(tok);

      if (!gotUnary)
        errorUnexpectedToken(tok);

      anyIntTypeCheck(synPtr);
      if (!constExpr)
        //error("ParseStatement(): constant integer expression expected in 'case expression :'\n");
        errorNotConst();

      // Check for dups
      exprVal = truncInt(exprVal);
      for (i = casesIdx; i < CasesCnt; i++)
        if (Cases[i][0] == exprVal)
          error("Duplicate case value\n");

      AddCase(exprVal, LabelCnt);
      GenNumLabel(LabelCnt++); // case exprVal:

      tok = GetToken();

      // a statement is needed after "case:"
      statementNeeded = 1;
    }
    else if (tok == tokDefault)
    {
      if (doAnnotations)
      {
        GenStartCommentLine(); printf2("default\n");
      }

      if (!casesIdx)
        //error("ParseStatement(): 'default' must be within 'switch' statement\n");
        errorCtrlOutOfScope();

      if (Cases[casesIdx - 1][1])
        //error("ParseStatement(): only one 'default' allowed in 'switch'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();
      if (tok != ':')
        //error("ParseStatement(): ':' expected after 'default'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();

      GenNumLabel(Cases[casesIdx - 1][1] = LabelCnt++); // default:

      // a statement is needed after "default:"
      statementNeeded = 1;
    }
    else if (tok == tok_Asm)
    {
      tok = GetToken();
      if (tok != '(')
        //error("ParseStatement(): '(' expected after 'asm'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();
      if (tok != tokLitStr)
        //error("ParseStatement(): string literal expression expected in 'asm ( expression )'\n");
        errorUnexpectedToken(tok);

      do
      {
        GetString('"', 0, 'a');
        tok = GetToken();
      } while (tok == tokLitStr); // concatenate adjacent string literals
      printf2("\n");

      if (tok != ')')
        //error("ParseStatement(): ')' expected after 'asm ( expression'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();
      if (tok != ';')
        //error("ParseStatement(): ';' expected after 'asm ( expression )'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();
    }
    else if (tok == tokGoto)
    {
      if ((tok = GetToken()) != tokIdent)
        errorUnexpectedToken(tok);
      GenStartCommentLine(); 
      if (doAnnotations)
      {
        printf2("goto ");
        printf2(TokenIdentName);
        printf2("\n");
      }
      GenJumpUncond(AddGotoLabel(TokenIdentName, 0));
      if ((tok = GetToken()) != ';')
        errorUnexpectedToken(tok);
      tok = GetToken();
    }
    else
    {
      tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, tokGotoLabel, 0);
      if (tok == tokGotoLabel)
      {
        // found a label
        if (doAnnotations)
        {
          GenStartCommentLine(); 
          printf2(IdentTable + stack[0][1]);
          printf2(":\n");
        }
        GenNumLabel(AddGotoLabel(IdentTable + stack[0][1], 1));
        // a statement is needed after "label:"
        statementNeeded = 1;
      }
      else
      {
        if (tok != ';')
          //error("ParseStatement(): ';' expected\n");
          errorUnexpectedToken(tok);
        if (gotUnary)
          GenExpr();
      }
      tok = GetToken();
    }
  } while (statementNeeded);

  return tok;
}

// TBD!!! think of ways of getting rid of casesIdx
word ParseBlock(word BrkCntTarget[2], word casesIdx)
{
  word tok = GetToken();

  // Catch redeclarations of function parameters by not
  // beginning a new scope if this block begins a function
  // (the caller of ParseBlock() must've begun a new scope
  // already, before the function parameters).
  if (BrkCntTarget == BrkCntTargetFxn)
    BrkCntTarget = NULL;
  else
  // Otherwise begin a new scope.
    PushSyntax('#');

  for (;;)
  {
    if (tok == 0)
      return tok;

    if (tok == '}' && ParseLevel > 0)
      return tok;

    if (TokenStartsDeclaration(tok, 0))
    {
      tok = ParseDecl(tok, NULL, 0, 1);

    }
    else if (ParseLevel > 0 || tok == tok_Asm)
    {
      tok = ParseStatement(tok, BrkCntTarget, casesIdx);
    }
    else
      //error("ParseBlock(): Unexpected token %s\n", GetTokenName(tok));
      errorUnexpectedToken(tok);
  }
}



int main() 
{
  printf("BCC Compiler\n");
  // Run-time initializer for SyntaxStack0[] to reduce
  // executable file size (SyntaxStack0[] will be in .bss)
  static unsigned char SyntaxStackInit[] =
  {
    tokVoid,     // SymVoidSynPtr
    tokInt,      // SymIntSynPtr
    tokUnsigned, // SymUintSynPtr
    tokVoid,     // SymWideCharSynPtr
    tokFloat,    // SymFloatSynPtr
    tokIdent,    // SymFuncPtr
    '[',
    tokNumUint,
    ']',
    tokChar
  }; // SyntaxStackCnt must be initialized to the number of elements in SyntaxStackInit[]
  memcpy(SyntaxStack0, SyntaxStackInit, sizeof SyntaxStackInit);
  SyntaxStackCnt = 10;

  SyntaxStack1[SymFuncPtr] = DummyIdent = AddIdent("");

  GenInit();

  // TODO: add BDOS path to filenames if not absolute path
  compileOS = 0;
  compileUserBDOS = 1;

  char infilename[64];
  BDOS_GetArgN(1, infilename);
  if (infilename[0] == 0)
  {
    strcat(infilename, BDOS_GetPath());
    if (infilename[strlen(infilename)-1] != '/')
    {
        strcat(infilename, "/");
    }
    strcat(infilename, "CODE.C");
    //strcpy(infilename, "/TEST/TEST.C");
  }
  if (infilename[0] != '/')
  {
    char bothPath[64];
    bothPath[0] = 0;
    strcat(bothPath, BDOS_GetPath());
    if (bothPath[strlen(bothPath)-1] != '/')
    {
        strcat(bothPath, "/");
    }
    strcat(bothPath, infilename);
    strcpy(infilename, bothPath);
  }

  if (FileCnt == 0)
  {
    // If it's none of the known options,
    // assume it's the source code file name
    if (strlen(infilename) > MAX_FILE_NAME_LEN)
    {
      //error("File name too long\n");
      errorFileName();
    }
    strcpy(FileNames[0], infilename);
    Files[0] = fopen(FileNames[0], 0);
    if (fcanopen(Files[0]) == EOF)
    {
      //error("Cannot open file \"%s\"\n", FileNames[0]);
      errorFile(FileNames[0]);
    }
    else
    {
      //printf("Opened input file\n");
    }
    LineNos[0] = LineNo;
    LinePoss[0] = LinePos;
    FileCnt++;
  }

  char outfilename[64];
  BDOS_GetArgN(2, outfilename);
  if (outfilename[0] == 0)
  {
    strcat(outfilename, BDOS_GetPath());
    if (outfilename[strlen(outfilename)-1] != '/')
    {
        strcat(outfilename, "/");
    }
    strcat(outfilename, "OUT.ASM");
    //strcpy(outfilename, "/TEST/A.OUT");
  }
  if (outfilename[0] != '/')
  {
    char bothPath[64];
    bothPath[0] = 0;
    strcat(bothPath, BDOS_GetPath());
    if (bothPath[strlen(bothPath)-1] != '/')
    {
        strcat(bothPath, "/");
    }
    strcat(bothPath, outfilename);
    strcpy(outfilename, bothPath);
  }

  if (FileCnt == 1 && OutFile == NULL)
  {
    // This should be the output file name
    OutFile = fopen(outfilename, 1);
    if (fcanopen(OutFile) == EOF)
    {
      //error("Cannot open output file \"%s\"\n", argv[i]);
      errorFile(outfilename);
    }
    else
    {
      //printf("Opened output file\n");
    }
  }

  if (!FileCnt)
    error("Input file not specified\n");
  if (!OutFile)
    error("Output file not specified\n");


  GenInitFinalize();

  // Define a few macros useful for conditional compilation
  /*
  DefineMacro("__SMALLER_C__", "0x0100");
  DefineMacro("__SMALLER_C_32__", "");
  DefineMacro("__SMALLER_C_SCHAR__", "");
  */


  // populate CharQueue[] with the initial file characters
  ShiftChar();

  puts2(FileHeader);

  // compile
  ParseBlock(NULL, 0);

  GenFin();

  if (doAnnotations)
  {
    DumpSynDecls();
    DumpMacroTable();
    DumpIdentTable();
  }
  

  /*
  GenStartCommentLine(); 
  printf2("Next label number: ");
  printd2(LabelCnt);
  printf2("\n");
  */

  //GenStartCommentLine(); 
  printf("Finished compiling\n");

  if (OutFile)
    fclose(OutFile);

  return 'q';
}

void interrupt()
{
}