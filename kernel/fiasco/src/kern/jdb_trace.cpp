INTERFACE:

#include "l4_types.h"

class Sys_ipc_frame;

class Jdb_trace
{
private:
  // PF
  typedef struct 
    {
      int other_thread;
      GThread_num thread;
      int pfa_res_set;
      Address y, z;
    } Pf_res;

  // IPC
  typedef struct 
    {
      int other_thread;
      GThread_num thread;
      int other_task;
      Task_num task;
      int send_only;
    } Ipc_res;

  // unmap
  typedef struct
    {
      int other_thread;
      GThread_num thread;
      int addr_set;
      Address y, z;
    } Unmap_res;
  
  static Pf_res pf_res;
  static Ipc_res ipc_res;
  static Unmap_res unmap_res;
};

IMPLEMENTATION:

#include "jdb_trace.h"
#include "entry_frame.h"

Jdb_trace::Pf_res Jdb_trace::pf_res =
  { 0, (GThread_num)-1, 0, 0, 0 };
Jdb_trace::Ipc_res Jdb_trace::ipc_res =
  { 0, (GThread_num)-1, 0, (Task_num)-1, 0 };
Jdb_trace::Unmap_res Jdb_trace::unmap_res =
  { 0, (GThread_num)-1, 0, 0, 0 };


PUBLIC static
void
Jdb_trace::get_pf_res(int *other_thread, GThread_num *thread, 
		      int *pfa_set, Address *y, Address *z)
{
  *other_thread = pf_res.other_thread;
  *thread       = pf_res.thread;
  *pfa_set      = pf_res.pfa_res_set;
  *y            = pf_res.y;
  *z            = pf_res.z;
}

PUBLIC static 
void 
Jdb_trace::set_pf_res(int other_thread, GThread_num thread, 
		      int pfa_set, Address y, Address z)
{
  pf_res.other_thread = other_thread;
  pf_res.thread       = thread;
  pf_res.pfa_res_set  = pfa_set;
  pf_res.y            = y;
  pf_res.z            = z;
}

PUBLIC static inline
int
Jdb_trace::check_pf_res(L4_uid id, Address pfa)
{
  return (   (((pf_res.thread == (GThread_num)-1)
	      || ((pf_res.other_thread) ^ (pf_res.thread == id.gthread()))))
	  && (!pf_res.pfa_res_set
	      || ((pf_res.y > pf_res.z)
	          ^ ((pfa >= pf_res.y) && (pfa <= pf_res.z))))
         );
}

PUBLIC static 
void 
Jdb_trace::clear_pf_res()
{
  pf_res.other_thread = 0;
  pf_res.thread       = (GThread_num)-1;
  pf_res.pfa_res_set  = 0;
  pf_res.y            = 0;
  pf_res.z            = 0;
}

PUBLIC static
void
Jdb_trace::get_unmap_res(int *other_thread, GThread_num *thread, 
			 int *addr_set, Address *y, Address *z)
{
  *other_thread = unmap_res.other_thread;
  *thread       = unmap_res.thread;
  *addr_set     = unmap_res.addr_set;
  *y            = unmap_res.y;
  *z            = unmap_res.z;
}

PUBLIC static 
void 
Jdb_trace::set_unmap_res(int other_thread, GThread_num thread,
			 int addr_set, Address y, Address z)
{
  unmap_res.other_thread = other_thread;
  unmap_res.thread = thread;
  unmap_res.addr_set = addr_set;
  unmap_res.y = y;
  unmap_res.z = z;
}

PUBLIC static inline
int
Jdb_trace::check_unmap_res(L4_uid id, Address addr)
{
  return (   (((unmap_res.thread == (GThread_num)-1)
	      || ((unmap_res.other_thread) ^ (unmap_res.thread == id.gthread()))))
	  && (!unmap_res.addr_set
	      || ((unmap_res.y > unmap_res.z)
		  ^ ((addr >= unmap_res.y) && (addr <= unmap_res.z))))
         );
}

PUBLIC static 
void 
Jdb_trace::clear_unmap_res()
{
  unmap_res.other_thread = 0;
  unmap_res.thread       = (GThread_num)-1;
  unmap_res.addr_set     = 0;
  unmap_res.y            = 0;
  unmap_res.z            = 0;
}

PUBLIC static
void 
Jdb_trace::get_ipc_res(int *other_thread, GThread_num *thread, 
		       int *other_task, Task_num *task, int *send_only)
{
  *other_thread = ipc_res.other_thread;
  *thread       = ipc_res.thread;
  *other_task   = ipc_res.other_task;
  *task         = ipc_res.task;
  *send_only    = ipc_res.send_only;
}

PUBLIC static 
void 
Jdb_trace::set_ipc_res(int other_thread, GThread_num thread,
		       int other_task, Task_num task, int send_only)
{
  ipc_res.other_thread = other_thread;
  ipc_res.thread       = thread;
  ipc_res.other_task   = other_task;
  ipc_res.task         = task;
  ipc_res.send_only    = send_only;
}

PUBLIC static inline NEEDS ["entry_frame.h"]
int
Jdb_trace::check_ipc_res(L4_uid id, Sys_ipc_frame *ipc_regs)
{
  return (   ((ipc_res.thread == (GThread_num)-1)
	      || ((ipc_res.other_thread) ^ (ipc_res.thread == id.gthread())))
          && ((!ipc_res.send_only || ipc_regs->snd_desc().has_send()))
	  && ((ipc_res.task == (Task_num)-1)
	      || ((ipc_res.other_task)   
		  ^ ((ipc_res.task == id.task())
		     && (ipc_res.task == ipc_regs->snd_dest().task()))))
	  );
}

PUBLIC static 
void 
Jdb_trace::clear_ipc_res()
{
  ipc_res.other_thread = 0;
  ipc_res.thread       = (GThread_num)-1;
  ipc_res.other_task   = 0;
  ipc_res.task         = (Task_num)-1;
  ipc_res.send_only    = 0;
}

