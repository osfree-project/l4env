/*
 * \brief   Nitpicker client representation interface
 * \date    2005-09-08
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2005  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef _NITPICKER_CLIENT_H_
#define _NITPICKER_CLIENT_H_

#include "nitpicker-server.h"
#include "view.h"
#include "buffer.h"

#define CLIENT_LABEL_LEN  32

/*
 * Every client is represented in Nitpicker as one chunk of
 * memory, donated by the client. At the beginning of the
 * memory block, we have the client information structure,
 * followed by the arrays of views and buffers.
 */

struct client;
typedef struct client {
	struct client    *next;         /* next client in list  */
	view             *views;        /* array of views       */
	buffer           *buffers;      /* array of buffers     */
	int               max_views;    /* size of view array   */
	int               max_buffers;  /* size of buffer array */
	CORBA_Object_base tid;          /* associated client id */
	view             *bg;           /* background view      */
	char              label[CLIENT_LABEL_LEN];
} client;


/*** INIT CLIENT STRUCTURE AND ADD CLIENT TO CLIENT LIST ***/
extern void add_client(CORBA_Object tid, char *addr, int max_views, int max_buffers);


/*** REGISTER AND INITIALIZE NEW CLIENT ***
 *
 * This function closes all views and unmaps all buffers of
 * a client and frees the local client data structures.
 *
 * \param tid client id used for client lookup
 *
 */
extern void remove_client(CORBA_Object tid);


/*** FIND THE CLIENT STRUCTURE FOR A GIVEN CLIENT ID ***/
extern client *find_client(CORBA_Object tid);


#endif /* _NITPICKER_CLIENT_H_ */
