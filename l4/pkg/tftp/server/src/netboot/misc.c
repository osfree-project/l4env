#include <ctype.h>
#include <l4/util/util.h>
#include "grub.h"

int
safe_parse_maxint (char **str_ptr, int *myint_ptr)
{
  char *ptr = *str_ptr;
  int myint = 0;
  int mult = 10, found = 0;

  /*
   *  Is this a hex number?
   */
  if (*ptr == '0' && tolower (*(ptr + 1)) == 'x')
    {
      ptr += 2;
      mult = 16;
    }

  while (1)
    {
      /* A bit tricky. This below makes use of the equivalence:
	 (A >= B && A <= C) <=> ((A - B) <= (C - B))
	 when C > B and A is unsigned.  */
      unsigned int digit;

      digit = tolower (*ptr) - '0';
      if (digit > 9)
	{
	  digit -= 'a' - '0';
	  if (mult == 10 || digit > 5)
	    break;
	  digit += 10;
	}

      found = 1;
      if (myint > ((MAXINT - digit) / mult))
	{
	  errnum = ERR_NUMBER_OVERFLOW;
	  return 0;
	}
      myint = (myint * mult) + digit;
      ptr++;
    }

  if (!found)
    {
      errnum = ERR_NUMBER_PARSING;
      return 0;
    }

  *str_ptr = ptr;
  *myint_ptr = myint;

  return 1;
}

int
getdec (const char **ptr)
{
  const char *p = *ptr;
  int ret = 0;
  
  if (*p < '0' || *p > '9')
    return -1;
  
  while (*p >= '0' && *p <= '9')
    {
      ret = ret * 10 + (*p - '0');
      p++;
    }
  
  *ptr = p;
  
  return ret;
}

int
nul_terminate (char *str)
{
  int ch;
  
  while (*str && ! grub_isspace (*str))
    str++;

  ch = *str;
  *str = 0;
  return ch;
}

#ifdef USE_OSKIT
int32_t
random(void)
{
  static int32_t seed = 0;
  int32_t q;
  if (!seed) /* Initialize linear congruential generator */
    seed = currticks() + *(int32_t *)&arptable[ARP_CLIENT].node
	 + ((int16_t *)arptable[ARP_CLIENT].node)[2];
  /* simplified version of the LCG given in Bruce Schneier's
     "Applied Cryptography" */
  q = seed/53668;
  if ((seed = 40014*(seed-53668*q) - 12211*q) < 0)
    seed += 2147483563L;
  return seed;
}
#endif

void
poll_interruptions(void)
{
}

void
twiddle(void)
{
}
