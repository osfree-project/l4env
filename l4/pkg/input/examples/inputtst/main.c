/* $Id$ */
/*****************************************************************************/
/**
 * \file   input/examples/inputtst/main.c
 * \brief  Test modes of L4INPUT
 *
 * \date   11/20/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 */
#include <l4/thread/thread.h>

/* C */
#include <stdio.h>
#include <string.h>

/* input */
#include <l4/input/libinput.h>

char LOG_tag[9] = "inputtst";
l4_ssize_t l4libc_heapsize = 128 * 1024;

/*****************************************************************************/

char *events[EV_MAX + 1] = { "Reset", "Key", "Relative", "Absolute", NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	"LED", "Sound", NULL, "Repeat", "ForceFeedback", NULL,
	"ForceFeedbackStatus"};

char *keys[KEY_MAX + 1] = { "Reserved", "Esc", "1", "2", "3", "4", "5", "6",
	"7", "8", "9", "0", "Minus", "Equal", "Backspace", "Tab", "Q", "W", "E",
	"R", "T", "Y", "U", "I", "O", "P", "LeftBrace", "RightBrace", "Enter",
	"LeftControl", "A", "S", "D", "F", "G", "H", "J", "K", "L", "Semicolon",
	"Apostrophe", "Grave", "LeftShift", "BackSlash", "Z", "X", "C", "V", "B",
	"N", "M", "Comma", "Dot", "Slash", "RightShift", "KPAsterisk", "LeftAlt",
	"Space", "CapsLock", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9",
	"F10", "NumLock", "ScrollLock", "KP7", "KP8", "KP9", "KPMinus", "KP4",
	"KP5", "KP6", "KPPlus", "KP1", "KP2", "KP3", "KP0", "KPDot", "103rd",
	"F13", "102nd", "F11", "F12", "F14", "F15", "F16", "F17", "F18", "F19",
	"F20", "KPEnter", "RightCtrl", "KPSlash", "SysRq", "RightAlt", "LineFeed",
	"Home", "Up", "PageUp", "Left", "Right", "End", "Down", "PageDown",
	"Insert", "Delete", "Macro", "Mute", "VolumeDown", "VolumeUp", "Power",
	"KPEqual", "KPPlusMinus", "Pause", "F21", "F22", "F23", "F24", "KPComma",
	"LeftMeta", "RightMeta", "Compose", "Stop", "Again", "Props", "Undo",
	"Front", "Copy", "Open", "Paste", "Find", "Cut", "Help", "Menu", "Calc",
	"Setup", "Sleep", "WakeUp", "File", "SendFile", "DeleteFile", "X-fer",
	"Prog1", "Prog2", "WWW", "MSDOS", "Coffee", "Direction", "CycleWindows",
	"Mail", "Bookmarks", "Computer", "Back", "Forward", "CloseCD", "EjectCD",
	"EjectCloseCD", "NextSong", "PlayPause", "PreviousSong", "StopCD",
	"Record", "Rewind", "Phone", "ISOKey", "Config", "HomePage", "Refresh",
	"Exit", "Move", "Edit", "ScrollUp", "ScrollDown", "KPLeftParenthesis",
	"KPRightParenthesis", "International1", "International2", "International3",
	"International4", "International5", "International6", "International7",
	"International8", "International9", "Language1", "Language2", "Language3",
	"Language4", "Language5", "Language6", "Language7", "Language8",
	"Language9", NULL, "PlayCD", "PauseCD", "Prog3", "Prog4", "Suspend",
	"Close", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, "Btn0", "Btn1", "Btn2", "Btn3", "Btn4", "Btn5", "Btn6",
	"Btn7", "Btn8", "Btn9", NULL, NULL,  NULL, NULL, NULL, NULL, "LeftBtn",
	"RightBtn", "MiddleBtn", "SideBtn", "ExtraBtn", "ForwardBtn", "BackBtn",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "Trigger",
	"ThumbBtn", "ThumbBtn2", "TopBtn", "TopBtn2", "PinkieBtn", "BaseBtn",
	"BaseBtn2", "BaseBtn3", "BaseBtn4", "BaseBtn5", "BaseBtn6", NULL, NULL,
	NULL, "BtnDead", "BtnA", "BtnB", "BtnC", "BtnX", "BtnY", "BtnZ", "BtnTL",
	"BtnTR", "BtnTL2", "BtnTR2", "BtnSelect", "BtnStart", "BtnMode",
	"BtnThumbL", "BtnThumbR", NULL, "ToolPen", "ToolRubber", "ToolBrush",
	"ToolPencil", "ToolAirbrush", "ToolFinger", "ToolMouse", "ToolLens", NULL,
	NULL, "Touch", "Stylus", "Stylus2" };

