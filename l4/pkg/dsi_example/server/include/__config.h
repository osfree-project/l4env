/* $Id$ */
/*****************************************************************************/
/**
 * \file  dsi_example/server/include/__config.h
 * \brief Server configuration
 *
 * \date   01/10/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _DSI_EXAMPLE___CONFIG_H
#define _DSI_EXAMPLE___CONFIG_H

/*****************************************************************************
 * benchmark config
 *****************************************************************************/

/**
 * number of rounds 
 */
#define NUM_ROUNDS  100000

/**
 * which test
 */
#define TEST_ROUNDTRIP  1   /// test roundtrip time for packets
#define TEST_OVERHEAD   0   /// test get/commit_packet overhead

/**
 * which DSI interface
 */
#define DSI_STD         1   /// standard DSI (staticly mapped)
#define DSI_MAP         0   /// map data
#define DSI_COPY        0   /// copy data

/**
 * use filters?
 */
#define USE_FILTER      0

/**
 * check results of DSI functions 
 */
#define DO_SANITY       0

/*****************************************************************************
 * stream config
 *****************************************************************************/

/**
 * packet size
 */
#define PACKET_SIZE 4096

/**
 * Number of packets 
 */
#define NUM_PACKETS  1

/*****************************************************************************
 * environment
 *****************************************************************************/

/**
 * dataspace manager 
 */
#define DSM_NAME  "SIMPLE_DM"

#endif /* !_DSI_EXAMPLE___CONFIG_H */
