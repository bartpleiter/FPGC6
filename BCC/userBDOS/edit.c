// Simple text editor

// NOTE: this uses more memory than is allowed in FPGC7, so update this when new BDOS memory map.
// Also, code should be refactored.

/* global structure of program:
- check first argument, exit if none
- read file from first argument into mem
  - using a rectangular buffer (uninitialized 2d array, pointer to FILE_MEMORY_ADDR) with a max linewidth
  - while reading file, fill line until newline or MAX_LINE_WIDTH (if max, then error and exit)
    - add zeros until max_line_width after newline character
  - if current line number >= MAX_LINES: error out of mem and exit
  - use a >8bit code for indicating end of file, so print can stop
- keep track of cursor x and y, which corresponds to line and pos in mem buffer
- need for fast (asm!) memory move operations for inserting and deleting line based stuff (within the same line does not matter that much)
- need for fast (asm!) print function which prints TEXT_WINDOW_HEIGHT lines with an xscroll offset (with palette indication that a line extends right)
  - end of file should have a special palette color
- header line as first line with filename, mode, etc (like nano)
- keep track of window (start) cursor (only y, line number)
- when cursor y < window cursor, window cursor --, if y > window cursor + TEXT_WINDOW_HEIGHT, window cursor++ (check on start and end of file boundaries)
- after each operation, print from window start
- print cursor on top of character, by inverting the palette
- escape for menu
  - save
    - function to 
  - exit
*/

/* future possible features that are not that hard to implement
- use control key (which should be implemented in hid fifo first!)
  - then, ctrl s and other stuff can be implemented
- selection mode (set start and end x,y), use a color palette to indicate selection
  - copy or delete (or cut, by copy and then delete) the selection into a separate buffer (check on bounds!)
  - paste by inserting from the buffer
- text search which selects the matches one by one (different mode again)
- tab inserts 2 spaces, maybe stops at even x for alignment?
- insert mode that overwrites characters instead of moving them
*/

#define word char

#include "lib/math.c"
#include "lib/sys.c"
#include "lib/gfx.c"
#include "lib/stdlib.c"
#include "lib/brfs.c"

#define FILE_MEMORY_ADDR    0x480000 // location of file content
#define MAX_LINE_WIDTH      240     // max width of line in mem buffer, multiple of 40 (screen width)
#define MAX_LINES           8192    // maximum number of lines to prevent out of memory writes (8192*256*4 = 8MiB)
#define TEXT_WINDOW_HEIGHT  20      // number of lines to print on screen
#define SCREEN_WIDTH        40
#define SCREEN_HEIGHT       25

// Palettes
#define PALETTE_CURSOR    1
#define PALETTE_NEWLINE   2
#define PALETTE_EOF       3
#define PALETTE_SELECTED  4
#define PALETTE_HEADER    5

// HID
#define ARROW_LEFT    256
#define ARROW_RIGHT   257
#define ARROW_UP      258
#define ARROW_DOWN    259
#define BTN_INSERT    260
#define BTN_HOME      261
#define BTN_PAGEUP    262
#define BTN_END       263
#define BTN_PAGEDOWN  264

char headerText[SCREEN_WIDTH]; // text on header (first line)

// Buffer for file contents in memory
char (*mem)[MAX_LINE_WIDTH] = (char (*)[MAX_LINE_WIDTH]) FILE_MEMORY_ADDR; // 2d array containing all lines of the input file

// Buffer for file reading
#define FBUF_LEN 4096
#define EOF -1
word lastLineNumber = 0;

// Cursors
word windowYscroll = 0; // vertical line offset for drawing window
word windowXscroll = 0; // horizontal position offset for drawing window
word userCursorX = 0; // char position within line, of user cursor
word userCursorY = 0; // line of user cursor

