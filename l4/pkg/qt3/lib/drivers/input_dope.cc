#include <l4/thread/thread.h>
#include <l4/log/l4log.h>
#include <l4/dope/keycodes.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include <qnamespace.h>

#include "input.h"

extern int dopeinput_ispending(void);
extern int dopeinput_get(void);

#define _DEBUG 0
#define _ERROR 1

static int initialized = 0;
static int mouse_x = 0;
static int mouse_y = 0;
static int mouse_button = 0;
static int key_key = 0;
static int key_pressed = 0;
static int mouse_changed = 0;
static int key_changed = 0;
static int key_mod_alt = 0;
static int key_mod_shift = 0;
static int key_mod_ctrl = 0;
static int key_mod_caps = 0;
static int key_mod_num = 0;

static int mouseinputchannel[2];
static int keyboardinputchannel[2];

static void dopeinput_flush(void)
{
	int type, code, value;
	int message[3];

	if(!initialized)
	{
		LOG("[mouse/keyboard:dope] not initialized\n");
		return;
	}

	if (!dopeinput_ispending()) return;
	LOGd(_DEBUG, "[mouse/keyboard:dope] got event(s)\n");
	while(dopeinput_ispending())
	{
		type = dopeinput_get();
		code = dopeinput_get();
		value = dopeinput_get();
		LOGd(_DEBUG, "[mouse/keyboard:dope] * event: %i/%i/%i\n", type, code, value);

		if(type == 0)
		{
			LOGd(_DEBUG, "@ mouse button\n");
			if(code == 0) mouse_button = 1;
			else mouse_button = 0;
			mouse_changed = 1;
			message[0] = 0;
			message[1] = mouse_button;
			message[0] = 0;
		}
		else if(type == 2)
		{
			LOGd(_DEBUG, "@ keyboard key or other\n");
			if(code == 1) key_pressed = 1;
			else key_pressed = 0;
			key_key = value;
			key_changed = 1;
			message[0] = 2;
			message[1] = key_key;
			message[2] = key_pressed;
		}
		else if(type == 1)
		{
			LOGd(_DEBUG, "@ absolute movement\n");
			mouse_x = code;
			mouse_y = value;
			mouse_changed = 1;
			message[0] = 1;
			message[1] = mouse_x;
			message[2] = mouse_y;
		}
		else
		{
			LOG("@ unknown\n");
		}
	}

	if(mouse_changed)
	{
		write(mouseinputchannel[0], message, sizeof(message));
		mouse_changed = 0;
	}
	if(key_changed)
	{
		write(keyboardinputchannel[0], message, sizeof(message));
		key_changed = 0;
	}
}

void drops_qws_notify(void)
{
	dopeinput_flush();
}

void drops_qws_init_devices(void)
{
	LOGd(_DEBUG, "[mouse/keyboard:dope] init\n");

	if(initialized) return;
	initialized = 1;

	socketpair(AF_LOCAL, SOCK_STREAM, 0, mouseinputchannel);
	socketpair(AF_LOCAL, SOCK_STREAM, 0, keyboardinputchannel);
}

int drops_qws_channel_mouse(void)
{
	return mouseinputchannel[1];
}

int drops_qws_channel_keyboard(void)
{
	return keyboardinputchannel[1];
}

int drops_qws_get_mouse(int *x, int *y, int *button)
{
	dopeinput_flush();
	if(mouse_changed)
	{
		mouse_changed = 0;
		if(x) *x = mouse_x;
		if(y) *y = mouse_y;
		if(button) *button = mouse_button;
		return 1;
	}
	return 0;
}

int drops_qws_get_keyboard(int *key, int *press)
{
	dopeinput_flush();
	if(key_changed)
	{
		key_changed = 0;
		if(key) *key = key_key;
		if(press) *press = key_pressed;
		return 1;
	}
	return 0;
}

int drops_qws_get_keymodifiers(int *alt, int *shift, int *ctrl, int *caps, int *num)
{
	*alt = key_mod_alt;
	*shift = key_mod_shift;
	*ctrl = key_mod_ctrl;
	*caps = key_mod_caps;
	*num = key_mod_num;
	return 1;
}

