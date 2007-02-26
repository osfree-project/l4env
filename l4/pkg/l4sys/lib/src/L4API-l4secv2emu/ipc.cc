#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/types.h>
#undef EXPECT_TRUE
#undef EXPECT_FALSE

#include <l4/sys/syscalls.h>
#include <l4/sys/compiler.h>

// circular i know, but better than define twice
#include <l4/util/l4_macros.h>

#undef EXPECT_TRUE
#undef EXPECT_FALSE

#include <l4_types.h>
#include <syscall-user.h>
#include <v2cs.h>
#include <l4_types.h>

#include <stdio.h>
#include <string.h>


#include "debug_glue.h"
#include "wrapper.h"





struct long_ipc_message_header {
  l4_fpage_t     fp;
  l4_msgdope_t   size_dope;
  l4_msgdope_t   snd_dope;
  l4_umword_t    words[1];
};

static inline void back_pt(Address start, Address end, Mword task = TASK_SELF)
{
  printf("back_pt from %#lx to %#lx task: %lx\n", start, end, task);
  start = start >> L4_SUPERPAGESHIFT;
  end   = end >> L4_SUPERPAGESHIFT;
  
  for(;start <= end; start++)
    {

#if 0
      printf("Syscall_user::map(%lx,%lx,%lx,%lx,%x,%x)", TASK_SELF,
	  kmem().raw(), task, 
	  Fpage_Xe(start << L4_SUPERPAGESHIFT,
		   L4_SUPERPAGESHIFT,0,15).raw(), 1, 0);
#endif

      bool synched = false;
      
      while(1)
	{
	  
	  Mword res;
	  res = Syscall_user::map(TASK_SELF, kmem(), 
				  task,
				  Fpage_Xe(start << L4_SUPERPAGESHIFT,
					   L4_SUPERPAGESHIFT,0,15),
				  1);
	  if(!res)
	    break;


	  assert(res != Error::Type && res != Error::Type_rcv);
	  
	  if((res == Error::Exist) 
	     && task != TASK_SELF && !synched)
	    { 
	      sync_capspace(Fpage_Xe(task, 12, Fpage_Xe::Type_cap, 15));
	      synched = true;
	      
	      continue;
	    }
	  else
	    {
	      printf("Pager: pagetable backing for %lx failed with %ld", 
		     start<< L4_SUPERPAGESHIFT, res);
	      panic("failed with pagetable backing");
	      assert(0);
	    }
	}
    }
}

static inline void back_pt(Fpage_Xe addr)
{
  assert(addr.type() == Fpage_Xe::Type_mem);
  back_pt(addr.page(), addr.page() + (1 << addr.size()));
}

static inline 
int setup_rcv_window(Cap_id current_thread, l4_fpage_t fp)
{
  // check for valid fpage
  if(!fp.fp.page && fp.fp.size != 32)
    return 0;

  if(!fp.fp.page && fp.fp.size < 12 && fp.fp.size > 31)
    return 0;

  Fpage_Xe rcv_window = fpagev2_to_fpagexe(fp);

  // if whole space, we let the sender handle this
  if(!rcv_window.is_whole())
    {
      printf("backing rcv window from %#lx to %#lx\n", 
	     rcv_window.page(), rcv_window.page() + (1 << rcv_window.size()));
      //	  asm("int3");
      back_pt(rcv_window);
    }

  // set receive window
  Syscall_user::set_thread_ctrl(current_thread, 1,
				Number::TCR_mem_rcv_window, rcv_window.raw());
  return 1;
}

