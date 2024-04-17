/*
* Standard library
* Contains basic functions, including timer and memory functions
* Modified version for BCC
*/

// uses math.c 

#define UART_TX_ADDR 0xC02723

// Timer I/O Addresses
#define TIMER1_VAL 0xC02739
#define TIMER1_CTRL 0xC0273A
#define TIMER2_VAL 0xC0273B
#define TIMER2_CTRL 0xC0273C
#define TIMER3_VAL 0xC0273D
#define TIMER3_CTRL 0xC0273E

word timer1Value = 0;
word timer2Value = 0;
word timer3Value = 0;

/*
* TODO:
* - Convert most of these functions to assembly
*/


// isalpha
word isalpha(word argument)
{
  if (argument >= 'A' && argument <= 'Z')
    return 2;
  if (argument >= 'a' && argument <= 'z')
    return 1;
  return 0;
}

// isdigit
word isdigit(word argument)
{
  if (argument >= '0' && argument <= '9')
    return 1;
  return 0;
}

// isalnum
word isalnum(word argument)
{
  if (isdigit(argument) || isalpha(argument))
    return 1;
  return 0;
}



void* memcpy(void *dest, const void *src, size_t len)
{
   // Typecast src and dest addresses to (char *)
   char *csrc = (char *)src;
   char *cdest = (char *)dest;
  
   // Copy contents of src[] to dest[]
   word i;
   for (i=0; i<len; i++)
       cdest[i] = csrc[i];
}

void* memmove(void* dest, const void* src, size_t n)
{
  unsigned char* from = (unsigned char*) src;
  unsigned char* to = (unsigned char*) dest;

  if (from == to || n == 0)
    return dest;
  if (to > from && to-from < (word)n) {
    /* to overlaps with from */
    /*  <from......>         */
    /*         <to........>  */
    /* copy in reverse, to avoid overwriting from */
    word i;
    for(i=n-1; i>=0; i--)
      to[i] = from[i];
    return dest;
  }
  if (from > to && from-to < (word)n) {
    /* to overlaps with from */
    /*        <from......>   */
    /*  <to........>         */
    /* copy forwards, to avoid overwriting from */
    size_t i;
    for(i=0; i<n; i++)
      to[i] = from[i];
    return dest;
  }
  memcpy(dest, src, n);
  return dest;
}

// Function to implement `strcpy()` function
char* strcpy(char* destination, const char* source)
{
    // return if no memory is allocated to the destination
    if (destination == NULL) {
        return NULL;
    }
 
    // take a pointer pointing to the beginning of the destination string
    char *ptr = destination;
 
    // copy the C-string pointed by source into the array
    // pointed by destination
    while (*source != '\0')
    {
        *destination = *source;
        destination++;
        source++;
    }
 
    // include the terminating null character
    *destination = '\0';
 
    // the destination is returned by standard `strcpy()`
    return ptr;
}


size_t strlen(const char *str)
{
        const char *s;
        for (s = str; *s; ++s);
        return (s - str);
}

char* strcat (char *dest, const char *src)
{
  strcpy (dest + strlen (dest), src);
  return dest;
}

char* strchr (const char *s, word c)
{
  do {
    if (*s == c)
      {
        return (char*)s;
      }
  } while (*s++);
  return (0);
}

/*
Returns a pointer to the last occurance of a character, or 0 if the character is not found.
*/
char* strrchr (const char *s, int c)
{
  char *rtnval = 0;

  do {
    if (*s == c)
      rtnval = (char*) s;
  } while (*s++);
  return (rtnval);
}


