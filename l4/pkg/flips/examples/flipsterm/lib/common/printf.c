#include <stdarg.h>

/*** L4 INCLUDES ***/
#include <l4/dope/term.h>

int printf(const char *format, ...) {
	va_list list;
	va_start(list, format);
	term_vprintf(format, list);
	va_end(list);
	return 0;
}

