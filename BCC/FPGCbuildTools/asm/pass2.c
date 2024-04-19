/*****************************************************************************/
/*                                                                           */
/*                           ASM (B322 Assembler)                            */
/*                                                                           */
/*                           Assembler for B322                              */
/*                      Contains all pass 2 functions                        */
/*                                                                           */
/*****************************************************************************/

word getNumberForLabel(char* labelName)
{
    word bdosOffset = USERBDOS_OFFSET;
    word i;
    for (i = 0; i < labelListIndex; i++)
    {
        if (strcmp(labelName, labelListName[i]) == 0)
        {
            return (labelListLineNumber[i] + bdosOffset);
        }
    }
    bdos_print("Could not find label: ");
    bdos_print(labelName);
    bdos_print("\n");
    exit(1);
    return 0;
}

void pass2Halt(char* outputAddr, char* outputCursor)
{
    char instr = 0xFFFFFFFF;
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2Read(char* outputAddr, char* outputCursor)
{
    word instr = 0xE0000000;

    word arg1num = getNumberAtArg(1);
    // arg1 should fit in 16 bits (signed numbers have 1 bit less)
    word bitsCheck = 16;
    if (arg1num < 0)
    {
        bitsCheck = 15;
    }
    if ((MATH_abs(arg1num) >> bitsCheck) > 0)
    {
        bdos_print("READ: arg1 is >16 bits\n");
        exit(1);
    }

    word mask = 0xffff;
    instr += ((arg1num & mask) << 12);

    // arg2
    char arg2buf[16];
    getArgPos(2, arg2buf);
    // arg2 should be a reg
    if (arg2buf[0] != 'r')
    {
        bdos_print("READ: arg2 not a reg\n");
        exit(1);
    }
    word arg2num = strToInt(&arg2buf[1]);

    instr += (arg2num << 8);

    // arg3
    char arg3buf[16];
    getArgPos(3, arg3buf);
    // arg3 should be a reg
    if (arg3buf[0] != 'r')
    {
        bdos_print("READ: arg3 not a reg\n");
        exit(1);
    }
    word arg3num = strToInt(&arg3buf[1]);

    instr += arg3num;

    // write to mem
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2Write(char* outputAddr, char* outputCursor)
{
    word instr = 0xD0000000;

    word arg1num = getNumberAtArg(1);
    // arg1 should fit in 16 bits (signed numbers have 1 bit less)
    word bitsCheck = 16;
    if (arg1num < 0)
    {
        bitsCheck = 15;
    }
    if ((MATH_abs(arg1num) >> bitsCheck) > 0)
    {
        bdos_print("READ: arg1 is >16 bits\n");
        exit(1);
    }

    word mask = 0xffff;
    instr += ((arg1num & mask) << 12);

    // arg2
    char arg2buf[16];
    getArgPos(2, arg2buf);
    // arg2 should be a reg
    if (arg2buf[0] != 'r')
    {
        bdos_print("WRITE: arg2 not a reg\n");
        exit(1);
    }
    word arg2num = strToInt(&arg2buf[1]);

    instr += (arg2num << 8);

    // arg3
    char arg3buf[16];
    getArgPos(3, arg3buf);
    // arg3 should be a reg
    if (arg3buf[0] != 'r')
    {
        bdos_print("WRITE: arg3 not a reg\n");
        exit(1);
    }
    word arg3num = strToInt(&arg3buf[1]);

    instr += (arg3num << 4);

    // write to mem
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2Readintid(char* outputAddr, char* outputCursor)
{
    word instr = 0xC0000000;

    // arg1
    char arg1buf[16];
    getArgPos(1, arg1buf);
    // arg1 should be a reg
    if (arg1buf[0] != 'r')
    {
        bdos_print("READINTID: arg1 not a reg\n");
        exit(1);
    }
    word arg1num = strToInt(&arg1buf[1]);

    instr += arg1num;

    // write to mem
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2Push(char* outputAddr, char* outputCursor)
{
    word instr = 0xB0000000;

    // arg1
    char arg1buf[16];
    getArgPos(1, arg1buf);
    // arg1 should be a reg
    if (arg1buf[0] != 'r')
    {
        bdos_print("PUSH: arg1 not a reg\n");
        exit(1);
    }
    word arg1num = strToInt(&arg1buf[1]);

    instr += (arg1num << 4);

    // write to mem
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2Pop(char* outputAddr, char* outputCursor)
{
    word instr = 0xA0000000;

    // arg1
    char arg1buf[16];
    getArgPos(1, arg1buf);
    // arg1 should be a reg
    if (arg1buf[0] != 'r')
    {
        bdos_print("POP: arg1 not a reg\n");
        exit(1);
    }
    word arg1num = strToInt(&arg1buf[1]);

    instr += arg1num;

    // write to mem
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2Jump(char* outputAddr, char* outputCursor)
{
    word instr = 0x90000000;
    
    // check if jump to label
    // if yes, replace label with line number
    char arg1buf[LABEL_NAME_SIZE+1];
    getArgPos(1, arg1buf);
    word arg1bufLen = strlen(arg1buf);
    word argIsLabel = 0;
    word i;
    for (i = 0; i < arg1bufLen; i++)
    {
        if (arg1buf[i] < '0' || arg1buf[i] > '9')
        {
            argIsLabel = 1;
            break;
        }
    }

    word arg1num = 0;
    if (argIsLabel)
    {
        arg1num = getNumberForLabel(arg1buf);
    }
    else
    {
        arg1num = getNumberAtArg(1);
    }

    // arg1 should fit in 27 bits
    if (((unsigned)arg1num >> 27) > 0)
    {
        bdos_print("JUMPO: arg1 is >27 bits\n");
        exit(1);
    }

    instr += (arg1num << 1);

    // write to mem
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2Jumpo(char* outputAddr, char* outputCursor)
{
    word instr = 0x90000000;

    word arg1num = getNumberAtArg(1);

    // arg1 should fit in 27 bits
    if (((unsigned)arg1num >> 27) > 0)
    {
        bdos_print("JUMPO: arg1 is >27 bits\n");
        exit(1);
    }

    instr += (arg1num << 1);
    instr ^= 1;

    // write to mem
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2Jumpr(char* outputAddr, char* outputCursor)
{
    word instr = 0x80000000;

    word arg1num = getNumberAtArg(1);

    // arg1 should fit in 16 bits
    if (((unsigned)arg1num >> 16) > 0)
    {
        bdos_print("JUMPR: arg1 is >16 bits\n");
        exit(1);
    }

    instr += (arg1num << 12);

    // arg2
    char arg2buf[16];
    getArgPos(2, arg2buf);
    // arg2 should be a reg
    if (arg2buf[0] != 'r')
    {
        bdos_print("JUMPR: arg2 not a reg\n");
        exit(1);
    }
    word arg2num = strToInt(&arg2buf[1]);

    instr += (arg2num << 4);

    // write to mem
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2Jumpro(char* outputAddr, char* outputCursor)
{
    bdos_print("JUMPRO: unimplemented\n");
    exit(1);
    return;
}

void pass2BranchBase(char* outputAddr, char* outputCursor, word branchOpCode, word branchSigned)
{
    word instr = 0x60000000;

    // arg1
    char arg1buf[16];
    getArgPos(1, arg1buf);
    // arg1 should be a reg
    if (arg1buf[0] != 'r')
    {
        bdos_print("BEQ: arg1 not a reg\n");
        exit(1);
    }
    word arg1num = strToInt(&arg1buf[1]);

    instr += (arg1num << 8);

    // arg2
    char arg2buf[16];
    getArgPos(2, arg2buf);
    // arg2 should be a reg
    if (arg2buf[0] != 'r')
    {
        bdos_print("BEQ: arg2 not a reg\n");
        exit(1);
    }
    word arg2num = strToInt(&arg2buf[1]);

    instr += (arg2num << 4);


    // arg3
    word arg3num = getNumberAtArg(3);
    // arg3 should fit in 16 bits (signed numbers have 1 bit less)
    word bitsCheck = 16;
    if (branchSigned)
    {
        bitsCheck = 15;
    }
    if ((MATH_abs(arg3num) >> bitsCheck) > 0)
    {
        bdos_print("READ: arg3 is >16 bits\n");
        exit(1);
    }

    word mask = 0xffff;
    instr += ((arg3num & mask) << 12);

    // opcode
    instr += (branchOpCode << 1);

    // signed bit
    if (branchSigned)
    {
        instr ^= 1;
    }

    // write to mem
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}


void pass2Beq(char* outputAddr, char* outputCursor)
{
    pass2BranchBase(outputAddr, outputCursor, 0x0, 0);
}

void pass2Bgt(char* outputAddr, char* outputCursor)
{
    pass2BranchBase(outputAddr, outputCursor, 0x1, 0);
}

void pass2Bgts(char* outputAddr, char* outputCursor)
{
    pass2BranchBase(outputAddr, outputCursor, 0x1, 1);
}

void pass2Bge(char* outputAddr, char* outputCursor)
{
    pass2BranchBase(outputAddr, outputCursor, 0x2, 0);
}

void pass2Bges(char* outputAddr, char* outputCursor)
{
    pass2BranchBase(outputAddr, outputCursor, 0x2, 1);
}

void pass2Bne(char* outputAddr, char* outputCursor)
{
    pass2BranchBase(outputAddr, outputCursor, 0x4, 0);
}

void pass2Blt(char* outputAddr, char* outputCursor)
{
    pass2BranchBase(outputAddr, outputCursor, 0x5, 0);
}

void pass2Blts(char* outputAddr, char* outputCursor)
{
    pass2BranchBase(outputAddr, outputCursor, 0x5, 1);
}

void pass2Ble(char* outputAddr, char* outputCursor)
{
    pass2BranchBase(outputAddr, outputCursor, 0x6, 0);
}

void pass2Bles(char* outputAddr, char* outputCursor)
{
    pass2BranchBase(outputAddr, outputCursor, 0x6, 1);
}

void pass2Savpc(char* outputAddr, char* outputCursor)
{
    word instr = 0x50000000;

    // arg1
    char arg1buf[16];
    getArgPos(1, arg1buf);
    // arg1 should be a reg
    if (arg1buf[0] != 'r')
    {
        bdos_print("SAVPC: arg1 not a reg\n");
        exit(1);
    }
    word arg1num = strToInt(&arg1buf[1]);

    instr += arg1num;

    // write to mem
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2Reti(char* outputAddr, char* outputCursor)
{
    word instr = 0x40000000;
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2Ccache(char* outputAddr, char* outputCursor)
{
    word instr = 0x70000000;
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2ArithBase(char* outputAddr, char* outputCursor, word arithOpCode)
{
    word instr = 0;

    // opcode
    instr += (arithOpCode << 24);

    // arg1
    char arg1buf[16];
    getArgPos(1, arg1buf);
    // arg1 should be a reg
    if (arg1buf[0] != 'r')
    {
        bdos_print("ARITH: arg1 not a reg\n");
        exit(1);
    }
    word arg1num = strToInt(&arg1buf[1]);

    // Add arg1num when arg2 is known to be a const or not

    // arg2
    char arg2buf[16];
    getArgPos(2, arg2buf);
    word arg2num = 0;
    // if arg2 is a const
    if (arg2buf[0] != 'r')
    {
        arg2num= getNumberAtArg(2);

        // arg2 should fit in 16 bits (signed numbers have 1 bit less)
        word bitsCheck = 16;
        if (arg2num < 0)
        {
            bitsCheck = 15;
        }
        if ((MATH_abs(arg2num) >> bitsCheck) > 0)
        {
            bdos_print("ARITH: arg2 is >16 bits\n");
            exit(1);
        }

        word mask = 0xffff;
        instr += ((arg2num & mask) << 8);
        instr ^= (1 << 28); // set constant bit
        instr += (arg1num << 4);
    }
    else // arg2 is a reg
    {
        arg2num = strToInt(&arg2buf[1]);
        instr += (arg2num << 4);
        instr += (arg1num << 8);
    }

    // arg3
    char arg3buf[16];
    getArgPos(3, arg3buf);
    // arg3 should be a reg
    if (arg3buf[0] != 'r')
    {
        bdos_print("ARITH: arg3 not a reg\n");
        exit(1);
    }
    word arg3num = strToInt(&arg3buf[1]);

    instr += arg3num;

    // write to mem
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2Or(char* outputAddr, char* outputCursor)
{
    pass2ArithBase(outputAddr, outputCursor, 0x0);
}

void pass2And(char* outputAddr, char* outputCursor)
{
    pass2ArithBase(outputAddr, outputCursor, 0x1);
}

void pass2Xor(char* outputAddr, char* outputCursor)
{
    pass2ArithBase(outputAddr, outputCursor, 0x2);
}

void pass2Add(char* outputAddr, char* outputCursor)
{
    pass2ArithBase(outputAddr, outputCursor, 0x3);
}

void pass2Sub(char* outputAddr, char* outputCursor)
{
    pass2ArithBase(outputAddr, outputCursor, 0x4);
}

void pass2Shiftl(char* outputAddr, char* outputCursor)
{
    pass2ArithBase(outputAddr, outputCursor, 0x5);
}

void pass2Shiftr(char* outputAddr, char* outputCursor)
{
    pass2ArithBase(outputAddr, outputCursor, 0x6);
}

void pass2Shiftrs(char* outputAddr, char* outputCursor)
{
    pass2ArithBase(outputAddr, outputCursor, 0xe);
}

void pass2Mults(char* outputAddr, char* outputCursor)
{
    pass2ArithBase(outputAddr, outputCursor, 0x8);
}

void pass2Multu(char* outputAddr, char* outputCursor)
{
    pass2ArithBase(outputAddr, outputCursor, 0x9);
}

void pass2Multfp(char* outputAddr, char* outputCursor)
{
    pass2ArithBase(outputAddr, outputCursor, 0xf);
}

void pass2Slt(char* outputAddr, char* outputCursor)
{
    pass2ArithBase(outputAddr, outputCursor, 0xa);
}

void pass2Sltu(char* outputAddr, char* outputCursor)
{
    pass2ArithBase(outputAddr, outputCursor, 0xb);
}

void pass2Not(char* outputAddr, char* outputCursor)
{
    word instr = 0x7000000;

    // arg1
    char arg1buf[16];
    getArgPos(1, arg1buf);
    // arg1 should be a reg
    if (arg1buf[0] != 'r')
    {
        bdos_print("ARITH: arg1 not a reg\n");
        exit(1);
    }
    word arg1num = strToInt(&arg1buf[1]);
    instr += (arg1num << 8);

    // arg2
    char arg2buf[16];
    getArgPos(2, arg2buf);
    // arg2 should be a reg
    if (arg2buf[0] != 'r')
    {
        bdos_print("ARITH: arg2 not a reg\n");
        exit(1);
    }
    word arg2num = strToInt(&arg2buf[1]);

    instr += arg2num;

    // write to mem
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2Load(char* outputAddr, char* outputCursor)
{
    word instr = 0x1C000000;

    word arg1num = getNumberAtArg(1);

    // arg1 should fit in 16 bits unsigned
    if (((unsigned)arg1num >> 16) > 0)
    {
        bdos_print("LOAD: arg1 is >16 bits\n");
        exit(1);
    }

    instr += (arg1num << 8);

    // arg2
    char arg2buf[16];
    getArgPos(2, arg2buf);
    // arg2 should be a reg
    if (arg2buf[0] != 'r')
    {
        bdos_print("LOAD: arg2 not a reg\n");
        exit(1);
    }
    word arg2num = strToInt(&arg2buf[1]);

    instr += (arg2num << 4);
    instr += arg2num;

    // write to mem
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2Loadhi(char* outputAddr, char* outputCursor)
{
    word instr = 0x1D000000;

    word arg1num = getNumberAtArg(1);

    // arg1 should fit in 16 bits unsigned
    if (((unsigned)arg1num >> 16) > 0)
    {
        bdos_print("LOADHI: arg1 is >16 bits\n");
        exit(1);
    }

    instr += (arg1num << 8);

    // arg2
    char arg2buf[16];
    getArgPos(2, arg2buf);
    // arg2 should be a reg
    if (arg2buf[0] != 'r')
    {
        bdos_print("LOADHI: arg2 not a reg\n");
        exit(1);
    }
    word arg2num = strToInt(&arg2buf[1]);

    instr += (arg2num << 4);
    instr += arg2num;

    // write to mem
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2LoadLabelLow(char* outputAddr, char* outputCursor)
{
    word instr = 0x1C000000;

    char arg1buf[LABEL_NAME_SIZE+1];
    getArgPos(1, arg1buf);
    word arg1num = getNumberForLabel(arg1buf);

    // only use the lowest 16 bits
    arg1num = arg1num << 16;
    arg1num = (unsigned)arg1num >> 16;

    instr += (arg1num << 8);

    // arg2
    char arg2buf[16];
    getArgPos(2, arg2buf);
    // arg2 should be a reg
    if (arg2buf[0] != 'r')
    {
        bdos_print("LOADLABELLOW: arg2 not a reg\n");
        exit(1);
    }
    word arg2num = strToInt(&arg2buf[1]);

    instr += (arg2num << 4);
    instr += arg2num;

    // write to mem
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2LoadLabelHigh(char* outputAddr, char* outputCursor)
{
    word instr = 0x1D000000;

    char arg1buf[LABEL_NAME_SIZE+1];
    getArgPos(1, arg1buf);
    word arg1num = getNumberForLabel(arg1buf);

    // only use the highest 16 bits
    arg1num = (unsigned)arg1num >> 16;

    instr += (arg1num << 8);

    // arg2
    char arg2buf[16];
    getArgPos(2, arg2buf);
    // arg2 should be a reg
    if (arg2buf[0] != 'r')
    {
        bdos_print("LOADLABELHIGH: arg2 not a reg\n");
        exit(1);
    }
    word arg2num = strToInt(&arg2buf[1]);

    instr += (arg2num << 4);
    instr += arg2num;

    // write to mem
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2Nop(char* outputAddr, char* outputCursor)
{
    word instr = 0;
    outputAddr[*outputCursor] = instr;
    (*outputCursor) += 1;
}

void pass2Dw(char* outputAddr, char* outputCursor)
{
    word dwValue = getNumberAtArg(1);

    // write to mem
    outputAddr[*outputCursor] = dwValue;
    (*outputCursor) += 1;
}

void pass2Dl(char* outputAddr, char* outputCursor)
{
    char arg1buf[LABEL_NAME_SIZE+1];
    getArgPos(1, arg1buf);
    word dlValue = getNumberForLabel(arg1buf);

    // write to mem
    outputAddr[*outputCursor] = dlValue;
    (*outputCursor) += 1;
}