// Write mem to file
void writeMemToFile(char* absolute_path)
{
  // Open file
  word fd = fs_open(absolute_path);
  if (fd == -1)
  {
    bdos_println("File not found");
    exit();
  }

  word y;
  for (y = 0; y <= lastLineNumber; y++)
  {
    word lineEndPos = 0;
    while (mem[y][lineEndPos] != '\n' && mem[y][lineEndPos] != EOF)
    {
      lineEndPos++;
      if (lineEndPos == MAX_LINE_WIDTH)
      {
        exitRoutine();
        bdos_print("E: could not find end of line\n");
        fs_close(fd);
        exit();
      }
    }

    if (mem[y][lineEndPos] == EOF)
    {
      fs_write(fd, mem[y], lineEndPos);
      fs_close(fd);
      return;
    }

    lineEndPos++; // include the newline token
    fs_write(fd, mem[y], lineEndPos);
  }
  fs_close(fd);
}


// returns the current char at cursor within the opened file (EOF if end of file)
// increments the cursor
word fgetc(word fd, word filesize)
{
  word cursor = fs_getcursor(fd);
  if (cursor == filesize)
  {
    return EOF;
  }

  char c;
  fs_read(fd, &c, 1);
  return c;
}


// reads a line from the input file
word readFileLine(word fd, word filesize)
{
  char c = fgetc(fd, filesize);
  char cprev = c;

  word currentChar = 0;

  // stop on EOF or newline or max line width reached
  while (c != EOF && c != '\n' && currentChar < (MAX_LINE_WIDTH-1))
  {
    mem[lastLineNumber][currentChar] = c;
    currentChar++;
    cprev = c;
    c = fgetc(fd, filesize);
  }

  // error when line was too long
  if (c != EOF && c != '\n' && currentChar >= (MAX_LINE_WIDTH-1))
  {
    bdos_print("E: line is too long\n");
    fs_close(fd);
    exit();
  }

  mem[lastLineNumber][currentChar] = c; // add EOF or \n

  // append line with zeros
  while (currentChar < (MAX_LINE_WIDTH-1))
  {
    currentChar++;
    char fillerChar = 0;
    if (c == EOF)
    {
      fillerChar = EOF;
    }
    mem[lastLineNumber][currentChar] = fillerChar;
  }

  return c;
}


void readInputFile(char* absolute_path, word filesize)
{
  // Open file
  word fd = fs_open(absolute_path);
  if (fd == -1)
  {
    bdos_println("File not found");
    exit();
  }

  while (readFileLine(fd, filesize) != EOF)
  {
    if (lastLineNumber >= MAX_LINES)
    {
      fs_close(fd);
      bdos_print("E: File too large\n");
      exit();
    }
    else
    {
      lastLineNumber++;
    }
  }

  fs_close(fd);
}


// Applies window to mem and prints on screen (using window plane)
void printWindow()
{
  printHeader();

  char* vramWindowpattern = (char*) 0xC01420;
  word vramPaletteOffset = 2048;

  word lastLineFound = 0;

  word x, y;
  for (y = 0; y < SCREEN_HEIGHT-1; y++) // leave first line open
  {
    for (x = 0; x < SCREEN_WIDTH; x++)
    {
      word c = mem[y + windowYscroll][x + windowXscroll];
      if (c == '\n')
      {
        word vramOffset = GFX_WindowPosFromXY(x, y+1);
        *(vramWindowpattern + vramOffset) = 7;
        *(vramWindowpattern + vramOffset + vramPaletteOffset) = PALETTE_NEWLINE;
      }
      else if (c == EOF)
      {
        // print empty space if EOF is already drawn
        if (lastLineFound)
        {
          word vramOffset = GFX_WindowPosFromXY(x, y+1);
          *(vramWindowpattern + vramOffset) = 0;
          *(vramWindowpattern + vramOffset + vramPaletteOffset) = 0;
        }
        else
        {
          word vramOffset = GFX_WindowPosFromXY(x, y+1);
          *(vramWindowpattern + vramOffset) = 4;
          *(vramWindowpattern + vramOffset + vramPaletteOffset) = PALETTE_EOF;
          lastLineFound = 1; // notify, but continue rendering the line
        }
      }
      else
      {
        word vramOffset = GFX_WindowPosFromXY(x, y+1);
        *(vramWindowpattern + vramOffset) = c;
        *(vramWindowpattern + vramOffset + vramPaletteOffset) = 0;
      }
    }
    if (lastLineFound)
    {
      return;
    }
  }
}

