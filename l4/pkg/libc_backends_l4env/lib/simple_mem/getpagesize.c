/**
 * \file   libc_backends_l4env/lib/simple_mem/getpagesize.c
 * \brief  Maps getpagesize() to L4_PAGESIZE.
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/sys/consts.h>

#include <unistd.h>

#ifdef USE_DIETLIBC
size_t getpagesize(void) /* Dietlibc does not conform to glibc here */
#else
int getpagesize(void)
#endif
{
    return L4_PAGESIZE;
}
