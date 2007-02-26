/* direct_cons_try_getchar() */

#include <flux/x86/pio.h>
#include <flux/x86/pc/keyboard.h>
#include <flux/base_critical.h>

#define SHIFT -1

static const char keymap[128][2] = {
  {0},			/* 0 */
  {27,	27},		/* 1 - ESC */
  {'1',	'!'},		/* 2 */
  {'2',	'@'},
  {'3',	'#'},
  {'4',	'$'},
  {'5',	'%'},
  {'6',	'^'},
  {'7',	'&'},
  {'8',	'*'},
  {'9',	'('},
  {'0',	')'},
  {'-',	'_'},
  {'=',	'+'},
  {8,	8},		/* 14 - Backspace */
  {'\t','\t'},		/* 15 */
  {'q',	'Q'},
  {'w',	'W'},
  {'e',	'E'},
  {'r',	'R'},
  {'t',	'T'},
  {'y',	'Y'},
  {'u',	'U'},
  {'i',	'I'},
  {'o',	'O'},
  {'p',	'P'},
  {'[',	'{'},
//   {']',	'}'},		/* 27 */
  {'+',	'*'},		/* 27 */
  {'\r',	'\r'},		/* 28 - Enter */
  {0,	0},		/* 29 - Ctrl */
  {'a',	'A'},		/* 30 */
  {'s',	'S'},
  {'d',	'D'},
  {'f',	'F'},
  {'g',	'G'},
  {'h',	'H'},
  {'j',	'J'},
  {'k',	'K'},
  {'l',	'L'},
  {';',	':'},
  {'\'',	'"'},		/* 40 */
  {'`',	'~'},		/* 41 */
  {SHIFT,	SHIFT},		/* 42 - Left Shift */
  {'\\',	'|'},		/* 43 */
  {'z',	'Z'},		/* 44 */
  {'x',	'X'},
  {'c',	'C'},
  {'v',	'V'},
  {'b',	'B'},
  {'n',	'N'},
  {'m',	'M'},
  {',',	'<'},
  {'.',	'>'},
  //  {'/',	'?'},		/* 53 */
  {'-',	'_'},		/* 53 */
  {SHIFT,	SHIFT},		/* 54 - Right Shift */
  {0,	0},		/* 55 - Print Screen */
  {0,	0},		/* 56 - Alt */
  {' ',	' '},		/* 57 - Space bar */
  {0,	0},		/* 58 - Caps Lock */
  {0,	0},		/* 59 - F1 */
  {0,	0},		/* 60 - F2 */
  {0,	0},		/* 61 - F3 */
  {0,	0},		/* 62 - F4 */
  {0,	0},		/* 63 - F5 */
  {0,	0},		/* 64 - F6 */
  {0,	0},		/* 65 - F7 */
  {0,	0},		/* 66 - F8 */
  {0,	0},		/* 67 - F9 */
  {0,	0},		/* 68 - F10 */
  {0,	0},		/* 69 - Num Lock */
  {0,	0},		/* 70 - Scroll Lock */
  {'7',	'7'},		/* 71 - Numeric keypad 7 */
  {'8',	'8'},		/* 72 - Numeric keypad 8 */
  {'9',	'9'},		/* 73 - Numeric keypad 9 */
  {'-',	'-'},		/* 74 - Numeric keypad '-' */
  {'4',	'4'},		/* 75 - Numeric keypad 4 */
  {'5',	'5'},		/* 76 - Numeric keypad 5 */
  {'6',	'6'},		/* 77 - Numeric keypad 6 */
  {'+',	'+'},		/* 78 - Numeric keypad '+' */
  {'1',	'1'},		/* 79 - Numeric keypad 1 */
  {'2',	'2'},		/* 80 - Numeric keypad 2 */
  {'3',	'3'},		/* 81 - Numeric keypad 3 */
  {'0',	'0'},		/* 82 - Numeric keypad 0 */
  {'.',	'.'},		/* 83 - Numeric keypad '.' */
};

int
direct_cons_try_getchar(void)
{
  static unsigned shift_state;
  unsigned status, scan_code, ch;

  base_critical_enter();

retry:

  /* Wait until a scan code is ready and read it. */
  status = inb(0x64);
  if ((status & K_OBUF_FUL) == 0) {
    base_critical_leave();
    return -1;
  }
  scan_code = inb(0x60);

  /* Drop mouse events */
  if ((status & K_AUX_OBUF_FUL) != 0) {
    base_critical_leave();
    return -1;
  }

  /* Handle key releases - only release of SHIFT is important. */
  if (scan_code & 0x80) {
    scan_code &= 0x7f;
    if (keymap[scan_code][0] == SHIFT)
      shift_state = 0;
    goto retry;
  }

  /* Translate the character through the keymap. */
  ch = keymap[scan_code][shift_state];
  if (ch == (unsigned)SHIFT) {
    shift_state = 1;
    goto retry;
  } else if (ch == 0)
    goto retry;

  base_critical_leave();

  return ch;
}

int
direct_cons_getchar(void)
{
  int c;
  
  do
    {
      c = direct_cons_try_getchar();
    } while (c == -1);

  return c;
}
