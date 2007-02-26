/**
 * \file   flips/examples/l4lx_flips/server.c
 * \brief  FLIPS L4Linux proxy
 *
 * \date   02/03/2006
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/*
 * This was completely reworked for L4Linux 2.6. I also removed the dependency
 * to libflips_server.a because of thread issues under 2.6 -- we run on Linux
 * threads completely and rely on L4Linux's hybrid task support. -- Christian.
 */

#define _GNU_SOURCE

#include "local.h"

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/env/errno.h>
#include <l4/env/env.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/l4vfs/types.h>

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>

/*** IDL INCLUDES ***/
#include <flips-server.h>

char LOG_tag[9] = "lxflips";

/********************
 ** Debug switches **
 ********************/

static int debug_write  = 0;
static int debug_read   = 0;
static int debug_malloc = 0;


/***************
 ** MISC UTIL **
 ***************/

static void signal_handler(int code)
{
	names_unregister("FLIPS");
	fprintf(stderr, "[%05d] ", getpid());
	psignal(code, "signal_handler");
	D("unregistered FLIPS server");
	exit(1);
}

void dice_ipc_error(l4_msgdope_t result,  CORBA_Server_Environment *ev)
{
	D("%s %s (%ld) ... sleeping 10 seconds",
	 __func__, l4env_strerror(L4_IPC_ERROR(result)), L4_IPC_ERROR(result));
	sleep(10);
}

/***********************
 ** DICE MEMORY STUFF **
 ***********************/

struct mlist {
	slist_t        e;
	void          *m;
	unsigned long  s;
	pthread_t      t;
};

static slist_t         *mlist;
static pthread_mutex_t  mlist_lock = PTHREAD_MUTEX_INITIALIZER;

static void * flips_malloc(unsigned long size)
{
	void *p = malloc(size);
	if (p) {
		mlock(p, size);
		memset(p, 0, size);

		pthread_mutex_lock(&mlist_lock);
		struct mlist *ml = malloc(sizeof(*ml));
		ml->m = p; ml->s = size;
		ml->t = pthread_self();
		mlist = list_add(mlist, &ml->e);
		pthread_mutex_unlock(&mlist_lock);

		if (debug_malloc)
			printf("allocated @ %p %5ld bytes (elem @ %p)\n", p, size, ml);
	}
	return p;
}

static void flips_free(void *p)
{
	unsigned long size = 0;
	pthread_mutex_lock(&mlist_lock);
	slist_t *e;
	struct mlist *ml = NULL;

	for (e = mlist; e; e = e->next) {
		ml = (struct mlist *)e;
		size = ml->s;
		if (ml->m == p) break;
	}
	/* ml implicitly freed */
	mlist = list_remove(mlist, e);
	pthread_mutex_unlock(&mlist_lock);

	free(p);
	if (debug_malloc)
		printf("    freed @ %p %5ld bytes (elem @ %p)\n", p, size, ml);
}

static void flips_free_all(pthread_t t)
{
	slist_t *e = mlist;
	struct mlist *ml = NULL;

	pthread_mutex_lock(&mlist_lock);
	while (e) {
		ml = (struct mlist *)e;

		unsigned long size = ml->s;
		void *p = ml->m;

		if (pthread_equal(ml->t, t)) {
			/* ml implicitly freed */
			mlist = list_remove(mlist, e);

			free(p);
			if (debug_malloc)
				printf("    freed @ %p %5ld bytes (elem @ %p)\n", p, size, ml);

			/* start over */
			e = mlist;
		} else
			e = e->next;
	}
	if (debug_malloc) {
		printf("mlist: ");
		for (e = mlist; e; e = e->next) printf("-%p", ((struct mlist *)e)->m);
		printf("\n");
	}
	pthread_mutex_unlock(&mlist_lock);
}

/***********************/
/*** THREAD HANDLING ***/
/***********************/

