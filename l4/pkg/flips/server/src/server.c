/*** L4-SPECIFIC INCLUDES ***/
#include <l4/sys/syscalls.h>
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>
#include <l4/log/l4log.h>
#include <l4/env/errno.h>

/*** LOCAL INCLUDES ***/
#include "local.h"
#include "flips-server.h"

#define FLIPS_DEBUG         0
#define FLIPS_DEBUG_VERBOSE 0

/* !!! here some security should be added .-) */
#define client_owns_sid(c,s) 1

/* check non block mode of notify message */
int check_mode(int mode);
int check_mode(int mode)
{
	if (! (mode & (SELECT_READ | SELECT_WRITE | SELECT_EXCEPTION)))
	{
		LOG_Error("unknown operation mode %d",mode);
		return -EINVAL;
	}
	else
	{
		return 0;
	}
}

void dice_ipc_error(l4_msgdope_t result, CORBA_Server_Environment *ev)
{
	LOG_Error("DICE %s\n", l4env_strerror(L4_IPC_ERROR(result)));
}

void flips_session_thread(void *arg)
{
	l4_threadid_t *my_tid_ptr = (l4_threadid_t *) arg;
	CORBA_Server_Environment env = dice_default_server_environment;

	/* keep all options open */
	env.malloc = CORBA_alloc;
	env.free = CORBA_free;

	l4_threadid_t my_tid = l4_myself();
	*my_tid_ptr = my_tid;
	LOGd(FLIPS_DEBUG, "Flips(session_thread): new thread_id %x.%x\n",
		my_tid.id.task, my_tid.id.lthread);
	l4dde_process_add_worker();
	l4thread_started(NULL);
	flips_server_loop(&env);
}

/** IDL INTERFACE: CLOSE SOCKET
 */
l4_int32_t l4vfs_common_io_close_component(CORBA_Object _dice_corba_obj,
                                           object_handle_t s, 
                                           CORBA_Server_Environment * _dice_corba_env)
{
	int err;
	if (!client_owns_sid(CORBA_Object, s))
		return -1;
	err = socket_close(s);
	return err;
}

/** IDL INTERFACE: READ FROM SOCKET
 *
 * The len parameter is a int-pointer to circumvent a misbehaviour
 * of DICE when specifying an [in]-parameter as size for an [out]-string.
 */
l4vfs_ssize_t
l4vfs_common_io_read_component (CORBA_Object _dice_corba_obj,
                                object_handle_t s,
                                char **buf,
                                l4vfs_size_t *len,
                                short *_dice_reply,
                                CORBA_Server_Environment *_dice_corba_env)
{
	int err;
	if (!client_owns_sid(CORBA_Object, s))
		return -1;
	LOGd(FLIPS_DEBUG_VERBOSE, "+read");
	err = socket_read(s, *buf, *len);
	/* reply read or 0 bytes */
	if (err > 0)
		*len= err;
	else
		*len = 0;
	LOGd(FLIPS_DEBUG_VERBOSE, "++read");
	return err;
}

/** IDL INTERFACE: WRITE TO SOCKET
 */
l4vfs_ssize_t
l4vfs_common_io_write_component (CORBA_Object _dice_corba_obj,
                                 object_handle_t s,
                                 const char *buf,
                                 l4vfs_size_t *count,
                                 short *_dice_reply,
                                 CORBA_Server_Environment *_dice_corba_env)
{
	int err;
	if (!client_owns_sid(CORBA_Object, s))
		return -1;
	LOGd(FLIPS_DEBUG_VERBOSE,"fd: %d, len: %d, buf: %s", s , *count, buf);
	err = socket_write(s, (void *)buf, *count);
	LOGd(FLIPS_DEBUG_VERBOSE,"++write, ret: %d", err);
	return err;
}

/** IDL INTERFACE: IOCTL
 */
l4_int32_t
l4vfs_common_io_ioctl_component (CORBA_Object _dice_corba_obj,
                                 object_handle_t fd,
                                 int cmd,
                                 char **arg,
                                 l4vfs_size_t *count,
                                 CORBA_Server_Environment *_dice_corba_env)
{

	int ret;

#if FLIPS_DEBUG_VERBOSE
	struct ifreq *ifr;

	ifr = (struct ifreq *) *arg;
	LOG("received: ");
	LOG("ioctl(%d, %d) called", fd, cmd);
	LOG(" arglen(arg)=%d", *count);
	LOG("ifreq_name: %s", ifr->ifr_ifrn.ifrn_name);
#endif

	ret = socket_ioctl(fd, cmd, (void *)*arg);

#if FLIPS_DEBUG_VERBOSE
	LOG("after ioctl: ");
	LOG("ioctl finished with ret=%d - back in component", ret);
#endif

	return ret;
}

void
l4vfs_select_notify_request_component (CORBA_Object _dice_corba_obj,
                                       object_handle_t fd,
                                       int mode,
                                       const l4_threadid_t *notif_tid,
                                       CORBA_Server_Environment *_dice_corba_env)
{
	int err;

	err = check_mode(mode);
	if (err)
	{
		LOG("Unknown mode (%d)", mode);
		return;
	}

	liblinux_notify_request(fd, mode, (l4_threadid_t *) notif_tid);
}

void
l4vfs_select_notify_clear_component (CORBA_Object _dice_corba_obj,
                                     object_handle_t fd,
                                     int mode,
                                     const l4_threadid_t *notif_tid,
                                     CORBA_Server_Environment *_dice_corba_env)
{
	int err;

	err = check_mode(mode);
	if (err)
	{
		LOG("Unknown mode (%d)", mode);
		return;
	}

	liblinux_notify_clear(fd, mode, (l4_threadid_t *) notif_tid);
}


/** IDL INTERFACE: READ FROM PROC ENTRY
 */
int
flips_proc_read_component (CORBA_Object _dice_corba_obj,
                           const char* path,
                           char dst[4096],
                           int offset,
                           int *len,
                           CORBA_Server_Environment *_dice_corba_env)
{
	int res;

	LOGd(FLIPS_DEBUG_VERBOSE,"proc_read(%s,%x,%d,%d) called", path, (int)dst, offset, *len);

	res = liblinux_proc_read(path, dst, offset, *len);

	LOGd(FLIPS_DEBUG_VERBOSE,"proc_read finished - back in component");

	return res;
}

/** IDL INTERFACE: WRITE TO PROC ENTRY
 */
int
flips_proc_write_component (CORBA_Object _dice_corba_obj,
                            const char* path,
                            const char *src,
                            int len,
                            CORBA_Server_Environment *_dice_corba_env)
{
	int err;

	err = liblinux_proc_write(path, (char *)src, len);
	return err;
}

/** DICE: Memory used (CORBA_alloc()) -- only pointer count */
static unsigned int CORBA_mem_pointers;

/** DICE: Dynamic buffer deallocation
 */
void CORBA_free(void *ptr)
{
	LOGd(FLIPS_DEBUG,"block at %p, %d pointers remain",
	     ptr, --CORBA_mem_pointers);
	free(ptr);
}

/** DICE: Dynamic buffer allocation
 */
void *CORBA_alloc(unsigned long size)
{
	void *new;
	LOGd(FLIPS_DEBUG,"size=%ld, %d pointers used",
	     size, ++CORBA_mem_pointers);
	new = malloc(size);
	LOGd(FLIPS_DEBUG,"got block at %p", new);
	if (!new)
		LOG_Error("malloc returned NULL");
	return new;
}

