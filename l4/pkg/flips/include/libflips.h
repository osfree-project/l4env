/**
 * \file   flips/include/libflips.h
 * \brief  FLexiple IP Stack client library API
 *
 * \date   02/03/2006
 * \author Norman Feske <nf2@inf.tu-dresden.de>
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __FLIPS_INCLUDE_LIBFLIPS_H_
#define __FLIPS_INCLUDE_LIBFLIPS_H_

/** FUNCTIONS FOR ACCESSING PROC ENTRIES ***/

extern int flips_proc_read(const char *path, char *dst,
                           int offset, int len);
extern int flips_proc_write(const char *path, char *src,
                            int len);
#endif
