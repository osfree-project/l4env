/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi/lib/include/__packet.h
 * \brief  Internal packet handling
 *
 * \date   07/26/2000 
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _DSI___PACKET_H
#define _DSI___PACKET_H

/* DSI includes */
#include <l4/dsi/types.h>

/* macros to check packet flags */
#define PACKET_RELEASE_CALLBACK(p)  (p->flags & DSI_PACKET_RELEASE_CALLBACK)

#endif /* !_DSI___PACKET_H */
