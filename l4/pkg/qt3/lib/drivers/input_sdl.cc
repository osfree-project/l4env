/* $Id$ */
/*****************************************************************************/
/**
 * \file   lib/drivers/input_sdl.cc
 * \brief  Inputlib driver for Qt/Embedded.
 *
 * \date   10/24/2004
 * \author Josef Spillner <js177634@inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2004-2005 Technische Universitaet Dresden
 * This file is part of the Qt3 port for L4/DROPS, which is distributed under
 * the terms of the GNU General Public License 2. Please see the COPYING file
 * for details.
 */

/*
 * Based on inputlib example
 */

#include <l4/thread/thread.h>
#include <l4/input/libinput.h>
#include <l4/log/l4log.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include <qnamespace.h>

#include "input.h"

#define _DEBUG 0
#define _ERROR 1

#define DROPS_MOUSE_BUFFERSIZE 1

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
/* no scroll-lock, meta */
static int mouseinputchannel[2];
static int keyboardinputchannel[2];

static void sdlinput_flush(void)
{
	static struct l4input ev[DROPS_MOUSE_BUFFERSIZE];
	int rd, i;
	int message[3];

	if(!initialized)
	{
                //LOG("[mouse/keyboard] not initialized\n");
		return;
	}

	if (!l4input_ispending()) return;
	rd = l4input_flush(ev, DROPS_MOUSE_BUFFERSIZE);
	LOGd(_DEBUG, "[mouse/keyboard] got event(s)\n");
	for (i = 0; i < rd; i++)
	{
		LOGd(_DEBUG, "[mouse/keyboard] * event: %i/%i/%i\n", ev[i].type, ev[i].code, ev[i].value);
		switch(ev[i].type)
		{
			case EV_KEY:
				if((ev[i].code == BTN_LEFT) || (ev[i].code == BTN_RIGHT) || (ev[i].code == BTN_MIDDLE))
				{
					LOGd(_DEBUG, "@ mouse button\n");
					if(ev[i].value == 1)
					{
						if(ev[i].code == BTN_LEFT)
							mouse_button = 1;
						else if(ev[i].code == BTN_RIGHT)
							mouse_button = 2;
						else if(ev[i].code == BTN_MIDDLE)
							mouse_button = 3;
					}
					else mouse_button = 0;
					mouse_changed = 1;
					message[0] = 0;
					message[1] = mouse_button;
					message[2] = 0;
				}
				else
				{
					LOGd(_DEBUG, "@ keyboard key or other\n");
					if(ev[i].value == 1) key_pressed = 1;
					else key_pressed = 0;
					key_key = ev[i].code;
					key_changed = 1;
					message[0] = 2;
					message[1] = key_key;
					message[2] = key_pressed;

					switch(key_key)
					{
						case KEY_CAPSLOCK:
							key_mod_caps = key_pressed;
							break;
						case KEY_NUMLOCK:
							key_mod_num = key_pressed;
							break;
						case KEY_LEFTALT:
						case KEY_RIGHTALT:
							key_mod_alt = key_pressed;
							break;
						case KEY_LEFTSHIFT:
						case KEY_RIGHTSHIFT:
							key_mod_shift = key_pressed;
							break;
						case KEY_LEFTCTRL:
						case KEY_RIGHTCTRL:
							key_mod_ctrl = key_pressed;
							break;
							
					}
				}
				break;
			case EV_REL:
				LOGd(_DEBUG, "@ relative movement\n");
				if(ev[i].code == ABS_X)
				{
					mouse_x += ev[i].value;
					mouse_changed = 1;
				}
				else if(ev[i].code == ABS_Y)
				{
					mouse_y += ev[i].value;
					mouse_changed = 1;
				}
				LOGd(_DEBUG, "  new mouse pos: %i/%i\n", mouse_x, mouse_y);
				message[0] = 1;
				message[1] = mouse_x;
				message[2] = mouse_y;
				break;
			default:
				LOG("@ unknown\n");
				break;
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
	sdlinput_flush();
}

void drops_qws_init_devices(void)
{
	LOGd(_DEBUG, "[mouse/keyboard] init\n");

	l4input_init(0, L4THREAD_DEFAULT_PRIO, NULL);

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
	sdlinput_flush();
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
	sdlinput_flush();
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
		{KEY_ESC, Qt::Key_Escape},
		{KEY_0, Qt::Key_0},
		{KEY_1, Qt::Key_1},
		{KEY_2, Qt::Key_2},
		{KEY_3, Qt::Key_3},
		{KEY_4, Qt::Key_4},
		{KEY_5, Qt::Key_5},
		{KEY_6, Qt::Key_6},
		{KEY_7, Qt::Key_7},
		{KEY_8, Qt::Key_8},
		{KEY_9, Qt::Key_9},
		{KEY_0, Qt::Key_0},
		{KEY_MINUS, Qt::Key_Minus},
		{KEY_EQUAL, Qt::Key_Equal},
		{KEY_BACKSPACE, Qt::Key_Backspace},
		{KEY_TAB, Qt::Key_Tab},
		{KEY_Q, Qt::Key_Q},
		{KEY_W, Qt::Key_W},
		{KEY_E, Qt::Key_E},
		{KEY_R, Qt::Key_R},
		{KEY_T, Qt::Key_T},
		{KEY_Y, Qt::Key_Y},
		{KEY_U, Qt::Key_U},
		{KEY_I, Qt::Key_I},
		{KEY_O, Qt::Key_O},
		{KEY_P, Qt::Key_P},
		{KEY_LEFTBRACE, Qt::Key_BraceLeft},
		{KEY_RIGHTBRACE, Qt::Key_BraceRight},
		{KEY_ENTER, Qt::Key_Enter},
		{KEY_LEFTCTRL, Qt::Key_Control},
		{KEY_A, Qt::Key_A},
		{KEY_S, Qt::Key_S},
		{KEY_D, Qt::Key_D},
		{KEY_F, Qt::Key_F},
		{KEY_G, Qt::Key_G},
		{KEY_H, Qt::Key_H},
		{KEY_J, Qt::Key_J},
		{KEY_K, Qt::Key_K},
		{KEY_L, Qt::Key_L},
		{KEY_SEMICOLON, Qt::Key_Semicolon},
		{KEY_APOSTROPHE, Qt::Key_Apostrophe},
		{KEY_GRAVE, Qt::Key_unknown}, // XXX wtf?
		{KEY_LEFTSHIFT, Qt::Key_Shift},
		{KEY_BACKSLASH, Qt::Key_Backslash},
		{KEY_Z, Qt::Key_Z},
		{KEY_X, Qt::Key_X},
		{KEY_C, Qt::Key_C},
		{KEY_V, Qt::Key_V},
		{KEY_B, Qt::Key_B},
		{KEY_N, Qt::Key_N},
		{KEY_M, Qt::Key_M},
		{KEY_COMMA, Qt::Key_Comma},
		{KEY_DOT, Qt::Key_unknown}, // XXX wtf?
		{KEY_SLASH, Qt::Key_Slash},
		{KEY_RIGHTSHIFT, Qt::Key_Shift},
		{KEY_KPASTERISK, Qt::Key_Asterisk},
		{KEY_LEFTALT, Qt::Key_Alt},
		{KEY_SPACE, Qt::Key_Space},
		{KEY_CAPSLOCK, Qt::Key_CapsLock},
		{KEY_F1, Qt::Key_F1},
		{KEY_F2, Qt::Key_F2},
		{KEY_F3, Qt::Key_F3},
		{KEY_F4, Qt::Key_F4},
		{KEY_F5, Qt::Key_F5},
		{KEY_F6, Qt::Key_F6},
		{KEY_F7, Qt::Key_F7},
		{KEY_F8, Qt::Key_F8},
		{KEY_F9, Qt::Key_F9},
		{KEY_F10, Qt::Key_F10},
		{KEY_NUMLOCK, Qt::Key_NumLock},
		{KEY_SCROLLLOCK, Qt::Key_ScrollLock},
		{KEY_KP7, Qt::Key_7},
		{KEY_KP8, Qt::Key_8},
		{KEY_KP9, Qt::Key_9},
		{KEY_KPMINUS, Qt::Key_Minus},
		{KEY_KP4, Qt::Key_4},
		{KEY_KP5, Qt::Key_5},
		{KEY_KP6, Qt::Key_6},
		{KEY_KPPLUS, Qt::Key_Plus},
		{KEY_KP1, Qt::Key_1},
		{KEY_KP2, Qt::Key_2},
		{KEY_KP3, Qt::Key_3},
		{KEY_KP0, Qt::Key_0},
		{KEY_KPDOT, Qt::Key_unknown}, // XXX wtf?
		/* from now on, selectively */
		{KEY_F11, Qt::Key_F11},
		{KEY_F12, Qt::Key_F12},
		{KEY_KPENTER, Qt::Key_Enter},
		{KEY_RIGHTCTRL, Qt::Key_Control},
		{KEY_RIGHTALT, Qt::Key_Alt},
		{KEY_HOME, Qt::Key_Home},
		{KEY_UP, Qt::Key_Up},
		{KEY_PAGEUP, Qt::Key_PageUp},
		{KEY_LEFT, Qt::Key_Left},
		{KEY_RIGHT, Qt::Key_Right},
		{KEY_END, Qt::Key_End},
		{KEY_DOWN, Qt::Key_Down},
		{KEY_PAGEDOWN, Qt::Key_PageDown},
		{KEY_INSERT, Qt::Key_Insert},
		{KEY_DELETE, Qt::Key_Delete},
		{KEY_KPEQUAL, Qt::Key_Equal},
		{KEY_PAUSE, Qt::Key_Pause},
		{KEY_KPCOMMA, Qt::Key_Comma},
		{KEY_RESERVED, Qt::Key_unknown} // must be last
	};
	int i;

	for(i = 0; keymap[i][0] != KEY_RESERVED; i++)
		if(keymap[i][0] == key)
		{
			LOGd(_DEBUG, "[mouse/keyboard] map key %i to %i\n", key, keymap[i][1]);
			return keymap[i][1];
		}

	LOGd(_ERROR, "[mouse/keyboard] unknown key mapping for %i\n", key);
	return Qt::Key_unknown;
}

extern int keymap_get_key(int value, int press);

int drops_qws_qt_keycode(int key, int press)
{
	/* This is a deadkeys-safe, extensible multi-byte keycode function */

	int unicode = keymap_get_key(key, press);

	LOGd(_DEBUG, "[mouse/keyboard:sdl] key %i yields keymap status %i\n", key, unicode);

	if((unicode == 0) || (unicode < 0))
		return 0xffff;

	LOGd(_DEBUG, "[mouse/keyboard:sdl] received unicode value %i\n", unicode);

	return unicode;
}
