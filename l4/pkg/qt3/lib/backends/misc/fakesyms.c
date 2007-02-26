/* $Id$ */
/*****************************************************************************/
/**
 * \file   lib/backends/misc/fakesyms.c
 * \brief  dummy function implementations.
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

/* Idea shamelessly stolen from l4/pkg/zlib/lib/stdio-ux/stdio.c: DUP(x) ... */
#define PROTO_IMPL(x) x; x

// dummies to make it link, until I fixed it
PROTO_IMPL(int getsockopt(void)){ return 0; }
PROTO_IMPL(int getpeername(void)){ return -1; }



