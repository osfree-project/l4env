#include <l4/sys/ipc.h>
#include <l4/sys/types.h>
#undef EXPECT_TRUE
#undef EXPECT_FALSE

#include <l4/sys/syscalls.h>
#include <l4/sys/compiler.h>
#include <l4/sys/kdebug.h>

// circular i know, but better than define twice
#include <l4/util/l4_macros.h>

#undef EXPECT_TRUE
#undef EXPECT_FALSE

#include <stdio.h>
#include <stdarg.h>

#include <l4_types.h>
#include <syscall-user.h>
#include <v2cs.h>
#include <l4_types.h>

#include "debug_glue.h"
#include "wrapper.h"


/* some dummy stuff to make the linker happy */
void *__dso_handle __attribute__((weak));

extern "C"
int
__cxa_atexit(void (*func)(void*), void *arg, void *dso_handle) __attribute__((weak));

int
__cxa_atexit(void (*func)(void*), void *arg, void *dso_handle)
{
  return 0;
}




#if 0
static void wrapper_lock()
{

  Mword buff[6];

  IPC_result res = Syscall_user::send_short_ipc(v2emu_lock_snd_ep(), 3, 0,
						V2_cs::Magic_request_mask | V2_cs::Sync_Lock, 0, 0,
						buff);
  if(res.error())
    {
      printf("error lock to v2emu res: %ld\n", res.raw());
      panic("ipc error");
    }
}

static void wrapper_unlock()
{

  Mword buff[6];
  Mword w0, w1, w2;

  IPC_result res =Syscall_user::recv_short_ipc(v2emu_lock_rcv_ep(), 3, 0, 
					       &w0, &w1, &w2, buff);
  
  if(res.error())
    {
      printf("error unlock to v2emu res: %ld\n", res.raw());
      panic("ipc error");
    }
}
#endif

void wrapper_log(l4_threadid_t self, const char *fmt, ...)
{
  va_list ap;


  // Achtung, das funzt nichtmehr, wenn cancel eingebaut wird!
  //  wrapper_lock();
  
  va_start (ap, fmt);

  printf(LOG_COLOR_RED "l4sys (" l4util_idfmt "): ", l4util_idstr(self));
  vprintf(fmt, ap);
  printf(LOG_COLOR_OFF "\n");

  va_end(ap);

  //  wrapper_unlock();  
}

void wrapper_log_end(l4_threadid_t self, const char *fmt, ...)
{
  va_list ap;

  //  wrapper_lock();
  
  va_start (ap, fmt);

  printf(LOG_COLOR_GREEN "l4sys (" l4util_idfmt "): ", l4util_idstr(self));
  vprintf(fmt, ap);
  printf(LOG_COLOR_OFF "\n");

  va_end(ap);

  //  wrapper_unlock();  
}



void panic(const char *msg)
{
  puts(msg);
  asm("int3");
  while(1);
}




void sync_capspace(Fpage_Xe addr)
{
  assert(addr.type() == Fpage_Xe::Type_cap);

  Mword m0, m1, m2;
  Cap_id rcv_ep = get_own_cap(SYSCALL_EP);


#if 0
  IPC_result res = Syscall_user::short_ipc
    (v2emu_ep(), V2_cs::Magic_request_mask | V2_cs::Sync_CapSpace, addr.raw(), 0,
     rcv_ep, &m0, &m1, &m2,
     0 /* badge */, 0 /* snd_map */, &map);
#endif

  Mword buff[5];
  
  IPC_result res = Syscall_user::short_ipc
    (v2emu_ep(), 3 /* snd words */, 0 /* snd map */,
     V2_cs::Magic_request_mask | V2_cs::Sync_CapSpace, addr.raw(), 0,
     rcv_ep, 3 /* rcv words */, 1 /* rcv map */,
     &m0, &m1, &m2, buff, 0 /* badge */, 0 /* timeout */);
    
  //  assert(res.rcv_typed());
  
  if(res.error())
    {
      printf("error call to v2emu res: %ld\n", res.raw());
      panic("ipc error");
    }
}



void sync_thread(l4_threadid_t dest)
{  

  Fpage_Xe addr = Fpage_Xe(V2_cs::get_cap_fpage(dest.id.task, dest.id.lthread)); 
  sync_capspace(addr);
}


