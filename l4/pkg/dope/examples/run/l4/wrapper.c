/*
 * \brief   Wrappers and implementation of some libcon functions
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * Run uses some functions from libcon which are not available in
 * libterm. Thus, we provide an implementation of those functions
 * here.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/dope/term.h>

struct contxt_fake_ihb {
	struct history *hist;
};

int contxt_init_ihb(struct contxt_fake_ihb* ihb, int count, int length);
int contxt_init_ihb(struct contxt_fake_ihb* ihb, int count, int length) {
	ihb->hist = term_history_create((void *)0, count*length);
	return 0;
}

void contxt_add_ihb(struct contxt_fake_ihb* ihb, const char *s);
void contxt_add_ihb(struct contxt_fake_ihb* ihb, const char *s) {
	term_history_add(ihb->hist, (char*)s);
}

void contxt_read(char* retstr, int maxlen, struct contxt_fake_ihb* ihb);
void contxt_read(char* retstr, int maxlen, struct contxt_fake_ihb* ihb) {
	if (!ihb) term_readline(retstr, maxlen, (void *)0);
	term_readline(retstr, maxlen, ihb->hist);
}

int contxt_init(long max_sbuf_size, int scrbuf_lines);
int contxt_init(long max_sbuf_size, int scrbuf_lines) {
	term_init("Run!");
	return 0;
}

int contxt_clrscr(void);
int contxt_clrscr(void) {
	return 0;
}

