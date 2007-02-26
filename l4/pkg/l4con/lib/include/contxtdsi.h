#ifndef __CONTXT_LIB_DSI_H__
#define __CONTXT_LIB_DSI_H__

#include <l4/dsi/dsi.h>

extern void *data_area;
extern l4_size_t data_size;
extern dsi_socket_t *send_socket;
extern struct contxtdsi_coord *vtc_coord;

#endif
