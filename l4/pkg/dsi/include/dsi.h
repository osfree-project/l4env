/* $Id */
/*****************************************************************************/
/*! 
 * \file   dsi/include/dsi.h
 * \brief  DROPS Stream Interface public API.
 *
 * \date   07/01/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * See dsi/doc/ for further information.
 */
/*****************************************************************************/
#ifndef _DSI_DSI_H
#define _DSI_DSI_H

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/env/cdefs.h>

/* DSI includes */
#include <l4/dsi/types.h>
#include <l4/dsi/errno.h>

/*****************************************************************************
 * global stuff                                                              *
 *****************************************************************************/

__BEGIN_DECLS;

int
dsi_init(void);

void
dsi_jcp_2_config(dsi_jcp_stream_t *jcp, dsi_stream_cfg_t *s_cfg);

extern int dsi_set_sync_thread_prio(int new_prio);
extern int dsi_set_select_thread_prio(int new_prio);
extern int dsi_set_event_thread_prio(int new_prio);

/*****************************************************************************
 * component API                                                             *
 *****************************************************************************/

/*
 * socket operations 
 */
int
dsi_socket_create(dsi_jcp_stream_t jcp_stream, dsi_stream_cfg_t cfg,
		  l4dm_dataspace_t * ctrl_ds, l4dm_dataspace_t * data_ds,
		  l4_threadid_t work_id, l4_threadid_t * sync_id,
		  l4_uint32_t flags, dsi_socket_t ** socket);

int 
dsi_socket_stop(dsi_socket_t * socket);

int 
dsi_socket_close(dsi_socket_t * socket);

int 
dsi_socket_connect(dsi_socket_t * socket,
                   const dsi_socket_ref_t * remote_socket);

int
dsi_socket_set_sync_callback(dsi_socket_t * socket, 
			     dsi_sync_callback_fn_t func);

int
dsi_socket_set_release_callback(dsi_socket_t * socket,
				dsi_release_callback_fn_t func);

int
dsi_socket_get_ref(dsi_socket_t * socket, dsi_socket_ref_t * ref);

int
dsi_socket_get_descriptor(dsi_socketid_t id, dsi_socket_t ** socket);

int 
dsi_socket_get_data_area(dsi_socket_t * socket, void ** data_area,
			 l4_size_t * area_size);

int
dsi_socket_set_flags(dsi_socket_t * socket, l4_uint32_t flags);

int
dsi_socket_clear_flags(dsi_socket_t * socket, l4_uint32_t flags);

int
dsi_socket_test_flag(dsi_socket_t * socket, l4_uint32_t flag);

int 
dsi_socket_set_event(dsi_socket_t * socket, l4_uint32_t events);

int
dsi_socket_share_ds(dsi_socket_t * socket, l4_threadid_t client);

int
dsi_socket_get_packet_num(dsi_socket_t * socket);

int
dsi_socket_get_num_committed_packets(dsi_socket_t * socket);


/* 
 * packet operations 
 */
int 
dsi_packet_get(dsi_socket_t * socket, dsi_packet_t ** packet);

int 
dsi_packet_get_abort(dsi_socket_t * socket);

int
dsi_packet_commit(dsi_socket_t * socket, dsi_packet_t * packet);

int 
dsi_packet_add_data(dsi_socket_t * socket, dsi_packet_t * packet, 
		    void *addr, l4_size_t size,
		    l4_uint32_t flags);

int 
dsi_packet_get_data(dsi_socket_t * socket, dsi_packet_t * packet,
		    void ** addr, l4_size_t * size);

int 
dsi_packet_set_no(dsi_socket_t * socket, dsi_packet_t * packet,
		  l4_uint32_t no);

int 
dsi_packet_get_no(dsi_socket_t * socket, dsi_packet_t * packet,
		  l4_uint32_t * no);

int 
dsi_packet_get_nr(dsi_socket_t * socket, unsigned nr, dsi_packet_t ** packet);

/* 
 * dataspace management 
 */
void 
dsi_ds_set_dataspace_manager(l4_threadid_t id);

int 
dsi_ds_setup_ctrl_dataspace(l4dm_dataspace_t * ds, dsi_stream_cfg_t cfg);

int
dsi_ds_create_ctrl_dataspace(dsi_stream_cfg_t cfg, l4dm_dataspace_t * ds);

/*****************************************************************************
 * application API                                                           *
 *****************************************************************************/

/* create new stream */
int
dsi_stream_create(dsi_component_t * sender, dsi_component_t * receiver,
		  l4dm_dataspace_t ctrl, l4dm_dataspace_t data,
		  dsi_stream_t ** stream);

int 
dsi_stream_close(dsi_stream_t * stream);

int
dsi_stream_start(dsi_stream_t * stream);

int
dsi_stream_stop(dsi_stream_t * stream);

int
dsi_stream_select(dsi_select_socket_t *sockets, const int num_sockets,
		  dsi_select_socket_t *events, int * num_events);


/*****************************************************************************
 * convenience functions                                                     *
 *****************************************************************************/

int dsi_thread_start_worker(dsi_socket_t * socket, l4_uint32_t * ret_code);
int dsi_thread_worker_wait(dsi_socket_t ** socket);
int dsi_thread_worker_started(int ret_code);
int dsi_socket_local_create(dsi_stream_cfg_t cfg, l4dm_dataspace_t *ctrl_ds,
			    l4dm_dataspace_t *data_ds, l4_threadid_t work_id,
			    l4_uint32_t flags, dsi_socket_t **socket,
			    dsi_component_t *comp);
int dsi_socket_local_close(dsi_component_t *comp);
int dsi_socket_local_connect(dsi_component_t * comp,
			     dsi_socket_ref_t * remote);
int dsi_socket_local_stop(dsi_component_t * comp);

__END_DECLS;

#endif /* !_DSI_DSI_H */
