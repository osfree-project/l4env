/*
 * \brief   Nitpicker client representation
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

/*** L4 INCLUDES ***/
#include <l4/l4rm/l4rm.h>
#include <l4/names/libnames.h>

/*** LOCAL INCLUDES ***/
#include "nitpicker.h"


static client *first_client = NULL; /* head of client list */


/*************************************************
 *** FUNCTIONS TO ACCESS CLIENT REPRESENTATION ***
 *************************************************/

/*** INIT CLIENT STRUCTURE AND ADD CLIENT TO CLIENT LIST ***/
void add_client(CORBA_Object tid, char *addr, int max_views, int max_buffers) {

	client *new_client = (client *)&addr[0];
	bzero(new_client, sizeof(client));

	new_client->views       = (view   *)&addr[sizeof(client)];
	new_client->buffers     = (buffer *)&addr[sizeof(client) + max_views*sizeof(view)];
	new_client->max_views   = max_views;
	new_client->max_buffers = max_buffers;

	bzero(new_client->buffers, sizeof(buffer) * max_buffers);
	bzero(new_client->views,   sizeof(view)   * max_views);

	if (tid) {
		new_client->tid = *tid;
		names_query_id(new_client->tid, &new_client->label[0], CLIENT_LABEL_LEN - 1);
	}

	/* insert client struct into client list */
	CHAIN_LISTELEMENT(&first_client, next, first_client, new_client);
}


/*** REMOVE CLIENT FROM NITPICKER SESSION ***/
void remove_client(CORBA_Object tid) {
	int i;

	/* look up client structure */
	client *c = find_client(tid);
	if (!c) return;

	/* close all views */
	for (i = 0; i < c->max_views; i++)
		nitpicker_destroy_view_component(tid, i, NULL);

	/* remove all buffers */
	for (i = 0; i < c->max_buffers; i++)
		nitpicker_remove_buffer_component(tid, i, NULL);

	/* exclude client from client list */
	UNCHAIN_LISTELEMENT(client, &first_client, next, c);

	/* unmap client memory */
	l4rm_detach(c);
}


/*** FIND THE CLIENT STRUCTURE FOR A GIVEN CLIENT ID ***/
client *find_client(CORBA_Object tid) {
	client *c;

	for (c = first_client; c; c = c->next)
		if (dice_obj_eq(&c->tid, tid)) return c;

	return NULL;
}


/***************************
 *** INTERFACE FUNCTIONS ***
 ***************************/

/*** INTERFACE: IMPORT AND INITIALIZE MEMORY FROM CLIENT ***/
int nitpicker_donate_memory_component(CORBA_Object _dice_corba_obj,
                                      const l4dm_dataspace_t *ds,
                                      int max_views, int max_buffers,
                                      CORBA_Server_Environment *_dice_corba_env) {
	char *addr;

	if (!ds) return -2;

	/* no not allow double donations */
	remove_client(_dice_corba_obj);

	/* map memory locally */
	TRY(l4rm_attach(ds, sizeof(buffer)*max_buffers
	                  + sizeof(view)*max_views
	                  + sizeof(client), 0, L4DM_RW, (void **)&addr),
	                   "Cannot attach dataspace");

	/*
	 * FIXME: We need to revoke the access right
	 *        to the memory block from the client.
	 */

	add_client(_dice_corba_obj, addr, max_views, max_buffers);

	return 0;
}
