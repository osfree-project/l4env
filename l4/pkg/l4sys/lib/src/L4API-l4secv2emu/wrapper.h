#ifndef __WRAPPER_H_
#define __WRAPPER_H_


#include <v2cs.h>



#define LOG_COLOR_RED  "\033[2;31m"
#define LOG_COLOR_GREEN   "\033[2;32m"
#define LOG_COLOR_YELLOW   "\033[2;33m"
#define LOG_COLOR_BLUE   "\033[2;34m"
#define LOG_COLOR_OFF    "\033[0m"

#define SYSCALL_BUFFEROFFSET 5

// some nice constants, i hate ugly long names
const Cap_id TASK_SELF = 0;

const Address OPEN_EP   = V2_cs::Thread_cap_open_ep;
const Address CLOSED_EP = V2_cs::Thread_cap_close_ep;
const Address SYSCALL_EP= V2_cs::Thread_cap_v2emu_ep;


// stuff printing v2 ids inclusive version nr
#define debug_idfmt "(" l4util_idfmt " v: %d)"
#define debug_idstr(v2_uid) l4util_idstr(v2_uid), (v2_uid).id.version_low

const Address V2_IPC_MAGIC = V2_cs::Magic_request_mask | V2_cs::V2_Sigma0_Request;

static inline Address v2emu_ep() { return V2_cs::get_cap
    (OPEN_EP, V2_cs::V2emu_task_num, V2_cs::V2emu_thread_num);
}

static inline Address v2emu_lock_snd_ep() 
{ return V2_cs::v2emu_locker_snd_ep(); }

static inline Address v2emu_lock_rcv_ep() 
{ return V2_cs::v2emu_locker_rcv_ep(); }

static inline Fpage_Xe kmem() { return V2_cs::get_kmem(); }


void panic(const char *msg);
void sync_capspace(Fpage_Xe addr);
void sync_thread(l4_threadid_t dest);



void wrapper_log(l4_threadid_t self, const char *fmt, ...)
__attribute__ ((format (printf, 2, 3)));

void wrapper_log_end(l4_threadid_t self, const char *fmt, ...)
__attribute__ ((format (printf, 2, 3)));

static inline void tid2thread_task(Mword id, Mword *task, Mword *thread)
{
  assert(thread);
  assert(task);

  *task    = (id >>7) & 0x7ff;
  *thread = id & 0x7f;
}

static inline l4_threadid_t tid2id(Mword id, unsigned version = 1)
{
  l4_threadid_t tmp = (l4_threadid_t){{0, 0}};
  tmp.id.task = (id >>7) & 0x7ff;  
  tmp.id.lthread = id & 0x7f;
  tmp.id.version_low = version;
  return tmp;
}

static inline Address tid2cap(Mword id, Mword type)
{
  Address thread, task;

  tid2thread_task(id, &task, &thread);
  //  return V2_cs::get_cap(V2_cs::Thread_cap_thread, task, thread);
  return V2_cs::get_cap(type, task, thread);
}

static inline Address get_own_cap(Mword type)
{

  Mword id;
  if (Syscall_user::ex_regs_get_tid(&id))
    panic("wrapper: Could not get TID");

  return tid2cap(id, type);   
}


#if 0
static inline l4_threadid_t cap2id(Address cap, unsigned type)
{
  Address thread, task;

  tid2thread_task(id, &task, &thread);
  //  return V2_cs::get_cap(V2_cs::Thread_cap_thread, task, thread);
  return V2_cs::get_cap(type, task, thread);
}
#endif

// dont handle grant
// XXX Fixme handle whole address spaces
static inline Fpage_Xe fpagev2_to_fpagexe(l4_fpage_t page)
{
  //  printf("\nfpage addr %#x size: %d write: %s\n", 
  //	 page.fp.page, page.fp.size, page.fp.write ? "yes" : "no");
  assert(!page.fp.grant);
  
  return Fpage_Xe(page.fp.page << L4_PAGESHIFT, page.fp.size, Fpage_Xe::Type_mem, 
		  //		  Capability::Write | Capability::Read | Capability::Execute);
		    		  page.fp.write ? Capability::Write : 0 |
		    		  Capability::Read | Capability::Execute);
  //  		  15);
  
}

static inline Fpage_Xe fpagev2_to_fpagexe(Mword raw_)
{
  return fpagev2_to_fpagexe((l4_fpage_t){raw:raw_});
}




#endif