static pthread_t        flips_main_thread;
static slist_t         *slist;
static pthread_mutex_t  slist_lock = PTHREAD_MUTEX_INITIALIZER;

/*** SET FLIPS MAIN THREAD ***/
static void i_am_main(pthread_t t)
{
	flips_main_thread = t;
}

/*** CHECK FOR FLIPS MAIN THREAD ***/
static inline int am_i_main(void)
{
	return (flips_main_thread == pthread_self());
}

/*** FAKE SIGNAL HANDLER **/
static void fake_sig_handler(int sig)
{
	D("%s: %s", __func__, strsignal(sig));
}

static void select_thread_restart(struct session_thread_info *info)
{
	/* signal restart to select thread */
	pthread_kill(info->select_thread, SIGHUP);
}

/*** SELECT THREAD STARTUP CODE ***/
static void select_thread(struct session_thread_info *info)
{
	/* SIGHUP restarts select thread */
	signal(SIGHUP, fake_sig_handler);

	info->select_l4thread = l4_myself();

	sem_post(&info->started);

	/* execute external select implementation */
	notify_select_thread(&info->select_fds);
}

/*** SESSION THREAD STARTUP CODE ***/
static void session_thread(struct session_thread_info *info)
{
	CORBA_Server_Environment env = dice_default_server_environment;

	env.malloc = flips_malloc;
	env.free = flips_free;
	env.user_data = (void *)info;

	info->session_l4thread = l4_myself();

	sem_post(&info->started);

	/* enter connection server loop */
	flips_server_loop(&env);
}

/*** CREATE SESSION THREAD ***/
static l4_threadid_t session_create(l4_threadid_t client)
{
	struct session_thread_info *info;

	/* alloc and init info object */
	info = malloc(sizeof(*info));
	if (!info) {
		perror("malloc");
		exit(1);
	}
	memset(info, 0, sizeof(*info));
	sem_init(&info->started, 0, 0);
	notify_init_fd_set(&info->select_fds);

	info->partner = client;

	/* create select thread first */
	pthread_create(&info->select_thread, 0,
	               (void*(*)(void*))select_thread,
	               (void *)info);
	sem_wait(&info->started);

	/* create session thread */
	pthread_create(&info->session_thread, 0,
	               (void*(*)(void*))session_thread,
	               (void *)info);
	sem_wait(&info->started);

	/* insert session into session list */
	slist_t *e = list_new_entry(info);
	info->elem = e;
	pthread_mutex_lock(&slist_lock);
	slist = list_add(slist, e);
	pthread_mutex_unlock(&slist_lock);

	return info->session_l4thread;
}

/*** EXIT SESSION AND SELECT THREAD ***/
static void session_exit(struct session_thread_info *info)
{
	/* cancel select thread */
	pthread_cancel(info->select_thread);

	/* remove session from list */
	pthread_mutex_lock(&slist_lock);
	slist = list_remove(slist, info->elem);
	pthread_mutex_unlock(&slist_lock);

	/* exit session thread */
	int code = 0;
	pthread_exit(&code);
}

/*** SHUTDOWN SESSION AND SELECT THREAD REMOTELY ***/
static void session_shutdown(struct session_thread_info *info)
{
	l4_msgdope_t r;

	/* cancel session thread */
	pthread_cancel(info->session_thread);

	/* cancel select thread */
	pthread_cancel(info->select_thread);

	/* make sure threads are canceled by waking them up */
	l4_ipc_send(info->session_l4thread, L4_IPC_SHORT_MSG, 0, 0, L4_IPC_SEND_TIMEOUT_0, &r);
	l4_ipc_send(info->select_l4thread, L4_IPC_SHORT_MSG, 0, 0, L4_IPC_SEND_TIMEOUT_0, &r);

	/* remove session from list */
	pthread_mutex_lock(&slist_lock);
	slist = list_remove(slist, info->elem);
	pthread_mutex_unlock(&slist_lock);

	/* free memory of session */
	flips_free_all(info->session_thread);

	free(info);
}

