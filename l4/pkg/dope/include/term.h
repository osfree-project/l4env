
extern int  term_init    (char *terminal_name);
extern void term_deinit  (void);
extern int  term_printf  (const char *format, ...);
extern int  term_getchar (void);

struct history_struct;

extern struct history_struct *term_history_create(void *buf, long buf_size);
extern int   term_history_add(struct history_struct *hist, char *new_str);
extern char *term_history_get(struct history_struct *hist, int index);

extern int term_readline(char *dst, int max_len, struct history_struct *hist);

