#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "dopelib.h"  /* public interface of dopelib */
#include "term.h"     /* public interface of this library */
#include "listener.h" /* from lib/dope/include */
#include "sync.h"     /* from lib/dope/include */

extern void dopelib_waitforexit(void);

static struct dopelib_mutex *printf_mutex;

static int app_id;

int term_init(char *app_name) {
	static int term_initialised;
	
	printf(" ");  /* !!! don't ask */
	
	/* do not initialise the library a second time */
	if (term_initialised++) return 0;
	
	dope_init();
	
	printf_mutex = dopelib_create_mutex(0);
	
	if (!app_name) app_name = "Terminal";
	app_id = dope_init_app(app_name);
	
	/* create and open DOpE terminal window */
	dope_cmd(app_id,"w = new Window()");
	dope_cmd(app_id,"t = new Terminal()");
	dope_cmd(app_id,"w.set(-scrollx yes -w 400 -h 200 -scrolly yes -background no -content t)");
	dope_cmd(app_id,"w.open()");
	dope_cmd(app_id,"t.bind(\"press\",\"\")");
	return app_id;
}


void term_deinit() {
	dope_cmd(app_id,"w.close()");
	dope_deinit_app(app_id);
	dopelib_destroy_mutex(printf_mutex);
	dope_deinit();
}


int term_printf(const char *format, ...) {
	static char strbuf[2048];
	static char subbuf[2048];
	int i,j,num,off=0;
	va_list list;
	
	dopelib_lock_mutex(printf_mutex);
	
	va_start(list, format);
	num = vsnprintf(strbuf, 2048, format, list);
	va_end(args);
	
	/* send pieces of 128 characters to the DOpE terminal widget */
	while (off<num) {
		/* substitute '"' characters */
		for (i=0, j=0; i<128; i++, j++) {
			if (strbuf[i+off] == '"') subbuf[j++] = '\\';
			subbuf[j] = strbuf[i+off];
			if (strbuf[i+off] == 0) break;
		}
		subbuf[j] = 0;

		/* send dope cmd */
		dope_cmdf(app_id, "t.print(\"%s\")", subbuf);
		off += 128;
	}
	
	dopelib_unlock_mutex(printf_mutex);
	
	return 0;
}


static long wait_for_key(void) {
	char *bindarg;
	dope_event *e;
	do {
		dopelib_wait_event(app_id, &e, &bindarg);
	} while (e->type != EVENT_TYPE_PRESS);
	return e->press.code;
}


int term_getchar(void) {
	int ascii;
	while (1) {
		ascii = dope_get_ascii(app_id, wait_for_key());
		if (ascii) return ascii;
	}
}


static void clear_chars(int num_chars) {
	int i;
	for (i=0; i<num_chars; i++) term_printf("%c", 8);
}


static int browse_history(char *dst, int max_len, struct history_struct *hist) {
	int keycode;
	char hidx = 0;
	char *curr_str = NULL;
	int   curr_len = 0;
	int ascii;
	do {
		clear_chars(curr_len);
		curr_str = term_history_get(hist, hidx);
		if (curr_str) {
			curr_len = strlen(curr_str);
			term_printf(curr_str);
		} else {
			curr_len = 0;
		}
		
		keycode = wait_for_key();
		ascii = dope_get_ascii(app_id, keycode);

		/* key up */
		if (keycode == 103) {
			if (term_history_get(hist,hidx+1)) hidx++;
		}

		/* key down */
		if (keycode == 108) {
			hidx--;
		}
	} while ((ascii==0) && (hidx>=0));
	
	if (hidx>=0) {
		strncpy(dst, curr_str, max_len);
	} else {
		clear_chars(curr_len);
		term_printf("%s",dst);
	}
	
	return ascii;
}


int term_readline(char *dst, int max_len, struct history_struct *hist) {
	int i = 0;
	int keycode, ascii;
	while (i < max_len - 1) {
		keycode = wait_for_key();
		ascii = dope_get_ascii(app_id, keycode);
		
		/* arrow up starts history browsing */
		if ((keycode == 103) && hist) {
			dst[i] = 0;
			clear_chars(i);
			ascii = browse_history(dst, max_len, hist);
			i = strlen(dst);
		}
		
		if (ascii == 10) break;
		if (ascii == 8) {
			i--;
		}
		
		if (i < 0) {
			ascii = 0;
			i = 0;
		}
		if (i > max_len-2) ascii = 0;

		if (ascii != 0) {
			term_printf("%c",ascii);
			dst[i] = ascii;
			if (ascii != 8) i++;
		}

	}
	dst[i]=0;
	if (hist) term_history_add(hist,dst);
	term_printf("\n");
	return 0;
}

