/* $Id$ */
/*****************************************************************************
 * exec/idl/exec-sys.h                                                       *
 *                                                                           *
 * Created:   22/08/2000                                                     *
 * Author(s): Frank Mehnert <fm3@os.inf.tu-dresden.de>                       *
 *                                                                           *
 * Common L4 Environment                                                     *
 *****************************************************************************/
       
#ifndef _EXEC_EXEC_SYS_H
#define _EXEC_EXEC_SYS_H

/* define prototype here because Flick uses it */
void bcopy(const void *from, void *to, unsigned n);

#define req_l4exec_bin_open		1
#define req_l4exec_bin_close		2
#define req_l4exec_bin_link		3
#define req_l4exec_bin_get_symbols	4
#define req_l4exec_bin_get_lines	5

#endif /* _EXEC_EXEC_SYS_H */

