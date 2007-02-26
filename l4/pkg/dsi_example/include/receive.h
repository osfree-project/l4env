/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi_example/include/receive.h
 * \brief  Public API receive component.
 *
 * \date   01/10/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _DSI_EXAMPLE_RECEIVE_H
#define _DSI_EXAMPLE_RECEIVE_H

/* L4/DROPS includes */
#include <l4/env/cdefs.h>
#include <l4/sys/types.h>
#include <l4/dsi/dsi.h>

/*****************************************************************************
 * defines 
 *****************************************************************************/

/**
 * Nameserver ID
 */
#define DSI_EXAMPLE_RECEIVE_NAMES  "DSI_EXAMPLE_RECEIVE"

/*****************************************************************************
 * prototypes
 *****************************************************************************/

__BEGIN_DECLS;

/*****************************************************************************/
/**
 * \brief Open receive socket
 * 
 * \param  ctrl_ds       Control dataspace
 * \param  data_ds       Data dataspace
 * \retval receiver      Reference to receive component
 * 
 * \return 0 on success, -1 if open failed.
 */
/*****************************************************************************/ 
int 
receive_open(l4dm_dataspace_t ctrl_ds,l4dm_dataspace_t data_ds,
	     dsi_component_t * receiver);

__END_DECLS;

#endif /* !_DSI_EXAMPLE_RECEIVE_H */
