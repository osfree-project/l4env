#include <stdio.h>
#include <unistd.h>
#include <malloc.h>

#include <flux/machine/pc/direct_cons.h>
#include <flux/machine/pc/reset.h>
#include <flux/machine/pio.h>
#include <flux/c/string.h>
#include <flux/base_critical.h>
#include <flux/lmm.h>

#include <l4/sys/kdebug.h>
#include <l4/util/cpu.h>
#include <l4/util/port_io.h>

#include "rmgr.h"
#include "version.h"
#include "libc_support.h"

static void __attribute__((noreturn))
reboot(void)
{
  l4util_out8(0x80, 0x70);
  l4util_iodelay();
  l4util_in8(0x71);
  l4util_iodelay();

  while (l4util_in8(0x64) & 0x02)
    ;

  l4util_out8(0x8F, 0x70);
  l4util_iodelay();
  l4util_out8(0x00, 0x71);
  l4util_iodelay();

  l4util_out8(0xFE, 0x64);
  l4util_iodelay();

  for (;;)
    ;
}

void
_exit(int fd)
{
  char c;

  printf("\n\"k\" enters L4 kernel debugger, all other keys reboot...\n");
  c = getchar();
  if (c == 'k' || c == 'K')
    enter_kdebug("before reboot");

  if (ux_running)
    enter_kdebug("*#^");

  reboot();
}

static void
kern_putchar(unsigned char c)
{
  if (!quiet)
    {
      outchar(c);
      if (c == '\n')
	outchar('\r');
    }
}

#include <flux/x86/pio.h>
#include <flux/x86/pc/keyboard.h>

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
//   {']','}'},		/* 27 */
  {'+',	'*'},		/* 27 */
  {'\r','\r'},		/* 28 - Enter */
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
  {'\'','"'},		/* 40 */
  {'`',	'~'},		/* 41 */
  {SHIFT,SHIFT},	/* 42 - Left Shift */
  {'\\','|'},		/* 43 */
  {'z',	'Z'},		/* 44 */
  {'x',	'X'},
  {'c',	'C'},
  {'v',	'V'},
  {'b',	'B'},
  {'n',	'N'},
  {'m',	'M'},
  {',',	'<'},
  {'.',	'>'},
  //  {'/','?'},	/* 53 */
  {'-',	'_'},		/* 53 */
  {SHIFT,SHIFT},	/* 54 - Right Shift */
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

static int
direct_cons_try_getchar(void)
{
  static unsigned shift_state;
  unsigned status, scan_code, ch;

  base_critical_enter();

retry:
  l4util_cpu_pause();

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

int
putchar(int c)
{
  kern_putchar(c);
  return c;
}

int
getchar(void)
{
  if (l4_version == VERSION_FIASCO)
    {
      int c;
      while (!(c = l4kd_inchar()))
	l4util_cpu_pause();
      return c;
    }
  return direct_cons_getchar();
}

int fflush(int dummy);
int
fflush(int dummy)
{
  return 0;
}

void reset_malloc(void);
void
reset_malloc(void)
{
  extern unsigned __malloc_pool[MALLOC_POOL_SIZE / sizeof(unsigned)];
  static lmm_region_t region;

  memset(__malloc_pool, 0, MALLOC_POOL_SIZE);
  lmm_init(&malloc_lmm);
  lmm_add_region(&malloc_lmm, &region, (void*)0, (vm_size_t)-1, 0, 0);
  lmm_add_free(&malloc_lmm, __malloc_pool, MALLOC_POOL_SIZE);
}
