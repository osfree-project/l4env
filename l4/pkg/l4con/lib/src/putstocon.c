/*!
 * \file	con/lib/src/putstocon.c
 *
 * \brief	Send a string to the console.
 *
 * \author	Mathias Noack <mn3@os.inf.tu-dresden.de>
 *
 */

/* intern */
#include "internal.h"

/*****************************************************************************/
/**
 * \brief   Send a string to the console
 *
 * \param   x, y       ... x, y postion
 * \param   s          ... string to be send
 * \param   len        ... length of string
 */
/*****************************************************************************/
static void
_putstocon(int x, int y, l4_int8_t *s, int len)
{
  CORBA_Environment env = dice_default_environment;

  con_vc_puts_call(&vtc_l4id, s, len, BITX(x), BITY(y), 
		   fg_color, bg_color, &env);
}

void (*putstocon)(int, int, l4_int8_t *, int) = _putstocon;

