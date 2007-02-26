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

/*** GENERIC INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>

/*** L4 INCLUDES ***/
#include <l4/log/l4log.h>
#include <l4/util/kip.h>
#include <l4/util/util.h>

/*** LOCAL INCLUDES ***/
#include "startup.h"

char LOG_tag[9] = "MiniTop";
l4_ssize_t l4libc_heapsize = 500*1024;


void native_startup(int argc, char **argv) {
  l4_kernel_info_t *kip = l4util_kip_map();
  if (!kip)
    {
      printf("Cannot map kip");
      exit(-1);
    }
  if (kip->version != 0x01004444)
    {
      printf("Only works with Fiasco!");
      exit(-2);
    }
}
