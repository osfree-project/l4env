/*
 * \brief   L4 specific startup of DOpEcmd
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * We just define the logtag and heapsize here.
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

char LOG_tag[9] = "DOpEcmd";
l4_ssize_t l4libc_heapsize = 500*1024;


void native_startup(int argc, char **argv) {
}
