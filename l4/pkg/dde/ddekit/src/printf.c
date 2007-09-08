/*
 * \brief   Logging facility with printf()-like interface
 * \author  Thomas Friebel <yaron@yaron.de>
 * \date    2006-03-01
 */

#include <l4/dde/ddekit/printf.h>

#include <l4/log/l4log.h>

/**
 * Log constant string message w/o arguments
 *
 * \param msg  message to be logged
 */
int ddekit_print(const char *msg)
{
	return LOG_printf("%s", msg);
}

/**
 * Log message with print()-like arguments
 *
 * \param fmt  format string followed by optional arguments
 */
int ddekit_printf(const char *fmt, ...)
{
	int res;
	va_list va;

	va_start(va, fmt);
	res = ddekit_vprintf(fmt, va);
	va_end(va);

	return res;
}

/* Log message with vprintf()-like arguments
 *
 * \param fmt  format string
 * \param va   variable argument list
 */
int ddekit_vprintf(const char *fmt, va_list va)
{
	return LOG_vprintf(fmt, va);
}
