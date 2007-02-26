/*	
 *
 *	`internal' send component specific part
 */

#ifndef _CONTXT_SEND_SERVER_H
#define _CONTXT_SEND_SERVER_H

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/dsi/types.h>

extern l4_int32_t send_server_open(dsi_jcp_stream_t *, dsi_stream_cfg_t *, 
				   dsi_socket_ref_t *, l4dm_dataspace_t *, 
				   l4dm_dataspace_t *);
extern l4_int32_t send_server_close(dsi_component_t *);
extern l4_int32_t send_server_connect(dsi_component_t *, dsi_socket_ref_t *);
extern int send_server_start(dsi_component_t *);
extern int send_server_stop(dsi_component_t *);
extern int send_server_init(void);

#endif

