INTERFACE:

enum {
  TBUF_UNUSED             = 0,
  TBUF_PF                 = 1,
  TBUF_IPC                = 2,
  TBUF_IPC_RES            = 3,
  TBUF_IPC_TRACE          = 4,
  TBUF_KE                 = 5,
  TBUF_KE_REG             = 6,
  TBUF_UNMAP              = 7,
  TBUF_SHORTCUT_FAILED    = 8,
  TBUF_SHORTCUT_SUCCEEDED = 9,
  TBUF_CONTEXT_SWITCH     = 10,
  TBUF_EXREGS             = 11,
  TBUF_BREAKPOINT         = 12,
  TBUF_TRAP               = 13,
  TBUF_PF_RES             = 14,
  TBUF_SCHED              = 15
};

#include "l4_types.h"

class Context;
class Sys_ipc_frame;
class Sys_ex_regs_frame;
struct trap_state;

class Tb_entry
{
public:
  void		number(Mword number);
  void		rdtsc();
  void		rdpmc1();
  void		rdpmc2();
  Mword		eip() const;
  Context	* tid() const;
  Unsigned8	type() const;
  Mword		number() const;
  Unsigned64	tsc() const;
  Unsigned32	pmc1() const;
  Unsigned32	pmc2() const;

protected:
  void		set_global(char type, Context *tid, Mword eip);
  Mword		_number;	// event number
  Mword		_eip;		// instruction pointer
  Context	*_tid;		// Context
  Unsigned64	_tsc;		// time stamp counter
  Unsigned32	_pmc1;		// performance counter value 1
  Unsigned32	_pmc2;		// performance counter value 2
  Unsigned8	_type;		// type of entry
} __attribute__((packed));

class Tb_entry_fit64 : public Tb_entry
{
public:
  void		init(void);
private:
  char		_reserved[64-sizeof(Tb_entry)];
};

// logged ipc
class Tb_entry_ipc : public Tb_entry
{
private:
  L4_snd_desc	_snd_desc;	// ipc send descriptor
  L4_rcv_desc	_rcv_desc;	// ipc send descriptor
  Mword		_dword[3]; 	// message words
  L4_uid	_dest;		// destination id
};

// logged ipc result
class Tb_entry_ipc_res : public Tb_entry
{
private:
  Unsigned8	_have_send;	// ipc had send part
  Mword		_dword[3];	// dwords
  L4_msgdope	_result;	// result
  L4_uid	_rcv_src;	// partner
  L4_rcv_desc	_rcv_desc;	// receive descriptor
  Mword		_pair_event;	// referred event
};

// logged ipc for user level tracing with Vampir
class Tb_entry_ipc_trace : public Tb_entry
{
private:
  Unsigned8	_send_desc;
  Unsigned8	_recv_desc;
  L4_msgdope	_result;	// result
  Unsigned64	_send_tsc;	// entry tsc
  L4_uid	_send_dest;	// send destination
  L4_uid	_recv_dest;	// recv partner
};

// logged pagefault
class Tb_entry_pf : public Tb_entry
{
private:
  Address	_pfa;		// pagefault address
  Mword		_error;		// pagefault error code
};

// pagefault result
class Tb_entry_pf_res : public Tb_entry
{
private:
  Mword		_pfa;
  L4_msgdope	_err;
  L4_msgdope	_ret;
};

// logged kernel event
class Tb_entry_ke : public Tb_entry
{
private:
  char		_msg[35];	// debug message
};

// logged kernel event plus register content
class Tb_entry_ke_reg : public Tb_entry
{
private:
  char		_msg[23];	// debug message
  Mword		_eax, _ecx, _edx; // registers
};

// logged unmap operation
class Tb_entry_unmap : public Tb_entry
{
private:
  L4_fpage	_fpage;		// flexpage to unmap
  Mword		_mask;		// mask
  bool		_result;	// result
};

// logged thread_ex_regs operation
class Tb_entry_ex_regs : public Tb_entry
{
private:
  Mword		_lthread;	// local thread number
  Mword		_old_esp;	// old stack pointer
  Mword		_new_esp;	// new stack pointer
  Mword		_old_eip;	// old instruction pointer
  Mword		_new_eip;	// new instruction pointer
};

// logged breakpoint
class Tb_entry_bp : public Tb_entry
{
private:
  Address	_address;	// breakpoint address
  int		_len;		// breakpoint length
  Mword		_value;		// value at address
  int		_mode;		// breakpoint mode
};

