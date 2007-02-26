/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi_example/include/send.h
 * \brief  Public API send component.
 *
 * \date   01/10/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _DSI_EXAMPLE_SEND_H
#define _DSI_EXAMPLE_SEND_H

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
#define DSI_EXAMPLE_SEND_NAMES  "DSI_EXAMPLE_SEND"

/*****************************************************************************
 * prototypes
 *****************************************************************************/

__BEGIN_DECLS;

/*****************************************************************************/
/**
 * \brief Open send socket
 * 
 * \retval sender        Reference to send component
 * \retval ctrl_ds       Control dataspace
 * \retval data_ds       Data dataspace
 * 
 * \return 0 on success, -1 if open failed
 */
/*****************************************************************************/ 
int 
send_open(dsi_component_t * sender, l4dm_dataspace_t * ctrl_ds, 
	  l4dm_dataspace_t * data_ds);

__END_DECLS;

#endif /* !_DSI_EXAMPLE_SEND_H */