static Address prepare_receive(l4_threadid_t *src, Address type,
			       void *rcv_msg, l4_timeout_t timeout,
			       unsigned &rcv_to, unsigned &rcv_count, unsigned &rcv_map)
{

  Mword id;

  if (Syscall_user::ex_regs_get_tid(&id))
    panic("wrapper: Could not get TID");

  Cap_id thread_cap = tid2cap(id, V2_cs::Thread_cap_thread);
  Cap_id rcv_cap; // = tid2cap(id, type);

  Mword rcv_desc = ((Mword) rcv_msg) & ~L4_IPC_OPEN_IPC;
  if(type == OPEN_EP)
    {
      *src = L4_INVALID_ID;
      rcv_cap = tid2cap(id, OPEN_EP);
    }
  else
    {
      rcv_cap = l4_is_nil_id(*src) ? 
	tid2cap(id, SYSCALL_EP) : tid2cap(id, CLOSED_EP);
    }

  if(rcv_desc == 0)
    {
      rcv_map = 0;
      rcv_count = 3;
    }
  else if(rcv_desc & (Mword) L4_IPC_SHORT_FPAGE)
    {
      rcv_map = 1;
      rcv_count = 5;      

      setup_rcv_window(thread_cap, (l4_fpage_t){raw: (l4_umword_t)rcv_msg});      
    }
  else
    {

      long_ipc_message_header *msg = (long_ipc_message_header *)
	((Mword)rcv_msg & (~(Mword)L4_IPC_SHORT_FPAGE));
     
      // XXX handle rcv fpage

      // we handle for now only very primtive long ipc
      //      assert(!msg->size_dope.md.strings
      //	     && msg->size_dope.md.dwords < 64);

      // optimization, we could rcv inplace (2 words are reserved at the beginning
      // of the direct string
      
      printf("setup rcv window %p\n", msg);
      
      if(setup_rcv_window(thread_cap, msg->fp))
	rcv_map = 1;
      else
	rcv_map = 0;

      if(msg->size_dope.md.dwords > 2)
	rcv_count = msg->size_dope.md.dwords + 1;
      else
	rcv_count = 3;

      if(rcv_count > 60)
	rcv_count = 60;
      
      printf("Long IPC size_dope: words %d, strings: %d, count: %d map: %d\n",
	     msg->size_dope.md.dwords, msg->size_dope.md.strings, rcv_count, rcv_map);
      asm("int3");
      
    }
      
  //XXX combine timeout and rcv-window to one syscall, use self
  unsigned to = 0;
  if(timeout.to.rcv_exp)
    {
      Syscall_user::set_thread_ctrl(thread_cap, 1,
				    Number::TCR_timeout0, 
				    ((15-timeout.to.rcv_exp+2 + 4) <<26) 
				    + (timeout.to.rcv_man<<16));
      to = 2;
    }

  rcv_to = to;
  return rcv_cap;

}




static inline unsigned convert_xe_error_code(Error::Errors res)
{
  // XXX use an translation table
  // XXX Xe needs much better and finer grained errorcodes

  assert(res != Error::Type && res != Error::Type_rcv);

  switch (res) {
  case Error::No:
    return 0;
  case Error::Exist:
    return L4_IPC_ENOT_EXISTENT;
  case Error::Cancel:
    return L4_IPC_SECANCELED;
  case Error::Timeout:
    return L4_IPC_SETIMEOUT;

  case Error::Exist_rcv:
    return L4_IPC_ENOT_EXISTENT;
  case Error::Cancel_rcv:
    return L4_IPC_RECANCELED;
  case Error::Timeout_rcv:
    return L4_IPC_RETIMEOUT;

  default:
    printf("untranslatable ipc error %d\n", res);
    panic("untranslatable ipc error");
  }
  return 0;
}

static inline l4_msgdope_t ipc_v2_msg_dope(unsigned _rcv_fpage,
					   unsigned _error_code,
					   unsigned _strings,
					   unsigned _dwords)
{
  return ( (l4_msgdope_t) {raw:     
	     ( (_dwords << 13)
	       | ((_strings << 5) & 0x1f)
	       | (_error_code & L4_IPC_ERROR_MASK)
	       | (_rcv_fpage ? 2 : 0))
	       });
}

static inline l4_msgdope_t convert_ipc_result(IPC_result res)
{
  return ipc_v2_msg_dope(res.rcv_typed(),
			 convert_xe_error_code(res.error()),
			 0,
			 res.rcv_cnt() - 1);
}