// logged short-cut ipc failed
class Tb_entry_ipc_sfl : public Tb_entry
{
private:
  L4_snd_desc	_snd_desc;	// short ipc send descriptor
  L4_rcv_desc	_rcv_desc;	// short ipc recv descriptor
  L4_timeout	_timeout;	// ipc timeout
  L4_uid	_dest;		// partner
  Unsigned8	_is_irq, _snd_lst, _dst_ok, _dst_lck;
};

// logged context switch
class Tb_entry_ctx_sw : public Tb_entry
{
private:
  Context	*_dest;		// switcher target
  Mword		_kernel_eip, _kernel_esp;
};

// logged trap
class Tb_entry_trap : public Tb_entry
{
private:
  char		_trapno;
  Unsigned16	_errno;
  Mword		_ebp, _ebx, _edx, _ecx, _eax, _eflags, _esp;
  Unsigned16	_cs,  _ds;
};

// logged scheduling event
class Tb_entry_sched: public Tb_entry
{
private:
  unsigned		_mode;
  int			_ticks_left;
  unsigned short	_id;
  unsigned short	_prio;
  unsigned short	_timeslice;
};

IMPLEMENTATION:

#include <cstring>
#include <cstdarg>

#include "entry_frame.h"
#include "cpu.h"
#include "initcalls.h"
#include "perf_cnt.h"
#include <flux/x86/base_trap.h>


extern "C" void _wrong_sizeof_tb_entry_fit	(void);
extern "C" void _wrong_sizeof_tb_entry_ipc	(void);
extern "C" void _wrong_sizeof_tb_entry_ipc_res	(void);
extern "C" void _wrong_sizeof_tb_entry_ipc_trace(void);
extern "C" void _wrong_sizeof_tb_entry_pf	(void);
extern "C" void _wrong_sizeof_tb_entry_pf_res	(void);
extern "C" void _wrong_sizeof_tb_entry_ke	(void);
extern "C" void _wrong_sizeof_tb_entry_ke_reg	(void);
extern "C" void _wrong_sizeof_tb_entry_unmap	(void);
extern "C" void _wrong_sizeof_tb_entry_ex_regs	(void);
extern "C" void _wrong_sizeof_tb_entry_bp	(void);
extern "C" void _wrong_sizeof_tb_entry_ipc_sfl	(void);
extern "C" void _wrong_sizeof_tb_entry_ctx_sw	(void);
extern "C" void _wrong_sizeof_tb_entry_trap	(void);
extern "C" void _wrong_sizeof_tb_entry_sched	(void);

IMPLEMENT FIASCO_INIT
void
Tb_entry_fit64::init()
{
  // ensure several assertions
  if (sizeof(Tb_entry_fit64)	!= 64)	_wrong_sizeof_tb_entry_fit();
  if (sizeof(Tb_entry_ipc)	 > 64)	_wrong_sizeof_tb_entry_ipc();
  if (sizeof(Tb_entry_ipc_res)	 > 64)	_wrong_sizeof_tb_entry_ipc_res();
  if (sizeof(Tb_entry_ipc_trace) > 64)	_wrong_sizeof_tb_entry_ipc_trace();
  if (sizeof(Tb_entry_pf)	 > 64)	_wrong_sizeof_tb_entry_pf();
  if (sizeof(Tb_entry_pf_res)	 > 64)	_wrong_sizeof_tb_entry_pf_res();
  if (sizeof(Tb_entry_ke)	 > 64)	_wrong_sizeof_tb_entry_ke();
  if (sizeof(Tb_entry_ke_reg)	 > 64)	_wrong_sizeof_tb_entry_ke_reg();
  if (sizeof(Tb_entry_unmap)	 > 64)	_wrong_sizeof_tb_entry_unmap();
  if (sizeof(Tb_entry_ex_regs)	 > 64)	_wrong_sizeof_tb_entry_ex_regs();
  if (sizeof(Tb_entry_bp)	 > 64)	_wrong_sizeof_tb_entry_bp();
  if (sizeof(Tb_entry_ipc_sfl)	 > 64)	_wrong_sizeof_tb_entry_ipc_sfl();
  if (sizeof(Tb_entry_ctx_sw)	 > 64)	_wrong_sizeof_tb_entry_ctx_sw();
  if (sizeof(Tb_entry_trap)	 > 64)	_wrong_sizeof_tb_entry_trap();
  if (sizeof(Tb_entry_sched)	 > 64)	_wrong_sizeof_tb_entry_sched();
}

