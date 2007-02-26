/* $Id$ */
/*****************************************************************************/
/**
 * \file   lib/backends/misc/new_delete.cc
 * \brief  new/delete operators
 *
 * \date   11/02/2004
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2004-2006 Technische Universitaet Dresden
 * This file is part of the Qt3 port for L4/DROPS, which is distributed under
 * the terms of the GNU General Public License 2. Please see the COPYING file
 * for details.
 */

/*** GENERAL INCLUDES ***/
#include <stdlib.h>

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/sys/types.h>

// Default heap size for Qt apps is 16 MB. It can be overridden by
// applications, e.g.: 'l4_ssize_t l4libc_heapsize = 64 * 1048576;'
l4_ssize_t __attribute__((weak)) l4libc_heapsize = 16 * 1048576;


void *operator new(size_t s) {
  return malloc(s);
}

void *operator new[](size_t s) {
  return malloc(s);
}

void operator delete(void *p) {
  free(p);
}

void operator delete[](void *p) {
  free(p);
}

