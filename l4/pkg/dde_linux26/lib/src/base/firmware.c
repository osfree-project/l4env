/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux26/lib/src/base/firmware.c
 * \brief  firmware subsystem hoohaw.
 *
 * \author Marek Menzer <mm19@os.inf.tu-dresden.de>
 *
 * Original by Patrick Mochel and Open Source Development Labs
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/init.h>

static decl_subsys(firmware,NULL,NULL);

int firmware_register(struct subsystem * s)
{
	kset_set_kset_s(s,firmware_subsys);
	return subsystem_register(s);
}

void firmware_unregister(struct subsystem * s)
{
	subsystem_unregister(s);
}

int __init firmware_init(void)
{
	return subsystem_register(&firmware_subsys);
}

EXPORT_SYMBOL(firmware_register);
EXPORT_SYMBOL(firmware_unregister);