/****************************/
/*** SELECT IDL INTERFACE ***/
/****************************/

/* check select node as we support READ, WRITE, and EXCEPTION only */
static int check_mode(int mode)
{
	if (! (mode & (SELECT_READ | SELECT_WRITE | SELECT_EXCEPTION))) {
		D("unknown select operation mode %d", mode);
		return -EINVAL;
	}

	return 0;
}

/*** SELECT IDL: CLEAR NOTIFICATION ***/
void
l4vfs_select_notify_clear_component(CORBA_Object _dice_corba_obj,
                                    object_handle_t fd,
                                    int mode,
                                    const l4_threadid_t *notif_tid,
                                    CORBA_Server_Environment *_dice_corba_env)
{
	if (check_mode(mode)) {
		D("Unknown mode (%d)", mode);
		return;
	}

	struct session_thread_info *info;
	info = (struct session_thread_info*) _dice_corba_env->user_data;

	notify_clear(&info->select_fds, fd, mode, *notif_tid);
	select_thread_restart(info);
}

/*** SELECT IDL: REQUEST NOTIFICATION ***/
void
l4vfs_select_notify_request_component(CORBA_Object _dice_corba_obj,
                                      object_handle_t fd,
                                      int mode,
                                      const l4_threadid_t *notif_tid,
                                      CORBA_Server_Environment *_dice_corba_env)
{
	if (check_mode(mode)) {
		D("Unknown mode (%d)", mode);
		return;
	}

	struct session_thread_info *info;
	info = (struct session_thread_info*) _dice_corba_env->user_data;

	notify_request(&info->select_fds, fd, mode, *notif_tid);
	select_thread_restart(info);
}

/*******************************/
/*** COMMON IO IDL INTERFACE ***/
/*******************************/

/*** COMMON IO IDL: CLOSE ***/
l4_int32_t
l4vfs_common_io_close_component(CORBA_Object _dice_corba_obj,
                                object_handle_t s,
                                CORBA_Server_Environment * _dice_corba_env)
{
	int err;

	err = close(s);
	return err;
}

/*** COMMON IO IDL: IOCTL ***/
l4_int32_t
l4vfs_common_io_ioctl_component(CORBA_Object _dice_corba_obj,
                                object_handle_t fd,
                                int cmd,
                                char **arg,
                                l4vfs_size_t *count,
                                CORBA_Server_Environment *_dice_corba_env)
{
	int err = 0;

	/* XXX I don't think we should do this here */
	D("ioctl(%d, %d, %p) -- not executing", fd, cmd, *arg);
//	err = ioctl(fd, cmd, (void *) *arg);

	return err;
}

/*** COMMON IO IDL: READ ***/
l4vfs_ssize_t
l4vfs_common_io_read_component(CORBA_Object _dice_corba_obj,
                               object_handle_t s,
                               char **buf,
                               l4vfs_size_t *len,
                               short *_dice_reply,
                               CORBA_Server_Environment *_dice_corba_env)
{
	int err;

	if (debug_read)
		printf("READ from socket %d: max %d bytes\n", s, *len);

	err = read(s, *buf, *len);

	if (debug_read) {
		if (err > 0) {
			int i;
			printf("READ %d bytes: <", err);
			for (i = 0; i < err; i++) printf("%c", (*buf)[i]);
			printf(">\n");
		} else
			printf("error %d on read\n", errno);
	}

	/* reply read or 0 bytes */
	if (err > 0)
		*len = err;
	else
		*len = 0;

	return err;
}