PUBLIC inline
void
Tb_entry::clear()
{
  _type = TBUF_UNUSED;
}

IMPLEMENT inline
void
Tb_entry::set_global(char type, Context *tid, Mword eip)
{
  _type    = type;
  _tid = tid;
  _eip     = eip;
}

IMPLEMENT inline
Mword
Tb_entry::eip() const
{
  return _eip;
}

IMPLEMENT inline
Context*
Tb_entry::tid() const
{
  return _tid;
}

IMPLEMENT inline
Unsigned8
Tb_entry::type() const
{
  return _type;
}

IMPLEMENT inline
Mword
Tb_entry::number() const
{
  return _number;
}

IMPLEMENT inline
void
Tb_entry::number(Mword number)
{
  _number = number;
}

IMPLEMENT inline NEEDS ["cpu.h"]
void
Tb_entry::rdtsc()
{
  _tsc = Cpu::rdtsc();
}

IMPLEMENT inline NEEDS ["perf_cnt.h"]
void
Tb_entry::rdpmc1()
{
  _pmc1 = (Unsigned32)Perf_cnt::read_pmc[0]();
}

IMPLEMENT inline NEEDS ["perf_cnt.h"]
void
Tb_entry::rdpmc2()
{
  _pmc2 = (Unsigned32)Perf_cnt::read_pmc[1]();
}

IMPLEMENT inline
Unsigned64
Tb_entry::tsc() const
{
  return _tsc;
}

IMPLEMENT inline
Unsigned32
Tb_entry::pmc1() const
{
  return _pmc1;
}

IMPLEMENT inline
Unsigned32
Tb_entry::pmc2() const
{
  return _pmc2;
}



PUBLIC inline NEEDS ["entry_frame.h"]
void
Tb_entry_ipc::set(Context *tid, Mword eip, Sys_ipc_frame *ipc_regs)
{
  set_global(TBUF_IPC, tid, eip);
  // hint for gcc
  register Mword tmp0 = ipc_regs->msg_word(0);
  register Mword tmp1 = ipc_regs->msg_word(1);
  register Mword tmp2 = ipc_regs->msg_word(2);
  _dword[0]  = tmp0;
  _dword[1]  = tmp1;
  _dword[2]  = tmp2;
  _snd_desc  = ipc_regs->snd_desc();
  _rcv_desc  = ipc_regs->rcv_desc();
  _dest      = ipc_regs->snd_dest();
}

PUBLIC inline
void
Tb_entry_ipc::set_irq(Context *tid, Mword eip, Mword irq)
{
  set_global(TBUF_IPC, tid, eip);
  _snd_desc = L4_snd_desc(0);
  _rcv_desc = L4_rcv_desc(0);
  _dest     = L4_uid::irq(irq);
}

PUBLIC inline
void
Tb_entry_ipc::set_sc(Context *tid, Mword eip, Sys_ipc_frame *ipc_regs)
{
  set_global(TBUF_SHORTCUT_SUCCEEDED, tid, eip);
  // hint for gcc
  register Mword tmp0 = ipc_regs->msg_word(0);
  register Mword tmp1 = ipc_regs->msg_word(1);
  register Mword tmp2 = ipc_regs->msg_word(2);
  _dword[0]  = tmp0;
  _dword[1]  = tmp1;
  _dword[2]  = tmp2;
  _snd_desc  = ipc_regs->snd_desc();
  _rcv_desc  = ipc_regs->rcv_desc();
  _dest      = ipc_regs->snd_dest();
}

PUBLIC inline
L4_snd_desc
Tb_entry_ipc::snd_desc() const
{
  return _snd_desc;
}

PUBLIC inline
L4_rcv_desc
Tb_entry_ipc::rcv_desc() const
{
  return _rcv_desc;
}

PUBLIC inline
L4_uid
Tb_entry_ipc::dest() const
{
  return _dest;
}

PUBLIC inline
Mword
Tb_entry_ipc::dword(unsigned index) const
{
  return _dword[index];
}



PUBLIC inline
void
Tb_entry_pf::set(Context *tid, Mword eip, Address pfa, Mword error)
{
  set_global(TBUF_PF, tid, eip);
  _pfa   = pfa;
  _error = error;
}

PUBLIC inline
Mword
Tb_entry_pf::error() const
{
  return _error;
}

PUBLIC inline
Address
Tb_entry_pf::pfa() const
{
  return _pfa;
}



