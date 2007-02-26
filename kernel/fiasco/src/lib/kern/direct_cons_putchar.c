/* 
 * Copyright (c) 1995-1994 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 */

/* This is a special "feature" (read: kludge)
   intended for use only for kernel debugging.
   It enables an extremely simple console output mechanism
   that sends text straight to CGA/EGA/VGA video memory.
   It has the nice property of being functional right from the start,
   so it can be used to debug things that happen very early
   before any devices are initialized.  */

#include <string.h>
#include <flux/base_critical.h>
#include <flux/x86/base_vm.h>
#include <flux/x86/pc/direct_cons.h>

#define VIDBASE ((unsigned char *)phystokv(0xb8000))

void
direct_cons_putchar(unsigned char c)
{
	static int ofs = -1;

	base_critical_enter();

	if (ofs < 0)
	{
		/* Called for the first time - initialize.  */
		ofs = 0;
		direct_cons_putchar('\n');
	}

	switch (c)
	{
	case '\n':
		memcpy(VIDBASE, VIDBASE+80*2, 80*2*24);
		memset(VIDBASE+80*2*24,0, 80*2);
		/* fall through... */
	case '\r':
		ofs = 0;
		break;

	case '\t':
		ofs = (ofs + 8) & ~7;
		break;

	default:
		/* Wrap if we reach the end of a line.  */
		if (ofs >= 80) {
			direct_cons_putchar('\n');
		}

		/* Stuff the character into the video buffer. */
		{
			volatile unsigned char *p = VIDBASE + 80*2*24 + ofs*2;
			p[0] = c;
			p[1] = 0x0f;
			ofs++;
		}
		break;
	}

	base_critical_leave();
}