void sync_thread_lazy(l4_threadid_t dest)
{  
  // test if mapped
  Address thread_cap = V2_cs::get_cap(V2_cs::Thread_cap_thread, 
				  dest.id.task, dest.id.lthread);
  Mword tid;
  Mword res = Syscall_user::get_thread_ctrl
    (thread_cap, 1, Number::TCR_tid, &tid);

  if(!res)
    {
      Mword thread, task;
      tid2thread_task(tid, &task, &thread);
      assert(task == dest.id.task && thread == dest.id.lthread);
      return;
    }
  
  if(res && res != Error::Exist)
    {
      l4_threadid_t self = l4_myself();
      wrapper_log(self, "sync_thread_lazy: could not sync thread"
		  debug_idfmt, debug_idstr(dest));

      panic("could not sync thread lazily");
    }  

  Fpage_Xe addr = Fpage_Xe(V2_cs::get_cap_fpage(dest.id.task, dest.id.lthread)); 
  sync_capspace(addr);
}


l4_threadid_t
l4_myself(void) 
{
  Mword id;

  if (Syscall_user::ex_regs_get_tid(&id))
    panic("Hello: Could not get TID");

  return tid2id(id);
}

int
l4_nchief(l4_threadid_t destination,
	  l4_threadid_t *next_chief)
{
  
  l4_threadid_t self = l4_myself();

  if(l4_is_nil_id(destination))
    {
      *next_chief = self;
      return L4_NC_SAME_CLAN;
    }
  
  l4_threadid_t chief;

  Mword type;
  Badge recv_badge;

  Cap_id rcv_ep = get_own_cap(SYSCALL_EP);

#if 0
  unsigned map = 0;
  IPC_result res = Syscall_user::short_ipc
    (v2emu_ep(), V2_cs::Magic_request_mask | V2_cs::Id_nearest, 
     destination.lh.low, destination.lh.high,
     rcv_ep, &chief.lh.low, &chief.lh.high, &type,
     &recv_badge /* badge */, 0 /* snd_map */, &map);
#endif

  Mword buff[5];
  
  IPC_result res = Syscall_user::short_ipc
    (v2emu_ep(), 3 /* snd words */, 0 /* snd map */,
     V2_cs::Magic_request_mask | V2_cs::Id_nearest, destination.lh.low, destination.lh.high,
     rcv_ep, 3 /* rcv words */, 0 /* rcv map */,
     &chief.lh.low, &chief.lh.high, &type, buff, &recv_badge /* badge */, 0 /* timeout */);

  if(res.error())
    {
      printf("error call to v2emu res: %ld\n", res.raw());
      panic("ipc error");
    }

  chief.id.version_low = 1;

  *next_chief = chief;  

  wrapper_log(self, "nchief"
	      " destination: " debug_idfmt
	      " next_chief: " debug_idfmt
	      " type: %lx",
	      debug_idstr(destination), debug_idstr(chief), type);

  return (int) type;
}



void
__do_l4_thread_ex_regs(l4_umword_t val0,
                       l4_umword_t eip,
                       l4_umword_t esp, 
                       l4_threadid_t *preempter,
                       l4_threadid_t *pager,
                       l4_umword_t *old_eflags,
                       l4_umword_t *old_eip,
                       l4_umword_t *old_esp)
{

  l4_threadid_t self = l4_myself();
  
  wrapper_log(self, "ex_regs (%lx)"
	      " ip: " l4_addr_fmt
	      " sp: " l4_addr_fmt
	      " pager: " debug_idfmt,
	      val0, eip, esp, debug_idstr(*pager));

  if(!(*pager).id.version_low)
    panic("wrong pager version nr");

  Mword local_cap = (V2_cs::get_local_cap(V2_cs::Thread_cap_thread, val0));
  Mword res;
  Address old_pager_cap;
  bool synched = false;
  
  while(1)
    {
      
      res = Syscall_user::get_thread_ctrl
	(local_cap, 1, Number::TCR_pager_snd, &old_pager_cap);

      if(res == Error::Exist && !synched)
	{

	  //	  printf("\nwrapper::request: ex_regs %lu (%#lx) %#lx %#lx\n", 
	  //		 val0, local_cap, eip, esp);
	  Mword m0, m1, m2;
	  Address rcv_ep = get_own_cap(SYSCALL_EP);

	  Mword buff[5];
	  
	  IPC_result res = Syscall_user::short_ipc
	    (v2emu_ep(), 3 /* snd words */, 0 /* snd map */,
	     V2_cs::Magic_request_mask | V2_cs::Create_thread, val0, 0,
	     rcv_ep, 3 /* rcv words */, 0 /* rcv map */,
	     &m0, &m1, &m2, buff, 0 /* badge */, 0 /* timeout */);

#if 0
	  IPC_result res = Syscall_user::short_ipc
	    (v2emu_ep(),  V2_cs::Magic_request_mask | V2_cs::Create_thread, 
	     val0, 0, rcv_ep, &m0, &m1, &m2);
#endif

	  synched = true;
	  continue;
	}
      else if(res)
	{
	  printf("thread_ctrl res: %ld\n", res);
	  panic("thread_ctrl failed"); 
	}
      break;
    }  
  
  if(!l4_is_invalid_id(*pager))
    {
      sync_thread_lazy(*pager);
      
      Address pager_ep = V2_cs::get_cap(V2_cs::Thread_cap_open_ep, 
					(*pager).id.task, (*pager).id.lthread);

      res = Syscall_user::set_thread_ctrl(local_cap, 1,
					  Number::TCR_pager_snd, pager_ep);      
    }
  
  if(old_pager_cap)
    {
      *pager = tid2id(V2_cs::get_id(old_pager_cap, V2_cs::Thread_cap_open_ep));
    }
  else
    *pager = L4_NIL_ID;

  *preempter = L4_NIL_ID;
  
  Mword old_tid;
  Syscall_user::ex_regs(local_cap, (Address) eip, (Address) esp, 
			(Mword) ~0UL, (Mword) ~0UL,
	 		(Address*) old_eip, (Address*) old_esp, 
			(Mword*) old_eflags, &old_tid,
			false);



  wrapper_log_end(self, "ex_regs end (%lu)"
	      " old_ip: " l4_addr_fmt
	      " old_sp: " l4_addr_fmt
	      " old_pager: " debug_idfmt,
	      val0, *old_eip, *old_esp, debug_idstr(*pager));
}





