/*
 * \brief   Proxygon events support interface
 * \date    2004-10-04
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

#ifndef _PROXYGON_EVENTS_H_
#define _PROXYGON_EVENTS_H_

/*** START EVENTS SERVER THREAD ***
 *
 * \returns 0 on success
 */
int start_events_server(void);

#endif /* _PROXYGON_EVENTS_H_ */
