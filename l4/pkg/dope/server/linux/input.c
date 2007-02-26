/*
 * \brief   DOpE pseudo input driver module
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * It uses SDL for requesting the mouse state under
 * Linux.  All other modules should use this one to
 * get information about the mouse state. The hand-
 * ling of mouse cursor and its appeariance is done
 * by the 'Screen' component.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** GENERAL INCLUDES ***/
#include <stdlib.h>

/*** SDL INCLUDES ***/
#include <SDL/SDL.h>

/*** LOCAL INCLUDES ***/
#include "dopestd.h"
#include "event.h"
#include "keymap.h"
#include "widget.h"
#include "window.h"
#include "scrdrv.h"
#include "input.h"

static long scr_w=200,scr_h=200;
static long curr_mx=100,curr_my=100,curr_mb=0;
static char keytab[KEY_MAX];

static struct scrdrv_services *scrdrv;
static struct keymap_services *keymap;

int init_input(struct dope_services *d);

/*** MAP SDL KEYCODES TO DOpE EVENT KEYCODES ***/
static long map_keycode(SDLKey sk) {
	switch (sk) {
	case SDLK_BACKSPACE:    return KEY_BACKSPACE;
	case SDLK_TAB:          return KEY_TAB;
//  case SDLK_CLEAR:        return KEY_CLEAR;
	case SDLK_RETURN:       return KEY_ENTER;
	case SDLK_PAUSE:        return KEY_PAUSE;
	case SDLK_ESCAPE:       return KEY_ESC;
	case SDLK_SPACE:        return KEY_SPACE;
//  case SDLK_EXCLAIM:      return KEY_EXCLAIM;
//  case SDLK_QUOTEDBL:     return KEY_QUOTEDBL;
//  case SDLK_HASH:         return KEY_HASH;
//  case SDLK_DOLLAR:       return KEY_DOLLAR;
//  case SDLK_AMPERSAND:    return KEY_AMPERSAND;
//  case SDLK_QUOTE:        return KEY_QUOTE;
//  case SDLK_LEFTPAREN:    return KEY_LEFTPAREN;
//  case SDLK_RIGHTPAREN:   return KEY_RIGHTPAREN;
//  case SDLK_ASTERISK:     return KEY_ASTERISK;
//  case SDLK_PLUS:         return KEY_PLUS;
	case SDLK_COMMA:        return KEY_COMMA;
	case SDLK_MINUS:        return KEY_MINUS;
	case SDLK_PERIOD:       return KEY_DOT;
	case SDLK_SLASH:        return KEY_SLASH;
	case SDLK_0:            return KEY_0;
	case SDLK_1:            return KEY_1;
	case SDLK_2:            return KEY_2;
	case SDLK_3:            return KEY_3;
	case SDLK_4:            return KEY_4;
	case SDLK_5:            return KEY_5;
	case SDLK_6:            return KEY_6;
	case SDLK_7:            return KEY_7;
	case SDLK_8:            return KEY_8;
	case SDLK_9:            return KEY_9;
//  case SDLK_COLON:        return KEY_COLON;
	case SDLK_SEMICOLON:    return KEY_SEMICOLON;
//  case SDLK_LESS:         return KEY_LESS;
//  case SDLK_EQUALS:       return KEY_EQUALS;
//  case SDLK_GREATER:      return KEY_GREATER;
	case SDLK_QUESTION:     return KEY_QUESTION;
//  case SDLK_AT:           return KEY_AT;
//  case SDLK_LEFTBRACKET:  return KEY_LEFTBRACKET;
	case SDLK_BACKSLASH:    return KEY_BACKSLASH;
//  case SDLK_RIGHTBRACKET: return KEY_RIGHTBRACKET;
//  case SDLK_CARET:        return KEY_CARET;
//  case SDLK_UNDERSCORE:   return KEY_UNDERSCORE;
//  case SDLK_BACKQUOTE:    return KEY_BACKQUOTE;
	case SDLK_a:            return KEY_A;
	case SDLK_b:            return KEY_B;
	case SDLK_c:            return KEY_C;
	case SDLK_d:            return KEY_D;
	case SDLK_e:            return KEY_E;
	case SDLK_f:            return KEY_F;
	case SDLK_g:            return KEY_G;
	case SDLK_h:            return KEY_H;
	case SDLK_i:            return KEY_I;
	case SDLK_j:            return KEY_J;
	case SDLK_k:            return KEY_K;
	case SDLK_l:            return KEY_L;
	case SDLK_m:            return KEY_M;
	case SDLK_n:            return KEY_N;
	case SDLK_o:            return KEY_O;
	case SDLK_p:            return KEY_P;
	case SDLK_q:            return KEY_Q;
	case SDLK_r:            return KEY_R;
	case SDLK_s:            return KEY_S;
	case SDLK_t:            return KEY_T;
	case SDLK_u:            return KEY_U;
	case SDLK_v:            return KEY_V;
	case SDLK_w:            return KEY_W;
	case SDLK_x:            return KEY_X;
	case SDLK_y:            return KEY_Y;
	case SDLK_z:            return KEY_Z;
	case SDLK_DELETE:       return KEY_DELETE;

	/* Numeric keypad */
//  case SDLK_KP0:          return KEY_KP0;
//  case SDLK_KP1:          return KEY_KP1;
//  case SDLK_KP2:          return KEY_KP2;
//  case SDLK_KP3:          return KEY_KP3;
//  case SDLK_KP4:          return KEY_KP4;
//  case SDLK_KP5:          return KEY_KP5;
//  case SDLK_KP6:          return KEY_KP6;
//  case SDLK_KP7:          return KEY_KP7;
//  case SDLK_KP8:          return KEY_KP8;
//  case SDLK_KP9:          return KEY_KP9;
//  case SDLK_KP_PERIOD:    return KEY_KP_DOT;
//  case SDLK_KP_DIVIDE:    return KEY_KP_DIVIDE;
//  case SDLK_KP_MULTIPLY:  return KEY_KP_MULTIPLY;
//  case SDLK_KP_MINUS:     return KEY_KP_MINUS;
//  case SDLK_KP_PLUS:      return KEY_KP_PLUS;
//  case SDLK_KP_ENTER:     return KEY_KP_ENTER;
//  case SDLK_KP_EQUALS:    return KEY_KP_EQUALS;


	/* Arrows + Home/End pad */
	case SDLK_UP:           return KEY_UP;
	case SDLK_DOWN:         return KEY_DOWN;
	case SDLK_RIGHT:        return KEY_RIGHT;
	case SDLK_LEFT:         return KEY_LEFT;
	case SDLK_INSERT:       return KEY_INSERT;
	case SDLK_HOME:         return KEY_HOME;
	case SDLK_END:          return KEY_END;
	case SDLK_PAGEUP:       return KEY_PAGEUP;
	case SDLK_PAGEDOWN:     return KEY_PAGEDOWN;

    /* Function keys */
	case SDLK_F1:           return KEY_F1;
	case SDLK_F2:           return KEY_F2;
	case SDLK_F3:           return KEY_F3;
	case SDLK_F4:           return KEY_F4;
	case SDLK_F5:           return KEY_F5;
	case SDLK_F6:           return KEY_F6;
	case SDLK_F7:           return KEY_F7;
	case SDLK_F8:           return KEY_F8;
	case SDLK_F9:           return KEY_F9;
	case SDLK_F10:          return KEY_F10;
	case SDLK_F11:          return KEY_F11;
	case SDLK_F12:          return KEY_F12;
	case SDLK_F13:          return KEY_F13;
	case SDLK_F14:          return KEY_F14;
	case SDLK_F15:          return KEY_F15;

    /* Key state modifier keys */
	case SDLK_NUMLOCK:      return KEY_NUMLOCK;
	case SDLK_CAPSLOCK:     return KEY_CAPSLOCK;
//  case SDLK_SCROLLOCK:    return KEY_SCROLLOCK;
	case SDLK_RSHIFT:       return KEY_RIGHTSHIFT;
	case SDLK_LSHIFT:       return KEY_LEFTSHIFT;
	case SDLK_RCTRL:        return KEY_RIGHTCTRL;
	case SDLK_LCTRL:        return KEY_LEFTCTRL;
	case SDLK_RALT:         return KEY_RIGHTALT;
	case SDLK_LALT:         return KEY_LEFTALT;
	case SDLK_RMETA:        return KEY_RIGHTALT;
	case SDLK_LMETA:        return KEY_LEFTALT;
//  case SDLK_LSUPER:       return KEY_LSUPER;
//  case SDLK_RSUPER:       return KEY_RSUPER;
//  case SDLK_MODE:         return KEY_MODE;
//  case SDLK_COMPOSE:      return KEY_COMPOSE;

    /* Miscellaneous function keys */
//  case SDLK_HELP:         return KEY_HELP;
//  case SDLK_PRINT:        return KEY_PRINT;
//  case SDLK_SYSREQ:       return KEY_SYSREQ;
//  case SDLK_BREAK:        return KEY_BREAK;
//  case SDLK_MENU:         return KEY_MENU;
//  case SDLK_POWER:        return KEY_POWER;
//  case SDLK_EURO:         return KEY_EURO;
//  case SDLK_UNDO:         return KEY_UNDO;

	default: return 0;
	}
	return 0;
}


