/* $Id$ */
/*****************************************************************************/
/**
 * \file   lib/backends/qws/l4qws_key.c
 * \brief  Very simple functionality to create 'unique' keys to address shared
 *         memory regions and QLocks.
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

#include <l4/qt3/l4qws_qlock-client.h>

l4qws_key_t l4qws_key(const char *fn, char c) {

  // fn is supposed to point to the same file (local socket)
  return 42 + c;
}


