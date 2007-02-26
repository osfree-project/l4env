/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/lib/src/ctor.c
 * \brief  Initcall handling
 *
 * \date   11/27/2002
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/dde_linux/ctor.h>
#include <l4/crtx/crt0.h>

void l4dde_do_initcalls(void)
{
  crt0_dde_construction();
}
