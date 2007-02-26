/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/lib/client/include/__request.h
 * \brief  Request handling.
 *
 * \date   02/11/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _GENERIC_BLK___REQUEST_H
#define _GENERIC_BLK___REQUEST_H

/* prototypes */
void
blkclient_init_requests(void);

void
blkclient_set_request_status(l4_uint32_t req_handle, 
			     l4_uint32_t status,
			     int error);

#endif /* !_GENERIC_BLK___REQUEST_H */
