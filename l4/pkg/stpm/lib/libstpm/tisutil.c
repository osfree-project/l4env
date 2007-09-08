/*
 * \brief   Glue code to use the TPM v1.2 TIS layer with STPM
 * \date    2006-05-12
 * \author  Bernhard Kauer <kauer@tudos.org>
 * \author  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006  Bernhard Kauer <kauer@tudos.org>
 * Copyright (C) 2006  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <l4/util/util.h>
#include <l4/env/errno.h>

#include "util.h"

///////////////////////////////////////////////////

/**
 * Wait roughly a given number of milliseconds.
 */
void
wait(int ms)
{
  l4_sleep(ms);
}


/**
 * Output a single char.
 */
int
out_char(unsigned value)
{
  printf("%c", (char) value);
  return value;
}


/**
 * Output a string.
 */
void
out_string(const char *value)
{
  printf("%s\n", value);
}


/**
 * Output a string with prefix.
 */
void
out_info(const char *value)
{
  printf("TIS: %s\n", value);
}


/**
 * Output a single hex value.
 */
void
out_hex(unsigned value, unsigned len)
{
  unsigned i;
  for (i=32; i>0; i-=4)
    {
      unsigned a = value>>(i-4);
      if (a || i==4 || (len*4>=i))
        {
          out_char("0123456789abcdef"[a & 0xf]);
        }
    }
}

/**
 * Output a string followed by a single hex value.
 */
void
out_description(const char *prefix, unsigned int value)
{
  out_string(prefix);
  out_char(' ');
  out_hex(value, 0);
  out_char('\n');
}
