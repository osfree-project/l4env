/**
 * \file   l4vfs/term_server/examples/vttest/config.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Björn Döbel  <doebel@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_TERM_SERVER_EXAMPLES_VTTEST_CONFIG_H_
#define __L4VFS_TERM_SERVER_EXAMPLES_VTTEST_CONFIG_H_
/* config.h.  Generated automatically by configure.  */
/* $Id$ */

/* define this if you have the alarm() function */
//#define HAVE_ALARM 1

/* define this if you have <fcntl.h> */
#define HAVE_FCNTL_H 1

/* define this if you have <ioctl.h> */
/* #undef HAVE_IOCTL_H */

/* define this if you have the Xenix rdchk() function */
/* #undef HAVE_RDCHK */

/* define this if the POSIX VDISABLE symbol is defined, not equal to -1 */
#define HAVE_POSIX_VDISABLE 1

/* define this if you have <sgtty.h> */
//#define HAVE_SGTTY_H 1

/* define this if you have <stdlib.h> */
#define HAVE_STDLIB_H 1

/* define this if you have <string.h> */
#define HAVE_STRING_H 1

/* define this if you have <sys/filio.h> */
/* #undef HAVE_SYS_FILIO_H */

/* define this if you have <sys/ioctl.h> */
#define HAVE_SYS_IOCTL_H 1

/* define this if you have the tcgetattr() function */
#define HAVE_TCGETATTR 1

/* define this if you have <termio.h> */
#define HAVE_TERMIO_H 1

/* define this if you have <termios.h> */
#define HAVE_TERMIOS_H 1

/* define this if you have <unistd.h> */
#define HAVE_UNISTD_H 1

/* define this if you have the usleep() function */
//#define HAVE_USLEEP 1

/* define this to be the return-type of functions manipulated by 'signal()' */
#define RETSIGTYPE void

/* define this if we can use ioctl(,FIONREAD,) */
/* #undef USE_FIONREAD */

#ifdef __L4

#include <l4/log/l4log.h>
#include <unistd.h>
#undef sleep
#include <l4/util/util.h>

#define sleep(x) l4_sleep(1000*x)

int do_main( int argc, char **argv );

#endif

#endif
