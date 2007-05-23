/*
 * \brief   Nitpicker L4 specific startup and configuration
 * \date    2004-08-24
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** L4 INCLUDES ***/
#include <l4/log/l4log.h>
#include <l4/generic_io/libio.h>

/*** LOCAL INCLUDES ***/
#include "nitpicker.h"

char LOG_tag[9] = "nitpicker";
l4_ssize_t l4libc_heapsize = 500*1024;

l4io_info_t *l4io_page = (l4io_info_t*) 0;  /* l4io info page */

int native_startup(int argc, char **argv) {

	TRY(l4io_init(&l4io_page, L4IO_DRV_INVALID), "Couldn't connect to L4IO server!");

	return 0;
}
