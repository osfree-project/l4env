/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi/include/errno.h 
 * \brief  DSI error codes.
 *
 * \date   07/02/2000 
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _DSI_ERRNO_H
#define _DSI_ERRNO_H

/* dataspace management */
#define DSI_ENODSM        0x10000001       /* no dataspace manager found */
#define DSI_ENOSOCKET     0x10000002       /* no socket descriptor available */
#define DSI_ENOSTREAM     0x10000003       /* no stream descriptor available */
#define DSI_ECONNECT      0x10000004       /* connect failed/not connected */
#define DSI_ENOSGELEM     0x10000005       /* no scatter gather list element 
					    * available */
#define DSI_ENOPACKET     0x10000006       /* no packet descriptor available */
#define DSI_ENODATA       0x10000007       /* no data in packet */
#define DSI_ESGLIST       0x10000008       /* scatter gather list too long */
#define DSI_EGAP          0x10000009       /* chunk contains no valid data,
					    * it describes a gap in the 
					    * stream */
#define DSI_EEOS          0x1000000A       /* end of stream */
#define DSI_ECOMPONENT    0x1000000B       /* component operation failed */
#define DSI_EPHYS	  0x1000000C       /* chunk contains physical address */

#endif /* !_DSI_ERRNO_H */
