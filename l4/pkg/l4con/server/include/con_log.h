/* $Id$ */
/**
 * \file	con/server/include/con_log.h
 * \brief	logging macros
 *
 * \date	2001
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * 		Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef _CON_LOG_H
#define _CON_LOG_H

/* L4 includes */
#include <l4/log/l4log.h>
#include <l4/util/macros.h>

/* OSKit includes */
#include <stdio.h>

#define CONTAG		"con"

#define L4MSG(format, args...)			\
  printf("(%s:%d): " format,			\
	 __FILE__, __LINE__ , ## args)

#endif /* !_CON_LOG_H */
