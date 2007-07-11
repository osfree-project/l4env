/**
 * \file   ldso/lib/ldso/binary_name.h
 * \brief  Determine the name of the binary.
 *
 * \date   01/2006
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef _BINARY_NAME_H_
#define _BINARY_NAME_H_

#include <l4/sys/l4int.h>

const char* binary_name(char *buffer, l4_size_t size);

#endif