l4_msgdope_t convert_rcv_message(l4_threadid_t current, l4_threadid_t &src, IPC_result res,
				 Mword rcv_m0, Mword rcv_m1, Mword rcv_m2, const Mword rcv_buff[],
				 Mword &v2_word0, Mword &v2_word1, void *rcv_msg)
{
  src.id.version_low = 1;
  
  if(!res.rcv_typed()) // no send items
    {
      if(rcv_m0 == V2_IPC_MAGIC)
	{
	  v2_word0 = rcv_m1;
	  v2_word1 = rcv_m2;

	  // XXX handle msg cut

	  if(res.rcv_cnt() > 3)
	    {
	      
	      long_ipc_message_header *msg = (long_ipc_message_header *)
		((Mword)rcv_msg & (~(Mword)L4_IPC_SHORT_FPAGE));     
	      
	      // handle overflow!!
	      memcpy(&msg->words + 2, rcv_buff + SYSCALL_BUFFEROFFSET,
		     (res.rcv_cnt() - 3) * sizeof(*msg->words));

	      printf("long IPC rcv detected: %d words\n", res.rcv_cnt());

	      for(unsigned i = 2; i < res.rcv_cnt() - 1; i++)
		printf("received n: %d val: %#lx\n", i, msg->words[i]);
	      
	      asm("int3");
	      
	    }
	  
	}
      else // kernel PF IPC
	{
	  v2_word0 = (rcv_m1 & ~3) | (rcv_m0 & 2 /* PF_ERR_WRITE */);
	  v2_word1 = rcv_m2;

	  src.id.version_low = 2;
	  
	}
    }
  else
    {  // map IPC
      v2_word0 = rcv_m2;
      v2_word1 = rcv_buff[2];
    }
  return convert_ipc_result(res);  
}


static void convert_send_fpage(l4_threadid_t dest,
			       l4_umword_t snd_dword0, l4_umword_t snd_dword1,
			       Mword &m0, Mword &m1,
			       Mword &copy_m2, Mword &copy_m3)
{
  l4_fpage_t addr = (l4_fpage_t) {raw: snd_dword1};

  Fpage_Xe snd_page = fpagev2_to_fpagexe(addr);
      

  // only necessary for delivering PF-faults
  // looking at the receiver TCR-rcv window is racy

  // always back the supposed rcv window, we dont want the receiver to back
  // for example the whole address space
  back_pt(snd_dword0, snd_dword0 + (1<< addr.fp.size), 
	  V2_cs::get_task_cap(dest.id.task));

  m0 = (snd_dword0 & L4_PAGEMASK) | Typed_item::Type_map; // send base
  m1 = snd_page.raw();

  // because xe translates them, we need to send the org values too
  copy_m2 = snd_dword0;
  copy_m3 = snd_dword1;
      
}


static void convert_send_dope(l4_threadid_t dest,
			      const void *snd_desc,
			      l4_umword_t snd_dword0, l4_umword_t snd_dword1,
			      Mword &m0, Mword &m1, Mword &m2, Mword buff[],
			      unsigned &count, unsigned &map)
{
  // no deceit
  assert(! (((Address) snd_desc) & 1));
  
  // only register value only
  if(!snd_desc)
    {
      // so we can send with the v2 bindings to v2emu
      // the shift is to be compatible with PF-Fault msg
      m0 = V2_IPC_MAGIC;
      m1 = snd_dword0;
      m2 = snd_dword1;
      count = 3;
      map   = 0;
    }
  // register based fpage
  else if(snd_desc == L4_IPC_SHORT_FPAGE)
    {

      convert_send_fpage(dest, snd_dword0, snd_dword1,
			 m0, m1, 
			 m2, buff[SYSCALL_BUFFEROFFSET + 0]);

      count = 4;
      map   = 1;
    } // long ipc
  else
    {
      long_ipc_message_header *msg = (long_ipc_message_header *)
	((Mword)snd_desc & (~(Mword)L4_IPC_SHORT_FPAGE));

      bool send_as_fpage = ((Address) snd_desc & ((Mword)L4_IPC_SHORT_FPAGE));      

      // we handle for now only very primtive long ipc
      assert(!send_as_fpage
	     && !msg->snd_dope.md.strings
	     && msg->snd_dope.md.dwords < 64);
      
      m0 = V2_IPC_MAGIC;
      m1 = snd_dword0;
      m2 = snd_dword1;   
      
      count = 3;
      map   = 0;

      if(msg->snd_dope.md.dwords > 2)
	{
	  memcpy(buff + SYSCALL_BUFFEROFFSET,
		 &msg->words[2], 
		 (msg->snd_dope.md.dwords - 2) * sizeof(*msg->words));
	  count = msg->snd_dope.md.dwords + 1; // +1 for the magic word
	}
	
      printf("convert snd_dope: words %d, strings: %d count: %d\n",
	     msg->snd_dope.md.dwords, msg->snd_dope.md.strings, count);

      for(unsigned i=3; i< count; i++)
	printf("send word %d:  %#lx\n", i, buff[SYSCALL_BUFFEROFFSET + i-3]);
      

      asm("int3");
      
    }
}