// Remove cursor from screen (bg plane)
void clearCursor()
{
  word vramCursorOffset = GFX_BackgroundPosFromXY(userCursorX, userCursorY+1);
  char* vramBackgroundpalette = (char*) 0xC00C20;
  *(vramBackgroundpalette + vramCursorOffset) = 0;
}

// Print the cursor on screen (using background plane)
void printCursor()
{
  word vramCursorOffset = GFX_BackgroundPosFromXY(userCursorX, userCursorY+1);
  char* vramBackgroundpalette = (char*) 0xC00C20;
  *(vramBackgroundpalette + vramCursorOffset) = PALETTE_CURSOR;
}

// Print the header on screen (using window plane)
void printHeader()
{
  GFX_printWindowColored(headerText, SCREEN_WIDTH, 0, PALETTE_HEADER);
}

void setPalettes()
{
  word* paletteAddress = (word*) 0xC00400;
  paletteAddress[PALETTE_CURSOR] = 0x7 << 24;
  paletteAddress[PALETTE_NEWLINE] = 12;
  paletteAddress[PALETTE_EOF] = 224;
  paletteAddress[PALETTE_HEADER] = (0x51 << 24) + 0xDA;
}

// Make sure the cursor stays left from the newline
void fixCursorToNewline()
{
  word xpos = userCursorX+windowXscroll;
  word ypos = userCursorY+windowYscroll;

  word newLinePos = 0;
  while (mem[ypos][newLinePos] != '\n' && mem[ypos][newLinePos] != EOF)
  {
    newLinePos++;
    if (newLinePos == MAX_LINE_WIDTH)
    {
      exitRoutine();
      bdos_print("E: could not find newline\n");
      exit();
    }
  }

  clearCursor();

  word appliedScolling = 0;
  while (xpos > newLinePos)
  {
    if (userCursorX > 0)
    {
      userCursorX--;
    }
    else
    {
      if (windowXscroll > 0)
      {
        windowXscroll--;
        appliedScolling = 1;
      }
      else
      {
        exitRoutine();
        bdos_print("E: scroll left limit reached\n");
        exit();
      }
    }
    xpos = userCursorX+windowXscroll;
  }

  if (appliedScolling)
  {
    printWindow();
  }
}


// Move the cursor to targetPos of the previous line, from the left
// Assumes targetPos is valid and ypos is not 0
// Does not print window at the end, caller should do that
void gotoPosOfPrevLine(word targetPos)
{
  clearCursor();

  userCursorX = 0;
  windowXscroll = 0;

  if (userCursorY > 0)
  {
    userCursorY--;
  }
  else
  {
    if (windowYscroll > 0)
    {
      windowYscroll--;
    }
    else
    {
      exitRoutine();
      bdos_print("E: could not move up a line\n");
      exit();
    }
  }

  word xpos = 0;
  word ypos = userCursorY+windowYscroll;
  while (xpos != targetPos)
  {
    if (userCursorX < SCREEN_WIDTH-1)
    {
      userCursorX++;
    }
    else
    {
      if (windowXscroll < MAX_LINE_WIDTH-SCREEN_WIDTH)
      {
        windowXscroll++;
      }
    }

    xpos = userCursorX + windowXscroll;

    if (xpos == MAX_LINE_WIDTH)
    {
      exitRoutine();
      bdos_print("E: target out of bounds\n");
      exit();
    }
  }

  printCursor();
}


// Move the cursor to the newline character of the previous line
void gotoEndOfPrevLine()
{
  userCursorX = 0;
  windowXscroll = 0;

  word appliedScolling = 0;

  if (userCursorY > 0)
  {
    userCursorY--;
  }
  else
  {
    if (windowYscroll > 0)
    {
      windowYscroll--;
      appliedScolling = 1;
    }
    else
    {
      // don't need to do anything when on the first line
      return;
    }
  }

  word xpos = 0;
  word ypos = userCursorY+windowYscroll;
  while (mem[ypos][xpos] != '\n' && mem[ypos][xpos] != EOF)
  {
    if (userCursorX < SCREEN_WIDTH-1)
    {
      userCursorX++;
    }
    else
    {
      if (windowXscroll < MAX_LINE_WIDTH-SCREEN_WIDTH)
      {
        windowXscroll++;
        appliedScolling = 1;
      }
    }

    xpos = userCursorX + windowXscroll;

    if (xpos == MAX_LINE_WIDTH)
    {
      exitRoutine();
      bdos_print("E: could not find newline\n");
      exit();
    }
  }

  if (appliedScolling)
  {
    printWindow();
  }
}


