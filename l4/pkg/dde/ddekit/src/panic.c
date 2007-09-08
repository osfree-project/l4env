#include <l4/dde/ddekit/panic.h>
#include <l4/dde/ddekit/printf.h>

#include <l4/sys/kdebug.h>
#include <stdarg.h>

void ddekit_panic(char *fmt, ...) {
	va_list va;

	va_start(va, fmt);
	ddekit_vprintf(fmt, va);
	va_end(va);
	ddekit_printf("\n");

	while (1)
		enter_kdebug("ddekit_panic()");
}

void ddekit_debug(char *fmt, ...) {
	va_list va;

	va_start(va, fmt);
	ddekit_vprintf(fmt, va);
	va_end(va);
	ddekit_printf("\n");

	enter_kdebug("ddekit_debug()");
}