PUBLIC inline NEEDS [<flux/x86/base_trap.h>]
void
Tb_entry_pf_res::set(Context *tid, Mword eip, Mword pfa, 
		     L4_msgdope err, L4_msgdope ret)
{
  set_global(TBUF_PF_RES, tid, eip);
  _pfa = pfa;
  _err = err;
  _ret = ret;
}

PUBLIC inline
Mword
Tb_entry_pf_res::pfa() const
{
  return _pfa;
}

PUBLIC inline
L4_msgdope
Tb_entry_pf_res::err() const
{
  return _err;
}

PUBLIC inline
L4_msgdope
Tb_entry_pf_res::ret() const
{
  return _ret;
}



PUBLIC inline NEEDS ["entry_frame.h"]
void
Tb_entry_ipc_res::set(Context *tid, Mword eip, Sys_ipc_frame *ipc_regs,
		      Mword result, Mword pair_event, Unsigned8 have_send)
{
  set_global(TBUF_IPC_RES, tid, eip);
  // hint for gcc
  register Mword tmp0 = ipc_regs->msg_word(0);
  register Mword tmp1 = ipc_regs->msg_word(1);
  register Mword tmp2 = ipc_regs->msg_word(2);
  _dword[0]   = tmp0;
  _dword[1]   = tmp1;
  _dword[2]   = tmp2;
  _pair_event = pair_event;
  _result     = result;
  _rcv_desc   = ipc_regs->rcv_desc();
  _rcv_src    = ipc_regs->rcv_source();
  _have_send  = have_send;
}

PUBLIC inline
int
Tb_entry_ipc_res::have_send() const
{
  return _have_send;
}

PUBLIC inline
L4_rcv_desc
Tb_entry_ipc_res::rcv_desc() const
{
  return _rcv_desc;
}

PUBLIC inline
L4_msgdope
Tb_entry_ipc_res::result() const
{
  return _result;
}

PUBLIC inline
L4_uid
Tb_entry_ipc_res::rcv_src() const
{
  return _rcv_src;
}

PUBLIC inline
Mword
Tb_entry_ipc_res::dword(unsigned index) const
{
  return _dword[index];
}

PUBLIC inline
Mword
Tb_entry_ipc_res::pair_event() const
{
  return _pair_event;
}



PUBLIC inline
void
Tb_entry_ipc_trace::set(Context *tid, Mword eip, Unsigned64 send_tsc,
			L4_uid send_dest, L4_uid recv_dest, L4_msgdope result,
			Unsigned8 send_desc, Unsigned8 recv_desc)
{
  set_global(TBUF_IPC_TRACE, tid, eip);
  _send_tsc  = send_tsc;
  _send_dest = send_dest;
  _recv_dest = recv_dest;
  _result    = result;
  _send_desc = send_desc;
  _recv_desc = recv_desc;
}



PUBLIC inline
void
Tb_entry_bp::set(Context *tid, Mword eip,
		 int mode, int len, Mword value, Address address)
{
  set_global(TBUF_BREAKPOINT, tid, eip);
  _mode    = mode;
  _len     = len;
  _value   = value;
  _address = address;
}



PUBLIC inline
int
Tb_entry_bp::mode() const
{
  return _mode;
}

PUBLIC inline
int
Tb_entry_bp::len() const
{
  return _len;
}

PUBLIC inline
Mword
Tb_entry_bp::value() const
{
  return _value;
}

PUBLIC inline
Address
Tb_entry_bp::addr() const
{
  return _address;
}



PUBLIC inline
void
Tb_entry_ke::set(Context *tid, Mword eip)
{
  set_global(TBUF_KE, tid, eip);
  _msg[sizeof(_msg)-1] = '\0';
}

PUBLIC inline NEEDS [<cstring>]
void
Tb_entry_ke::set(Context *tid, Mword eip, const char *msg, Mword len)
{
  set_global(TBUF_KE, tid, eip);
  if (len > sizeof(_msg)-1)
    len = sizeof(_msg)-1;
  strncpy(_msg, msg, len);
  _msg[len] = '\0';
}

PUBLIC inline
void
Tb_entry_ke::set_const(Context *tid, Mword eip, const char * const msg)
{
  set_global(TBUF_KE, tid, eip);
  _msg[0] = '\0';
  *(char const ** const)(_msg + 3) = msg;
}

PUBLIC inline
void
Tb_entry_ke::set_buf(unsigned i, char c)
{
  if (i < sizeof(_msg))
    _msg[i] = c;
}

PUBLIC inline
const char *
Tb_entry_ke::msg() const
{
  return _msg[0] ? _msg : *(char const ** const)(_msg + 3);
}


