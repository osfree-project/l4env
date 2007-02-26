/* $Id$ */
/*****************************************************************************
 * exec/include/l4/exec/errno.h                                              *
 *                                                                           *
 * Created:   08/18/2000                                                     *
 * Author(s): Frank Mehnert <fm3@os.inf.tu-dresden.de>                       *
 *                                                                           *
 * Common L4 environment                                                     *
 * L4 execution layer public error codes                                     *
 *****************************************************************************/

#ifndef _L4_EXEC_ERRNO_H
#define _L4_EXEC_ERRNO_H

#define L4_EXEC_BADFORMAT	2001	/* invalid file format */
#define L4_EXEC_BADARCH		2002	/* invalid architecture */
#define L4_EXEC_CORRUPT		2003	/* file is corrupt */
#define L4_EXEC_NOSTANDARD	2004	/* defined entry point not found */
#define L4_EXEC_LINK		2005	/* errors while linking */

#endif /* _L4_EXEC_ERRNO_H */