void moveCursorDown()
{
  // skip if current line contains EOF
  word ypos = userCursorY+windowYscroll;
  word i;
  for (i = 0; i < MAX_LINE_WIDTH; i++)
  {
    if (mem[ypos][i] == '\n')
    {
      break;
    }
    if (mem[ypos][i] == EOF)
    {
      return;
    }
  }

  clearCursor();

  if (userCursorY < SCREEN_HEIGHT-2)
  {
    userCursorY++;
  }
  else
  {
    if (windowYscroll < lastLineNumber-(SCREEN_HEIGHT-2))
    {
      windowYscroll++;
      printWindow();
    }
  }
  fixCursorToNewline();
  printCursor();
}

void moveCursorUp()
{
  clearCursor();

  if (userCursorY > 0)
  {
    userCursorY--;
  }
  else
  {
    if (windowYscroll > 0)
    {
      windowYscroll--;
      printWindow();
    }
  }
  fixCursorToNewline();
  printCursor();
}

void moveCursorLeft()
{
  clearCursor();

  if (userCursorX > 0)
  {
    userCursorX--;
  }
  else
  {
    if (windowXscroll > 0)
    {
      windowXscroll--;
      printWindow();
    }
    else
    {
      // move to previous line
      windowXscroll = MAX_LINE_WIDTH - SCREEN_WIDTH;
      userCursorX = SCREEN_WIDTH - 1;
      gotoEndOfPrevLine();
    }
  }
  fixCursorToNewline();
  printCursor();
}

void moveCursorRight()
{
  word xpos = userCursorX + windowXscroll;
  if (mem[userCursorY+windowYscroll][xpos] == EOF)
  {
    // do nothing at EOF
    return;
  }

  clearCursor();

  if (mem[userCursorY+windowYscroll][xpos] == '\n')
  {
    // if we are at the end of the line, move to next line
    word appliedScolling = windowXscroll;
    windowXscroll = 0;
    userCursorX = 0;
    moveCursorDown();
    if (appliedScolling)
    {
      printWindow();
    }
  }
  else
  {
    if (userCursorX < SCREEN_WIDTH-1)
    {
      userCursorX++;
    }
    else
    {
      if (windowXscroll < MAX_LINE_WIDTH-SCREEN_WIDTH)
      {
        windowXscroll++;
        printWindow();
      }
    }

  }
  printCursor();
}


void pageUp()
{
  clearCursor();

  word i;
  for (i = 0; i < SCREEN_HEIGHT-2; i++)
  {
    if (userCursorY > 0)
    {
      userCursorY--;
    }
    else
    {
      if (windowYscroll > 0)
      {
        windowYscroll--;
      }
    }
  }

  printWindow();
  fixCursorToNewline();
  printCursor();
  
}

void pageDown()
{
  clearCursor();

  word lastLineFound = 0;

  word i;
  for (i = 0; i < SCREEN_HEIGHT-2; i++)
  {
    if (lastLineFound)
    {
      break;
    }

    // skip if current line contains EOF
    word ypos = userCursorY+windowYscroll;
    word i;
    for (i = 0; i < MAX_LINE_WIDTH; i++)
    {
      if (mem[ypos][i] == '\n')
      {
        break;
      }
      if (mem[ypos][i] == EOF)
      {
        lastLineFound = 1;
        break;
      }
    }

    if (userCursorY < SCREEN_HEIGHT-2)
    {
      userCursorY++;
    }
    else
    {
      if (windowYscroll < lastLineNumber-(SCREEN_HEIGHT-2))
      {
        windowYscroll++;
      }
    }
  }

  printWindow();
  fixCursorToNewline();
  printCursor();
}

void home()
{
  clearCursor();
  userCursorX = 0;
  windowXscroll = 0;
  printWindow();
  printCursor();
}

