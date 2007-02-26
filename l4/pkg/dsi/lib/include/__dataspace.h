/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi/lib/include/__dataspace.h
 * \brief  Dataspace handling
 *
 * \date   07/10/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _DSI___DATASPACE_H
#define _DSI___DATASPACE_H

/* L4/DROPS includes */
#include <l4/sys/types.h>

/* library includes */
#include <l4/dsi/types.h>

/*****************************************************************************
 * prototypes                                                                *
 *****************************************************************************/

int
dsi_create_ctrl_area(dsi_socket_t * socket, dsi_jcp_stream_t jcp_stream, 
		     dsi_stream_cfg_t cfg);

int
dsi_release_ctrl_area(dsi_socket_t * socket);

int
dsi_set_ctrl_area(dsi_socket_t * socket, l4dm_dataspace_t ctrl_ds,
		  dsi_jcp_stream_t jcp_stream, dsi_stream_cfg_t cfg);

int
dsi_set_data_area(dsi_socket_t * socket, l4dm_dataspace_t data_ds);

int 
dsi_release_data_area(dsi_socket_t * socket);

void
dsi_init_dataspaces(void);

#endif /* !_DSI___DATASPACE_H */