PUBLIC inline
void
Tb_entry_ke_reg::set(Context *tid, Mword eip, Mword v1, Mword v2, Mword v3)
{
  set_global(TBUF_KE_REG, tid, eip);
  _msg[sizeof(_msg)-1] = '\0';
  _eax = v1; _ecx = v2; _edx = v3;
}

PUBLIC inline NEEDS [<cstring>,<flux/x86/base_trap.h>]
void
Tb_entry_ke_reg::set(Context *tid, Mword eip, const char *msg, Mword len,
		     trap_state *ts)
{
  set_global(TBUF_KE_REG, tid, eip);
  if (len > 0)
    {
      if (len > sizeof(_msg))
	len = sizeof(_msg);
      strncpy(_msg, msg, len);
    }
  _msg[len] = '\0';
  _eax = ts->eax; _ecx = ts->ecx; _edx = ts->edx;
}

PUBLIC inline
void
Tb_entry_ke_reg::set_const(Context *tid, Mword eip, const char * const msg,
			   Mword eax, Mword ecx, Mword edx)
{
  set_global(TBUF_KE_REG, tid, eip);
  _msg[0] = '\0';
  *(char const ** const)(_msg + 3) = msg;
  _eax = eax; _ecx = ecx; _edx = edx;
}

PUBLIC inline
void
Tb_entry_ke_reg::set_buf(unsigned i, char c)
{
  if (i < sizeof(_msg))
    _msg[i] = c;
}

PUBLIC inline
const char *
Tb_entry_ke_reg::msg() const
{
  return _msg[0] ? _msg : *(char const ** const)(_msg + 3);
}

PUBLIC inline
Mword
Tb_entry_ke_reg::eax() const
{
  return _eax;
}

PUBLIC inline
Mword
Tb_entry_ke_reg::ecx() const
{
  return _ecx;
}

PUBLIC inline
Mword
Tb_entry_ke_reg::edx() const
{
  return _edx;
}

PUBLIC inline
void
Tb_entry_ctx_sw::set(Context *tid, Mword eip, Context *dest,
		     Mword kernel_esp, Mword kernel_eip)
{
  set_global(TBUF_CONTEXT_SWITCH, tid, eip);
  _kernel_esp = kernel_esp;
  _kernel_eip = kernel_eip;
  _dest       = dest;
}

PUBLIC inline
Mword
Tb_entry_ctx_sw::kernel_eip() const
{
  return _kernel_eip;
}

PUBLIC inline
Mword
Tb_entry_ctx_sw::kernel_esp() const
{
  return _kernel_esp;
}

PUBLIC inline
Context*
Tb_entry_ctx_sw::dest() const
{
  return _dest;
}

PUBLIC inline
void
Tb_entry_ipc_sfl::set(Context *tid, Mword eip,
		      L4_snd_desc snd_desc, L4_rcv_desc rcv_desc,
		      L4_timeout timeout, L4_uid dest,
		      Unsigned8 is_irq, Unsigned8 snd_lst,
		      Unsigned8 dst_ok, Unsigned8 dst_lck)
{
  set_global(TBUF_SHORTCUT_FAILED, tid, eip);
  _snd_desc   = snd_desc;
  _rcv_desc   = rcv_desc;
  _timeout    = timeout;
  _dest       = dest;
  _is_irq     = is_irq;
  _snd_lst    = snd_lst;
  _dst_ok     = dst_ok;
  _dst_lck    = dst_lck;
}

PUBLIC inline
L4_timeout
Tb_entry_ipc_sfl::timeout() const
{
  return _timeout;
}

PUBLIC inline
L4_snd_desc
Tb_entry_ipc_sfl::snd_desc() const
{
  return _snd_desc;
}

PUBLIC inline
L4_rcv_desc
Tb_entry_ipc_sfl::rcv_desc() const
{
  return _rcv_desc;
}

PUBLIC inline
L4_uid
Tb_entry_ipc_sfl::dest() const
{
  return _dest;
}

PUBLIC inline
Unsigned8
Tb_entry_ipc_sfl::is_irq() const
{
  return _is_irq;
}

PUBLIC inline
Unsigned8
Tb_entry_ipc_sfl::snd_lst() const
{
  return _snd_lst;
}

PUBLIC inline
Unsigned8
Tb_entry_ipc_sfl::dst_ok() const
{
  return _dst_ok;
}

