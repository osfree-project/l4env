/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/idl/blk-sys.h
 * \brief  RPC funtion numbers.
 *
 * \date   02/09/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _GENERIC_BLK_BLK_SYS_H
#define _GENERIC_BLK_BLK_SYS_H

/* function numbers */
#define req_l4blk_driver_open        0x01
#define req_l4blk_driver_close       0x02

#define req_l4blk_cmd_create_stream  0x10
#define req_l4blk_cmd_close_stream   0x11
#define req_l4blk_cmd_start_stream   0x12
#define req_l4blk_cmd_put_requests   0x13
#define req_l4blk_cmd_put_sg_request 0x14
#define req_l4blk_cmd_ctrl           0x15

#define req_l4blk_notify_wait        0x20

#endif /* !_GENERIC_BLK_BLK_SYS_H */
