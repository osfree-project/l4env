/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux26/lib/src/base/init.c
 * \brief  driver classes initialisation
 *
 * \author Marek Menzer <mm19@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <linux/device.h>
#include <linux/init.h>

extern int devices_init(void);
extern int buses_init(void);
extern int classes_init(void);
extern int firmware_init(void);
extern int platform_bus_init(void);
extern int sys_bus_init(void);
extern int cpu_dev_init(void);

/**
 *	driver_classes_init - initialize driver model.
 *
 *	Call the driver model init functions to initialize their
 *	subsystems. Called early from init/main.c.
 */

int __init l4dde_driver_classes_init(void)
{
	/* These are the basic core pieces */
	devices_init();
	buses_init();
	classes_init();
	firmware_init();

	/* These are also core pieces, but must come after the 
	 * core core pieces.
	 */
	platform_bus_init();
	sys_bus_init();
	cpu_dev_init();

	return 0;
}
