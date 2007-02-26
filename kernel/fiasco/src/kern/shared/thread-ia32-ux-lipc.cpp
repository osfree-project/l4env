IMPLEMENTATION[{ia32,ux}-v2-lipc]:

#include <l4_types.h>
#include <feature.h>

#include "mem_layout.h"
#include "thread.h"
#include "space.h"
#include "kip.h"
#include "kmem.h"

KIP_KERNEL_FEATURE("localipc");

/* 
   generic fixup func:
   if in restart code eip -> set eip back to start
   if in forward code eip -> lipc_end, and do the rest 
			     of the lipc code in kernelspace
   copy kernelstack if necessary 
*/

/*
  Possibilities of restart/forward fixup code
   1. only a restart point exists -> need to store reset infos, cycles++ :(
      but very small fixup code
   2. complex fixup code: big switch :(, but the forward code can be 
      very? optimized
   3. the here implementented solution, i have invariant 2-3 regs,
      and with these regs i can guess the complete lipc operation
      current regs are:
      eax: new utcb ptr
      ecx: old utcb ptr
      esi:_ receiver status, only valid up to asm_rcv_desc_invalid
*/

/** Calculate TCB pointer from local thread ID.
    @param id local thread id, i.e. utcb pointer
    @param s the adressspace, where the local is valid.
    @return TCB Pointer if the id is valid, 0 otherwise
*/
PUBLIC static inline NEEDS["mem_layout.h","l4_types.h"]
Thread *
Thread::lookup (Local_id _lid, Space *s)
{
  Address address = reinterpret_cast<Address> (_lid);
 
  if (address < Mem_layout::V2_utcb_addr)
    return 0;
  address -= Mem_layout::V2_utcb_addr;

  /* proper aligment */
  if ((address % sizeof(Utcb)) != 0)
    return 0;

  /* now calculate the thread number */
  address /= sizeof(Utcb);
  if (address >= Global_id::threads_per_task())
    return 0;

  return lookup(Global_id(s->id(), address));
}


/** Copy the current kernel stack to the new thread after a thread switch
    in userland via LIPC. The portion between param and end of stack is
    copied to the other Thread.
    @param kernel_esp startaddress of stack, which needs to be copied
 */
extern "C"
Address
copy_kernelstack(Address kernel_esp ) 
{
  Space *space = current_thread()->space();
  
  if (*global_utcb_ptr == current_thread()->local_id()) {
    //    current_thread()->utcb()->set_no_lipc_possible();
    return kernel_esp;
  }
  
  Thread *dst = Thread::lookup(reinterpret_cast<Local_id> (*global_utcb_ptr),
			       space);

  if (!dst) 
    {
      printf("LIPC: invalid utcb ptr %08lx\n", *global_utcb_ptr);
      kdb_ke("LIPC: invalid utcb ptr");
      return kernel_esp;
    }
  
  assert (dst != current_thread());
  
  /* 
     Check for a valid kern state, 
     the thread should be in Thread_lipc_ready
     Also test if the new Thread is locked, if yes ->poff.
     If this test fails, dont to anything.
  */
  if ((!(dst->state() & Thread_lipc_ready)) || dst->thread_lock()->test())
    {
      printf("LIPC: invalid dst ");
      //      dst->id().print();
      printf(" state %08lx | locked: %s\n",
	     dst->state(),  dst->thread_lock()->test() ? "true" : "false");

      kdb_ke("LIPC: invalid target state");
      return kernel_esp;
    }

  dst->state_change_dirty(~(Thread_ipc_receiving_mask 
			    | Thread_ipc_in_progress
			    | Thread_lipc_ready
			    | Thread_utcb_ip_sp),
			  Thread_ready);

  dst->utcb()->set_no_lipc_possible();
  
  Address size = Config::thread_block_size
    - (kernel_esp - (Address) current_thread());

  Address dststack_ptr = ((Address) dst) + Config::thread_block_size;
  dststack_ptr -= size;
  
  memcpy((void *) dststack_ptr, (void *) kernel_esp, size);  
  
  *(Kmem::kernel_esp()) = reinterpret_cast<Address>(dst->regs() + 1);
  LOG_LIPC_STACK_COPY;
  return dststack_ptr;
}

/** Do the rollback or roll forward stuff if a timer interrupt occured 
    during the execution of the LIPC code.
    user-eip is adjusted if the LIPC code was in rollback or
    rollforward section
    the others only for roll-forward
    @param eip ref. to user eip, is setted to restart or finisch point
    @param eax user eax for the LIPC destination thread
    @param ecx user ecx for the LIPC source thread
    @param ebp user ebp for the new user eip after the LIPC
    @param esp user esp for the new user esp
    @param esi user esi backward compatibility to v2 ipc, src V2 - L4_uid
    @param edi user edi backward compatibility to v2 ipc, src V2 - L4_uid
 */
