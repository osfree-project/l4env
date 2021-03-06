/*
 * \brief   L4 specific native startup code
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * Not really much to do here anymore - only defining the log tag
 * and size of used heap.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/log/l4log.h>
#include <l4/sys/l4int.h>
#include "startup.h"

char LOG_tag[9] = "vscrtest";
l4_ssize_t l4libc_heapsize = 500*1024;

void native_startup(int argc, char **argv) {
}
