/**
 * \file   input/lib/src/emul_res.c
 * \brief  L4INPUT: Linux I/O resource management emulation
 *
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 * Currently we only support I/O port requests.
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/generic_io/libio.h>


void release_region(unsigned long start, unsigned long len)
{
#ifndef ARCH_arm
	l4io_release_region(start, len);
#endif
}


void * request_region(unsigned long start, unsigned long len, const char *name)
{
#ifndef ARCH_arm
	int err = l4io_request_region(start, len);
	return err ? 0 : (void *)1;
#else
	return 0;
#endif
}
