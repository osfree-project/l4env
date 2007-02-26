/*
 * \brief   Interface of DOpE terminal library
 * \date    2003-03-06
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

extern int  term_init    (char *terminal_name);
extern void term_deinit  (void);
extern int  term_printf  (const char *format, ...);
extern int  term_getchar (void);

struct history;

extern struct history *term_history_create(void *buf, long buf_size);
extern int   term_history_add(struct history *hist, char *new_str);
extern char *term_history_get(struct history *hist, int index);

extern int term_readline(char *dst, int max_len, struct history *hist);