int drops_qws_qt_key(int key)
{
	const int keymap[][2] =
	{
		{DOPE_KEY_ESC, Qt::Key_Escape},
		{DOPE_KEY_0, Qt::Key_0},
		{DOPE_KEY_1, Qt::Key_1},
		{DOPE_KEY_2, Qt::Key_2},
		{DOPE_KEY_3, Qt::Key_3},
		{DOPE_KEY_4, Qt::Key_4},
		{DOPE_KEY_5, Qt::Key_5},
		{DOPE_KEY_6, Qt::Key_6},
		{DOPE_KEY_7, Qt::Key_7},
		{DOPE_KEY_8, Qt::Key_8},
		{DOPE_KEY_9, Qt::Key_9},
		{DOPE_KEY_0, Qt::Key_0},
		{DOPE_KEY_MINUS, Qt::Key_Minus},
		{DOPE_KEY_EQUAL, Qt::Key_Equal},
		{DOPE_KEY_BACKSPACE, Qt::Key_Backspace},
		{DOPE_KEY_TAB, Qt::Key_Tab},
		{DOPE_KEY_Q, Qt::Key_Q},
		{DOPE_KEY_W, Qt::Key_W},
		{DOPE_KEY_E, Qt::Key_E},
		{DOPE_KEY_R, Qt::Key_R},
		{DOPE_KEY_T, Qt::Key_T},
		{DOPE_KEY_Y, Qt::Key_Y},
		{DOPE_KEY_U, Qt::Key_U},
		{DOPE_KEY_I, Qt::Key_I},
		{DOPE_KEY_O, Qt::Key_O},
		{DOPE_KEY_P, Qt::Key_P},
		{DOPE_KEY_LEFTBRACE, Qt::Key_BraceLeft},
		{DOPE_KEY_RIGHTBRACE, Qt::Key_BraceRight},
		{DOPE_KEY_ENTER, Qt::Key_Enter},
		{DOPE_KEY_LEFTCTRL, Qt::Key_Control},
		{DOPE_KEY_A, Qt::Key_A},
		{DOPE_KEY_S, Qt::Key_S},
		{DOPE_KEY_D, Qt::Key_D},
		{DOPE_KEY_F, Qt::Key_F},
		{DOPE_KEY_G, Qt::Key_G},
		{DOPE_KEY_H, Qt::Key_H},
		{DOPE_KEY_J, Qt::Key_J},
		{DOPE_KEY_K, Qt::Key_K},
		{DOPE_KEY_L, Qt::Key_L},
		{DOPE_KEY_SEMICOLON, Qt::Key_Semicolon},
		{DOPE_KEY_APOSTROPHE, Qt::Key_Apostrophe},
		{DOPE_KEY_GRAVE, Qt::Key_unknown}, // XXX wtf?
		{DOPE_KEY_LEFTSHIFT, Qt::Key_Shift},
		{DOPE_KEY_BACKSLASH, Qt::Key_Backslash},
		{DOPE_KEY_Z, Qt::Key_Z},
		{DOPE_KEY_X, Qt::Key_X},
		{DOPE_KEY_C, Qt::Key_C},
		{DOPE_KEY_V, Qt::Key_V},
		{DOPE_KEY_B, Qt::Key_B},
		{DOPE_KEY_N, Qt::Key_N},
		{DOPE_KEY_M, Qt::Key_M},
		{DOPE_KEY_COMMA, Qt::Key_Comma},
		{DOPE_KEY_DOT, Qt::Key_unknown}, // XXX wtf?
		{DOPE_KEY_SLASH, Qt::Key_Slash},
		{DOPE_KEY_RIGHTSHIFT, Qt::Key_Shift},
		{DOPE_KEY_KPASTERISK, Qt::Key_Asterisk},
		{DOPE_KEY_LEFTALT, Qt::Key_Alt},
		{DOPE_KEY_SPACE, Qt::Key_Space},
		{DOPE_KEY_CAPSLOCK, Qt::Key_CapsLock},
		{DOPE_KEY_F1, Qt::Key_F1},
		{DOPE_KEY_F2, Qt::Key_F2},
		{DOPE_KEY_F3, Qt::Key_F3},
		{DOPE_KEY_F4, Qt::Key_F4},
		{DOPE_KEY_F5, Qt::Key_F5},
		{DOPE_KEY_F6, Qt::Key_F6},
		{DOPE_KEY_F7, Qt::Key_F7},
		{DOPE_KEY_F8, Qt::Key_F8},
		{DOPE_KEY_F9, Qt::Key_F9},
		{DOPE_KEY_F10, Qt::Key_F10},
		{DOPE_KEY_NUMLOCK, Qt::Key_NumLock},
		{DOPE_KEY_SCROLLLOCK, Qt::Key_ScrollLock},
		{DOPE_KEY_KP7, Qt::Key_7},
		{DOPE_KEY_KP8, Qt::Key_8},
		{DOPE_KEY_KP9, Qt::Key_9},
		{DOPE_KEY_KPMINUS, Qt::Key_Minus},
		{DOPE_KEY_KP4, Qt::Key_4},
		{DOPE_KEY_KP5, Qt::Key_5},
		{DOPE_KEY_KP6, Qt::Key_6},
		{DOPE_KEY_KPPLUS, Qt::Key_Plus},
		{DOPE_KEY_KP1, Qt::Key_1},
		{DOPE_KEY_KP2, Qt::Key_2},
		{DOPE_KEY_KP3, Qt::Key_3},
		{DOPE_KEY_KP0, Qt::Key_0},
		{DOPE_KEY_KPDOT, Qt::Key_unknown}, // XXX wtf?
		/* from now on, selectively */
		{DOPE_KEY_F11, Qt::Key_F11},
		{DOPE_KEY_F12, Qt::Key_F12},
		{DOPE_KEY_KPENTER, Qt::Key_Enter},
		{DOPE_KEY_RIGHTCTRL, Qt::Key_Control},
		{DOPE_KEY_RIGHTALT, Qt::Key_Alt},
		{DOPE_KEY_HOME, Qt::Key_Home},
		{DOPE_KEY_UP, Qt::Key_Up},
		{DOPE_KEY_PAGEUP, Qt::Key_PageUp},
		{DOPE_KEY_LEFT, Qt::Key_Left},
		{DOPE_KEY_RIGHT, Qt::Key_Right},
		{DOPE_KEY_END, Qt::Key_End},
		{DOPE_KEY_DOWN, Qt::Key_Down},
		{DOPE_KEY_PAGEDOWN, Qt::Key_PageDown},
		{DOPE_KEY_INSERT, Qt::Key_Insert},
		{DOPE_KEY_DELETE, Qt::Key_Delete},
		{DOPE_KEY_KPEQUAL, Qt::Key_Equal},
		{DOPE_KEY_PAUSE, Qt::Key_Pause},
		{DOPE_KEY_KPCOMMA, Qt::Key_Comma},
		{DOPE_KEY_RESERVED, Qt::Key_unknown} // must be last
	};
	int i;

	for(i = 0; keymap[i][0] != DOPE_KEY_RESERVED; i++)
		if(keymap[i][0] == key)
		{
			LOGd(_DEBUG, "[mouse/keyboard:dope] map key %i to %i\n", key, keymap[i][1]);
			return keymap[i][1];
		}

	LOGd(_ERROR, "[mouse/keyboard:dope] unknown key mapping for %i\n", key);
	return Qt::Key_unknown;
}

extern int keymap_get_key(int value, int press);

int drops_qws_qt_keycode(int key, int press)
{
	/* This is a deadkeys-safe, extensible multi-byte keycode function */

	if(!press) return 0xffff;

	int unicode = keymap_get_key(key, press);

	LOGd(_DEBUG, "[mouse/keyboard:dope] key %i yields keymap status %i\n", key, unicode);

	if((unicode == 0) || (unicode < 0))
		return 0xffff;

	LOGd(_DEBUG, "[mouse/keyboard:dope] received unicode value %i\n", unicode);

	return unicode;
}
