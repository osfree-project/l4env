/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/lib/client/src/libblk.c
 * \brief  Generic Block Device Driver Interface, client library.
 *
 * \date   02/11/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Client library initialization.
 */
/*****************************************************************************/

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>

/* library includes */
#include "__driver.h"
#include "__request.h"

/*****************************************************************************/
/**
 * \brief Setup client library.
 */
/*****************************************************************************/ 
void
l4blk_init(void)
{
  /* setup driver handling */
  blkclient_init_drivers();

  /* setup request handling */
  blkclient_init_requests();
}

