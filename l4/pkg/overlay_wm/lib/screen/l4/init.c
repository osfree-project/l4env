/*
 * \brief   Overlay Screen library - connect to overlay wm server
 * \date    2003-08-18
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Overlay WM package, which is distributed
 * under the  terms  of the GNU General Public Licence 2. Please see
 * the COPYING file for details.
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>

/*** L4 INCLUDES ***/
#include <l4/names/libnames.h>

/*** LOCAL INCLUDES ***/
#include "overlay-client.h"
#include "ovl_screen.h"

static CORBA_Object_base ovl_tid;
CORBA_Object ovl_screen_srv = &ovl_tid;
int ovl_phys_width, ovl_phys_height, ovl_phys_mode;


/*** INTERFACE: INIT OVERLAY SCREEN LIBRARY ***/
int ovl_screen_init(char *ovl_name) {
	static int initialized;
	CORBA_Environment env = dice_default_environment;

	if (initialized) return 0;
	if (!ovl_name) ovl_name = "OvlWM";
	if (names_waitfor_name(ovl_name, ovl_screen_srv, 10000) == 0) {
		printf("%s is not registered at names!\n",ovl_name);
		return -1;
	}
	
	initialized = 1;

	/* request information about the physical screen */
	overlay_get_screen_info_call(ovl_screen_srv,  &ovl_phys_width,
	                            &ovl_phys_height, &ovl_phys_mode, &env);
	printf("found %s at names, physical screen is %dx%d@%d\n", ovl_name,
	       ovl_phys_width, ovl_phys_height, ovl_phys_mode);
	return 0;
}

