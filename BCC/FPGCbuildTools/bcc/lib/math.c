/*
* Math library
* Contains functions math operation that are not directly supported by the ALU
*/

// Divide two signed integer numbers using MU
word MATH_div(word dividend, word divisor)
{
  word retval = 0;
  asm(
    "load32 0xC02744 r2 ; r2 = addr idiv_writea\n"
    "write 0 r2 r4 ; write a to divider\n"
    "write 1 r2 r5 ; write b to divider and perform signed division\n"
    "read 1 r2 r2  ; read result to r2\n"
    "write -4 r14 r2  ; write result to stack for return\n"
    );
  return retval;
}

// Modulo from division of two signed integer numbers using MU
word MATH_mod(word dividend, word divisor)
{
  word retval = 0;
  asm(
    "load32 0xC02744 r2 ; r2 = addr idiv_writea\n"
    "write 0 r2 r4 ; write a to divider\n"
    "write 3 r2 r5 ; write b to divider and perform signed modulo\n"
    "read 3 r2 r2  ; read remainder to r2\n"
    "write -4 r14 r2  ; write result to stack for return\n"
    );
  return retval;
}

// Divide two unsigned integer numbers using MU
word MATH_divU(word dividend, word divisor) 
{
  word retval = 0;
  asm(
    "load32 0xC02744 r2 ; r2 = addr idiv_writea\n"
    "write 0 r2 r4 ; write a to divider\n"
    "write 2 r2 r5 ; write b to divider and perform unsigned division\n"
    "read 2 r2 r2  ; read result to r2\n"
    "write -4 r14 r2  ; write result to stack for return\n"
    );
  return retval;
}

// Modulo from division of two unsigned integer numbers using MU
word MATH_modU(word dividend, word divisor) 
{
  word retval = 0;
  asm(
    "load32 0xC02744 r2 ; r2 = addr idiv_writea\n"
    "write 0 r2 r4 ; write a to divider\n"
    "write 4 r2 r5 ; write b to divider and perform unsiged modulo\n"
    "read 4 r2 r2  ; read remainder to r2\n"
    "write -4 r14 r2  ; write result to stack for return\n"
    );
  return retval;
}

// Signed Division and Modulo without / and %
word MATH_SW_divmod(word dividend, word divisor, word* rem)
{
  word quotient = 1;

  word neg = 1;
  if ((dividend>0 &&divisor<0)||(dividend<0 && divisor>0))
    neg = -1;

  // Convert to positive
  word tempdividend = (dividend < 0) ? -dividend : dividend;
  word tempdivisor = (divisor < 0) ? -divisor : divisor;

  if (tempdivisor == tempdividend) {
    *rem = 0;
    return 1*neg;
  }
  else if (tempdividend < tempdivisor) {
    if (dividend < 0)
      *rem = tempdividend*neg;
    else
      *rem = tempdividend;
    return 0;
  }
  while (tempdivisor<<1 <= tempdividend)
  {
    tempdivisor = tempdivisor << 1;
    quotient = quotient << 1;
  }

  // Call division recursively
  if(dividend < 0)
    quotient = quotient*neg + MATH_SW_divmod(-(tempdividend-tempdivisor), divisor, rem);
  else
    quotient = quotient*neg + MATH_SW_divmod(tempdividend-tempdivisor, divisor, rem);
   return quotient;
}

word MATH_SW_div(word dividend, word divisor)
{
  word rem = 0;
  return MATH_SW_divmod(dividend, divisor, &rem);
}

word MATH_SW_mod(word dividend, word divisor)
{
  word rem = 0;
  MATH_SW_divmod(dividend, divisor, &rem);
  return rem;
}


// Unsigned Division and Modulo without / and %
word MATH_SW_divmodU(word dividend, word divisor, word mod)
{
  word quotient = 0;
  word remainder = 0;

  if(divisor == 0) 
    return 0;

  word i;
  for(i = 31 ; i >= 0 ; i--)
  {
    quotient = quotient << 1;
    remainder = remainder << 1;
    remainder = remainder | ((unsigned) (dividend & (1 << i)) >> i);

    if((unsigned int) remainder >= (unsigned int) divisor)
    {
      remainder = remainder - divisor;
      quotient = quotient | 1;
    }

    if (i == 0)
      if (mod == 1)
        return remainder;
      else
        return quotient;
  }

  return 0;
}

// Unsigned positive integer division
word MATH_SW_divU(word dividend, word divisor) 
{
  return MATH_SW_divmodU(dividend, divisor, 0);
}

// Unsigned positive integer modulo
word MATH_SW_modU(word dividend, word divisor) 
{
  return MATH_SW_divmodU(dividend, divisor, 1);
}


// Returns absolute value
word MATH_abs(word x)
{
  if (x >= 0)
    return x;
  else
    return -x;
}


word division(word dividend, word divisor)
{
    return MATH_divU(dividend, divisor);
}

word modulo(word dividend, word divisor)
{
    return MATH_modU(dividend, divisor);
}
