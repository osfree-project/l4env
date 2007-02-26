#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/names/libnames.h>
#include <l4/omega0/client.h>
#include <omega0_proto.h>	/* krishna: I assume MANAGEMENT_THREAD is `0' */
#include "config.h"
#include "internal.h"

#ifdef OMEGA0_LIB_MEASURE_CALL 
#include <l4/log/l4log.h>
#include <l4/util/rdtsc.h>
#endif

int omega0_initalized=0;
l4_threadid_t omega0_management_thread = L4_INVALID_ID;

/* returns 0 on success. */
int omega0_init(void){
 if(names_waitfor_name(OMEAG0_SERVER_NAME, &omega0_management_thread,
                       NAMESERVER_WAIT_MS) == 0) return -1;
  if(l4_is_invalid_id(omega0_management_thread)) return -1;
  omega0_initalized=1;
  return 0;
}

/* krishna: handle values
 *          0  ... RPC to management thread of omega0 task
 *          !0 ... local request RPC partner in omega0 task */
int omega0_call(int handle, omege0_request_descriptor type,
                l4_umword_t param, l4_timeout_t timeout){
  int error;
#ifdef OMEGA0_LIB_MEASURE_CALL
  l4_umword_t t;
#endif
  l4_msgdope_t result;
  l4_umword_t dw0, dw1;
  l4_threadid_t server = omega0_management_thread;

/* krishna: talk to `omega0_management_thread' if handle is 0 */
  if(handle)
    server.id.lthread = handle;

  error = l4_ipc_call(server, L4_IPC_SHORT_MSG, type, param,
                           L4_IPC_SHORT_MSG, &dw0, &dw1,
                           timeout, &result);
  if(error) return -error;
#ifdef OMEGA0_LIB_MEASURE_CALL
  t = (unsigned)(l4_rdtsc().ll);
  LOGl("timediff is %u\n", t-dw1);
#endif
  return dw0;
}
  
/* krishna: handle values
 *          0  ... RPC to management thread of omega0 task
 *          !0 ... local request RPC partner in omega0 task */
int omega0_open_call(int handle, omege0_request_descriptor type,
                     l4_umword_t param, l4_timeout_t timeout,
                     l4_threadid_t *alien, l4_umword_t *d0,
                     l4_umword_t *d1){
  int error;
#ifdef OMEGA0_LIB_MEASURE_CALL
  l4_umword_t t;
#endif
  l4_msgdope_t result;
  l4_threadid_t server = omega0_management_thread;

/* krishna: talk to `omega0_management_thread' if handle is 0 */
  if(handle)
    server.id.lthread = handle;

  error = l4_ipc_reply_and_wait(server, L4_IPC_SHORT_MSG, type, param,
                                alien, L4_IPC_SHORT_MSG, d0, d1,
                                timeout, &result);
  if(error) return -error;
#ifdef OMEGA0_LIB_MEASURE_CALL
  t = (unsigned)(l4_rdtsc().ll);
  LOGl("timediff is %u\n", t-dw1);
#endif
  return error;
}
  
/* krishna: handle values
 *          0  ... RPC to management thread of omega0 task
 *          !0 ... local request RPC partner in omega0 task */
int omega0_call_long(int handle, omege0_request_descriptor type,
                     l4_umword_t param, l4_threadid_t thread){
  int error;
#ifdef OMEGA0_LIB_MEASURE_CALL
  l4_umword_t t;
#endif
  l4_msgdope_t result;
  l4_umword_t dw0, dw1;
  l4_threadid_t server = omega0_management_thread;
  struct{
  	l4_fpage_t	rcv_fpage;
  	l4_msgdope_t	size;
  	l4_msgdope_t	snd;
	l4_umword_t     dw0, dw1;
  	l4_threadid_t	threadid;
  	} message= {rcv_fpage:{0}, size:L4_IPC_DOPE(4,0), snd:L4_IPC_DOPE(4,0),
                    dw0:0, dw1:0, threadid:thread};
  
/* krishna: talk to `omega0_management_thread' if handle is 0 */
  if(handle)
    server.id.lthread = handle;

  error = l4_ipc_call(server, &message, type, param,
                           L4_IPC_SHORT_MSG, &dw0, &dw1,
                           L4_IPC_NEVER, &result);
  if(error) return -error;
#ifdef OMEGA0_LIB_MEASURE_CALL
  t = (unsigned)(l4_rdtsc().ll);
  LOGl("timediff is %u\n", t-dw1);
#endif
  return dw0;
}
