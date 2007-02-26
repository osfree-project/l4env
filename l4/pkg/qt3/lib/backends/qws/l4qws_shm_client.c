/* $Id$ */
/*****************************************************************************/
/**
 * \file   lib/backends/qws/l4qws_shm_client.c
 * \brief  L4-specific QSharedMemory implementation.
 *
 * \date   11/02/2004
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2004-2006 Technische Universitaet Dresden
 * This file is part of the Qt3 port for L4/DROPS, which is distributed under
 * the terms of the GNU General Public License 2. Please see the COPYING file
 * for details.
 */

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>

#include <l4/qt3/l4qws_shm_client.h>

/*
 * ************************************************************************
 */

DICE_DECLARE_ENV(l4qws_dice_shm_env);
l4_threadid_t     l4qws_shm_server   = L4_INVALID_ID_INIT;

/*
 * ************************************************************************
 */

void l4qws_shm_client_init(void) {

  if (names_waitfor_name("qws-shm", &l4qws_shm_server, 5000) == 0)
    LOG_Error("names-query for 'qws-shm' failed\n");
}
