#include <l4/dope/term.h>

struct contxt_fake_ihb {
	struct history_struct *hist;
};

int contxt_init_ihb(struct contxt_fake_ihb* ihb, int count, int length);
int contxt_init_ihb(struct contxt_fake_ihb* ihb, int count, int length) {
	ihb->hist = term_history_create((void *)0, count*length);
	return 0;
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

