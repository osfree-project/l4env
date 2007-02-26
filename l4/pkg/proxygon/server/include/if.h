/*
 * \brief   Proxygon internal if interface
 * \date    2004-09-30
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef _PROXYGON_IF_H_
#define _PROXYGON_IF_H_

#include <l4/thread/thread.h>

extern l4_threadid_t if_tid;   /* thread id of interface server thread */

/*** START IF SERVER THREAD AND REGISTER AT NAMES ***
 *
 * \returns 0 on success
 */
int start_if_server(void);

#endif /* _PROXYGON_IF_H_ */
