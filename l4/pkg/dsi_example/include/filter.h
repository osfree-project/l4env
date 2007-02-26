/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi_example/include/filter.h
 * \brief  Public API filter component
 *
 * \date   01/15/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _DSI_EXAMPLE_FILTER_H
#define _DSI_EXAMPLE_FILTER_H

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/env/cdefs.h>
#include <l4/dsi/dsi.h>

/*****************************************************************************
 * defines
 *****************************************************************************/

/**
 * Nameserver ID
 */
#define DSI_EXAMPLE_FILTER_NAMES "DSI_EXAMPLE_FILTER"

/*****************************************************************************
 * prototypes
 *****************************************************************************/

__BEGIN_DECLS;

/*****************************************************************************/
/**
 * \brief Open filter.
 * 
 * \param  rcv_ctrl_ds   Control dataspace input socket
 * \param  rcv_data_ds   Data dataspace input socket
 * \retval rcv_filter    Reference to input socket
 * \retval snd_filter    Reference to output socket
 * \retval snd_ctrl_ds   Control dataspace output socket
 * \retval snd_data_ds   Data dataspace output socket
 *	
 * \return 0 on success, -1 if open failed.
 */
/*****************************************************************************/ 
int
filter_open(l4dm_dataspace_t rcv_ctrl_ds, l4dm_dataspace_t rcv_data_ds,
	    dsi_component_t * rcv_filter, dsi_component_t * snd_filter,
	    l4dm_dataspace_t * snd_ctrl_ds, l4dm_dataspace_t * snd_data_ds);

__END_DECLS;
	    
#endif /* !_DSI_EXAMPLE_FILTER_H */
