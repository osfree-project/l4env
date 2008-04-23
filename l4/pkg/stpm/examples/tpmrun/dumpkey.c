/*
 * @author Alexander Boettcher
 */

/* (c) 2008 Technische Universitaet Dresden
 * This file is part of TUD-OS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <stdlib.h>
#include <l4/log/l4log.h>
#include "tpmrun.h"

void dumpkey(keydata * _key)
{
  unsigned int size = sizeof (*_key);
  int i;
  unsigned char * key = (unsigned char *)_key;
  char hex [2];

  for (i=0; i<size; i++)
  {
    sprintf(hex, "%02x", key[i]);
    LOG_printf("%2s", hex);
  }
  LOG_printf("\n");
}

void redumpkey(const char * dumped, keydata * _key)
{
  unsigned int size = sizeof (*_key);
  int i;
  unsigned char * key = (unsigned char *)_key;
  char hex [] = { 0, 0, 0};

  for (i=0; i<size; i++)
  {
    hex[0] = *(dumped++);
    hex[1] = *(dumped++);
    key[i] = strtoul(hex, NULL, 16);
  }
}
