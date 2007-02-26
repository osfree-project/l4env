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

#ifndef _PROXYGON_VC_H_
#define _PROXYGON_VC_H_

/*** L4 INCLUDES ***/
#include <l4/thread/thread.h>


/*** START VC SERVER THREAD ***
 *
 * \param  sbuf1_size  size of string receive buffer1
 * \param  sbuf2_size  size of string receive buffer2
 * \param  sbuf3_size  size of string receive buffer3
 * \param  priority    priority of belonging vc thread
 * \param  vfbmode     is it a virtual frame buffer?
 * \retval vc_tid      thread id of virtual console
 *
 * \return 0 on success, negative error codes otherwise
 *
 * The three receive buffers are only needed by pslim_cscs() where
 * the YUV color components are written into separate buffers.
 */
int start_vc_server(l4_uint32_t sbuf1_size,
                    l4_uint32_t sbuf2_size,
                    l4_uint32_t sbuf3_size,
                    l4_uint8_t priority,
                    l4_threadid_t *vc_tid,
                    l4_int16_t vfbmode);


/*** CLOSE ALL VIRTUAL CONSOLES OF SPECIFIED TASK ***/
void piss_off_client(l4_threadid_t tid);

#endif /* _PROXYGON_VC_H_ */
