/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/lib/client/include/__config.h
 * \brief  Client library configuration.
 *
 * \date   02/11/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * \note Be carefully with changes of BLKCLIENT_MAX_RPC_REQUESTS and 
 *       BLKCLIENT_MAX_REPLACE, those numbers are also defined in the
 *       IDL (file idl/blk.idl). 
 *       The RPC request list is allocated on the stack of the calling thread, 
 *       be carefully if you change the size of the request list!
 */
/*****************************************************************************/
#ifndef _GENERIC_BLK___CONFIG_H
#define _GENERIC_BLK___CONFIG_H

/*****************************************************************************
 *** requests
 *****************************************************************************/

///  max. number of simultaneously requests 
#define BLKCLIENT_MAX_REQUESTS      1024

/// max. length of request list in RPC call
#define BLKCLIENT_MAX_RPC_REQUESTS  8

/// max. number of replace streams in request structure
#define BLKCLIENT_MAX_REPLACE       4

/*****************************************************************************
 *** drivers
 *****************************************************************************/

/// max. number of drivers
#define BLKCLIENT_MAX_DRIVERS   4

/// timeout for nameserver request (ms)
#define BLKCLIENT_NAMES_WAIT    60000

/*****************************************************************************
 *** memory
 *****************************************************************************/

/// notification thread stack size
#define BLKCLIENT_NOTIFY_STACK_SIZE   16384

#endif /* !_GENERIC_BLK___CONFIG_H */
