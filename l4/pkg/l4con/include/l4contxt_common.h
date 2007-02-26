/*!
 * \file	con/include/l4/con/l4contxt_common.h
 *
 * \brief	libcontxt common client interface (intern)
 *
 * \author	Mathias Noack <mn3@os.inf.tu-dresden.de>
 *
 */
#ifndef _L4CONTXT_COMMON_L4CONTXT_COMMON_H
#define _L4CONTXT_COMMON_L4CONTXT_COMMON_H

/* con includes */
#include <l4/con/l4con.h>

/******************************************************************************
 * data types
 ******************************************************************************/
/*!\brief	contxt input history buffer
 */
typedef struct contxt_ihb
{
  char *buffer;		/*!< input history buffer head */
  int  first, last;	/*!< first and last history line */
  int  lines;		/*!< history buffer size in lines */
  int  length;		/*!< max number of characters per line */
} contxt_ihb_t;

/*****************************************************************************/
/**
 * \brief   Reads a maxcount of character
 *
 * \param   maxlen         ... size of return string buffer
 *
 * \retval  retstr         ... return string buffer
 * \retval  ihb            ... used input history buffer
 *                             if 0, no input history buffer will be used
 *
 * This function reads a number (maximum maxlen) of character. 
 */
/*****************************************************************************/
void contxt_read(char* retstr, int maxlen, contxt_ihb_t* ihb);

/*****************************************************************************/
/**
 * \brief   Init of input history buffer
 *
 * \param   ihb            ... input history buffer
 * \param   count          ... number of lines
 * \param   length         ... number of characters per line
 *          
 * This is the init-function of the input history buffer. It allocates the
 * history buffer.
 */
/*****************************************************************************/
int contxt_init_ihb(contxt_ihb_t* ihb, int count, int length);

/*****************************************************************************/
/**
 * \brief   Read a character
 *
 * \return  a character
 *
 * This function reads a character. (libc) 
 */
/*****************************************************************************/
int getchar(void);

/** Try to get next character. Return -1 if no character is available */
int trygetchar(void);

/*****************************************************************************/
/**
 * \brief   Read a character
 *
 * \return  a character
 *
 * This function reads a character.
 */
/*****************************************************************************/
int contxt_getchar(void);

/** Try to get next character. Return -1 no character is available. */
int contxt_trygetchar(void);

/*****************************************************************************/
/**
 * \brief   Read a character
 *
 * \return  a character
 *
 * This function reads a character. (OSKit)
 */
/*****************************************************************************/
int direct_cons_getchar(void);

/** Try to get next character. Return -1 no character is available. */
int direct_cons_trygetchar(void);

/*****************************************************************************/
/**
 * \brief   Write a character
 * \ingroup libcontxt
 *
 * \param   c        ... character
 *
 * \return  a character
 *
 * This function writes a character. (libc)
 */
/*****************************************************************************/
int putchar(int c);

/*****************************************************************************/
/**
 * \brief   Write a character
 * \ingroup libcontxt
 *
 * \param   c        ... character
 *
 * \return  a character
 *
 * This function writes a character.
 */
/*****************************************************************************/
int contxt_putchar(int c);

/*****************************************************************************/
/**
 * \brief   Write a string + \n
 * \ingroup libcontxt
 *
 * \param   s        ... string
 * \return  >=0 ok, EOF else
 *
 * This function writes a string + \n. (libc)
 */
/*****************************************************************************/
int puts(const char*s);

/*****************************************************************************/
/**
 * \brief   Write a string + \n
 * \ingroup libcontxt
 *
 * \param   s        ... string
 * \return  >=0 ok, EOF else
 *
 * This function writes a string.
 */
/*****************************************************************************/
int contxt_puts(const char*s);

/*****************************************************************************/
/**
 * \brief   set graphic mode
 * 
 * \param   gmode  ... coded graphic mode
 *
 * \return  0 on success (set graphic mode)
 *          
 * empty
 */
/*****************************************************************************/
int contxt_set_graphmode(long gmode);

/*****************************************************************************/
/**
 * \brief   get graphic mode
 * 
 * \return  gmode (graphic mode)
 *          
 * empty
 */
/*****************************************************************************/
int contxt_get_graphmode(void);

/*****************************************************************************/
/**
 * \brief   clear screen
 *          
 * This function fills the screen with the current background color
 */
/*****************************************************************************/
void contxt_clrscr(void);


/** overwrite direct_cons_getchar from oskit10_support_l4env library */
asm(".globl direct_cons_getchar");

#endif

