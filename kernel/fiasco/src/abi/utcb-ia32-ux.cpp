INTERFACE [v2-utcb]:
class Utcb
{
};


INTERFACE [{ia32,ux}-utcb]:

#include "types.h"
#include "l4_types.h"

EXTENSION class Utcb
{
  /* must be 2^n bytes */
public:
  Mword		status;
  Mword		values[16];
  Unsigned32	snd_size;
  Unsigned32	rcv_size;

  /**
   * UTCB Userland Thread Control Block.
   *
   * This data structure is the userland thread control block of
   * a thread. It is modifiable from userland.
   */
public:
  
  enum {
    State_openwait	= 128,

    State_locked	= 1,
    State_no_lipc	= 2,

    Utcb_addr_mask	= 0xfffffffc,

    Send_state_queued	= 1,
    Send_state_fpu	= 2
  };  

private:
  /*
    The complete v2 L4 id, needed for fallback to kernel ipc
    and for V2 ABI compatibility
  */
  Global_id	_id;

  /* User sp and ip, if the thread is in ipc wait. */
  Address	_esp;
  Address	_eip;
  
  /* It contains the ipc partner, ipc state and the thread lock bit */
  Mword		_state;
  /* Contains fpu and sendqueue bit */
  Mword		_snd_state;

private:
  Unsigned32	_padding[7];
};


IMPLEMENTATION[{ia32,ux}-v2-lipc]:

#include <assert.h>
#include "atomic.h"
#include "mem_layout.h"

/** Set the receiver in the UTCB, 
    it does not enable LIPC in the UTCB.
    @param id	ipc send partner
 */
PUBLIC inline NEEDS["atomic.h"]
void
Utcb::set_rcv(Local_id id)
{

  Mword new_state = reinterpret_cast<Mword> (id) |
    static_cast<Mword> (State_no_lipc);
  
  atomic_change(&_state, 
		static_cast<Mword> (~Utcb_addr_mask),
		new_state);  
}

/** Set the IPC state in the Utcb to closed wait on id, 
    it does not enable LIPC in the UTCB.
    @pre	cpu lock
    @param id	the ipc send partner
 */
PUBLIC inline NEEDS["atomic.h"]
void
Utcb::set_closed_rcv_dirty(Local_id id)
{

  Mword new_state = reinterpret_cast<Mword> (id) | 
    static_cast<Mword> (State_no_lipc);

  // This is for the ipc shortcut with interrupts disabled.
  // Because we are running and do not hold locks, we aren`t locked,
  // so we don't need to test the lock bit here
  _state =  new_state;
}


/** "Calculates" the ipc state for open wait.
    The reason behind this is simple. 
    The user supplies only local thread numbers from 0 up to max threads
    to the LIPC code. The LIPC code now calculates the utcb pointer from
    this given id, by shifting the id by the utcb size and by adding the
    mapping adress of the Utcb area.
    LIPC is speed critical, so we don't want to have an extra branch for
    testing open wait/closed wait. Therefore we encode open wait to a special
    thread id, here Utcb::State_openwait.
    Because this special id goes through the calculation to the Utcb pointer
    too, we need a workaround for calculating the real open wait ipc state.
    A normal compiler should smart enough to do this at compile time. An
    another method would be a different entry point for closed wait/openwait
    but from user land this is not desirable.
    In X.2 the problem don't exists, because the user always supplies real
    utcb pointers.
    See genoffset.cc, for the assembler shortcut if you change this here.
    @return	ipc state in the utcb for open wait
 */
PUBLIC static inline NEEDS["mem_layout.h"]
Mword
Utcb::open_wait_ipc_state()
{
  return (Mword)(Mem_layout::V2_utcb_addr + sizeof(Utcb) * State_openwait);
}



/** Set the IPC state in the Utcb to open wait
    @pre	cpu lock
    @param id	the ipc send partner
 */
PUBLIC inline NEEDS["atomic.h"]
void
Utcb::set_open_rcv()
{

  Mword new_state = open_wait_ipc_state() | static_cast<Mword> (State_no_lipc);

  /*
    This is for the ipc shortcut with interrupts disabled.
    Because we are running and do not hold locks, we aren`t locked,
    so we don't need to test the lock bit here
  */
  _state =  new_state;
}



/** Disallows LIPC in the UTCB
 */
PUBLIC inline NEEDS["atomic.h"]
void
Utcb::set_no_lipc_possible() 
{ atomic_or(&_state, static_cast<Mword> (State_no_lipc)); }

/** Allows LIPC in the UTCB
 */
PUBLIC inline NEEDS["atomic.h"]
void
Utcb::lipc_ready() 
{ atomic_and(&_state, static_cast<Mword> (~State_no_lipc)); }

/** Set the thread lock bit in the UTCB, so it is possible for the user
    to see thread locks.
 */
PUBLIC inline
void
Utcb::lock_dirty() 
{ _state |= static_cast<Mword> (State_locked); }

/** Clear the thread lock bit in the UTCB, so it is possible for the user
    to see thread locks.
 */
PUBLIC inline
void
Utcb::unlock_dirty() 
{ _state &= static_cast<Mword> (~State_locked); }

