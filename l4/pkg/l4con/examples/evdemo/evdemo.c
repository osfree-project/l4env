/* $Id$ */

/*	con/examples/evdemo/evdemo.c
 *
 *	demonstration server for con
 *
 *	event distribution demonstration
 */

/* L4 includes */
#include <l4/thread/thread.h>
#include <l4/sys/kdebug.h>
#include <l4/util/util.h>
#include <l4/input/libinput.h>

#include <l4/con/l4con.h>
#include <l4/con/con-client.h>
#include <l4/con/stream-server.h>

#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/oskit10_l4env/support.h>

#define PROGTAG		"_evdemo"

#define MY_SBUF_SIZE	4096

/* OSKit includes */
#include <stdio.h>

/* local includes */
#include "util.h"

/* internal prototypes */
void ev_loop(void);

/* global vars */
l4_threadid_t my_l4id,			/* it's me */
	ev_l4id,			/* my event handler */
	con_l4id,			/* con at names */
	vc_l4id;			/* partner VC */

/******************************************************************************
 * stream_io - IDL server function                                            *
 ******************************************************************************/

/* grabbed from evtest.c */
char *keys[KEY_MAX + 1] = { "Reserved", "Esc", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "Minus", "Equal", "Backspace",
"Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "LeftBrace", "RightBrace", "Enter", "LeftControl", "A", "S", "D", "F", "G",
"H", "J", "K", "L", "Semicolon", "Apostrophe", "Grave", "LeftShift", "BackSlash", "Z", "X", "C", "V", "B", "N", "M", "Comma", "Dot",
"Slash", "RightShift", "KPAsterisk", "LeftAlt", "Space", "CapsLock", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
"NumLock", "ScrollLock", "KP7", "KP8", "KP9", "KPMinus", "KP4", "KP5", "KP6", "KPPlus", "KP1", "KP2", "KP3", "KP0", "KPDot", "103rd",
"F13", "102nd", "F11", "F12", "F14", "F15", "F16", "F17", "F18", "F19", "F20", "KPEnter", "RightCtrl", "KPSlash", "SysRq",
"RightAlt", "LineFeed", "Home", "Up", "PageUp", "Left", "Right", "End", "Down", "PageDown", "Insert", "Delete", "Macro", "Mute",
"VolumeDown", "VolumeUp", "Power", "KPEqual", "KPPlusMinus", "Pause", "F21", "F22", "F23", "F24", "KPComma", "LeftMeta", "RightMeta",
"Compose", "Stop", "Again", "Props", "Undo", "Front", "Copy", "Open", "Paste", "Find", "Cut", "Help", "Menu", "Calc", "Setup",
"Sleep", "WakeUp", "File", "SendFile", "DeleteFile", "X-fer", "Prog1", "Prog2", "WWW", "MSDOS", "Coffee", "Direction",
"CycleWindows", "Mail", "Bookmarks", "Computer", "Back", "Forward", "CloseCD", "EjectCD", "EjectCloseCD", "NextSong", "PlayPause",
"PreviousSong", "StopCD", "Record", "Rewind", "Phone", "ISOKey", "Config", "HomePage", "Refresh", "Exit", "Move", "Edit", "ScrollUp",
"ScrollDown", "KPLeftParenthesis", "KPRightParenthesis",
"International1", "International2", "International3", "International4", "International5",
"International6", "International7", "International8", "International9",
"Language1", "Language2", "Language3", "Language4", "Language5", "Language6", "Language7", "Language8", "Language9",
NULL, 
"PlayCD", "PauseCD", "Prog3", "Prog4", "Suspend", "Close",
NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
"Btn0", "Btn1", "Btn2", "Btn3", "Btn4", "Btn5", "Btn6", "Btn7", "Btn8", "Btn9",
NULL, NULL,  NULL, NULL, NULL, NULL,
"LeftBtn", "RightBtn", "MiddleBtn", "SideBtn", "ExtraBtn", "ForwardBtn", "BackBtn",
NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
"Trigger", "ThumbBtn", "ThumbBtn2", "TopBtn", "TopBtn2", "PinkieBtn",
"BaseBtn", "BaseBtn2", "BaseBtn3", "BaseBtn4", "BaseBtn5", "BaseBtn6",
NULL, NULL, NULL, NULL,
"BtnA", "BtnB", "BtnC", "BtnX", "BtnY", "BtnZ", "BtnTL", "BtnTR", "BtnTL2", "BtnTR2", "BtnSelect", "BtnStart", "BtnMode",
NULL, NULL, NULL,
"ToolPen", "ToolRubber", "ToolBrush", "ToolPencil", "ToolAirbrush", "ToolFinger", "ToolMouse", "ToolLens", NULL, NULL,
"Touch", "Stylus", "Stylus2" };

