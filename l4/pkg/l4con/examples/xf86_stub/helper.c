/*!
 * \file	con/examples/xf86_stub/helper.c
 * \brief	
 *
 * \date	01/2002
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include <stdarg.h>
//#include <stdio.h>
#include <l4/sys/types.h>
#include <l4/log/l4log.h>
#include <l4/env/env.h>
#include <xf86.h>

#undef size_t
#undef strcpy
#undef strncpy

typedef unsigned int size_t;

char* strcpy(char *to, const char *from);
char* strncpy(char *to, const char *from, size_t count);

void
LOG_flush(void)
{
}

void
LOG_log(const char *function, const char *format, ...)
{
}

#if 0
int
printf(const char *format, ...)
{
  va_list list;
  va_start(list, format);
  xf86DrvMsg(0, 5 /*X_ERROR*/, format, list);
  va_end(list);
  return 0;
}
#endif

l4_threadid_t
l4env_get_default_dsm(void)
{
  return L4_INVALID_ID;
}

char*
strncpy(char *to, const char *from, size_t count)
{
  register char *ret = to;

  while (count > 0) 
    {
      count--;
      if ((*to++ = *from++) == '\0')
	break;
    }

  while (count > 0) 
    {
      count--;
      *to++ = '\0';
    }
  
  return ret;
}

char*
strcpy(char *to, const char *from)
{
  register char *ret = to;

  while ((*to++ = *from++) != 0);

  return ret;
}

