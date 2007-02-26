/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi/lib/include/__debug.h
 * \brief  Debug configuration
 *
 * \date   07/01/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _DSI___DEBUG_H
#define _DSI___DEBUG_H

/* enable debug assertions */
#define DEBUG_ASSERTIONS      1

/* be more verbose on errors */
#define DEBUG_ERRORS          1

/*****************************************************************************
 * set to 1 to enable subsystem debug output, 0 to disable                   *
 *****************************************************************************/

#define DEBUG_CTRL_DS         0
#define DEBUG_DATA_DS         0
#define DEBUG_THREAD          0
#define DEBUG_SYNC            0
#define DEBUG_SYNC_RECEIVE    0
#define DEBUG_SYNC_SEND       0
#define DEBUG_SOCKET          0
#define DEBUG_STREAM          0
#define DEBUG_CONNECT         0
#define DEBUG_SEND_PACKET     0
#define DEBUG_RECEIVE_PACKET  0
#define DEBUG_MAP_PACKET      0
#define DEBUG_COPY_PACKET     0
#define DEBUG_EVENT           0
#define DEBUG_SELECT          0

#endif /* !_DSI___DEBUG_H */
