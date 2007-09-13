/*
 * \brief   Glue code to use the TPM v1.2 TIS layer with STPM
 * \date    2006-05-12
 * \author  Bernhard Kauer <kauer@tudos.org>
 * \author  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007       Bernhard Kauer <kauer@tudos.org>
 * Copyright (C) 2006-2007  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <l4/crtx/ctor.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/util/util.h>
#include <l4/util/macros.h>
#include <l4/util/rdtsc.h>
#include <l4/l4rm/l4rm.h>
#include <l4/generic_io/libio.h>
#include <l4/rmgr/librmgr.h>
#include <l4/env/errno.h>

#include "tis.h"

///////////////////////////////////////////////////

/**
 * Transmit a blob to a TPM.
 */
int
stpm_transmit(const char *write_buf, int write_count, char **read_buf, int *read_count)
{
  int res;
  res = tis_transmit((const unsigned char*)write_buf, write_count,
                     (unsigned char*)*read_buf, *read_count);
  *read_count = 0 < res ? res : 0;
  return res;
}

/**
 * Abort the tpm call.
 *
 * FIXME: Not implemented.
 */
int
stpm_abort(void)
{
    return -L4_EINVAL;
}
///////////////////////////////////////////////////

/**
 * Map pages containing TIS area from RMGR
 */
static int
map_tis_area(l4_addr_t *map_base_out)
{
    int ret;

    l4io_info_t *io_info;

    ret = l4io_init(&io_info, L4IO_DRV_INVALID);
    if (ret < 0)
    {
        printf("Failed to initialize I/O lib: %d\n", ret);
        return ret;
    }

    *map_base_out = l4io_request_mem_region(TIS_BASE, 0x5000, 0);
    if (*map_base_out == 0)
    {
        printf("Failed to get TIS area from L4IO");
        return -1;
    }

    printf("Mapped TIS area at 0x%08lx\n", *map_base_out);

    return 0;
}

/**
 * Init everthing we need to do our job!
 */
static void
init(void)
{    
    int ret;
    l4_addr_t map_base;
 
    ret = map_tis_area(&map_base);
    if (ret < 0)
    {
       printf("Failed to map TIS region: %d\n", ret);
       exit(-1);
    }

    ret = tis_init(map_base);
    if (ret < 0)
    {
       printf("Failed to initalize TIS driver: %d\n", ret);
       exit(-1);
    }

    ret = tis_access(TIS_LOCALITY_0, 0);
    if (ret == 0)
    {
       printf("Failed to activate locality: %d\n", ret);
       exit(-1);
    }
}

L4C_CTOR(init, L4CTOR_AFTER_BACKEND);

