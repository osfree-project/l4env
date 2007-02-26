#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <l4/sys/compiler.h>
#include <l4/sys/kdebug.h>
#include <l4/util/reboot.h>
#include <l4/util/util.h>

#ifndef USE_OSKIT
#include <l4/env_support/getchar.h>
#endif

#include "boot_error.h"
#include "memmap.h"
#include "region.h"
#include "rmgr.h"

int boot_warnings = 0;
int boot_errors = 0;

#define COLOR_DEFAULT	"\033[m"
#define COLOR_RED	"\033[31m"
#define COLOR_CYAN	"\033[36m"
#define COLOR_MAGENTA	"\033[35m"
#define COLOR_WHITE	"\033[37m"

#define STR_SIZE	1024

#ifdef USE_OSKIT
int fflush(int dummy);
#endif

static void
print_color(const char *color, const char *fmt)
{
  printf("%s%s%s", color, fmt, COLOR_DEFAULT);
}

extern void l4util_reboot_arch(void);

static void
prompt(int may_continue)
{
  for (;;)
    {
      char c;
      printf("\r\n %s[Esc] reboot, [k] kdebug, [m] memory map, [r] region map",
	  may_continue ? "[Return] continue, " : "");
      fflush(0);
      c = getchar();
      switch (c)
	{
	case 0x1B:
	  printf("\n Rebooting...\n");
	  fflush(0);
	  if (ux_running)
	    enter_kdebug("*#^");
	  l4util_reboot_arch();
	  break;
	case 0x0A:
	case 0x0D:
	  if (may_continue)
	    {
	      printf("\n\n");
	      return;
	    }
	  break;
	case 'k':
	case 'K':
	  enter_kdebug("roottask");
	  break;
	case 'm':
	case 'M':
	  printf("\n\n");
	  memmap_dump();
	  break;
	case 'r':
	case 'R':
	  printf("\n\n");
	  regions_dump();
	  break;
	}
    }
}

void
boot_wait(void)
{
  printf("\nRoottask boot wait: \n");
  printf("there were %d boot warnings and %d boot errors\n", 
      boot_warnings, boot_errors);
  prompt(1);
}

/**
 * a boot warning occured.
 **/
void
boot_warning(const char *fmt, ...)
{
  char str[STR_SIZE], fmt_str[STR_SIZE];
  va_list args;

  snprintf(str, STR_SIZE, "\nRoottask boot warning: ");
  va_start(args, fmt);
  vsnprintf(fmt_str, STR_SIZE, fmt, args);
  va_end(args);
  strncat(str, fmt_str, STR_SIZE);
  print_color(COLOR_CYAN, str);
  prompt(1);
  boot_warnings++;
}

/**
 * a boot error occured.
 *
 **/
void
boot_error(const char *fmt, ...)
{
  char str[STR_SIZE], fmt_str[STR_SIZE];
  va_list args;

  snprintf(str, STR_SIZE,  "\nRoottask boot error: ");
  va_start(args, fmt);
  vsnprintf(fmt_str, STR_SIZE, fmt, args);
  va_end(args);
  strncat(str, fmt_str, STR_SIZE);
  print_color(COLOR_RED, str);
  prompt(1);
  boot_errors++;
}


/**
 * A boot panic occured.
 */
void
boot_panic(const char *fmt, ...)
{
  char str[STR_SIZE], fmt_str[STR_SIZE];
  va_list args;

  snprintf(str, STR_SIZE, "\nRoottask boot panic: ");
  va_start(args, fmt);
  vsnprintf(fmt_str, STR_SIZE, fmt, args);
  va_end(args);
  strncat(str, fmt_str, STR_SIZE);
  print_color(COLOR_MAGENTA, str);
  prompt(0);
}
