#ifndef DROPS_QWS_OUTPUT_H
#define DROPS_QWS_OUTPUT_H

long  drops_qws_set_screen(long width, long height, long depth);

long  drops_qws_get_scr_width(void);
long  drops_qws_get_scr_height(void);
long  drops_qws_get_scr_depth(void);
long  drops_qws_get_scr_line(void);
void *drops_qws_get_scr_adr(void);

void  drops_qws_refresh_screen(void);

#endif

