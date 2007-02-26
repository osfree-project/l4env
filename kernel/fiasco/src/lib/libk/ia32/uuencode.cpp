IMPLEMENTATION:

// adapted freely from FreeBSD's /usr/src/usr.bin/uuencode/uuencode.c

/*-
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if 0
#ifndef lint
static const char copyright[] =
"@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)uuencode.c	8.2 (Berkeley) 4/2/94";
#endif
static const char rcsid[] =
	"$Id$";
#endif /* not lint */
#endif

#include <stdio.h>
#include <string.h>

/* ENC is the basic 1 character encoding function to make a char printing */
#define	ENC(c) ((c) ? ((c) & 077) + ' ': '`')

void
uu_open(const char *name, int mode)
{
	(void)printf("begin %o %s\n", mode, name);
}

void
uu_close()
{
	char ch = ENC('\0');
	(void)putchar(ch);
	(void)putchar('\n');
	(void)printf("end\n");
}

/*
 * copy from in to out, encoding as you go along.
 */
void
uu_write(const char *in, int len)
{
	register int ch, n;
	register const char *p;
	static char buf[48];

	while (len > 0) {
		if (len >= 45)
		{
			n = 45;
		} else {
			n = len;
		}
		memcpy(buf, in, n);
		len -= n;
		in += n;

		ch = ENC(n);
		putchar(ch);
		for (p = buf; n > 0; n -= 3, p += 3) {
			ch = *p >> 2;
			ch = ENC(ch);
			putchar(ch);
			ch = (*p << 4) & 060 | (p[1] >> 4) & 017;
			ch = ENC(ch);
			putchar(ch);
			ch = (p[1] << 2) & 074 | (p[2] >> 6) & 03;
			ch = ENC(ch);
			putchar(ch);
			ch = p[2] & 077;
			ch = ENC(ch);
			putchar(ch);
		}
		putchar('\n');
	}
}
