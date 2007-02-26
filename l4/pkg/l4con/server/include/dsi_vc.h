/* $ID$ */

/*	con/server/include/dsi_vc.h
 *
 *	internals of `con' submodule
 *	DSI specific vc stuff
 */

#ifndef _DSI_CON_H
#define _DSI_CON_H

#include "con.h"

extern void dsi_vc_loop(struct l4con_vc *this_vc);

/* dsi_vc mode */
#define CON_NOT_DSI	0x00	/* not in DSI mode */
#define CON_DSI_STD	0x01	/* standard mode */
#define CON_DSI_RAWSET	0x02	/* pSLIM SET  - `raw' data mode */
#define CON_DSI_RAWCSCS 0x03	/* pSLIM CSCS - `raw' data mode */
#define CON_DSI_SET     0x04	/* pSLIM SET  - `normal' data mode */
#define CON_DSI_CSCS	0x05	/* pSLIM CSCS - `normal' data mode */
#define CON_DSI_BMAP	0x06	/* pSLIM BMAP - `normal' data mode */
#define CON_DSI_FILL	0x07	/* pSLIM FILL - `normal' data mode */
#define CON_DSI_COPY	0x08	/* pSLIM COPY - `normal' data mode */
#endif /* !_DSI_CON_H */