static Address prepare_send(l4_threadid_t current, l4_threadid_t dest, 
			    unsigned type, const void *snd_msg, 
			    l4_umword_t snd_dword0, l4_umword_t snd_dword1,
			    Mword &m0, Mword &m1, Mword &m2, Mword buff[],
			    unsigned &count, unsigned &map)
{

  if(!dest.id.version_low)
    panic("wrong version nr");
  
  //  bool pf_ipc = test_and_reset_pf_reply(current, dest);  
  bool pf_ipc = (dest.id.version_low == 2);

  if(pf_ipc)
    {
      if(type != CLOSED_EP)
	{
	  printf("error (%x.%x) is using the wrong binding to reply an PF\n"
		 "requ l4_ipc_reply or l4_reply_and_wait\n",
		 current.id.task, current.id.lthread);
	  asm("int3");
	}
      //      printf("PF (%x.%x)reply detected\n",current.id.task, current.id.lthread);
      //      asm("int3");
      
      type = SYSCALL_EP;
    }
	  
  Address snd_cap = V2_cs::get_cap(type, dest.id.task, dest.id.lthread);

  Mword snd_desc = (Mword) snd_msg;

  // make sure we have an send
  assert(snd_desc != ~0UL);
  
  if(snd_desc & 1)
    panic("deceiting IPC");

  //from this point, bit 1 is cleared

  convert_send_dope(dest, snd_msg, 
		    snd_dword0, snd_dword1,
		    m0, m1, m2, buff,
		    count, map);
 

  wrapper_log(current, "snd_prepare: " debug_idfmt "  snd_cap: %#lx"
	      "w0: %#lx w1: %#lx c: %d map: %d m0: %#lx m1: %#lx m2: %#lx",
	      debug_idstr(dest), snd_cap,
	      snd_dword0, snd_dword1, count, map, m0, m1, m2);

  return snd_cap;
}




static int
ipc_send(l4_threadid_t dest, Address snd_type, const char *txt,
            const void *snd_msg,
            l4_umword_t snd_dword0,
            l4_umword_t snd_dword1,
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  Address snd_m0, snd_m1, snd_m2;
  unsigned map, count;

  Mword buff[68];
 
  l4_threadid_t self = l4_myself();
 
  Address snd_cap = prepare_send(self, dest, snd_type, snd_msg, snd_dword0, snd_dword1,
				 snd_m0, snd_m1, snd_m2, buff, count, map);


  wrapper_log(self, "%s: "debug_idfmt " snc_cap: %lx w0: %#lx w1: %#lx",
	      txt, debug_idstr(dest), snd_cap, snd_dword0, snd_dword1);

  bool synched = false;
  IPC_result res;

  while(1)
    {
      
      res = Syscall_user::send_short_ipc(snd_cap, count, map,
					 snd_m0, snd_m1, snd_m2, buff);
      assert(res.error() != Error::Type);

      if((res.error() == Error::Exist) && !synched)
	{
	  sync_thread(dest);
	  synched = true;
	  continue;
	}
      // success or error, maybe also Error::Exist, which means, the task has been deleted
      break;
    }

  *result = convert_ipc_result(res);  

  wrapper_log_end(self, "%s end: res: %#lx v2dope: %#lx",
		  txt, res.raw(), (*result).raw);  

  return L4_IPC_ERROR(*result);
}



