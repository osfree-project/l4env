/* $Id$ */
/**
 * \file   ldso/lib/ldso/helper.c
 * \brief  Support functions.
 *
 * \date   2005/05/12
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <l4/sys/l4int.h>
#include <l4/sys/kdebug.h>
#include <l4/util/util.h>

void*
memcpy(void *dst, const void *src, size_t len)
{
  register char *a = dst-1;
  register const char *b = src-1;

  while (len)
    {
      *++a = *++b;
      --len;
    }
  return dst;
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

int
strcmp(const char *s1, const char *s2)
{
  while (*s1 == *s2++)
    if (*s1++ == 0)
      return 0;
  return *s1 - *(s2 - 1);
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

char*
strchr(const char *s, int c)
{
  while (1)
    {
      if (*s == c)
	return (char *)s;
      if (!*s)
	return 0;
      s++;
    }
}