extern "C"
void 
fixup_kern_stack(Address *eip, /* ip */ 
		 Address *eax, /* ecx, dst utcb */
		 Address *ecx, /* ecx, source utcb */
		 Address *ebp, /* new user eip */
		 Address *esp, /* esp */
		 Address *esi, /* rcv desc */
		 Address *edi) 
{
  Space *space = current_thread()->space();

  
  Address kip_lipc = user_kip_location + offsetof(Kip, lipc_code);

  Address user_eip = *eip - kip_lipc;

  if (user_eip > (Address) &Mem_layout::lipc_restart_point_offset
      && user_eip <= (Address) &Mem_layout::lipc_forward_point_offset)
    {
      LOG_LIPC_ROLLBACK;
      *eip = kip_lipc + (Address) &Mem_layout::lipc_restart_point_offset;
    }
  else if (user_eip > (Address) &Mem_layout::lipc_forward_point_offset
	   && user_eip < (Address) &Mem_layout::lipc_finish_point_offset)
    {

      Thread *src_thread = Thread::lookup(reinterpret_cast<Local_id> (*ecx),
					  space);
      Thread *dst_thread = Thread::lookup(reinterpret_cast<Local_id> (*eax),
					  space);
      
      if (!src_thread || !dst_thread)
	{
	  printf("LIPC: invalid utcb ptrs eax: %08lx ecx:%08lx\n", *eax, *ecx);
	  kdb_ke("LIPC: invalid regs in forwardcode");
	  return;
	}
      Utcb *src = src_thread->utcb(); assert(src);
      Utcb *dst = dst_thread->utcb(); assert(dst);
      
      /* receiving status already set? */
      if (user_eip <= (Address) &Mem_layout::lipc_rcv_desc_invalid)
	  src->state((Address) *esi );

      assert(src->state() != 0xff0);

      *eip = kip_lipc + (Address) &Mem_layout::lipc_finish_point_offset;
      
      *esp = dst->sp();
      *ebp = dst->ip();

      *edi = src_thread->id().raw() >> 32;
      *esi = src_thread->id().raw();

      /* valid utcb ptr */
      assert((*eax >= Mem_layout::V2_utcb_addr) 
	     && (*eax <= Mem_layout::V2_utcb_addr 
		 + Config::PAGE_SIZE));
      
      *global_utcb_ptr = (Local_id) *eax;

      dst->set_no_lipc_possible();

      LOG_LIPC_FORWARD;
    } 
  return;
}

/** Wrapper function for Assembler.
    @param old_kernel_thread Thread
 */
extern "C"
void 
fixup_old_kernel_stack(Thread *old_kernel_thread) 
{
  assert (old_kernel_thread != current_thread()); 
  old_kernel_thread->setup_iret_return();
}

/** Setup the UTCB of this thread. This includes filling the KTCB ptr,
    the L4_uid of this thread and denying LIPC  in the UTCB.
    Setting the utcb kernel-ptr for kernel access
    and utcb user-ptr, which is the local id, in the KTCB.
 */
PRIVATE
void
Thread::setup_lipc_utcb()
{
  _lipc_possible = false;
  deny_lipc();
  
  state_del(Thread_utcb_ip_sp| Thread_lipc_ready);

  utcb()->set_globalid(id());
  utcb()->ip(0);
  utcb()->sp(0);

  set_snd_local_id(local_id());
  set_snd_space(space());
}

/** After copying the current kernel stack, this routine sets up a
    "dummy" ipc stack on the old thread. A switch_to to this stack
    after a completed IPC is possible. Run under cpu lock.
 */
PUBLIC
void
Thread::setup_iret_return() 
{

  /* check running */
  assert(state() & Thread_ready);

  /* ok we check if we are reading from kernel_thread utcb */
  assert (space_index() != Config::kernel_taskno);

  if(!utcb()->in_wait())
    {
       printf("utcb state: %08lx\n", utcb()->state());
       kdb_ke("wrong utcb state");
    }

  assert(!(*sender_list()));
  
  state_change_dirty(~Thread_ready, 
		     (Thread_ipc_in_progress | Thread_receiving
		      | Thread_lipc_ready | Thread_utcb_ip_sp));

  set_partner(0);
  
  Entry_frame *r = regs();
  Sys_ipc_frame *ipc_regs = sys_frame_cast<Sys_ipc_frame>(r);
  set_rcv_regs(ipc_regs);
  
  //  ready_dequeue() is done lazily in the scheduler
  /* kernelsp fake*/
  Mword *sp = (Mword *) r;
  *--sp = reinterpret_cast<Mword> (&Mem_layout::asm_user_invoke_from_localipc);

  set_kernel_sp(sp);

  // correct segment values already on the stack

  // if someone is waiting on me, then disallows further LIPC send
  if (!*sender_list())
    utcb()->clear_lipc_nosnd_bit();
  
  ipc_regs->snd_desc(0);
  ipc_regs->rcv_desc(0);

  ipc_regs->rcv_src(L4_uid::Invalid);
  ipc_regs->timeout(L4_timeout::Never);

  LOG_LIPC_SETUP_IRET_STACK;

  /* we dont wake up any senders, because if are waiting senders,
     then send from this was always impossible */
}

/** Set the sender local-id in the receiving ipc regs if the space
    of the sender and receiver are equal, -1 otherwise.
    @param rcv receiver
    @param rcv_regs receiver's ipc registers
 */
IMPLEMENT inline
void
Thread::set_source_local_id(Thread *rcv,  Sys_ipc_frame *rcv_regs)
{
  rcv_regs->snd_utcb((Mword)(rcv->space() == space() ? id().lthread() : -1U));
}

/** Unlock the Receiver locked with ipc_try_lock().
    If the sender goes to wait for a registered message enable LIPC.
    @param receiver receiver
    @param sender_regs sender's ipc registers, for short register-msg check
 */
PRIVATE inline NEEDS ["entry_frame.h"]
void
Thread::unlock_receiver (Receiver *receiver, const  Sys_ipc_frame* sender_regs)
{
  if (!sender_regs->rcv_desc().is_register_ipc())
    {
      receiver->ipc_unlock();
      return;
    }

  /* 
     Lock here
    
     1. we dont want to get our stack overwritten by the lipc fixup routine
     before we release the thread lock via ipc_unlock
     
     2. the sender_queue in maybe_enable_lipc() test should be atomic
  */
  Lock_guard <Cpu_lock> guard (&cpu_lock);
  maybe_enable_lipc();
  receiver->ipc_unlock();
}


