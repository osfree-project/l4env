
void err(const char *, ...);
void fork_exec(char *);
void sig_handler(int);
int handle_xerror(Display *, XErrorEvent *);
int ignore_xerror(Display *, XErrorEvent *);
int send_xmessage(Window, Atom, long);
void get_mouse_position(int *, int *);