char *pressed = "pressed";
char *released = "released";

/******************************************************************************
 * stream_io_server_push                                                      *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 *      event         ... struct describing the event                         *
 * out: _ev           ... Flick exception (unused)                            *
 *                                                                            *
 * push event into con event stream                                           *
 ******************************************************************************/
void stream_io_server_push(sm_request_t *request, 
			   const stream_io_input_event_t *event, 
			   sm_exc_t *_ev)
{
	l4input_t *input_ev = (l4input_t*) event;

	static unsigned short code;
	static unsigned char down = 0;

	static char *value_str;		/* `pressed' or `released' */
	static char *code_str;		/* key description */

	if (input_ev->type != EV_KEY) return;

	/* filter autorepeated keys out --- needed if h/w doesn't support 
	 * disabling (look in server/src/ev.c) */
	if (down && input_ev->value)
		if (code == input_ev->code) return;

	code_str = keys[input_ev->code];
	code = input_ev->code;

	if (input_ev->value) {
		value_str = pressed;
		down = 1;
	}
	else {
		value_str = released;
		down = 0;
	}

	printf("You %s the %s key.\n", value_str, code_str);
}

/******************************************************************************
 * ev_loop                                                                    *
 *                                                                            *
 * event reception loop                                                       *
 ******************************************************************************/
void ev_loop()
{
	int ret;
	l4_msgdope_t result;
	sm_request_t request;
  	l4_ipc_buffer_t ipc_buf;

	printf("Serving events as %x.%02x\n",
	       ev_l4id.id.task, ev_l4id.id.lthread);

	flick_init_request(&request, &ipc_buf);

	/* tell creator that we are running */
	l4thread_started(NULL);

	/* IDL server loop */
	while (1) {
		result = flick_server_wait(&request);

		while (!L4_IPC_IS_ERROR(result)) {
			/* dispatch request */
			ret = stream_io_server(&request);

			switch(ret) {
			case DISPATCH_ACK_SEND:
				/* reply and wait for next request */
				result = flick_server_reply_and_wait(&request);
				break;
				
			default:
				printf("Flick dispatch error (%d)!\n", ret);
				
				/* wait for next request */
				result = flick_server_wait(&request);
				break;
			}
		} /* !L4_IPC_IS_ERROR(result) */

		/* Ooops, we got an IPC error -> do something */
		printf(" Flick IPC error (%#x)", L4_IPC_ERROR(result));
		enter_kdebug("PANIC");
	}
}

/******************************************************************************
 * main                                                                       *
 *                                                                            *
 * Main function                                                              *
 ******************************************************************************/
int main(int argc, char *argv[])
{
	sm_exc_t _ev;

	/* init */
	LOG_init(PROGTAG);
	my_l4id = l4thread_l4_id( l4thread_myself() );

	printf("How are you? I'm running as %x.%02x\n", 
	       my_l4id.id.task, my_l4id.id.lthread);

	/* ask for 'con' (timeout = 5000 ms) */
	if (names_waitfor_name(CON_NAMES_STR, &con_l4id, 50000) == 0) {
		printf("PANIC: %s not registered at names", CON_NAMES_STR);
		enter_kdebug("panic");
	}

	if (con_if_openqry(con_l4id, MY_SBUF_SIZE, 0, 0, 
			   L4THREAD_DEFAULT_PRIO,
			   (con_threadid_t*) &vc_l4id, 
			   CON_VFB,
			   &_ev))
		enter_kdebug("Ouch, open vc failed");
	printf("Hey, openqry okay\n");

	/* let's start the event handler */
	ev_l4id = l4thread_l4_id( l4thread_create((l4thread_fn_t) ev_loop,
						  NULL,
						  L4THREAD_CREATE_SYNC) );

	if (con_vc_smode(vc_l4id, CON_INOUT, (con_threadid_t*) &ev_l4id, &_ev))
		enter_kdebug("Ouch, setup vc failed");
	printf("Cool, smode okay\n");

	/* do something ... */
	for(;;) {}

	if (con_vc_close(vc_l4id, &_ev))
		enter_kdebug("Ouch, close vc failed?!");
	printf("Finally closed vc\n");

	printf("Going to bed ...\n");
	l4_sleep(-1);

	exit(0);
}