void end()
{
  clearCursor();
  
  userCursorX = 0;
  windowXscroll = 0;


  word xpos = 0;
  word ypos = userCursorY+windowYscroll;
  while (mem[ypos][xpos] != '\n' && mem[ypos][xpos] != EOF)
  {
    if (userCursorX < SCREEN_WIDTH-1)
    {
      userCursorX++;
    }
    else
    {
      if (windowXscroll < MAX_LINE_WIDTH-SCREEN_WIDTH)
      {
        windowXscroll++;
      }
    }

    xpos = userCursorX + windowXscroll;

    if (xpos == MAX_LINE_WIDTH)
    {
      exitRoutine();
      bdos_print("E: could not find newline\n");
      exit();
    }
  }
  
  printWindow();
  printCursor();
}


void removeCurrentLine()
{
  word ypos = userCursorY+windowYscroll;

  // move all lines below ypos up by one, starting at beginning
  word i;
  for (i = ypos+1; i <= lastLineNumber; i++)
  {
    memcpy(mem[i-1], mem[i], MAX_LINE_WIDTH);
  }
  lastLineNumber--;
}

void insertNewLine()
{
  word ypos = userCursorY+windowYscroll;

  if (lastLineNumber == MAX_LINES-1)
  {
    // ignore if we reached the memory limit
    return;
  }

  clearCursor();

  // move all lines below ypos down by one, starting at the end
  word i;
  for (i = lastLineNumber; i > ypos; i--)
  {
    memcpy(mem[i+1], mem[i], MAX_LINE_WIDTH);
  }
  lastLineNumber++;

  // move everything right from cursor to new line
  // this should include the newline/EOF automatically
  word newLineTmpCursor = 0;
  word xpos = userCursorX+windowXscroll;
  while(xpos < MAX_LINE_WIDTH)
  {
    mem[ypos+1][newLineTmpCursor] = mem[ypos][xpos];
    mem[ypos][xpos] = 0;
    newLineTmpCursor++;
    xpos++;
  }

  // insert newline character at current line
  xpos = userCursorX+windowXscroll;
  mem[ypos][xpos] = '\n';

  // move the cursor down to the start of the new line
  windowXscroll = 0;
  userCursorX = 0;
  moveCursorDown();
  printWindow(); // always needed in case of a new line

  printCursor();
}

// Insert character c at cursor
void insertCharacter(char c)
{
  word xpos = userCursorX+windowXscroll;
  word ypos = userCursorY+windowYscroll;
  memmove(&mem[ypos][xpos+1], &mem[ypos][xpos], MAX_LINE_WIDTH-(xpos+1));
  mem[ypos][xpos] = c;
  printWindow();
  moveCursorRight();
}

// Remove character at cursor
void removeCharacter()
{
  word xpos = userCursorX+windowXscroll;
  word ypos = userCursorY+windowYscroll;
  if (xpos > 0)
  {
    memmove(&mem[ypos][xpos-1], &mem[ypos][xpos], MAX_LINE_WIDTH-(xpos-1));
    mem[ypos][MAX_LINE_WIDTH-1] = 0; // remove new char at right of line
    printWindow();
    moveCursorLeft();
  }
  else
  {
    if (ypos > 0)
    {
      // append current line to previous line at newline
      word prevLinePasteLocation = 0;
      word newLinePos = 0;
      while (mem[ypos-1][newLinePos] != '\n')
      {
        newLinePos++;
        if (newLinePos == MAX_LINE_WIDTH)
        {
          exitRoutine();
          bdos_print("E: could not find newline\n");
          exit();
        }
      }
      // copy to overwrite the newline
      memcpy(&mem[ypos-1][newLinePos], &mem[ypos][0], MAX_LINE_WIDTH-(newLinePos));
      prevLinePasteLocation = newLinePos;

      // check if resulting line has a newline or EOF, else add to end
      newLinePos = 0;
      while (mem[ypos-1][newLinePos] != '\n' && mem[ypos-1][newLinePos] != EOF && newLinePos < MAX_LINE_WIDTH)
      {
        newLinePos++;
        if (newLinePos == MAX_LINE_WIDTH)
        {
          if (ypos == lastLineNumber)
          {
            mem[ypos-1][MAX_LINE_WIDTH-1] = EOF;
          }
          else
          {
            mem[ypos-1][MAX_LINE_WIDTH-1] = '\n';
          }
          
        }
      }

      // remove the current line
      removeCurrentLine();

      // move cursor to the place we pasted the line
      gotoPosOfPrevLine(prevLinePasteLocation);

      // update the view
      printWindow();
    }
  }
}

