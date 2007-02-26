/*
 * \brief   Wraper for UNIX-stdcalls to grubfs for VERNER's sync component
 * \date    2004-02-11
 * \author  Carsten Rietzschel <cr7@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2003  Carsten Rietzschel  <cr7@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef _DROPS_COMPAT_H_
#define _DROPS_COMPAT_H_

#include "fileops.h"

#define printf printf
#define sprintf sprintf
#define open fileops_open
#define read  fileops_read
#define close fileops_close
#define lseek fileops_lseek
#define write fileops_write
#define ftruncate fileops_ftruncate

#define fopen #error
#define fwrite #error

#endif
