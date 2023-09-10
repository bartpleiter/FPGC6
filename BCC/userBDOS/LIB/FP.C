/*
* Fixed-Point number library
* Contains functions for decimal numbers encoded in 16.16 format
*/

#define fixed_point_t char

// Fixed point arithmetic operations using 16.16 format
#define FP_FRACT_BITS 16
#define FP_MULTIPLY_CONST 15259
#define FP_FRACT_MASK 0xFFFF


// Convert a fixed-point variable to a string with a given number of decimals (truncated, not rounded)
void FP_FPtoString(word fixedPointValue, char* outputString, word numDecimals) {
  word fractMask = FP_FRACT_MASK; // Needed to enforce a load(32) instruction, as constants have limited bits in arith operations
  word integerPart = fixedPointValue >> FP_FRACT_BITS; // Extract the integer part
  word fractionalPart = fixedPointValue & fractMask; // Extract the fractional part

  // Convert the integer part, which can be signed, to a string using itoa()
  if (integerPart < 0)
  {
    outputString[0] = '-';
    if (fractionalPart > 0)
    {
      itoa(- (integerPart + 1), &outputString[1]); // integerPart-1 to fix neg numbers
    }
    else
    {
      itoa(- integerPart, &outputString[1]);
    }
  }
  else
  {
    itoa(integerPart, outputString);
  }

  // Find the end of the integer part string
  char* endOfString = outputString;
  while (*endOfString)
  {
    endOfString++;
  }

  // Add a dot to separate the integer and fractional parts
  *endOfString = '.';
  endOfString++;

  char* decimalPosition = endOfString;

  char decbuf[10]; // 10 with current multiply constant, as number can be 9 decimal max
  // Convert the fractional part to intermediate buffer
  if (integerPart < 0 && fractionalPart > 0)
  {
    // Fix for neg numbers (2's complement)
    fractionalPart = ( (~fractionalPart) & fractMask) + 1;
  }

  fractionalPart *= FP_MULTIPLY_CONST;
  itoa(fractionalPart, decbuf);

  // Get length of decimal part string
  int declen = strlen(decbuf);
  // Prepend with 9-(length of decimal part) zeros before adding decimal part
  while (declen < 9)
  {
    *endOfString = '0';
    endOfString++;
    declen++;
  }
  // Terminate string
  *endOfString = 0;

  // Convert and append the fractional part directly to the result
  strcat(endOfString, decbuf);

  // Truncate at number of decimals given by placing string terminator
  decimalPosition[numDecimals] = 0;
}


// Convert a string to a fixed-point number
fixed_point_t FP_StringToFP(char* decimalString)
{

  // Get index of the dot, also return int part if no dot
  char* dotp = strchr(decimalString, '.');
  if (dotp == 0) return strToInt(decimalString) << FP_FRACT_BITS;
  word dotIndex = dotp - decimalString;

  // Copy integer part
  char integerPart[12];
  memcpy(integerPart, decimalString, dotIndex);

  // Terminate integerPart
  integerPart[dotIndex] = 0;

  // Copy decimal part
  char decimalPart[12];
  strcpy(decimalPart, decimalString + dotIndex + 1);

  // Add integer part to result
  word result = strToInt(integerPart) << FP_FRACT_BITS;

  // Get decimal part as an integer
  word decimalPartInt = strToInt(decimalPart);

  // Return if no or invalid decimal part
  if (decimalPartInt == 0)
    return result;

  // Get number of decimals of the decimal part string which we need later
  word lenDecimalPart = strlen(decimalPart);

  // Alternative for 10^lenDecimalPart
  word divisionNumber = 1;
  while (lenDecimalPart > 0)
  {
    divisionNumber *= 10;
    lenDecimalPart--;
  }
  
  // Calculate decimal part
  word decResult = decimalPartInt << FP_FRACT_BITS;
  decResult = MATH_div(decResult, divisionNumber);

  // Correct for negative numbers (look at string because -0.x)
  if (decimalString[0] == '-')
  {
    result -= (1 << FP_FRACT_BITS);
    decResult = (1 << FP_FRACT_BITS) - decResult;
  }

  // Add decimal part to integer part
  word fractMask = FP_FRACT_MASK; // Needed to enforce a load(32) instruction, as constants have limited bits in arith operations
  result |= (decResult & fractMask);

  return result;
}


// Convert an integer to a fixed-point number
fixed_point_t FP_intToFP(word x)
{
  return x << FP_FRACT_BITS;
}


// Convert a fixed-point number to an integer (truncate)
word FP_FPtoInt(fixed_point_t x)
{
  return x >> FP_FRACT_BITS;
}


// Multiply two fixed-point numbers using special instruction
fixed_point_t FP_Mult(fixed_point_t a, fixed_point_t b)
{
  // r4: a, r5: b
  fixed_point_t retval = 0;
  asm(
    "multfp r4 r5 r2  ; r2 = a*b (FP signed)\n"
    "write -4 r14 r2  ; write result to stack for return\n"
    );

  return retval;
}
