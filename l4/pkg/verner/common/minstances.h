/*
 * \brief   Support for multiple instances of VERNER
 * \date    2004-02-15
 * \author  Carsten Rietzschel <cr7@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2004  Carsten Rietzschel  <cr7@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*
 *  This file is stolen from GPL'ed DOpE.
 */

#ifndef _MINSTANCES_HH_
#define _MINSTANCES_HH_

#include <l4/sys/types.h>

int register_instance(char* name);
l4_threadid_t find_instance(char* name, int instance_no, int timeout);

#endif
