
/* external interface */


#ifndef __GOOM_H_
#define __GOOM_H_

/* initial window size */
#define INIT_SCREEN_W 400
#define INIT_SCREEN_H 256

int jeko_init (void);
void jeko_cleanup (void);
void jeko_render_pcm (signed short data[2][512]);
void jeko_set_text (char *text);

#endif
