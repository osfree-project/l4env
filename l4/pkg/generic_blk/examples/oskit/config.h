/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/examples/oskit/config.h
 * \brief  OSKit block driver, config
 *
 * \date   09/13/2003
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _BLKSRV_CONFIG_H
#define _BLKSRV_CONFIG_H

/** max. number of clients */
#define BLKSRV_MAX_CLIENTS           2

/** command thread stack size, dice needs a bit more stack */
#define BLKSRV_CMD_STACK_SIZE        (256 * 1024)

/** max. scatter-gather list length, 
 *  we don't have a real limit here since we copy data from/to a temporary 
 *  buffer 
 */
#define BLKSRV_MAX_SG_LEN            64

#endif /* !_BLKSRV_CONFIG_H */