PUBLIC inline
Unsigned8
Tb_entry_ipc_sfl::dst_lck() const
{
  return _dst_lck;
}

PUBLIC inline
void
Tb_entry_unmap::set(Context *tid, Mword eip, 
		    L4_fpage fpage, Mword mask, bool result)
{
  set_global(TBUF_UNMAP, tid, eip);
  _fpage  = fpage;
  _mask   = mask;
  _result = result;
}

PUBLIC inline
L4_fpage
Tb_entry_unmap::fpage() const
{
  return _fpage;
}

PUBLIC inline
Mword
Tb_entry_unmap::mask() const
{
  return _mask;
}

PUBLIC inline
bool
Tb_entry_unmap::result() const
{
  return _result;
}

PUBLIC inline
void
Tb_entry_ex_regs::set(Context *tid, Mword eip, Sys_ex_regs_frame *regs,
		      Mword old_esp, Mword old_eip)
{
  set_global(TBUF_EXREGS, tid, eip);
  _lthread = regs->lthread();
  _old_esp = old_esp; _new_esp = regs->sp();
  _old_eip = old_eip; _new_eip = regs->ip();
}

PUBLIC inline
Mword
Tb_entry_ex_regs::lthread() const
{
  return _lthread;
}

PUBLIC inline
Mword
Tb_entry_ex_regs::old_esp() const
{
  return _old_esp;
}

PUBLIC inline
Mword
Tb_entry_ex_regs::new_esp() const
{
  return _new_esp;
}

PUBLIC inline
Mword
Tb_entry_ex_regs::old_eip() const
{
  return _old_eip;
}

PUBLIC inline
Mword
Tb_entry_ex_regs::new_eip() const
{
  return _new_eip;
}



PUBLIC inline NEEDS [<flux/x86/base_trap.h>]
void
Tb_entry_trap::set(Context *tid, Mword eip, trap_state *ts)
{
  set_global(TBUF_TRAP, tid, eip);
  _trapno = ts->trapno; _errno = ts->err;
  _ebx = ts->ebx; _edx = ts->edx; _ecx = ts->ecx; _eax = ts->eax; 
  _cs  = (Unsigned16)ts->cs;  _ds = (Unsigned16)ts->ds;  
  _esp = ts->esp; _eflags = ts->eflags;
}

PUBLIC inline
void
Tb_entry_trap::set(Context *tid, Mword eip, Mword trapno)
{
  set_global(TBUF_TRAP, tid, eip);
  _trapno = trapno | 0x80;
}

PUBLIC inline
char
Tb_entry_trap::trapno() const
{
  return _trapno;
}

PUBLIC inline
Unsigned16
Tb_entry_trap::errno() const
{
  return _errno;
}

PUBLIC inline
Mword
Tb_entry_trap::eax() const
{
  return _eax;
}

PUBLIC inline
Mword
Tb_entry_trap::ebx() const
{
  return _ebx;
}

PUBLIC inline
Mword
Tb_entry_trap::ecx() const
{
  return _ecx;
}

PUBLIC inline
Mword
Tb_entry_trap::edx() const
{
  return _edx;
}

PUBLIC inline
Mword
Tb_entry_trap::ebp() const
{
  return _ebp;
}

PUBLIC inline
Unsigned16
Tb_entry_trap::cs() const
{
  return _cs;
}

PUBLIC inline
Unsigned16
Tb_entry_trap::ds() const
{
  return _ds;
}

PUBLIC inline
Mword
Tb_entry_trap::esp() const
{
  return _esp;
}

PUBLIC inline
Mword
Tb_entry_trap::eflags() const
{
  return _eflags;
}

PUBLIC inline
void
Tb_entry_sched::set (Context *tid, Mword eip, unsigned mode,
                     int ticks_left, unsigned short id,
		     unsigned short prio, unsigned short timeslice)
{
  set_global (TBUF_SCHED, tid, eip);
  _mode       = mode;
  _ticks_left = ticks_left;
  _id         = id;
  _prio       = prio;
  _timeslice  = timeslice;
}

PUBLIC inline
unsigned
Tb_entry_sched::mode()
{
  return _mode;
}

PUBLIC inline
int
Tb_entry_sched::ticks_left()
{
  return _ticks_left;
}

PUBLIC inline
unsigned short
Tb_entry_sched::id()
{
  return _id;
}

PUBLIC inline
unsigned short
Tb_entry_sched::prio()
{
  return _prio;
}

PUBLIC inline
unsigned short
Tb_entry_sched::timeslice()
{
  return _timeslice;
}
