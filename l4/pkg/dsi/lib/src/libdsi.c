/* $Id$ */
/*****************************************************************************/
/*! 
 * \file dsi/lib/src/libdsi.c
 *
 * \brief 	DROPS Stream Interface. Initialization and global stuff
 *
 * \author      Lars Reuther <reuther@os.inf.tu-dresden.de>
 * \date        07/01/2000
 *
 */
/*****************************************************************************/

/* library includes */
#include "__socket.h"
#include "__app.h"
#include "__dataspace.h"
#include "__event.h"
#include "__error.h"
#include <l4/dsi/dsi.h>

/*!\brief Library initialization.
 * \ingroup general
 */
int
dsi_init(void)
{
  /* init error messages */
  dsi_init_error();

  /* init dataspace handling */
  dsi_init_dataspaces();

  /* init socket descriptors */
  dsi_init_sockets();

  /* init stream descriptors */
  dsi_init_streams();

  /* init event handling */
  dsi_init_event_signalling();

  return 0;
}

