/*
* \brief   FLIPS server idl wrapper to L4Linux
* \date    2004-05-25
* \author  Jens Syckor <js712688@inf.tu-dresden.de>
*/

#include "local.h"
/*** L4-SPECIFIC INCLUDES ***/
#include <l4/env/errno.h>
#include <l4/env/env.h>
#include <l4/util/util.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include <l4/names/libnames.h>

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

/*** LOCAL INCLUDES ***/
#include <flips-server.h>

#define L4LX_FLIPS_DEBUG

void * flips_session_thread(l4_threadid_t * my_tid_ptr);

char LOG_tag[9] = "l4xflips";

static void signal_handler(int code){
	names_unregister("FLIPS");
	fprintf(stdout,"unregistered FLIPS server, signal was %d\n",code);
	exit(-1);
}

static int check_mode(int mode);
static int check_mode(int mode)
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

void dice_ipc_error(l4_msgdope_t result,  CORBA_Server_Environment *ev)
{
	fprintf(stderr,"DICE %s\n", l4env_strerror(L4_IPC_ERROR(result)));
}

void *CORBA_alloc(unsigned long size)
{
	void *new;
	new = malloc(size);
	if (!new)
	fprintf(stderr,"Error: malloc returned NULL\n");
	return new;
}

void CORBA_free(void *ptr) {
	free(ptr);
}

void * flips_session_thread(l4_threadid_t * my_tid_ptr) {
	CORBA_Server_Environment env = dice_default_server_environment;
	l4_threadid_t my_tid = l4thread_l4_id(l4thread_myself());
	*my_tid_ptr = my_tid;

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

        err = close(s);
        return err;
}

l4_int32_t
l4vfs_common_io_ioctl_component(CORBA_Object _dice_corba_obj,
                               object_handle_t fd,
                               l4_int32_t cmd,
                               l4_int8_t **arg,
                               l4vfs_size_t *count,
                               CORBA_Server_Environment *_dice_corba_env)
{
	int err;
 
	err = ioctl(fd,cmd,(void *) *arg);

	return err;
}

/** IDL INTERFACE: READ FROM SOCKET
 *
 * The len parameter is a int-pointer to circumvent a misbehaviour
 * of DICE when specifying an [in]-parameter as size for an [out]-string.
 */
l4_int32_t l4vfs_common_io_read_component(CORBA_Object _dice_corba_obj,
                                          object_handle_t s,
                                          l4_int8_t **buf,
                                          l4vfs_size_t *len,
                                          l4_int16_t *_dice_reply,
                                          CORBA_Server_Environment * _dice_corba_env)
{
	int err;
	
	err = read(s,(void *) buf, *len);
	/* reply read or 0 bytes */
	if (err > 0)
		*len = err;
	else
	        *len = 0;

	return err;
}

/** IDL INTERFACE: WRITE
 */
l4_int32_t l4vfs_common_io_write_component(CORBA_Object _dice_corba_obj,
                                           object_handle_t s,
                                           const l4_int8_t *buf,
                                           l4vfs_size_t *count,
                                           l4_int16_t *_dice_reply,
                                           CORBA_Server_Environment * _dice_corba_env)
{
	int err;
	err = write(s, buf, *count);

	return err;
}

/** IDL INTERFACE: PROC READ
 */
l4_int32_t flips_proc_read_component(CORBA_Object _dice_corba_obj,
                                     const char *path,
                                     l4_int8_t dst[4096],
                                     l4_int32_t offset,
                                     l4_int32_t *len,
                                     CORBA_Server_Environment * _dice_corba_env)
{
	fprintf(stdout,"flips_proc_read: not required...!\n");
	return -1;
}

void
l4vfs_select_notify_request_component(CORBA_Object _dice_corba_obj,
                                      object_handle_t fd,
                                      l4_int32_t mode,
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

	internal_notify_request(fd, mode, _dice_corba_obj, (l4_threadid_t *) notif_tid);
}

void
l4vfs_select_notify_clear_component(CORBA_Object _dice_corba_obj,
                                    object_handle_t fd,
                                    l4_int32_t mode,
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

	internal_notify_clear(fd, mode, _dice_corba_obj, (l4_threadid_t *) notif_tid);
}



/** IDL INTERFACE: PROC WRITE
 */
l4_int32_t flips_proc_write_component(CORBA_Object _dice_corba_obj,
                                      const char *path,
                                      const l4_int8_t *src,
                                      l4_int32_t len,
                                      CORBA_Server_Environment * _dice_corba_env)
{
	fprintf(stdout,"flips_proc_write: not required...!\n");
	return -1;
}

l4_uint32_t flips_inet_addr_type_component(CORBA_Object _dice_corba_obj,
                                           l4_uint32_t addr,
                                           CORBA_Server_Environment * _dice_corba_env)
{

	fprintf(stdout,"flips_inet_addr_type: not implemented, because needs kernel access!\n");
	return -1;
}

int main (int argc, char **argv)
{
	struct stat buf;
	CORBA_Server_Environment env = dice_default_server_environment;

	if (stat("/proc/l4", &buf)) {
		fprintf(stderr, "This binary requires L4Linux!\n");
		exit(-1);
	}

	l4thread_init();

	tt_init();

	fprintf(stdout,"Starting FLIPS server\n");

	if (!names_register("FLIPS")) {
        	fprintf(stderr,"Error: Failed to register at nameserver");
	        exit(-2);
        }

	signal(SIGTERM,signal_handler);
	signal(SIGINT,signal_handler);

	flips_server_loop(&env);

	exit(0);
}
