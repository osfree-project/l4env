/**
 * \file   server/src/logger.c
 * \brief  Dmon logserver
 *
 * \date   10/01/2004
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 */
#include <l4/env/errno.h>
#include <l4/sys/types.h>
#include <l4/util/macros.h>
#include <l4/util/sll.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>
#include <l4/lock/lock.h>
#include <l4/dope/dopelib.h>
#include <l4/dope/vscreen.h>
#include <l4/dope/keycodes.h>

/* XXX vt100 needs this ugly stuff */
int _DEBUG = 0;
struct winsize;
#include <l4/term_server/vt100.h>

/* standard */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* local */
#include "local.h"


/*** OPTIONS ***/

#define TERM_WIDTH  82
#define TERM_HEIGHT 30
#define TERM_HIST   (TERM_HEIGHT * 50)

static int opt_jmp_to_end_on_activity = 1;

/**********************/
/***     SCREEN     ***/
/**********************/

/* user specific type (private ptr in termstate_t */
typedef struct termstate_spec_s
{
	long app_id;      /* DOpE application ID */
	char *phys;       /* VTextScreen buffer */
} termstate_spec_s;

/*** TOOL: X,Y-POSITION TO INDEX IN C8A8PLN ***/
static inline void local_xy2index_C8A8PLN(termstate_t *term,
                                          unsigned x, unsigned y,
                                          unsigned *c_index, unsigned *a_index)
{
//	if (x*y > width*height)
//		return;

	*c_index = (y * term->w) + x;
	*a_index = (term->w * term->phys_h) + (y * term->w) + x;
}

/*** TOOL: ADAPT SCROLLBAR TO CURRENT VIEWPORT ***
 *
 * This function must be called when the viewport changed due
 * to key commands such as page up/down, and after writing
 * new log output that may extend the size of the viewport.
 */
static inline void update_scrollbar(termstate_t *term) {
	dope_cmdf(term->spec->app_id, "logger_sb.set(-realsize %d -offset %d)",
	          term->hist_len + term->phys_h, term->hist_len - term->vis_off);
}

/*** VT100: REDRAW ONE CHARACTER ***/
void vt100_redraw_xy(termstate_t *term, int x, int y)
{
	int idx;
	unsigned c_idx, a_idx;

	l4semaphore_down(&term->termsem);

	idx = xy2index(term, x, y - term->vis_off);
	local_xy2index_C8A8PLN(term, x, y, &c_idx, &a_idx);

	term->spec->phys[c_idx] = term->text[idx];
	term->spec->phys[a_idx] = term->attrib[idx];

	l4semaphore_up(&term->termsem);

	dope_cmdf(term->spec->app_id, "logger_vts.refresh(-x %d -y %d -w 1 -h 1)",
	          x, y);
}

/*** VT100: REDRAW WHOLE SCREEN ***/
void vt100_redraw(termstate_t *term)
{
	int y;

	l4semaphore_down(&term->termsem);

	for (y = 0; y < term->phys_h; y++) {
		/* line-by-line memcpy from VT100 buffer into DOpE text buffer */
		/* XXX ...and if buffer wraps mid-line ? */
		int start = xy2index(term, 0, y - term->vis_off);
		int end   = xy2index(term, term->w - 1, y - term->vis_off);
		unsigned c_idx, a_idx;

		local_xy2index_C8A8PLN(term, 0, y, &c_idx, &a_idx);

		memcpy(term->spec->phys + c_idx, term->text + start, end - start);
		memcpy(term->spec->phys + a_idx, term->attrib + start, end - start);
	}

	l4semaphore_up(&term->termsem);

	dope_cmdf(term->spec->app_id, "logger_vts.refresh(-x 0 -y 0 -w %d -h %d)",
	          term->w, term->phys_h);
}

void vt100_show_cursor(termstate_t *term) {}
void vt100_hide_cursor(termstate_t *term) {}

/*** SCREEN: SCROLLBAR CALLBACK ***/
static void sb_callback(dope_event *ev, void *priv)
{
	termstate_t *term = (termstate_t *)priv;

	unsigned newpos, oldpos;
	char buf[16];

	if (ev->type != EVENT_TYPE_COMMAND) {
		LOG_Error("Got unexpected event type %ld from DOpE", ev->type);
		return;
	}

	dope_req(term->spec->app_id, buf, sizeof(buf),
	         "logger_sb.offset");

	newpos = atoi(buf);
	oldpos = term->hist_len - term->vis_off;

	if (oldpos < newpos)
		vis_down(term, newpos - oldpos);
	else
		vis_up(term, oldpos - newpos);
}

