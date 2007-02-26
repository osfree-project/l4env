/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/examples/oskit/blksrv.h
 * \brief  OSKit block driver, internal functions
 *
 * \date   09/13/2003
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _BLKSRV_BLKSRV_H
#define _BLKSRV_BLKSRV_H

#include "types.h"

/* name ate nameserver */
#define NAMES_OSKITBLK  "OSKITBLK"

/* driver interface */
void
blksrv_start_driver(void);

blksrv_driver_t *
blksrv_get_driver(l4blk_driver_id_t drv);

/* command thread */
int
blksrv_start_command_thread(void);

/* request handling */
int
blksrv_enqueue_request(blksrv_driver_t * driver, 
                       const l4blk_blk_request_t * request,
                       const l4blk_sg_ds_elem_t * sg_list,
                       l4_int32_t sg_num);

int
blksrv_start_request_thread(void);

/* client notification */
int
blksrv_do_notification(blksrv_driver_t * driver, l4_uint32_t req_handle,
                       l4_uint32_t status, l4_int32_t error);

int 
blksrv_start_notification_thread(blksrv_driver_t * driver);

void
blksrv_shutdown_notification_thread(blksrv_driver_t * driver);

/* OSKit device stuff */
void
blksrv_dev_use_ide(void);

void
blksrv_dev_use_scsi(const char * adapter);

void
blksrv_dev_set_device(const char * device);

void
blksrv_dev_read_write(void);

int
blksrv_dev_num(void);

unsigned long
blksrv_dev_size(void);

#endif /* _BLKSRV_BLKSRV_H */
