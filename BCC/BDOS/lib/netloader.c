/*
* Network bootloader library
* Could be / is being extended to handle more network based commands
* Uses wiz5500 library
*/

// uses wiz5500.c

// Port for network bootloader
#define NETLOADER_PORT 3220
// Socket to listen to (0-7)
#define NETLOADER_SOCKET 0

// Checks if p starts with cmd
// Returns 1 if true, 0 otherwise
word NETLOADER_frameCompare(char* p, char* cmd)
{
    word i = 0;
    while (cmd[i] != 0)
    {
        if (cmd[i] != p[i])
        {
            return 0;
        }
        i++;
    }
    return 1;
}


word NETLOADER_wordPosition = 0;
word NETLOADER_currentByteShift = 24;

// Appends bytes from buffer to words in run address
void NETLOADER_appendBufferToRunAddress(char* b, word len)
{
    char* dst = (char*) RUN_ADDR;

    word i;
    for (i = 0; i < len; i++) 
    {
        char readByte = b[i];

        // Read 4 bytes into one word, from left to right
        readByte = readByte << NETLOADER_currentByteShift;
        dst[NETLOADER_wordPosition] = dst[NETLOADER_wordPosition] + readByte;


        if (NETLOADER_currentByteShift == 0)
        {
            NETLOADER_currentByteShift = 24;
            NETLOADER_wordPosition++;
            dst[NETLOADER_wordPosition] = 0;
        }
        else
        {
            NETLOADER_currentByteShift -= 8;
        }
    }
}

word NETLOADER_getContentLength(char* rbuf, word rsize)
{
    word contentLengthStr[32];
    word i = 5; // content length always at 5
    char c = rbuf[i];
    while (i < rsize && c != ':')
    {
        contentLengthStr[i-5] = c;
        i++;
        c = rbuf[i];
    }
    contentLengthStr[i-5] = 0; // terminate

    return strToInt(contentLengthStr);
}

void NETLOADER_getFileName(char* rbuf, word rsize, char* fileNameStr)
{
    word i = 0;
    
    char c = rbuf[i];
    while (i < rsize && c != ':')
    {
        i++;
        c = rbuf[i];
    }

    i++; // skip the :
    word x = 0;
    c = rbuf[i];
    while (i <rsize && c != '\n')
    {
        fileNameStr[x] = c;
        i++;
        x++;
        c = rbuf[i];
    }

    fileNameStr[x] = 0; // terminate
}

word NETLOADER_getContentStart(char* rbuf, word rsize)
{
    word i = 5; // content length always at 5
    char c = rbuf[i];
    while (i < rsize && c != '\n')
    {
        i++;
        c = rbuf[i];
    }

    return (i+1);
}

void NETLOADER_runProgramFromMemory()
{

    // indicate that a user program is running
    bdos_userprogram_running = 1;

    // jump to the program
    asm(
        "; backup registers\n"
        "push r1\n"
        "push r2\n"
        "push r3\n"
        "push r4\n"
        "push r5\n"
        "push r6\n"
        "push r7\n"
        "push r8\n"
        "push r9\n"
        "push r10\n"
        "push r11\n"
        "push r12\n"
        "push r13\n"
        "push r14\n"
        "push r15\n"

        //"ccache\n"
        "savpc r1\n"
        "push r1\n"
        "jump 0x400000\n"

        "; restore registers\n"
        "pop r15\n"
        "pop r14\n"
        "pop r13\n"
        "pop r12\n"
        "pop r11\n"
        "pop r10\n"
        "pop r9\n"
        "pop r8\n"
        "pop r7\n"
        "pop r6\n"
        "pop r5\n"
        "pop r4\n"
        "pop r3\n"
        "pop r2\n"
        "pop r1\n"
        );

    // indicate that no user program is running anymore
    bdos_userprogram_running = 0;

    // clear the shell
    shell_clear_command();

    // setup the shell again
    bdos_restore();
    shell_print_prompt();
}

word NETLOADER_percentageDone(word remaining, word full)
{
  word x = remaining * 100;
  return 100 - MATH_divU(x, full);
}

