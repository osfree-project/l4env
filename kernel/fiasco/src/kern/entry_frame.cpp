INTERFACE:

#include "l4_types.h"
#include "utcb.h"

/**
 * Encapsulation of syscall data.
 *
 * This class must be defined in arch dependent parts
 * and has to represent the data necessary for a 
 * system call as layed out on the kernel stack. 
 */
class Syscall_frame
{};

class Return_frame
{
public:
  Mword ip() const;
  void  ip(Mword _pc);

  Mword sp() const;
  void  sp(Mword _sp);
};

/**
 * Encapsulation of a syscall entry kernel stack.
 *
 * This class encapsulates the complete top of the 
 * kernel stack after a syscall (including the 
 * iret return frame).
 */
class Entry_frame : public Syscall_frame, public Return_frame
{};

/** 
 * IPC specific interpretation of syscall data 
 */
class Sys_ipc_frame : public Syscall_frame
{
public:
  /// Get the given register/utcb message word.
  Mword msg_word (Utcb *dst, unsigned index) const;

  /// Set the given message word to the given value.
  void set_msg_word (Utcb *u, unsigned index, Mword value);

  /** Set the given message word to the given value.
      @pre index < 3 */
  void set_msg_word (unsigned index, Mword value);

  /// set the IPC source for the recipient
  void rcv_src(L4_uid const &id); 

  /// get the IPC source for the recipient
  L4_uid rcv_src() const;

  /// get the destination for the IPC
  L4_uid snd_dst() const;

  /// does the IPC have a destination
  Mword has_snd_dst() const;

  /// has the IPC a send part?
  Mword has_snd() const;

  L4_msg_tag tag() const;
  void tag(L4_msg_tag const &tag);

  /// get the IRQ destination of the IPC
  Mword irq() const;
  
  /// get the message timeout
  L4_timeout_pair timeout() const;

  /// number of words sent in registers
  static unsigned num_snd_reg_words();

  /// number of words received in registers
  static unsigned num_rcv_reg_words();

  /// set the send descriptor
  void snd_desc(Mword w);

  /// get the send descriptor
  L4_snd_desc snd_desc() const;

  /// copy this msg to the given IPC data
  void copy_msg(Sys_ipc_frame *to) const;

  /// set the receive descriptor
  void rcv_desc(L4_rcv_desc d);

  /// get the receive descriptor
  L4_rcv_desc rcv_desc() const;

  /// get the given register message word.
  Mword msg_word(unsigned index) const;

  /// get the msg dope
  L4_msgdope msg_dope() const;

  /// set the msg dope
  void msg_dope(L4_msgdope d);

  /// set the error in a msg dope
  void msg_dope_set_error(Mword);

  /// or some extra bits to the msg dope
  void msg_dope_combine(Ipc_err e);

  /// number of words transmitted in registers
  static unsigned num_reg_words();

  Mword next_period() const;
};

/**
 * id_nearest specific interpretation of syscall data 
 */
class Sys_id_nearest_frame : public Syscall_frame
{
public:
  /// get the dst parameter of the syscall
  L4_uid dst() const; 

  /// set the return type of the syscall
  void type( Mword type );

  /// set the result of the syscall
  void nearest(L4_uid const &id);
};

/**
 * ex_regs specific interpretation of syscall data
 */
class Sys_ex_regs_frame : public Syscall_frame
{
public:
  /// get the stack pointer parameter
  Mword sp() const;

  /// get the instruction pointer parameter
  Mword ip() const;

  /// get the pager id
  L4_uid pager()  const;
  
  /// set the old eflags (x86)
  void old_eflags( Mword oefl );

  /// set the old stack pointer
  void old_sp( Mword osp );

  /// set the old instruction pointer
  void old_ip( Mword oip );

  /// set the old pager id
  void old_pager(L4_uid const &id);

  /// get the lthread parameter of the syscall
  LThread_num lthread() const;

  /// get the task parameter of the syscall
  Task_num task() const;

  /// get no_cancel bit change bits
  Mword no_cancel() const;

  /// get the preempter id
  L4_uid preempter() const;

  /// set the old preempter id
  void old_preempter(L4_uid const &id);

  /// get the task-capability-fault handler id
  L4_uid cap_handler(const Utcb* utcb) const;

  /// set the old task-capability-fault handler id
  void old_cap_handler(L4_uid const &id, Utcb* utcb);

  Mword alien() const;

