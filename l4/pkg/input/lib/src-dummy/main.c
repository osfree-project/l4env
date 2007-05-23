/* $Id$ */
/*
 * Dummy inputlib with no functionality.
 *
 * by Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/sys/ipc.h>
#include <l4/input/libinput.h>
#include <l4/env/errno.h>

int l4input_ispending(void)
{
	return 0;
}

int l4input_flush(void *buffer, int count)
{
	return 0;
}

int l4input_pcspkr(int tone)
{
	return -L4_ENODEV;
}

int l4input_init(int prio, void (*handler)(struct l4input *))
{
	return 0;
}