void addEditHeader()
{
  headerText[29] = 'Y';
  headerText[30] = ':';
  itoa(userCursorY+windowYscroll, &headerText[31]);
  headerText[35] = 'X';
  headerText[36] = ':';
  itoa(userCursorX+windowXscroll, &headerText[37]);
}


void exitRoutine()
{
  GFX_clearWindowtileTable();
  GFX_clearWindowpaletteTable();
  GFX_clearBGtileTable();
  GFX_clearBGpaletteTable();
}


int main() 
{
  // Read number of arguments
  word argc = shell_argc();
  if (argc < 2)
  {
    bdos_println("Usage: edit <file>");
    return 1;
  }
  
  // Read filename
  char** args = shell_argv();
  char* filename = args[1];

  char absolute_path[MAX_PATH_LENGTH];
  // Check if absolute path
  if (filename[0] != '/')
  {
    char* cwd = fs_getcwd();
    strcpy(absolute_path, cwd);
    strcat(absolute_path, "/");
    strcat(absolute_path, filename);
  }
  else
  {
    strcpy(absolute_path, filename);
  }

  // Get file size
  struct brfs_dir_entry* entry = (struct brfs_dir_entry*)fs_stat(absolute_path);
  if ((word)entry == -1)
  {
    bdos_println("File not found");
    return 1;
  }
  word filesize = entry->filesize;

  // Get filename
  char decompressed_filename[17];
  strdecompress(decompressed_filename, entry->filename);
  strcpy(headerText, decompressed_filename);

  readInputFile(absolute_path, filesize);

  // init gfx
  GFX_clearWindowtileTable();
  GFX_clearWindowpaletteTable();
  GFX_clearBGtileTable();
  GFX_clearBGpaletteTable();
  setPalettes();
  addEditHeader();
  printWindow();
  printCursor();

  // main loop
  while (1)
  {
    if (hid_checkfifo())
    {
      word c = hid_fiforead();
      switch (c)
      {
        case 27: // escape
          memcpy(&headerText[24], "Save? Type y/n/c", 16);
          printHeader();

          // ask for confirmation
          while (c != 'y' && c != 'n' && c != 'Y' && c != 'N' && c != 'c' && c != 'C')
          {
            // wait until a character is pressed
            while (!hid_checkfifo());

            c = hid_fiforead();
          }

          if (c == 'y' || c == 'Y')
          {
            // First delete file
            fs_delete(absolute_path);
            // Then create file
            fs_mkfile(absolute_path);
            // Then write to file
            writeMemToFile(absolute_path);
            exitRoutine();
            return 'q'; // exit
          }
          if (c == 'n' || c == 'N')
          {
            exitRoutine();
            return 'q'; // exit
          }

          memcpy(&headerText[24], "                ", 16); // clear header text

          break;
        case ARROW_LEFT:
          moveCursorLeft();
          break;
        case ARROW_RIGHT:
          moveCursorRight();
          break;
        case ARROW_DOWN:
          moveCursorDown();
          break;
        case ARROW_UP:
          moveCursorUp();
          break;
        case BTN_HOME:
          home();
          break;
        case BTN_END:
          end();
          break;
        case BTN_PAGEUP:
          pageUp();
          break;
        case BTN_PAGEDOWN:
          pageDown();
          break;
        case 0x8: // backspace
          removeCharacter();
          break;
        case '\n':
          insertNewLine();
          break;
        case '\t':
          insertCharacter(' ');
          insertCharacter(' ');
          break;
        default:
          // default to inserting the character as text
          insertCharacter(c);
          break;
      }
      addEditHeader();
      printHeader();
    }

  }

  return 'q';
}

void interrupt()
{
  // Handle all interrupts
  word i = get_int_id();
  switch(i)
  {
    case INTID_TIMER1:
      timer1Value = 1;  // Notify ending of timer1
      break;
  }
}