/*** SCREEN: PRESS EVENT CALLBACK ***
 *
 * - Page Up + Page Down
 */
static void press_callback(dope_event *ev, void *priv)
{
	termstate_t *term = (termstate_t *)priv;
	press_event *e = (press_event *)ev;

	if (ev->type != EVENT_TYPE_PRESS) {
		LOG_Error("Got unexpected event type %ld from DOpE", ev->type);
		return;
	}

	switch (e->code) {
	case DOPE_KEY_PAGEUP:
		vis_up(term, TERM_HEIGHT / 2);
		break;

	case DOPE_KEY_PAGEDOWN:
		vis_down(term, TERM_HEIGHT / 2);
		break;
	}

	update_scrollbar(term);
}

/*** SCREEN: KEYREPEAT EVENT CALLBACK ***
 *
 * - Page Up + Page Down
 */
static void keyrepeat_callback(dope_event *ev, void *priv)
{
	termstate_t *term = (termstate_t *)priv;
	keyrepeat_event *e = (keyrepeat_event *)ev;

	if (ev->type != EVENT_TYPE_KEYREPEAT) {
		LOG_Error("Got unexpected event type %ld from DOpE", ev->type);
		return;
	}

	switch (e->code) {
	case DOPE_KEY_PAGEUP:
		vis_up(term, TERM_HEIGHT / 2);
		break;
	case DOPE_KEY_PAGEDOWN:
		vis_down(term, TERM_HEIGHT / 2);
		break;
	}

	update_scrollbar(term);
}

/*** SCREEN: INIT ****/
static int screen_init(long app_id, termstate_t **term)
{
	int err;
	static termstate_t termstate;
	static termstate_spec_s termstate_spec;

	dope_cmdf(app_id, "logger_vts.setmode(%d, %d, C8A8PLN)", TERM_WIDTH, TERM_HEIGHT);

	/* VT100 stuff */
	memset(&termstate, 0, sizeof(termstate));
	termstate.termsem = L4SEMAPHORE_UNLOCKED;

	l4semaphore_down(&termstate.termsem);
	err = init_termstate(&termstate, TERM_WIDTH, TERM_HEIGHT, TERM_HIST);
	l4semaphore_up(&termstate.termsem);
	if (err) {
		LOG_Error("init_termstate() returned %d", err);
		return -1;
	}

	termstate_spec.app_id = app_id;
	termstate_spec.phys = vscr_get_fb(app_id, "logger_vts");

	termstate.spec = &termstate_spec;
	termstate.cursor_vis = 0;
	termstate.insert_mode = VT100_INSMODE_REPLACE;

	dope_bind(app_id, "logger_vts", "press", press_callback, &termstate);
	dope_bind(app_id, "logger_vts", "keyrepeat", keyrepeat_callback, &termstate);
	dope_bind(app_id, "logger_sb",  "change", sb_callback, &termstate);

	dope_cmdf(app_id, "logger_sb.set(-realsize %d -viewsize %d)",
	          TERM_HEIGHT, TERM_HEIGHT);

	*term = &termstate;
	return 0;
}

/*** LOGGER MESSAGE QUEUE ***/
static slist_t *logmsg;
static l4semaphore_t logmsg_sema = L4SEMAPHORE_INIT(0);
static l4lock_t logmsg_mutex = L4LOCK_UNLOCKED;

/*************************/
/***     LOGSERVER     ***/
/*************************/

#define LOG_BUFFERSIZE 81
#define LOG_NAMESERVER_NAME "stdlogV05"
#define LOG_COMMAND_LOG 0