void NETLOADER_handleSession(word s)
{
    word firstResponse = 1;
    word contentLengthOrig = 0;
    word contentLength = 0;
    word currentProgress = 0;

    word downloadToFile = 0; // whether we need to download to a file instead
    char fileNameStr[32];

    char dbuf[10]; // percentage done for progress indication
    dbuf[0] = 0; // terminate

    word fp = -1;

    word shift = 24; // shift for compressing data, kept the same for the whole session

    word leftOverData[4];
    word leftOverBytes = 0;

    while (wizGetSockReg8(s, WIZNET_SnSR) == WIZNET_SOCK_ESTABLISHED)
    {
        word rsize = wizGetSockReg16(s, WIZNET_SnRX_RSR);
        if (rsize != 0)
        {
            char* rbuf = (char *) TEMP_ADDR;
            wizReadRecvData(s, rbuf, rsize);
            if (firstResponse)
            {
                GFX_PrintConsole("\n");

                // save to and run from memory
                if (rbuf[0] == 'E' && rbuf[1] == 'X' && rbuf[2] == 'E' && rbuf[3] == 'C')
                {
                    GFX_PrintConsole("Receiving program\n");
                    downloadToFile = 0;

                    // reset position counters
                    NETLOADER_wordPosition = 0;
                    NETLOADER_currentByteShift = 24;

                    // clear first address of run addr
                    char* dst = (char*) RUN_ADDR;
                    dst[0] = 0;
                }
                // save to file
                else if (rbuf[0] == 'D' && rbuf[1] == 'O' && rbuf[2] == 'W' && rbuf[3] == 'N')
                {
                    GFX_PrintConsole("Receiving file\n");
                    downloadToFile = 1;

                    // get the filename
                    NETLOADER_getFileName(rbuf, rsize, fileNameStr);

                    word failedToCreateFile = 1;

                    // check length of filename
                    if (strlen(fileNameStr) < 16)
                    {
                        char new_file_path[MAX_PATH_LENGTH];
                        strcpy(new_file_path, shell_path);
                        strcat(new_file_path, "/");
                        strcat(new_file_path, fileNameStr);

                        // try to delete file in case it exists
                        brfs_delete_file(new_file_path);
                        if (brfs_create_file(shell_path, fileNameStr))
                        {
                            
                            strcpy(new_file_path, shell_path);
                            strcat(new_file_path, "/");
                            strcat(new_file_path, fileNameStr);
                            fp = brfs_open_file(new_file_path);
                            if (fp != -1)
                            {
                                brfs_set_cursor(fp, 0);
                                failedToCreateFile = 0;
                            }
                        }
                    }

                    if (failedToCreateFile)
                    {
                        wizWriteDataFromMemory(s, "ERR!", 4);
                        wizCmd(s, WIZNET_CR_DISCON);
                        GFX_PrintConsole("E: Could not create file\n");
                        // clear the shell
                        shell_clear_command();
                        shell_print_prompt();
                        return;
                    }
                }
                // unknown command
                else
                {
                    // unsupported command
                    wizWriteDataFromMemory(s, "ERR!", 4);
                    wizCmd(s, WIZNET_CR_DISCON);
                    GFX_PrintConsole("E: Unknown netloader cmd\n");
                    // clear the shell
                    shell_clear_command();
                    shell_print_prompt();
                    return;
                }

                word dataStart = NETLOADER_getContentStart(rbuf, rsize);
                contentLength = NETLOADER_getContentLength(rbuf, rsize);
                contentLengthOrig = contentLength;

                contentLength -= (rsize - dataStart);

                if (downloadToFile)
                {
                    // compress each 4 bytes into a word
                    word compressed_data[WIZNET_MAX_RBUF];
                    memset(compressed_data, 0, WIZNET_MAX_RBUF);
                    word i;
                    for (i = 0; i < rsize - dataStart; i++)
                    {
                        compressed_data[i >> 2] |= rbuf[dataStart + i] << shift;
                        if (shift == 0)
                        {
                            shift = 24;
                        }
                        else
                        {
                            shift -= 8;
                        }
                    }
                    brfs_write(fp, compressed_data, (rsize - dataStart)>>2);
                    if (shift != 24)
                    {

                        // save the left over data
                        leftOverBytes = (rsize - dataStart)&3;
                        switch (leftOverBytes)
                        {
                            case 1:
                                leftOverData[0] = rbuf[dataStart + (rsize - dataStart) - 1];
                                break;
                            case 2:
                                leftOverData[0] = rbuf[dataStart + (rsize - dataStart) - 2];
                                leftOverData[1] = rbuf[dataStart + (rsize - dataStart) - 1];
                                break;
                            case 3:
                                leftOverData[0] = rbuf[dataStart + (rsize - dataStart) - 3];
                                leftOverData[1] = rbuf[dataStart + (rsize - dataStart) - 2];
                                leftOverData[2] = rbuf[dataStart + (rsize - dataStart) - 1];
                                break;
                            default:
                                break;
                        }
                    }
                    else
                    {
                        leftOverBytes = 0;
                    }
                }
                else
                {
                    NETLOADER_appendBufferToRunAddress(rbuf+dataStart, rsize - dataStart);
                }


                firstResponse = 0;

                // all data downloaded
                if (contentLength == 0)
                {
                    wizWriteDataFromMemory(s, "THX!", 4);
                    wizCmd(s, WIZNET_CR_DISCON);

                    if (downloadToFile)
                    {
                        brfs_close_file(fp);
                        // clear the shell
                        shell_clear_command();
                        shell_print_prompt();
                    }
                    else
                    {
                        NETLOADER_runProgramFromMemory();
                    }
                    return;
                }
            }
            // not the first response
            else
            {

                // indicate progress
                // remove previous percentage
                word i = strlen(dbuf);
                while (i > 0)
                {
                    GFX_PrintcConsole(0x8); // backspace
                    i--;
                }
                if (strlen(dbuf) != 0)
                {
                    GFX_PrintcConsole(0x8); // backspace
                }

                itoa(NETLOADER_percentageDone(contentLength, contentLengthOrig), dbuf);
                GFX_PrintConsole(dbuf);
                GFX_PrintcConsole('%');


                contentLength -= rsize;

                if (downloadToFile)
                {
                    // compress each 4 bytes into a word
                    word compressed_data[WIZNET_MAX_RBUF];
                    memset(compressed_data, 0, WIZNET_MAX_RBUF);

                    // add the left over data
                    switch (leftOverBytes)
                    {
                        case 1:
                            compressed_data[0] = leftOverData[0] << 24;
                            shift = 16;
                            break;
                        case 2:
                            compressed_data[0] = leftOverData[0] << 24;
                            compressed_data[0] |= leftOverData[1] << 16;
                            shift = 8;
                            break;
                        case 3:
                            compressed_data[0] = leftOverData[0] << 24;
                            compressed_data[0] |= leftOverData[1] << 16;
                            compressed_data[0] |= leftOverData[2] << 8;
                            shift = 0;
                            break;
                    }

                    word i;
                    word j = leftOverBytes;
                    for (i = 0; i < rsize; i++)
                    {
                        compressed_data[j >> 2] |= rbuf[i] << shift;
                        if (shift == 0)
                        {
                            shift = 24;
                        }
                        else
                        {
                            shift -= 8;
                        }
                        j++;
                    }
                    if (shift != 24)
                    {

                        // save the left over data
                        if (shift == 16)
                        {
                            leftOverBytes = 1;
                        }
                        else if (shift == 8)
                        {
                            leftOverBytes = 2;
                        }
                        else if (shift == 0)
                        {
                            leftOverBytes = 3;
                        }
                        //leftOverBytes = (rsize)&3;
                        switch (leftOverBytes)
                        {
                            case 1:
                                leftOverData[0] = rbuf[rsize - 1];
                                break;
                            case 2:
                                leftOverData[0] = rbuf[rsize - 2];
                                leftOverData[1] = rbuf[rsize - 1];
                                break;
                            case 3:
                                leftOverData[0] = rbuf[rsize - 3];
                                leftOverData[1] = rbuf[rsize - 2];
                                leftOverData[2] = rbuf[rsize - 1];
                                break;
                            default:
                                break;
                        }
                    }
                    else
                    {
                        leftOverBytes = 0;
                    }
                    brfs_write(fp, compressed_data, rsize >> 2);
                    //brfs_write(fp, rbuf, rsize);
                }
                else
                {
                    NETLOADER_appendBufferToRunAddress(rbuf, rsize);
                }

                // all data downloaded
                if (contentLength == 0)
                {
                    wizWriteDataFromMemory(s, "THX!", 4);
                    wizCmd(s, WIZNET_CR_DISCON);

                    // remove progress prints
                    word i = strlen(dbuf);
                    while (i > 0)
                    {
                        GFX_PrintcConsole(0x8); // backspace
                        i--;
                    }
                    GFX_PrintcConsole(0x8); // backspace

                    if (downloadToFile)
                    {
                        brfs_close_file(fp);
                        // clear the shell
                        shell_clear_command();
                        shell_print_prompt();
                    }
                    else
                    {
                        NETLOADER_runProgramFromMemory();
                    }
                    return;
                }
            }
        }
    }
}