char *absval[5] = { "Value", "Min  ", "Max  ", "Fuzz ", "Flat " };

char *relatives[REL_MAX + 1] = { "X", "Y", "Z", NULL, NULL, NULL, "HWheel",
	"Dial", "Wheel" };

char *absolutes[ABS_MAX + 1] = { "X", "Y", "Z", "Rx", "Ry", "Rz", "Throttle",
	"Rudder", "Wheel", "Gas", "Brake", NULL, NULL, NULL, NULL, NULL, "Hat0X",
	"Hat0Y", "Hat1X", "Hat1Y", "Hat2X", "Hat2Y", "Hat3X", "Hat 3Y", "Pressure",
	"Distance", "XTilt", "YTilt"};

char *leds[LED_MAX + 1] = { "NumLock", "CapsLock", "ScrollLock", "Compose",
	"Kana", "Sleep", "Suspend", "Mute" };

char *repeats[REP_MAX + 1] = { "Delay", "Period" };

char *sounds[SND_MAX + 1] = { "Bell", "Click" };

char **names[EV_MAX + 1] = { events, keys, relatives, absolutes, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, leds,
	sounds, NULL, repeats, NULL, NULL, NULL };

#define BITS_PER_LONG        (sizeof(long) * 8)
#define NBITS(x)             ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)               ((x)%BITS_PER_LONG)
#define BIT(x)               (1UL<<OFF(x))
#define LONG(x)              ((x)/BITS_PER_LONG)
#define test_bit(bit, array) ((array[LONG(bit)] >> OFF(bit)) & 1)

/*****************************************************************************/

/** LOG L4INPUT EVENT TO SCREEN **/
static void log_event(struct l4input *ev)
{
	if (!ev->type)
		printf("inputtst: XXX type = 0 XXX\n");

	printf("Event: type %d (%s), code %d (%s), value %d\n",
	       ev->type, events[ev->type] ? events[ev->type] : "?",
	       ev->code,
	       names[ev->type] ?
	         (names[ev->type][ev->code] ?
	            names[ev->type][ev->code] : "?") :
	         "?", ev->value);

	if ((ev->type == EV_KEY) && (ev->value == 1)) {
		l4input_pcspkr(800);
		l4thread_sleep(20);
		l4input_pcspkr(0);
	}
}

/** CALLBACK MODE TEST **/
static void event_cb(struct l4input *ev)
{
	/* XXX seems stupid but it's historical */
	log_event(ev);
}

/** BUFFER MODE TEST **/
static int event_buf(void)
{
	int rd, i;
	static struct l4input ev[64];

	while (1) {
		l4thread_sleep(2);
		if (!l4input_ispending()) continue;

		rd = l4input_flush(ev, 64);

		for (i = 0; i < rd; i++)
			log_event(&ev[i]);
	}
}

/*****************************************************************************/

static int usage(void)
{
	printf("Usage: inputtst MODE [-omega0]\n");
	printf("MODE is one of:\n");
	printf("  -cb      callback mode\n");
	printf("  -buf     buffer mode\n");
	printf("\n");
	printf("  -omega0  use Omega0 IRQ server\n");
	return 1;
}

int main(int argc, char **argv)
{
	int use_omega0 = 0;

	if (argc < 2)
		return usage();

	if (argc > 2) {
		if (strcmp(argv[2], "-omega0") == 0)
			use_omega0 = 1;
		else
			return usage();
	}
	if (strcmp(argv[1], "-cb") == 0) {
		printf("Testing L4INPUT callback mode...\n");
		printf("init => %d\n",
		       l4input_init(use_omega0, L4THREAD_DEFAULT_PRIO, event_cb));
		l4thread_sleep(-1);
	}
	else if (strcmp(argv[1], "-buf") == 0) {
		printf("Testing L4INPUT buffer mode...\n");
		printf("init => %d\n",
		       l4input_init(use_omega0, L4THREAD_DEFAULT_PRIO, NULL));
		event_buf();
	}
	else
		return usage();

	return 0;
}
