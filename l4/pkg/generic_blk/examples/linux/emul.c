/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/examples/linux/enum.c
 * \brief  Emulation of some C-Funktions which are inline-functions in the 
 *         Linux kernel but needed by the stub / libnames.
 *
 * \date   09/04/2003
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/


char *
strncpy(char * to, const char * from, int count);
char *
strncpy(char * to, const char * from, int count)
{
  register char *ret = to;

  while ((count-- > 0) && (*to++ = *from++))
    ;
  
  while (count-- > 0)
    *to++ = '\0';

  return ret;
}

void * 
memcpy(void * dst, const void*  src, unsigned int count);
void * 
memcpy(void * dst, const void*  src, unsigned int count) 
{
  register char * d = dst;
  register const char * s = src;

  ++count;      /* this actually produces better code than using count-- */
  while (--count) 
    {
      *d = *s;
      ++d; 
      ++s;
    }
  return dst;
}
