/* $Id$ */

/*	con/server/include/con_log.h
 *
 *	logging macros
 */

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