int
ipc_receive(l4_threadid_t *src, Address rcv_type, const char *txt,
               void *rcv_msg,
               l4_umword_t *rcv_dword0,
               l4_umword_t *rcv_dword1,
               l4_timeout_t timeout,
               l4_msgdope_t *result)
{

  unsigned to;
  unsigned map;
  unsigned count;

  l4_threadid_t self = l4_myself();

  Address rcv_cap = prepare_receive(src, rcv_type, rcv_msg, timeout, to, count, map);



  if(rcv_type == CLOSED_EP)
    wrapper_log(self, "%s from: " debug_idfmt
		" rcv_cap: %#lx, rcv_msg: %p, t: %lx",
		txt, debug_idstr(*src), rcv_cap, rcv_msg, timeout.timeout);
  else
    wrapper_log(self, "%s rcv_cap: %#lx, rcv_msg: %p, t: %lx",
		txt, rcv_cap, rcv_msg, timeout.timeout);    

  Mword rcv_m0, rcv_m1, rcv_m2;
  Mword buff[68];


  Badge rcv_badge;
  IPC_result res = Syscall_user::recv_short_ipc(rcv_cap, count, map, 
						&rcv_m0, &rcv_m1, &rcv_m2, buff,
						&rcv_badge, to);

  if(res.error() 
     && res.error() != Error::Exist_rcv 
     && res.error() != Error::Timeout_rcv 
     && res.error() != Error::Cancel_rcv)
    panic("could not receive\n");

  // maybe we should deliver the previous one
  l4_threadid_t rcv_id = L4_INVALID_ID;

  if(!res.error())
    rcv_id = tid2id(rcv_badge.raw());

  *result = convert_rcv_message(self, rcv_id, res, 
				rcv_m0, rcv_m1, rcv_m2, buff, 
				*rcv_dword0, *rcv_dword1, rcv_msg);
  *src = rcv_id;

  wrapper_log_end(self, "%s recv from " debug_idfmt " badge: %#lx"
		  " w0: %#lx w1: %#lx res %#lx v2dope: %#lx", 
		  txt, debug_idstr(rcv_id), rcv_badge.raw(),
		  *rcv_dword0, *rcv_dword1, res.raw(), (*result).raw);

  return L4_IPC_ERROR(*result);
}


// XXX test, if send or receive should be skipped
static int
ipc_combined_send_receive(l4_threadid_t dest, Address snd_type, Address rcv_type, const char *txt,
			  const void *snd_msg,
			  l4_umword_t snd_dword0,
			  l4_umword_t snd_dword1,
			  l4_threadid_t *src,
			  void *rcv_msg,
			  l4_umword_t *rcv_dword0,
			  l4_umword_t *rcv_dword1,
			  l4_timeout_t timeout,
			  l4_msgdope_t *result)
{

  l4_threadid_t self = l4_myself();

  Address snd_m0, snd_m1, snd_m2;
  unsigned snd_map, snd_count;
  Mword buff[68];

  Address snd_cap = prepare_send(self, dest, snd_type, snd_msg, snd_dword0, snd_dword1,
				 snd_m0, snd_m1, snd_m2, buff, snd_count, snd_map);


  unsigned to;
  unsigned rcv_map;
  unsigned rcv_count;
  
  Address rcv_cap = prepare_receive(src, rcv_type, rcv_msg,
				    timeout, to, rcv_count, rcv_map);


  wrapper_log(self, "%s to " debug_idfmt " "
	      "snd_cap: %#lx w0: %#lx w1: %#lx "
	      "rcv_cap: %#lx rcv_msg: %p",
	      txt, debug_idstr(dest),
	      snd_cap, snd_dword0, snd_dword1, 
	      rcv_cap, rcv_msg);


  l4_threadid_t rcv_id = L4_INVALID_ID;
  Mword rcv_m0, rcv_m1, rcv_m2;

  bool synched = false;
  IPC_result res;
  Badge rcv_badge;

  while(1)
    {

      res = Syscall_user::short_ipc(snd_cap, snd_count, snd_map, 
				    snd_m0,snd_m1, snd_m2,
				    rcv_cap, rcv_count, rcv_map,
				    &rcv_m0, &rcv_m1, &rcv_m2,
				    buff,
				    &rcv_badge, to);
      
      assert(res.error() != Error::Type);

      if(!res.error())
	rcv_id = tid2id(rcv_badge.raw());
      if((res.error() == Error::Exist) && !synched)
	{
	  sync_thread(dest);
	  synched = true;
	  continue;
	}
      // success or error, maybe also Error::Exist, which means, the task has been deleted
      break;
    }

  *result = convert_rcv_message(self, rcv_id, res,
				rcv_m0, rcv_m1, rcv_m2, buff, 
				*rcv_dword0, *rcv_dword1, rcv_msg);

  *src = rcv_id;

  wrapper_log_end(self, "%s recv from " debug_idfmt " badge: %#lx"
		  "           w0: %#lx w1: %#lx res %#lx v2dope: %#lx", 
		  txt, debug_idstr(rcv_id), rcv_badge.raw(),
		  *rcv_dword0, *rcv_dword1, res.raw(), (*result).raw);

  return L4_IPC_ERROR(*result);
}