char * strtok_old_str;
/* 
Parse str into tokens separated by characters in delim.
If S is NULL, the last string strtok() was called with is used.
Note that strtok() modifies the input string.
For example:
	char s[] = "-abc-=-def";
	x = strtok(s, "-");		// x = "abc"
	x = strtok(NULL, "-=");		// x = "def"
	x = strtok(NULL, "=");		// x = NULL
		// s = "abc\0=-def\0"
*/
char* strtok(char* str, const char* delim)
{
  if (str != (word*)-1)
    strtok_old_str = str;

  if (strtok_old_str == (word*)-1)
    return (word*)-1;

  // Return reached end of string
  if (*strtok_old_str == 0)
  {
    return (word*)-1;
  }
  // Skip leading delimiters
  while (strchr(delim, *strtok_old_str) != 0)
    strtok_old_str++;

  // Find end of token
  char* start = strtok_old_str;
  while (*strtok_old_str != 0 && strchr(delim, *strtok_old_str) == 0)
    strtok_old_str++;

  if (*strtok_old_str == 0)
  {
    strtok_old_str = (word*)-1;
    return start;
  }

  *strtok_old_str = 0;
  strtok_old_str++;
  return start;
}


word strcmp(const char* s1, const char* s2)
{
  while(*s1 && (*s1 == *s2))
  {
    s1++;
    s2++;
  }
  return *(unsigned char*)s1 - *(unsigned char*)s2;
}

word strncmp(const char * s1, const char * s2, size_t n )
{
  while ( n && *s1 && ( *s1 == *s2 ) )
  {
    ++s1;
    ++s2;
    --n;
  }
  if ( n == 0 )
  {
    return 0;
  }
  else
  {
    return ( *(unsigned char *)s1 - *(unsigned char *)s2 );
  }
}

/**
 * Return the basename of a path
 * path: full path
*/
char* basename(char *path)
{
  char *base = strrchr(path, '/');
  return base ? base + 1 : path;
}

/**
 * Return the dirname of a path
 * output: buffer to store the dirname
 * path: full path
*/
char* dirname(char* output, char *path)
{
  strcpy(output, path);
  char *last_slash = strrchr(output, '/');
  if (last_slash != 0)
  {
    *last_slash = 0;
    // If the last slash is the first character, return "/"
    if (last_slash == output)
    {
      strcpy(output, "/");
    }
  } else
  {
    // No slash found, return "."
    strcpy(output, ".");
  }
  return output;
}


/*
Recursive helper function for itoa
Eventually returns the number of digits in n
s is the output buffer
*/
word itoar(word n, char *s)
{
    word digit = MATH_modU(n, 10);
    word i = 0;

    n = MATH_divU(n,10);
    if ((unsigned int) n > 0)
        i += itoar(n, s);

    s[i++] = digit + '0';

    return i;
}


/*
Converts integer n to characters.
The characters are placed in the buffer s.
The buffer is terminated with a 0 value.
Uses recursion, division and mod to compute.
*/
void itoa(word n, char *s)
{
    // compute and fill the buffer
    word i = itoar(n, s);

    // end with terminator
    s[i] = 0;
} 


/*
Prints a single char c by writing it to UART_TX_ADDR
*/
void uprintc(char c) 
{
    word *p = (word *)UART_TX_ADDR; // address of UART TX
    *p = (word)c;                   // write char over UART
}


/*
Sends each character from str over UART
by writing them to UART_TX_ADDR
until a 0 value is found.
Does not send a newline afterwards.
*/
void uprint(char* str) 
{
    word *p = (word *)UART_TX_ADDR; // address of UART TX
    char chr = *str;                // first character of str

    while (chr != 0)                // continue until null value
    {
        *p = (word)chr;             // write char over UART
        str++;                      // go to next character address
        chr = *str;                 // get character from address
    }
}


/*
Same as uprint(char* str),
except it sends a newline afterwards.
*/
void uprintln(char* str) 
{
    uprint(str);
    uprintc('\n');
}


// Converts char c to uppercase if possible
char toUpper(char c)
{
    if (c>96 && c<123) 
        c = c ^ 0x20;

    return c;
}


// Converts string str to uppercase if possible
void strToUpper(char* str) 
{
    char chr = *str;            // first character of str

    while (chr != 0)            // continue until null value
    {
        *str = toUpper(chr);    // uppercase char
        str++;                  // go to next character address
        chr = *str;             // get character from address
    }
}
