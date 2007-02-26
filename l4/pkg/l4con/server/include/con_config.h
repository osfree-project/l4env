/* $Id$ */

/*	con/server/include/con_config.h
 *
 *	con configuration macros
 */

/* OSKit malloc */
#define CONFIG_MALLOC_MAX_SIZE	(1024*1024)

/* visual --- unused */
#define CONFIG_XRES		640
#define CONFIG_YRES		480
#define CONFIG_MY_BITS		16

/* vc */
#define CONFIG_MAX_VC		5		/* number of virtual consoles */
#define CONFIG_MAX_SBUF_SIZE	(4*256*256)	/* max string buffer */

/* ev */
#define IRQ_HANDLER_STACKSIZE	0x2000

