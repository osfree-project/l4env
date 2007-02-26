INTERFACE:

#include "l4_types.h"

/// Encapsulation of syscall data
/**
 * This class must be defined in arch dependent parts
 * and has to represent the data necessary for a 
 * system call as layed out on the kernel stack. 
 */
class Syscall_frame
{
};

class Return_frame
{
};

/// Encapsulation of a syscall entry kernel stack
/**
 * This class encapsulates the complete top of the 
 * kernel stack after a syscall (including the 
 * iret return frame) 
 */
class Entry_frame : public Syscall_frame, public Return_frame
{
public:
  Mword pc() const;
  void  pc( Mword _pc );

  Mword sp() const;
  void  sp( Mword _sp );
};

/** 
 * \brief IPC specific interpretation of syscall data 
 */
class Sys_ipc_frame : public Syscall_frame
{
public:
  /// set the IPC source for the recipient
  void rcv_source( L4_uid id ); 

  /// get the IPC source for the recipient
  L4_uid rcv_source(); 

  /// get the destination for the IPC
  L4_uid snd_dest() const;

  /// does the IPC have a destination
  Mword has_snd_dest() const;

  /// get the IRQ destination of the IPC
  Mword irq() const;
  
  /// set the send descriptor
  void snd_desc( Mword w );

  /// get the send descriptor
  L4_snd_desc snd_desc() const;

  /// set the receive descriptor
  void rcv_desc( L4_rcv_desc d );

  /// get the receive descriptor
  L4_rcv_desc rcv_desc() const;

  /// get the message timeout
  L4_timeout timeout() const;

  /// get the given register message word.
  Mword msg_word( unsigned index ) const;

  /// set the given message word to the given value.
  void set_msg_word( unsigned index, Mword value );

  /// copy this msg to the given IPC data
  void copy_msg( Sys_ipc_frame *to ) const ;

  /// get the msg dope
  L4_msgdope msg_dope() const;

  /// get the msg dope
  void msg_dope_set_error( Mword );

  /// set the msg dope
  void msg_dope( L4_msgdope d );

  /// or some extra bits to the msg dope
  void msg_dope_combine( L4_msgdope d );

  /// numer of words transmitted in registers
  static unsigned const num_reg_words();
};

/**
 * \brief id_nearest specific interpretation of syscall data 
 */
class Sys_id_nearest_frame : public Syscall_frame
{
public:
  
  /// get the dest parameter of the syscall
  L4_uid dest() const; 

  /// set the result of the syscall
  void nearest( L4_uid id ) ;
};

/**
 * \brief ex_regs specific interpretation of syscall data
 */
class Sys_ex_regs_frame : public Syscall_frame
{
public:

  /// get the lthread parameter of the syscall
  Mword lthread() const;

  /// get the stack pointer parameter
  Mword sp() const;

  /// get the instruction pointer parameter
  Mword ip() const;

  /// get the preempter id
  L4_uid preempter() const;

  /// get the pager id
  L4_uid pager()  const;
  
  /// set the old eflags (x86)
  void old_eflags( Mword oefl );

  /// set the old stack pointer
  void old_sp( Mword osp );

  /// set the old instruction pointer
  void old_ip( Mword oip );

  /// set the old preempter id
  void old_preempter( L4_uid id );

  /// set the old pager id
  void old_pager( L4_uid id );
};

/**
 * \brief thread_switch specific interpretation of syscall data
 */
class Sys_thread_switch_frame : public Syscall_frame
{
public:

  /// get the dest id of the switch
  L4_uid dest() const;

  /// returns true if dest is valid
  Mword has_dest() const;

};

/**
 * \brief thread_schedule specific interpretation of syscall data
 */
class Sys_thread_schedule_frame : public Syscall_frame
{
public:
  
  /// get the scheduling parameters
  L4_sched_param param() const;

  /// get the preempter id
  L4_uid preempter() const;

  /// get the destination id 
  L4_uid dest() const;
  
  /// set the old scheduling params
  void old_param( L4_sched_param op );

  /// set the consumed time
  void time( Unsigned64 t );

  /// set the old preempter
  void old_preempter( L4_uid id );

  /// set the partner of a pending IPC
  void partner( L4_uid id );
};

/**
 * \brief fpage_unmap specific interpretation of syscall data
 */
class Sys_unmap_frame : public Syscall_frame 
{
public:

  /// get the fpage to unmap
  L4_fpage fpage() const;

  /// get the mask, say rights for the unmap
  Mword map_mask() const; 

  /// returns true if the operation is a downgrade
  bool downgrade() const; 

  /// returns true if also the current space flushes the fpage
  bool self_unmap() const;
};

/**
 * \brief task_new specific interpretation of syscall data
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

  /// get the task id of the new task
  L4_uid dest() const;

  /// set the new tasks id
  void new_taskid( L4_uid id );

};


IMPLEMENTATION:
//-
