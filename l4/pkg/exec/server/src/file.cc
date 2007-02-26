/* $Id$ */
/**
 * \file	exec/server/src/file.cc
 * \brief	Basic file class
 *
 * \date	10/30/2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include "file.h"

#include <l4/env/errno.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

file_t::file_t()
  : _fname(0)
{
}

void
file_t::msg(const char *format, ...)
{
  va_list list;

  printf("%s: ", _fname);
  va_start(list, format);
#ifdef USE_OSKIT
  vprintf(format, (oskit_va_list)list);
#else
  vprintf(format, list);
#endif
  va_end(list);
  printf("\n");
}

int
file_t::check(int error, const char *format, ...)
{
  if (error)
    {
      va_list list;
      
      printf("%s: Error %d (%s) ", _fname, error, l4env_errstr(error));
      va_start(list, format);
#ifdef USE_OSKIT
      vprintf(format, (oskit_va_list)list);
#else
      vprintf(format, list);
#endif
      va_end(list);
      printf("\n");
    }
  
  return error;
}

void
file_t::set_fname(const char *pathname)
{
  if (!(_fname = strrchr(pathname, '/')))
    _fname = pathname;
  else
    _fname++;	// skip '/'
}