#if 0
static void dump_sched_param(l4_sched_param_t &sched, const char *prefix = "")
{

  if(l4_is_invalid_sched_param(sched))
    {
      printf("\n%s invalid sched param\n", prefix);
      return ;
    }

  char *units = "us";

  unsigned long timeslice = 1<< 2 * (15 - sched.sp.time_exp);


  if(sched.sp.time_exp && !sched.sp.time_man)
    {
      timeslice = 0;
      units = "none";
    }
  else if(sched.sp.time_exp > 0 && sched.sp.time_exp <= 5)
    {
      timeslice /=1000000;
      units = "sec";
    }
  else if(sched.sp.time_exp <= 10 && sched.sp.time_exp >5)
    {
      timeslice /=1000;
      units = "ms";
    }
  else if(!sched.sp.time_exp)
    {
      units = "nil";
      timeslice = 0;
    }
  
  printf("\n %s sched param: prio: %u, small: %d, state: %d, time_exp: %d, time_man: %d timeslice %ld %s\n",
	 prefix, sched.sp.prio, sched.sp.small, sched.sp.state, sched.sp.time_exp, 
	 sched.sp.time_man, timeslice * sched.sp.time_man, units);
}
#endif

l4_cpu_time_t
l4_thread_schedule(l4_threadid_t dest,
		   l4_sched_param_t param,
		   l4_threadid_t *ext_preempter,
		   l4_threadid_t *partner,
		   l4_sched_param_t *old_param)
{

  l4_threadid_t self = l4_myself();

  wrapper_log(self, "schedule " debug_idfmt ""
	      " param: %#lx",
	      debug_idstr(dest), param.sched_param);

  if(!dest.id.version_low)
    panic("wrong dest version nr");

  *old_param = L4_INVALID_SCHED_PARAM;
  *ext_preempter = L4_NIL_ID;
  *partner = L4_NIL_ID;

  if(l4_is_nil_id(dest) || l4_is_invalid_id(dest))
    return 0;

  Address thread_cap = V2_cs::get_cap(V2_cs::Thread_cap_thread, 
				      dest.id.task, dest.id.lthread);

  Mword old_prio;
  Mword old_mcp;
  Mword res;

  bool sync = false;
  while(1)
    {      
      
      res = Syscall_user::get_thread_ctrl(thread_cap, 2,
					  Number::TCR_prio, &old_prio,
					  Number::TCR_mcp,  &old_mcp);
      if(!res)
	break;
      
      if(res != Error::Exist)
	panic("could not threadctrl the thread\n");

      // sync goes wrong, simply return an invalid sched param
      if(sync)
	  return 0;
      
      // XX check if the target in the same task as we
      sync_thread(dest);

      sync = true;
    }

  l4_sched_param_t old_sched = ((l4_sched_param_t){sp: {0, 0, 0, 0, 0}});
  old_sched.sp.prio = old_prio;

  // ca 10ms time quantum
  old_sched.sp.time_exp = 12;
  old_sched.sp.time_man = 156;

  //  dump_sched_param(old_sched, "old");
  *old_param = old_sched;

  if(l4_is_invalid_sched_param(param))
    return 0;

  //  dump_sched_param(param, "new");
  Mword prio = param.sp.prio;

  res = Syscall_user::set_thread_ctrl(thread_cap, 1,
				      Number::TCR_prio, prio);
  if(res)
    panic("could not threadctrl the thread\n");
  
  return 0;
}

