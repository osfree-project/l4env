/*!
 * \file	con/lib/src/putstocon_dsi.c
 *
 * \brief	Send a string to the console via DSI.
 *
 * \author	Mathias Noack <mn3@os.inf.tu-dresden.de>
 *
 */

/* intern */
#include "internal.h"

/*****************************************************************************/
/**
 * \brief   Send a string to the console via DSI
 *
 * \param   px, py     ... x, y postion
 * \param   s          ... string to be send
 * \param   l          ... length of string
 */
/*****************************************************************************/
static void
_putstocon_dsi(int px, int py, l4_uint8_t *s, int l)
{
  int ret;
  static unsigned long count = 1;
  dsi_packet_t * p;
  
  vtc_coord[py].x = px;
  vtc_coord[py].y = py;
  vtc_coord[py].str_addr = (unsigned long) s;
  vtc_coord[py].bgc = bg_color;
  vtc_coord[py].fgc = fg_color;
  vtc_coord[py].len = l;

  if ((ret = dsi_packet_get(send_socket, &p)))
    enter_kdebug("get packet failed");

  /* add data */
  if ((ret = dsi_packet_add_data(send_socket, p, &vtc_coord[py], 
			    vtc_coord[py].len, 0)))
    enter_kdebug("add data failed");
  
  /* set packet number */
  if ((ret = dsi_packet_set_no(send_socket, p, count++)))
    enter_kdebug("set packet no failed");

  if ((ret = dsi_packet_commit(send_socket, p)))
    enter_kdebug("commit packet failed");
}

void (*putstocon)(int, int, l4_uint8_t *, int) = _putstocon_dsi;

