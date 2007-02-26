/*!
 * \file	con/include/l4/con/l4contxt.h
 *
 * \brief	libcontxt client interface
 *
 * \author	Mathias Noack <mn3@os.inf.tu-dresden.de>
 *
 */
#ifndef _L4CONTXT_L4CONTXT_H
#define _L4CONTXT_L4CONTXT_H

/* common interface include */
#include <l4/con/l4contxt_common.h>

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Init of contxt library
 * 
 * \param   max_sbuf_size  ... max IPC string buffer
 * \param   scrbuf_lines   ... number of additional screenbuffer lines 
 *
 * This is the init-function of libcontxt. It opens a console and allocates 
 * the screen history buffer.
 */
/*****************************************************************************/ 
int contxt_init(long max_sbuf_size, int scrbuf_lines);

/*****************************************************************************/
/**
 * \brief   close contxt library
 * 
 * \return  0 on success (close a console)
 *          PANIC otherwise
 *
 * Close the libcontxt console.
 */
/*****************************************************************************/
int contxt_close(void);

#endif

