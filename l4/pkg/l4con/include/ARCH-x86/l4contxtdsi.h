/*!
 * \file	con/include/l4/con/l4contxtdsi.h
 *
 * \brief	libcontxtdsi client interface
 *
 * \author	Mathias Noack <mn3@os.inf.tu-dresden.de>
 *
 * This lib uses DSI to transfer characters to con.
 */

#ifndef _L4CONTXTDSI_L4CONTXTDSI_H
#define _L4CONTXTDSI_L4CONTXTDSI_H

/* common interface include */
#include <l4/con/l4contxt_common.h>

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Init of contxtdsi library
 * 
 * \param   scrbuf_lines   ... number of additional screenbuffer lines 
 *
 * This is the init-function of libcontxtdsi. It opens a dsi-console, 
 * initialises the DSI sender/reciever and starts the DSI stream.
 */
/*****************************************************************************/
int contxtdsi_init(int scrbuf_lines);

/*****************************************************************************/
/**
 * \brief   close contxtdsi library 
 * 
 * \return  0 on success (close a dsi-console)
 *          PANIC otherwise
 *
 * Close the libcontxtdsi console.
 */
/*****************************************************************************/
int contxtdsi_close(void);


/** dsi packet */
struct contxtdsi_coord
{
  int x, y;
  unsigned str_addr;
  int bgc;
  int fgc;
  int len;
};

#endif

