#ifndef DROPS_QWS_INPUT_H
#define DROPS_QWS_INPUT_H

void drops_qws_init_devices(void);
int  drops_qws_channel_mouse(void);
int  drops_qws_channel_keyboard(void);

int  drops_qws_get_mouse(int &x, int &y, int &button);
int  drops_qws_get_keyboard(int *key, int *press);
int  drops_qws_get_keymodifiers(int *alt, int *shift, int *ctrl, int *caps, int *num);

int  drops_qws_qt_key(int key);
int  drops_qws_qt_keycode(int key, int press);

void drops_qws_notify(void);

#endif