  Mword trigger_exception() const;
};

/**
 * thread_switch specific interpretation of syscall data
 */
class Sys_thread_switch_frame : public Syscall_frame
{
public:
  /// get switch destination id
  L4_uid dst() const;

  /// get timeslice id
  Mword id() const;

  /// set remaining time quantum
  void left (Unsigned64 t);

  /// set return value
  void ret (Mword val);
};

/**
 * fpage_unmap specific interpretation of syscall data
 */
class Sys_unmap_frame : public Syscall_frame 
{
public:
  /// returns true if also the current space flushes the fpage
  bool self_unmap() const;

  /// get the fpage to unmap
  L4_fpage fpage() const;

  /// get the mask, say rights for the unmap
  Mword map_mask() const;

  /// returns true if the operation is a downgrade (removes write, but
  /// not read permission).
  bool downgrade() const;

  /// returns true if the operation does not remove read or write
  /// permission.  Overrides downgrade().
  bool no_unmap() const;

  /// returns true if the operation returns and resets accessed and
  /// dirty flags.
  bool reset_references() const;

  /// returns task to which the unmap should be restricted, or 0 if
  /// there is no restriction
  Task_num restricted() const;

  /// set return value
  void ret (Mword status);
};

/**
 * task_new specific interpretation of syscall data
 */
class Sys_task_new_frame : public Syscall_frame 
{
public:
  /// get the mcp of the new task (if created active)
  Mword mcp() const; 

  /// get the new chief of the task (if created inactive)
  L4_uid new_chief() const;
  
  /// get the stack pointer of the thread 0 (active)
  Mword sp() const;

  /// get the instruction pointer of thread 0 (active)
  Mword ip() const;
  
  /// is a pager specified (if not then create inactive task)
  Mword has_pager() const;

  /// get the pager id (active)
  L4_uid pager() const; 

  /// get the pager id (active)
  L4_uid cap_handler(const Utcb*) const; 

  /// get the quota descriptor (active)
  L4_quota_desc quota_descriptor(const Utcb*) const; 
  
  /// get the task id of the new task
  L4_uid dst() const;

  /// set the new tasks id
  void new_taskid(L4_uid const &id);

  /// get the alien flag
  Mword alien() const;

  /// should task capabilities be enabled for the new task?
  Mword extra_args() const;

  /// get the trigger_exception flag
  Mword trigger_exception() const;
};

/**
 * thread_schedule specific interpretation of syscall data
 */
class Sys_thread_schedule_frame : public Syscall_frame
{
public:
  /// get the scheduling parameters
  L4_sched_param param() const;

  /// get scheduling time point
  Unsigned64 time() const;

  /// get the preempter id
  L4_uid preempter() const;

  /// get the destination id
  L4_uid dst() const;

  /// set the old scheduling params
  void old_param(L4_sched_param op);

  /// set the consumed time
  void time(Unsigned64 t);

  /// set the old preempter
  void old_preempter(L4_uid const &id);

  /// set the partner of a pending IPC
  void partner(L4_uid const &id);
};

/**
 * thread_privctrl specific interpretation of syscall data
 */
class Sys_thread_privctrl_frame : public Syscall_frame
{
public:
  Mword command() const;
  L4_uid dst() const;
  Mword entry_func() const;
  void ret_val(Mword v);
};

/**
 * id_nearest specific interpretation of syscall data 
 */
class Sys_u_lock_frame : public Syscall_frame
{
public:
  enum Op 
  {
    New = 1, Lock = 3, Unlock = 4,
    New_semaphore = 5, Sem_sleep = 6, Sem_wakeup = 7
  };

  Op op() const;
  unsigned long lock() const;
  void result(unsigned long res);
  L4_timeout timeout() const;
  L4_semaphore *semaphore() const;
};


extern "C" void Entry_frame_Syscall_frame_cast_problem();


//-----------------------------------------------------------------------------
IMPLEMENTATION:

inline
template< typename Cl >
Cl *sys_frame_cast( Entry_frame *e ) 
{
  Cl *r = nonull_static_cast<Cl*>(nonull_static_cast<Syscall_frame*>(e));
  if(((void*)e) != ((void*)r))
    Entry_frame_Syscall_frame_cast_problem();
  return r;
}

IMPLEMENT inline
Mword Sys_ipc_frame::msg_word (Utcb*, unsigned index) const
{
  return msg_word (index);
}
