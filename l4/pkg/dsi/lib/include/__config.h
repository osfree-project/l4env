/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi/lib/include/__config.h
 * \brief  DSI lib configuration
 *
 * \date   07/01/2000 
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _DSI___CONFIG_H
#define _DSI___CONFIG_H

/* L4/L4Env includes */
#include <l4/dm_phys/dm_phys.h>

/*****************************************************************************
 * sockets
 *****************************************************************************/

/* number of socket descriptors */
#define DSI_MAX_SOCKETS        64

/*****************************************************************************
 * streams
 *****************************************************************************/

/* number of application streams */
#define DSI_MAX_STREAMS        64

/*****************************************************************************
 * memory/dataspaces
 *****************************************************************************/

/* default dataspace manager */
#define DSI_MAX_DM_NAME_LEN    32

/* attach area control dataspaces */
#define DSI_USE_CDS_AREA       1
#define DSI_CDS_MAP_START      0x60000000UL
#define DSI_CDS_MAP_SIZE       0x20000000UL

/* attach area data dataspaces */
#define DSI_USE_DDS_AREA       1
#define DSI_DDS_MAP_START      0x80000000UL
#define DSI_DDS_MAP_SIZE       0x20000000UL

/* stacks */
#define DSI_SYNC_STACK_SIZE    16384
#define DSI_EVENT_STACK_SIZE   16384
#define DSI_SELECT_STACK_SIZE  4096

/*****************************************************************************
 * internal stuff 
 *****************************************************************************/

/**
 * do sanity checks
 */
#define DO_SANITY  0

/**
 * do IPC call for release callbacks 
 */
#define RELEASE_DO_CALL  0

#endif /* !_DSI___CONFIG_H */