/** Checks IPC state in the UTCB
    @return true if the IPC state in the UTCB is waiting, false otherwise.
 */
PUBLIC inline
Mword
Utcb::in_wait() const
{ return ( _state & Utcb_addr_mask ); }

/** Checks IPC state in the UTCB
    @return true if the IPC state in the UTCB is in open wait, false otherwise.
 */
PUBLIC inline
Mword
Utcb::open_wait() const
{ return  ((_state & Utcb_addr_mask) == Utcb::open_wait_ipc_state()); }

/** Read the IPC partner from the UTCB.
    @return IPC partner
 */
PUBLIC inline
Local_id
Utcb::partner() const
{  
  return reinterpret_cast<Local_id> (_state & Utcb_addr_mask);
}

/** Set the global V2 Global_id in the UTCB
    @param id Global_id
 */
PUBLIC inline
void 
Utcb::set_globalid(Global_id id) 
{ _id = id; }

/** Return the user ip stored in the UTCB, it is necessary to
    reload user ip/sp if LIPC was enabled
    @return the user ip from the UTCB
 */
PUBLIC inline
Address
Utcb::ip() const
{ return _eip; }

/** Return the user sp stored in the UTCB, it is necessary to
    reload user ip/sp if LIPC was enabled
    @return the user sp from the UTCB
 */
PUBLIC inline
Address
Utcb::sp() const 
{ return _esp; }

/** Set the user ip in the UTCB, it is necessary for LIPC.
    @param ip user ip, normaly from the entry frame
 */
PUBLIC inline
void
Utcb::ip(Mword ip)
{ _eip = ip; }

/** Set the user ip in the UTCB, it is necessary for LIPC.
    @param sp user sp, normaly from the entry frame
 */
PUBLIC inline
void
Utcb::sp(Mword sp)
{ _esp = sp; }

/** Set the LIPC no send bit in the UTCB IPC send_state.
    Necessary if someone is waiting for this Thread.
 */
PUBLIC inline  NEEDS["atomic.h"]
void
Utcb::set_lipc_nosnd_bit()
{ atomic_or(&_snd_state, static_cast<Mword> (Send_state_queued)); }

/** Clear the LIPC no send bit in the UTCB IPC send_state.
    Allows LIPC from this thread.
 */
PUBLIC inline  NEEDS["atomic.h"]
void
Utcb::clear_lipc_nosnd_bit()
{ atomic_and(&_snd_state, static_cast<Mword> (~Send_state_queued)); }

/** Clear the LIPC no send bit in the UTCB IPC send_state.
    Allows LIPC from this thread.
    @pre cpu lock
 */
PUBLIC inline
void
Utcb::clear_lipc_nosnd_bit_dirty()
{ _snd_state &= static_cast<Mword> (~Send_state_queued); }

/** Set the FPU used bit in the UTCB IPC send_state.
    Disallows LIPC from this thread.
 */
PUBLIC inline  NEEDS["atomic.h"]
void
Utcb::set_fpu_bit()
{ atomic_or(&_snd_state, static_cast<Mword> (Send_state_fpu)); }

/** Set the FPU used bit in the UTCB IPC send_state.
    Disallows LIPC from this thread.
    This requires CPU Lock
    @pre cpu lock
 */
PUBLIC inline  NEEDS["atomic.h"]
void
Utcb::set_fpu_bit_dirty()
{ _snd_state |= static_cast<Mword> (Send_state_fpu); }

/** Clear the FPU used bit in the UTCB IPC send_state.
    Allows LIPC from this thread.
 */
PUBLIC inline  NEEDS["atomic.h"]
void
Utcb::clear_fpu_bit()
{ atomic_and(&_snd_state, static_cast<Mword> (~Send_state_fpu)); }

/** Clear the FPU used bit in the UTCB IPC send_state.
    This requires CPU lock
    @pre cpu lock
 */
PUBLIC inline  NEEDS["atomic.h"]
void
Utcb::clear_fpu_bit_dirty()
{ _snd_state &= static_cast<Mword> (~Send_state_fpu); }
 

/** Return the raw IPC state in the UTCB
 */
PUBLIC inline
Mword
Utcb::state() const
{ return _state; }

/** Set the raw IPC state in the UTCB
 */
PUBLIC inline
void
Utcb::state(Mword state)
{ _state = state; }



IMPLEMENTATION[{ia32,ux}-v2-lipc-debug]:

#include <stdio.h>
/** Dump the content of the UTCB
 */
PUBLIC
void
Utcb::print() const
{
  printf("thread:   ");	_id.print();
  printf("\n"
         "address:  "L4_PTR_FMT"\n"
         "state:    %08lx ", (Address)this, _state);

  if (open_wait())
    printf("openwait ");
  if (_state & Utcb_addr_mask) 
    printf("waiting ");
  if (_state &  State_no_lipc)
    printf("no lipc ready ");

  printf("\npartner:  ");

  if (open_wait())
    printf("*\n");
  else if (in_wait())
    printf("%08lx\n", partner());
  else
    printf("-\n");

  printf("sndstate: %08lx\n", _snd_state);
  printf("\nESP="L4_PTR_FMT" EIP="L4_PTR_FMT"\n", _esp, _eip);
}
