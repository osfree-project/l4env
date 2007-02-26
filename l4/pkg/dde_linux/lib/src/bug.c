/*!
 * \file   dde_linux/lib/src/bug.c
 * \brief  BUG() handler
 *
 * \date   02/26/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 * You can set your own BUG() handler by overwriting the dde_BUG function
 * pointer.
 */

#include <l4/dde_linux/dde.h>
#include <l4/log/l4log.h>
#include <linux/kernel.h>
#include <l4/sys/kdebug.h>

static void do_bug(const char*, const char*, int) __attribute__ ((noreturn));

void (*dde_BUG)(const char*file, const char*function, int line)
	__attribute__ ((noreturn)) = do_bug;

static void do_bug(const char*file, const char*function, int line){
	printk("BUG() called in %s:%d(%s)",
	     file, line, function);
	LOG_flush();
	while(1) enter_kdebug("bug");
}