/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/


static long get_mx(void) {return curr_mx;}
static long get_my(void) {return curr_my;}
static long get_mb(void) {return curr_mb;}


/*** UPDATE MOUSE STATE, THIS FUNCTION SHOULD BE FREQUENTLY CALLED ***/
static void update(WIDGET *dst) {
	static EVENT e;
	WINDOW *w;
	static SDL_Event sdl_event;
	static int new_mx,new_my;

	SDL_GetMouseState(&new_mx,&new_my);

    while ( SDL_PollEvent(&sdl_event) ) {
		switch (sdl_event.type) {
		case SDL_KEYUP:
			e.type=EVENT_RELEASE;
			e.code=map_keycode(sdl_event.key.keysym.sym);
			keytab[e.code]=0;
			if (dst) dst->gen->handle_event(dst,&e);
			break;

		case SDL_KEYDOWN:
			e.type=EVENT_PRESS;
			e.code=map_keycode(sdl_event.key.keysym.sym);
			keytab[e.code]=1;
			if (dst) dst->gen->handle_event(dst,&e);
			break;

		case SDL_QUIT:
			scrdrv->restore_screen();
			exit(0);

		case SDL_MOUSEBUTTONDOWN:
			if (dst) {
				w = (WINDOW *)dst->gen->get_window(dst);
				if (w) w->win->activate(w);
			}
			switch (sdl_event.button.button) {
			case SDL_BUTTON_LEFT:
				curr_mb = curr_mb | 0x0001;
				e.type=EVENT_PRESS;
				e.code=BTN_LEFT;
				keytab[BTN_LEFT]=1;
				if (dst) dst->gen->handle_event(dst,&e);
				break;
			case SDL_BUTTON_RIGHT:
				curr_mb = curr_mb | 0x0002;
				e.type=EVENT_PRESS;
				e.code=BTN_RIGHT;
				keytab[BTN_RIGHT]=1;
				if (dst) dst->gen->handle_event(dst,&e);
				break;
			}
			break;

		case SDL_MOUSEBUTTONUP:

			switch (sdl_event.button.button) {
			case SDL_BUTTON_LEFT:
				curr_mb = curr_mb & 0x00fe;
				e.type=EVENT_RELEASE;
				e.code=BTN_LEFT;
				keytab[BTN_LEFT]=1;
				if (dst) dst->gen->handle_event(dst,&e);
				break;
			case SDL_BUTTON_RIGHT:
				curr_mb = curr_mb & 0x00fd;
				e.type=EVENT_RELEASE;
				e.code=BTN_RIGHT;
				keytab[BTN_RIGHT]=1;
				if (dst) dst->gen->handle_event(dst,&e);
				break;
			}
			break;

		}
	}

	curr_mx=new_mx;
	curr_my=new_my;
}


