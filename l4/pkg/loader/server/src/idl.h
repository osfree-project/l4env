/*!
 * \file	loader/server/src/idl.h
 * \brief	idl server functions
 *
 * \date	2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __IDL_H_
#define __IDL_H_

#include "loader-server.h"

#define MAX_ERROR_MSG	80

void server_loop(void) __attribute__ ((noreturn));
int  return_error_msg(int error, const char *msg, const char *fname);

#endif