int handle_irq_quirks(l4_threadid_t src,
		      void *rcv_msg,
		      l4_timeout_t timeout,
		      l4_msgdope_t *result)
{
  assert(l4_is_irq_id(src));

  l4_threadid_t self = l4_myself();  
  unsigned irq_num = l4_get_irqnr(src);

  // test if its an associate op. (rcv timeout is 0)
  if(timeout.to.rcv_man == 0 && timeout.to.rcv_exp)
    {
      bool assoc_op = !l4_is_nil_id(src);
      
      if(assoc_op)
	wrapper_log(self, "associate irq (%d)", irq_num);
      else
	wrapper_log(self, "deassociate irq");

      Address rcv_ep = get_own_cap(SYSCALL_EP);

      Mword buff[SYSCALL_BUFFEROFFSET];
      Mword dummy1, dummy2;
      Badge rcv_badge;
      Mword m0;

      IPC_result res = Syscall_user::short_ipc
	(v2emu_ep(), 3, 0, 
	 V2_cs::Magic_request_mask | V2_cs::Attach_irq, 
	 irq_num, assoc_op,
	 rcv_ep, 3, 0,
	 &m0, &dummy1, &dummy2,
	 buff,
	 &rcv_badge, 0);

      if(!res.error())
	{  
	  // assoc successful and irq was pending
	  if(m0 == 0x00)
	    {
	      *result = ipc_v2_msg_dope(0, 0, 0, 0);
	    }
	  else if(m0 == 0x10) /* assoc not successful */
	    {
	      // XXX Fixme correct error handling
	      *result = ipc_v2_msg_dope(0, 0x10, 0, 0);
	    }
	  else if(m0 == 0x20) /* assoc successful and no irq was pending */
	    {
	      *result = ipc_v2_msg_dope(0, L4_IPC_RETIMEOUT , 0, 0);
	    }
	      
	  else
	    panic("unknown result on irq operation");
	}
      else
	{
	  
	  panic("error on irq operation"); // retry on cancel
#if 0
	  *result = ipc_v2_msg_dope(res.rcv_typed() ? 1 : 0, 
				    0, /* its always an receive error */
				    convert_xe_error_code(res.error()), // skip the snd error bit
				    0, 0);
#endif
	}
      
      wrapper_log_end(self, "l4_ipc_receive from irq: %u badge: %#lx res %#lx v2dope: %#lx", 
		      irq_num, rcv_badge.raw(), res.raw(), (*result).raw);
    }
  else
    {
      
      /* rcv from irq */
      panic("rcv from irq is not implemented");
    }  

  return L4_IPC_ERROR(*result);
}