/*** COMMON IO IDL: WRITE ***/
l4vfs_ssize_t
l4vfs_common_io_write_component(CORBA_Object _dice_corba_obj,
                                object_handle_t s,
                                const char *buf,
                                l4vfs_size_t *count,
                                short *_dice_reply,
                                CORBA_Server_Environment *_dice_corba_env)
{
	if (debug_write) {
		int i;
		printf("WRITE to socket %d: <", s);
		for (i = 0; i < *count; i++) printf("%c", buf[i]);
		printf(">\n");
	}

	int err;
	err = write(s, buf, *count);

	if (debug_write)
		printf("WROTE: %d bytes\n", *count);

	return err;
}

/********************************/
/*** CONNECTION IDL INTERFACE ***/
/********************************/

/*** CONNECTION IDL: START SESSION ***/
l4_threadid_t
l4vfs_connection_init_connection_component(CORBA_Object _dice_corba_obj,
                                           CORBA_Server_Environment * _dice_corba_env)
{
	if (!am_i_main()) {
		D("connection init request to session thread, skipping");
		return L4_INVALID_ID;
	}

	/* create session and select-worker threads */
	return session_create(*_dice_corba_obj);
}

/*** CONNECTION IDL: CLOSE SESSION ***/
void l4vfs_connection_close_connection_component(CORBA_Object _dice_corba_obj,
                                                 const l4_threadid_t *server,
                                                 CORBA_Server_Environment * _dice_corba_env)
{
	if (am_i_main()) {
		/* we're main thread - shut down specified connection */

		l4_threadid_t tid = *server;

		/* lookup session thread */
		slist_t *p;
		pthread_mutex_lock(&slist_lock);
		for (p = slist; p; p = p->next) {
			struct session_thread_info *info = (struct session_thread_info *)p->data;
			if (!l4_thread_equal(info->session_l4thread, tid)) continue;

			/* shutdown session remotely and return */
			pthread_mutex_unlock(&slist_lock);
			session_shutdown(info);
			return;
		}
		pthread_mutex_unlock(&slist_lock);

	} else {
		/* we're session thread - shut down connection and exit */

		/* commit at client */
		l4_msgdope_t r;
		l4_ipc_send(*_dice_corba_obj, L4_IPC_SHORT_MSG, 0, 0, L4_IPC_SEND_TIMEOUT_0, &r);

		/* and cleanup session */
		session_exit((struct session_thread_info *)_dice_corba_env->user_data);
	}
}

/***************************/
/*** FLIPS IDL INTERFACE ***/
/***************************/

/*** FLIPS IDL: PROC READ (DUMMY) ***/
int
flips_proc_read_component(CORBA_Object _dice_corba_obj,
                          const char* path,
                          char dst[4096],
                          int offset,
                          int *len,
                          CORBA_Server_Environment *_dice_corba_env)
{
	D("%s not implemented", __func__);
	return -L4_ENOTSUPP;
}

/*** FLIPS IDL: PROC WRITE (DUMMY) ***/
int
flips_proc_write_component(CORBA_Object _dice_corba_obj,
                           const char* path,
                           const char *src,
                           int len,
                           CORBA_Server_Environment *_dice_corba_env)
{
	D("%s not implemented", __func__);
	return -L4_ENOTSUPP;
}

/***************************/
/*** L4LINUX FLIPS PROXY ***/
/***************************/

int main(int argc, char **argv)
{
	struct stat buf;
	CORBA_Server_Environment env = dice_default_server_environment;

	env.malloc = flips_malloc;
	env.free = flips_free;

	if (stat("/proc/l4", &buf)) {
		fprintf(stderr, "This binary requires L4Linux!\n");
		exit(1);
	}

	printf("Starting \"%s\" server\n", LOG_tag);

	if (!names_register("FLIPS")) {
		fprintf(stderr, "Error: Failed to register at nameserver");
		exit(2);
	}

	/* XXX check for other relevant signals */
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	/* remember me */
	i_am_main(pthread_self());

	/* init events support */
	init_events(&slist, &slist_lock);

	/* the main thread handles connection setup only */
	l4vfs_connection_server_loop(&env);

	exit(0);
}
