/*!
 * \file	loader/examples/run-kdebug/ihb.h
 * \brief	input history buffer functions
 *
 * \date	2002
 * \author	Mathias Noack <mn3@os.inf.tu-dresden.de>
 * 		Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef DIRECT_IHB_H
#define DIRECT_IHB_H

/** input history buffer 
 * \ingroup ihb_if */
typedef struct
{
  char *buffer;		/**< input history buffer head */
  int  first;		/**< first history line */
  int  last;		/**< last history line */
  int  lines;		/**< history buffer size in lines */
  int  length;		/**< max number of characters per line */
} direct_ihb_t;

/** Initialize input history buffer.
 * \ingroup ihb_if
 *
 * \param   ihb            ... input history buffer structure
 * \param   count          ... number of lines
 * \param   length         ... number of characters per line
 *          
 * This is the init-function of the input history buffer. The history
 * buffer has to be already allocated. See the \b run-kdebug example of
 * the loader package. */
int direct_ihb_init(direct_ihb_t* ihb, int count, int length);

/** Add string to history buffer
 * \ingroup ihb_if
 *
 * \param   ihb            ... input history buffer structure
 * \param   s              ... string to add
 */
void direct_ihb_add(direct_ihb_t* ihb, const char *s);

/** Reads a maximum amount of count of characters from keyboard
 * \ingroup ihb_if
 *
 * \param   maxlen         ... size of return string buffer
 *
 * \retval  retstr         ... return string buffer
 * \retval  ihb            ... used input history buffer
 *                             if 0, no input history buffer will be used
 *
 * This function reads a number (maximum maxlen) of character. */
void direct_ihb_read(char* retstr, int maxlen, direct_ihb_t* ihb);

#define contxt_ihb_init       direct_ihb_init
#define contxt_ihb_add        direct_ihb_add
#define contxt_ihb_read       direct_ihb_read
#define contxt_ihb_t          direct_ihb_t

#endif