/*** FORCE A NEW MOUSE POSITION ***/
static void set_pos(long new_mx,long new_my) {
	SDL_WarpMouse(new_mx,new_my);
	update(NULL);
}


static long get_keystate(long keycode) {
	if (keycode>=KEY_MAX) return 0;
	return keytab[keycode];
}


static char get_ascii(long keycode) {
	long switches=0;
	if (keycode>=KEY_MAX) return 0;
	if (keytab[42] ) switches = switches | KEYMAP_SWITCH_LSHIFT;
	if (keytab[54] ) switches = switches | KEYMAP_SWITCH_RSHIFT;
	if (keytab[29] ) switches = switches | KEYMAP_SWITCH_LCONTROL;
	if (keytab[97] ) switches = switches | KEYMAP_SWITCH_RCONTROL;
	if (keytab[56] ) switches = switches | KEYMAP_SWITCH_ALT;
	if (keytab[100]) switches = switches | KEYMAP_SWITCH_ALTGR;
	return keymap->get_ascii(keycode,switches);
}


static void update_properties(void) {
	scr_w = scrdrv->get_scr_width();
	scr_h = scrdrv->get_scr_height();

	if (curr_mx<0) curr_mx=0;
	if (curr_mx>=scr_w) curr_mx=scr_w-1;
	if (curr_my<0) curr_my=0;
	if (curr_my>=scr_h) curr_my=scr_h-1;
}


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct input_services input = {
	get_mx,
	get_my,
	get_mb,
	set_pos,
	get_keystate,
	get_ascii,
	update,
	update_properties
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_input(struct dope_services *d) {

	scrdrv = d->get_module("ScreenDriver 1.0");
	keymap = d->get_module("Keymap 1.0");

	d->register_module("Input 1.0",&input);
	return 1;
}