l4_taskid_t
l4_task_new(l4_taskid_t destination,
	    l4_umword_t mcp_or_new_chief,
	    l4_umword_t esp,
	    l4_umword_t eip,
	    l4_threadid_t pager)
{
  l4_threadid_t self = l4_myself();

  wrapper_log(self, "task_new " l4util_idtskfmt
	      " (chief/mcp) " l4_addr_fmt
	      " ip: " l4_addr_fmt
	      " sp: " l4_addr_fmt
	      " pager: " debug_idfmt,
	      l4util_idtskstr(destination), mcp_or_new_chief, 
	      eip, esp, debug_idstr(pager));

  //  XXX use an own EP for this
  Address rcv_ep = get_own_cap(SYSCALL_EP);

  Mword buff[SYSCALL_BUFFEROFFSET + 5];
  buff[SYSCALL_BUFFEROFFSET + 0] = destination.lh.low;
  buff[SYSCALL_BUFFEROFFSET + 1] = destination.lh.high;

  buff[SYSCALL_BUFFEROFFSET + 2] = pager.lh.low;
  buff[SYSCALL_BUFFEROFFSET + 3] = pager.lh.high;

  buff[SYSCALL_BUFFEROFFSET + 4] = (Mword) mcp_or_new_chief;
  
  l4_threadid_t new_task;
  Mword dummy;
  Badge recv_badge;

  IPC_result res = Syscall_user::short_ipc(v2emu_ep(), 3 + 5, 0, 
					   V2_cs::Magic_request_mask | V2_cs::Create_task,
					   esp, eip,
					   rcv_ep, 3, 0,
					   &new_task.lh.low, &new_task.lh.high, &dummy,
					   buff, 
					   &recv_badge, 0);
  if(res.error())
    panic("ipc error at ipc with the v2emu");
  
  new_task.id.version_low = 1;
  
  wrapper_log_end(self, "task_new end: new task:" 
	      debug_idfmt 
	      " v_low: %d v_high: %d n: %d s: %d c: %x"
	      " res %x",
	      debug_idstr(new_task), 
	      new_task.id.version_low, new_task.id.version_high,
	      new_task.id.nest, new_task.id.site, new_task.id.chief,
	      res.error());
  asm("int3");
  
  return new_task;
}


void
l4_thread_switch(l4_threadid_t dest)
{
  l4_threadid_t self = l4_myself();

  wrapper_log(self, "switch to:" debug_idfmt,
	      debug_idstr(dest));

  // we need an unspecific schedule
  if(l4_is_nil_id(dest) || l4_is_invalid_id(dest))
    return;

  if(!dest.id.version_low)
    panic("wrong dest version nr");  

  Address thread_cap = V2_cs::get_cap(V2_cs::Thread_cap_thread, 
				      dest.id.task, dest.id.lthread);    

  Mword res;
  bool sync = false;
  
  while(1)
    {
      res = Syscall_user::debug_schedule(thread_cap);
      
      if(!res)
	break;
      
      if(res != Error::Exist)
	panic("could not threadctrl the thread\n");

      if(sync)
	  return; 

      sync_thread(dest);
      sync = true;
    }
}


void
l4_fpage_unmap(l4_fpage_t fpage,
	       l4_umword_t map_mask)
{

  l4_threadid_t self = l4_myself();


  Fpage_Xe addr(fpage.fp.page << L4_PAGESHIFT, fpage.fp.size, Fpage_Xe::Type_mem, 
		(map_mask & L4_FP_FLUSH_PAGE) ? 0 : Capability::Read | Capability::Execute);

  unsigned unmap_cnt = 0;
  unsigned flush_cnt = 0;
  
  if(map_mask & L4_FP_ALL_SPACES)
    unmap_cnt = 1;
  else
    flush_cnt = 1;
  
  Mword res = Syscall_user::unmap(unmap_cnt, flush_cnt, addr.raw());

  wrapper_log_end(self, "fpage_unmap fpage: %#lx, mask: %lx res: %#lx",
		  fpage.raw, map_mask, res);
}


void
fiasco_register_symbols(l4_taskid_t tid, l4_addr_t addr, l4_size_t size)
{
  printf("warning fiasco_register_symbols is unimplemented\n");
}


void
fiasco_register_lines(l4_taskid_t tid, l4_addr_t addr, l4_size_t size)
{
  printf("warning fiasco_register_lines is unimplemented\n");
}


void
fiasco_register_thread_name(l4_threadid_t tid, const char *name)
{
  printf("warning fiasco_register_thread_name is unimplemented\n");
}


int
l4_privctrl(l4_umword_t cmd,
	    l4_umword_t param)
{
  printf("l4_privctrl is unimplemented\n");
  asm("int3");
  panic("l4_privctrl is unimplemented\n");
  return 0;
}


