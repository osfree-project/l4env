/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/helper_dyn.c
 * \brief  Support functions to use l4rm with dynamic linking.
 *         We don't want to link the OSKit into the shared libloader.s
 *         library so we define here some standard libc functions.
 *
 * \date   06/15/2001
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/

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

