/* $Id$ */
/**
 * \file   l4rm/lib/src/helper_dyn.c
 * \brief  Support functions to use l4rm with dynamic linking.
 *         We don't want to link the OSKit into the shared libloader.s
 *         library so we define here some standard libc functions.
 *
 * \date   06/15/2001
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <l4/sys/kdebug.h>
#include <l4/util/util.h>

void
*memcpy(void *dest, const void *src, unsigned int n)
{
  int d0, d1, d2;
  asm volatile(
      "rep   movsl	\n\t"
      "testb $2,%b4	\n\t"
      "je    1f		\n\t"
      "movsw		\n\t"
      "1: 		\n\t"
      "testb $1,%b4	\n\t"
      "je 2f		\n\t"
      "movsb		\n\t"
      "2:"
      : "=&c"(d0), "=&D"(d1), "=&S" (d2)
      :"0"(n/4), "q"(n), "1"((long) dest), "2"((long)src)
      : "memory");
  return dest;
}

int
memcmp(const void *dst, const void *src, unsigned count)
{
  register int r;
  register const char *d=dst;
  register const char *s=src;
  ++count;
  while (--count)
    {
      if ((r=(*d - *s)))
	return r;
      ++d;
      ++s;
    }
  return 0;
}

void
*memset(void *dest, int c, unsigned int n)
{
  register char *d = dest;

  while (n-- > 0)
    *d++ = c;

  return dest;
}

int
strncmp(const char *s1, const char *s2, unsigned int n)
{
  for (; n>0; s1++, s2++, n--)
    {
      if (*s1 != *s2)
	return *s1 - *s2;
      if (*s1 == 0)
	break;
    }

  return 0;
}

char*
strncpy(char *dest, const char *src, unsigned int n)
{
  register char *ret = dest;

  while (n > 0) 
    {
      n--;
      if ((*dest++ = *src++) == '\0')
	break;
    }

  while (n > 0)
    {
      n--;
      *dest++ = '\0';
    }
  
  return ret;
}

char *
strcpy(char *dest, const char *src)
{
  register char *ret = dest;

  while ((*dest++ = *src++) != 0);
  
  return ret;
}

unsigned
strlen(const char *s)
{
  const char *o = s;
  while (*s++);

  return s-1-o;
}

char*
strstr(const char *haystack, const char *needle)
{
  unsigned nl=strlen(needle);
  unsigned hl=strlen(haystack);
  int i;
  if (!nl)
    goto found;
  if (nl>hl)
    return 0;
  for (i=hl-nl+1; i; --i)
    {
      if (*haystack==*needle && !memcmp(haystack,needle,nl))
	found:
	  return (char*)haystack;
      ++haystack;
    }
  return 0;
}

