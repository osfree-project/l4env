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
_putstocon(int x, int y, l4_uint8_t *s, int len)
{
  l4_strdope_t str;
  sm_exc_t _ev;

  str.rcv_size = 0;
  str.snd_str = (l4_umword_t) s;
  str.snd_size = len;

  con_vc_puts(vtc_l4id, str, BITX(x), BITY(y), fg_color, bg_color, &_ev);
}

void (*putstocon)(int, int, l4_uint8_t *, int) = _putstocon;