int
l4_ipc_send(l4_threadid_t dest,
            const void *snd_msg,
            l4_umword_t snd_dword0,
            l4_umword_t snd_dword1,
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  return ipc_send(dest, OPEN_EP, "l4_ipc_send", snd_msg, 
		  snd_dword0, snd_dword1, timeout, result);
}


int
l4_ipc_reply(l4_threadid_t dest,
            const void *snd_msg,
            l4_umword_t snd_dword0,
            l4_umword_t snd_dword1,
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  return ipc_send(dest, CLOSED_EP, "l4_ipc_reply", snd_msg, 
		  snd_dword0, snd_dword1, timeout, result);
}

int
l4_ipc_receive(l4_threadid_t src,
               void *rcv_msg,
               l4_umword_t *rcv_dword0,
               l4_umword_t *rcv_dword1,
               l4_timeout_t timeout,
               l4_msgdope_t *result)
{
  
  if(l4_is_irq_id(src) || (l4_is_nil_id(src) && !timeout.to.rcv_man && timeout.to.rcv_exp))
      return handle_irq_quirks(src, rcv_msg, timeout, result);
  
  l4_threadid_t rcv_id = src;  

  int res = ipc_receive(&rcv_id, CLOSED_EP, "l4_ipc_receive",
		    rcv_msg, rcv_dword0, rcv_dword1, timeout, result);
  
  assert(res || 
	 (!res && rcv_id.id.task == src.id.task && rcv_id.id.lthread ==  src.id.lthread));
  return res;
}


int
l4_ipc_wait(l4_threadid_t *src,
            void *rcv_msg,
            l4_umword_t *rcv_dword0,
            l4_umword_t *rcv_dword1,
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  return ipc_receive(src, OPEN_EP, "l4_ipc_wait",
		     rcv_msg, rcv_dword0, rcv_dword1, timeout, result);
}

int
l4_ipc_reply_and_wait(l4_threadid_t dest,
                      const void *snd_msg,
                      l4_umword_t snd_dword0,
                      l4_umword_t snd_dword1,
                      l4_threadid_t *src,
                      void *rcv_msg,
                      l4_umword_t *rcv_dword0,
                      l4_umword_t *rcv_dword1,
                      l4_timeout_t timeout,
                      l4_msgdope_t *result)
{

  return ipc_combined_send_receive(dest, CLOSED_EP, OPEN_EP, "l4_ipc_reply_wait",
				   snd_msg, snd_dword0, snd_dword1,
				   src, rcv_msg, rcv_dword0, rcv_dword1,
				   timeout, result);
}

int
l4_ipc_forward(l4_threadid_t dest,
                      const void *snd_msg,
                      l4_umword_t snd_dword0,
                      l4_umword_t snd_dword1,
                      l4_threadid_t *src,
                      void *rcv_msg,
                      l4_umword_t *rcv_dword0,
                      l4_umword_t *rcv_dword1,
                      l4_timeout_t timeout,
                      l4_msgdope_t *result)
{

  return ipc_combined_send_receive(dest, OPEN_EP, OPEN_EP, "l4_ipc_forward",
				   snd_msg, snd_dword0, snd_dword1,
				   src, rcv_msg, rcv_dword0, rcv_dword1,
				   timeout, result);
}

int
l4_ipc_call(l4_threadid_t dest,
            const void *snd_msg,
            l4_umword_t snd_dword0,
            l4_umword_t snd_dword1,
            void *rcv_msg,
            l4_umword_t *rcv_dword0,
            l4_umword_t *rcv_dword1,
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  l4_threadid_t rcv_id = dest;

  int res = ipc_combined_send_receive(dest, OPEN_EP, CLOSED_EP, "l4_ipc_call",
				      snd_msg, snd_dword0, snd_dword1,
				      &rcv_id, rcv_msg, rcv_dword0, rcv_dword1,
				      timeout, result);
  assert(res || 
	 (!res && rcv_id.id.task == dest.id.task && rcv_id.id.lthread ==  dest.id.lthread));

  return res;
}

/*
Local Variables:
compile-command: "make -k -C ../"
End:
*/