// Initialize network bootloader on socket s
void NETLOADER_init(word s)
{
    // Open socket in TCP Server mode
    wizInitSocketTCP(s, NETLOADER_PORT);
}


// Check for a change in the socket status
// Handles change if exists
word NETLOADER_loop(word s)
{
    // Get status for socket s
    word sxStatus = wizGetSockReg8(s, WIZNET_SnSR);

    if (sxStatus == WIZNET_SOCK_CLOSED)
    { 
        // Open the socket when closed
        wizInitSocketTCP(s, NETLOADER_PORT);
    }
    else if (sxStatus == WIZNET_SOCK_ESTABLISHED)
    {
        // Handle session when a connection is established
        NETLOADER_handleSession(s);
    }
    else if (sxStatus == WIZNET_SOCK_LISTEN)
    {
        // Keep on listening
        return 1;
    }
    else if (sxStatus == WIZNET_SOCK_SYNSENT || sxStatus == WIZNET_SOCK_SYNRECV || sxStatus == WIZNET_SOCK_FIN_WAIT || sxStatus == WIZNET_SOCK_TIME_WAIT)
    {
        // Do nothing in these cases
        return 2;
    }
    else
    {
        // In other cases, reset the socket
        wizInitSocketTCP(s, NETLOADER_PORT);
    }

    return 0;
}

