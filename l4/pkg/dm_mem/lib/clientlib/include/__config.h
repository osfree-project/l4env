/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_mem/client-lib/include/__config.h
 * \brief  Generic dataspace manager client library configuration
 *
 * \date   01/31/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _DM_MEM___CONFIG_H
#define _DM_MEM___CONFIG_H

/* L4 includes */
#include <l4/sys/types.h>

/*****************************************************************************
 *** default values for l4dm_allocate*
 *****************************************************************************/

/**
 * default alignment
 */
#define DMMEM_ALLOCATE_ALIGN  (L4_PAGESIZE)

#endif /* !_DM_MEM___CONFIG_H */
