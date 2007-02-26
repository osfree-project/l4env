/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stddef.h>

void *memset(void *s, int c, size_t n);

void *memset(void *s, int c, size_t n)
{
  size_t x;
  char *p = s;
  for (x=0; x<n; ++x)
    *p++ = c;

  return s;
}
