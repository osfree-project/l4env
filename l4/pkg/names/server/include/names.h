/*!
 * \file   names/server/include/names.h
 * \brief  Internal prototypes
 *
 * \date   05/27/2003
 * \author Uwe Dannowski <Uwe.Dannowski@ira.uka.de>
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __NAMES_SERVER_INCLUDE_NAMES_H_
#define __NAMES_SERVER_INCLUDE_NAMES_H_

#include <l4/sys/types.h>

/* Internal prototypes */
extern l4_int32_t server_names_register(l4_threadid_t *client,
                                        const char *name,
                                        const l4_threadid_t *id,
                                        int weak);
extern int preregister(void);

#endif