/*** LOGSERVER: GET NEXT IPC ***/
static int get_msg(char *msgbuf, unsigned msgbuf_size)
{
	int err;

	static l4_threadid_t client = L4_INVALID_ID;
	static struct {
		l4_fpage_t   fpage;
		l4_msgdope_t size;
		l4_msgdope_t send;
		l4_umword_t  dw0, dw1/*, dw2, dw3, dw4, dw5, dw6*/;
		l4_strdope_t string;
		l4_msgdope_t result;
	} msg;

	msg.size.md.strings = 1;
	msg.size.md.dwords = 2;
	msg.string.rcv_size = msgbuf_size;
	msg.string.rcv_str = (l4_umword_t) msgbuf;
	msg.fpage.fpage = 0;

	memset(msgbuf, 0, LOG_BUFFERSIZE);
	while (1) {
		if (l4_thread_equal(client, L4_INVALID_ID)) {
			err = l4_ipc_wait(&client, &msg, &msg.dw0, &msg.dw1,
			                  L4_IPC_NEVER,
			                  &msg.result);
		} else {
			err = l4_ipc_reply_and_wait(client, NULL, msg.dw0, 0,
			                            &client, &msg, &msg.dw0, &msg.dw1,
			                            L4_IPC_SEND_TIMEOUT_0,
			                            &msg.result);
			/* don't care for non-listening clients */
			if (err == L4_IPC_SETIMEOUT) {
				client = L4_INVALID_ID;
				continue;
			}
		}

		if (err == 0 || err == L4_IPC_REMSGCUT) {
			if (msg.result.md.fpage_received == 0) {
				switch (msg.dw0) {
				case LOG_COMMAND_LOG:
					if (msg.result.md.strings != 1) {
						msg.dw0 = -L4_EINVAL;
						continue;
					}
					/* GOTCHA */
					return 0;
				}
			}
			msg.dw0 = -L4_EINVAL;
			continue;
		}

		/* IPC was canceled - try again */
		if (err == L4_IPC_RECANCELED)
			continue;

		/* hm, serious error ? */
		snprintf(msgbuf, msgbuf_size,
		         "dmon     | Error %#x getting log message from " l4util_idfmt"\n",
		         err, l4util_idstr(client));
		client = L4_INVALID_ID;
		return err;
	}
}

/*** LOGSERVER: SERVER LOOP ***/
static void logsrv_loop(void *p)
{
	/* setup logserver */
	if (!names_register(LOG_NAMESERVER_NAME))
		Panic("Could not register %s at names", LOG_NAMESERVER_NAME);

	l4thread_started(NULL);

	while (1) {
		slist_t *p;
		char *msgbuf;

		/* alloc new buffer */
		msgbuf = malloc(LOG_BUFFERSIZE + 5);
		if (!msgbuf) {
			enter_kdebug("OUCH!");
		}

		if (get_msg(msgbuf, LOG_BUFFERSIZE))
			strcpy(msgbuf + LOG_BUFFERSIZE, "...\n");

		/* 1: write to kernel debugger */
		outstring(msgbuf);

		/* 2: hand over message to logger */
		p = list_new_entry(msgbuf);
		if (!p) {
			outstring("OUCH!\n");
			free(msgbuf);
			continue;
		}
		l4lock_lock(&logmsg_mutex);
		logmsg = list_append(logmsg, p);
		l4lock_unlock(&logmsg_mutex);
		l4semaphore_up(&logmsg_sema);
	}

}

/*** LOGSERVER: INIT ****/
static int logsrv_init(void)
{
	int err;

	err = l4thread_create_named(logsrv_loop, ".logsrv",
	                            NULL, L4THREAD_CREATE_SYNC);
	if (err < 0)
		return err;

	return 0;
}

/*****************************/
/***     LOGGER THREAD     ***/
/*****************************/

/*** LOGGER: LOGGER THREAD LOOP ***/
static void logger_loop(void *p)
{
	termstate_t *term = (termstate_t *)p;
	char *msg;

	l4thread_started(NULL);

	while (1) {
		/* XXX DEBUG */
		if (!l4semaphore_try_down(&logmsg_sema)) {
			if (logmsg)
				enter_kdebug("logmsg ???");
			l4semaphore_down(&logmsg_sema);
		}
		if (!logmsg)
			enter_kdebug("!logmsg ???");

		/* get next log message */
		l4lock_lock(&logmsg_mutex);
		msg = logmsg->data;
		logmsg = list_remove(logmsg, logmsg);
		l4lock_unlock(&logmsg_mutex);

		/* XXX go down to current phys position */
		if (opt_jmp_to_end_on_activity)
			vis_down(term, 9999999);
		vt100_write(term, msg, strlen(msg));
		update_scrollbar(term);

		/* free message storage */
		free(msg);
	}
}

/*** LOGGER: INIT ****/
static int logger_init(termstate_t *term)
{
	int err;

	err = l4thread_create_named(logger_loop, ".logger",
	                            term, L4THREAD_CREATE_SYNC);
	if (err < 0)
		return err;

	return 0;
}

/*************************************************/
/***     LOGGER (LOGSERVER) INITIALIZATION     ***/
/*************************************************/

int dmon_logsrv_init()
{
	if (logsrv_init() < 0) {
		LOG_Error("LOGGER SRV init failed");
		return -1;
	}

	return 0;
}

int dmon_logger_init(long app_id)
{
	termstate_t *term;

	if (screen_init(app_id, &term) < 0) {
		LOG_Error("LOGGER SCREEN init failed");
		return -1;
	}

	if (logger_init(term) < 0) {
		LOG_Error("LOGGER THREAD init failed");
		return -1;
	}

	return 0;
}
