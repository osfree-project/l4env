INTERFACE:

#include "initcalls.h"

enum {
  Tbuf_unused             = 0,
  Tbuf_pf                 = 1,
  Tbuf_ipc                = 2,
  Tbuf_ipc_res            = 3,
  Tbuf_ipc_trace          = 4,
  Tbuf_ke                 = 5,
  Tbuf_ke_reg             = 6,
  Tbuf_unmap              = 7,
  Tbuf_shortcut_failed    = 8,
  Tbuf_shortcut_succeeded = 9,
  Tbuf_context_switch     = 10,
  Tbuf_exregs             = 11,
  Tbuf_breakpoint         = 12,
  Tbuf_trap               = 13,
  Tbuf_pf_res             = 14,
  Tbuf_sched              = 15,
  Tbuf_preemption         = 16,
  Tbuf_id_nearest         = 17,
  Tbuf_jean1              = 18,
  Tbuf_task_new           = 19,
  Tbuf_max                = 32,
  Tbuf_hidden             = 0x20,
};

#include "l4_types.h"

class Context;
class Space;
class Sched_context;
class Sys_ipc_frame;
class Sys_ex_regs_frame;
class Sys_task_new_frame;
class Trap_state;

class Tb_entry
{
protected:
  Mword		_number;	///< event number
  Address	_ip;		///< instruction pointer
  Context	*_ctx;		///< Context
  Unsigned64	_tsc;		///< time stamp counter
  Unsigned32	_pmc1;		///< performance counter value 1
  Unsigned32	_pmc2;		///< performance counter value 2
  Unsigned32	_kclock;	///< lower 32 bits of kernel clock 
  Unsigned8	_type;		///< type of entry
  static Mword (*rdcnt1)();
  static Mword (*rdcnt2)();
} __attribute__((packed));

class Tb_entry_fit : public Tb_entry
{
private:
  char		_reserved[Tb_entry_size-sizeof(Tb_entry)];
};

/** logged ipc. */
class Tb_entry_ipc : public Tb_entry
{
private:
  L4_snd_desc	_snd_desc;	///< ipc send descriptor
  L4_rcv_desc	_rcv_desc;	///< ipc receive descriptor
  Mword		_dword[2]; 	///< first two message words
  Global_id	_dst;		///< destination id
  Mword		_flags;         ///< flags
  L4_timeout_pair	_timeout;	///< timeout
};

/** logged ipc result. */
class Tb_entry_ipc_res : public Tb_entry
{
private:
  Unsigned8	_have_snd;	///< ipc had send part
  Unsigned8	_is_np;		///< next period bit set
  Mword		_dword[2];	///< first two dwords
  L4_msgdope	_result;	///< result
  Global_id	_rcv_src;	///< partner
  L4_rcv_desc	_rcv_desc;	///< receive descriptor
  Mword		_pair_event;	///< referred event
};

/** logged ipc for user level tracing with Vampir. */
class Tb_entry_ipc_trace : public Tb_entry
{
private:
  Unsigned8	_snd_desc;
  Unsigned8	_rcv_desc;
  L4_msgdope	_result;	///< result
  Unsigned64	_snd_tsc;	///< entry tsc
  Global_id	_snd_dst;	///< send destination
  Global_id	_rcv_dst;	///< rcv partner
};

/** logged short-cut ipc failed. */
class Tb_entry_ipc_sfl : public Tb_entry
{
private:
  L4_snd_desc	_snd_desc;	///< short ipc send descriptor
  L4_rcv_desc	_rcv_desc;	///< short ipc rcv descriptor
  L4_timeout_pair	_timeout;	///< ipc timeout
  Global_id	_dst;		///< partner
  Unsigned8	_is_irq, _snd_lst, _dst_ok, _dst_lck, _preempt;
};

/** logged pagefault. */
class Tb_entry_pf : public Tb_entry
{
private:
  Address	_pfa;		///< pagefault address
  Mword		_error;		///< pagefault error code
  Space		*_space;
};

/** pagefault result. */
class Tb_entry_pf_res : public Tb_entry
{
private:
  Address	_pfa;
  Ipc_err	_err;
  Ipc_err	_ret;
};

/** logged kernel event. */
class Tb_entry_ke : public Tb_entry
{
private:
  char		_msg[31];	///< debug message
};

/** logged unmap operation. */
class Tb_entry_unmap : public Tb_entry
{
private:
  L4_fpage	_fpage;		///< flexpage to unmap
  Mword		_mask;		///< mask
  bool		_result;	///< result
};

/** logged thread_ex_regs operation. */
class Tb_entry_ex_regs : public Tb_entry
{
private:
  Mword		_lthread;	///< local thread number
  Mword		_task;		///< local thread number
  Address	_old_sp;	///< old stack pointer
  Address	_new_sp;	///< new stack pointer
  Address	_old_ip;	///< old instruction pointer
  Address	_new_ip;	///< new instruction pointer
  Mword		_failed;	///< inter-task ex-regs failed?
};

/** logged id_nearest operation. */
class Tb_entry_id_nearest : public Tb_entry
{
private:
  Global_id	_dst;
};

/** logged task_new operation. */
class Tb_entry_task_new : public Tb_entry
{
private:
  Global_id	_task;
  Global_id	_pager;
  Address	_new_sp;
  Address	_new_ip;
  Mword		_mcp_or_chief;
};

/** logged breakpoint. */
class Tb_entry_bp : public Tb_entry
{
private:
  Address	_address;	///< breakpoint address
  int		_len;		///< breakpoint length
  Mword		_value;		///< value at address
  int		_mode;		///< breakpoint mode
};

/** logged context switch. */
class Tb_entry_ctx_sw : public Tb_entry
{
private:
  Context	*_dst;		///< switcher target
  Context	*_dst_orig;
  Address	_kernel_ip;
  Mword		_lock_cnt;
  Space	*_from_space;
  Sched_context *_from_sched;
  Mword		_from_prio;
};

/** logged scheduling event. */
class Tb_entry_sched : public Tb_entry
{
private:
  unsigned short _mode;
  Context 	 *_owner;
  unsigned short _id;
  unsigned short _prio;
  signed long	 _left;
  unsigned long  _quantum;
};

/** logged preemption */
class Tb_entry_preemption : public Tb_entry
{
private:
  Context	 *_preempter;
};

class Tb_entry_jean1 : public Tb_entry
{
private:
  Context	*_sched_owner1;
  Context	*_sched_owner2;
};


IMPLEMENTATION:

#include <cstring>
#include <cstdarg>

#include "entry_frame.h"
#include "cpu.h"
#include "kip.h"
#include "static_init.h"
#include "trap_state.h"


extern "C" void _wrong_sizeof_tb_entry_fit	 (void);
extern "C" void _wrong_sizeof_tb_entry_ipc	 (void);
extern "C" void _wrong_sizeof_tb_entry_ipc_res	 (void);
extern "C" void _wrong_sizeof_tb_entry_ipc_trace (void);
extern "C" void _wrong_sizeof_tb_entry_pf	 (void);
extern "C" void _wrong_sizeof_tb_entry_pf_res	 (void);
extern "C" void _wrong_sizeof_tb_entry_ke	 (void);
extern "C" void _wrong_sizeof_tb_entry_unmap	 (void);
extern "C" void _wrong_sizeof_tb_entry_ex_regs	 (void);
extern "C" void _wrong_sizeof_tb_entry_bp	 (void);
extern "C" void _wrong_sizeof_tb_entry_ipc_sfl	 (void);
extern "C" void _wrong_sizeof_tb_entry_ctx_sw	 (void);
extern "C" void _wrong_sizeof_tb_entry_sched	 (void);
extern "C" void _wrong_sizeof_tb_entry_preemption(void);
extern "C" void _wrong_sizeof_tb_entry_id_nearest(void);
extern "C" void _wrong_sizeof_tb_entry_jean1     (void);
extern "C" void _wrong_sizeof_tb_entry_task_new  (void);

STATIC_INITIALIZE(Tb_entry_fit);

PRIVATE static Mword Tb_entry::dummy_read_pmc() { return 0; }

Mword (*Tb_entry::rdcnt1)() = dummy_read_pmc;
Mword (*Tb_entry::rdcnt2)() = dummy_read_pmc;

PUBLIC static FIASCO_INIT
void
Tb_entry_fit::init()
{
  // ensure several assertions
  if (sizeof(Tb_entry_fit)	 != Tb_entry_size)
    _wrong_sizeof_tb_entry_fit();
  if (sizeof(Tb_entry_ipc)	  > Tb_entry_size)
    _wrong_sizeof_tb_entry_ipc();
  if (sizeof(Tb_entry_ipc_res)	  > Tb_entry_size)
    _wrong_sizeof_tb_entry_ipc_res();
  if (sizeof(Tb_entry_ipc_trace)  > Tb_entry_size)
    _wrong_sizeof_tb_entry_ipc_trace();
  if (sizeof(Tb_entry_pf)	  > Tb_entry_size)
    _wrong_sizeof_tb_entry_pf();
  if (sizeof(Tb_entry_pf_res)	  > Tb_entry_size)
    _wrong_sizeof_tb_entry_pf_res();
  if (sizeof(Tb_entry_ke)	  > Tb_entry_size)
    _wrong_sizeof_tb_entry_ke();
  if (sizeof(Tb_entry_unmap)	  > Tb_entry_size)
    _wrong_sizeof_tb_entry_unmap();
  if (sizeof(Tb_entry_ex_regs)	  > Tb_entry_size)
    _wrong_sizeof_tb_entry_ex_regs();
  if (sizeof(Tb_entry_bp)	  > Tb_entry_size)
    _wrong_sizeof_tb_entry_bp();
  if (sizeof(Tb_entry_ipc_sfl)	  > Tb_entry_size)
    _wrong_sizeof_tb_entry_ipc_sfl();
  if (sizeof(Tb_entry_ctx_sw)	  > Tb_entry_size)
    _wrong_sizeof_tb_entry_ctx_sw();
  if (sizeof(Tb_entry_sched)	  > Tb_entry_size)
    _wrong_sizeof_tb_entry_sched();
  if (sizeof(Tb_entry_preemption) > Tb_entry_size)
    _wrong_sizeof_tb_entry_preemption();
  if (sizeof(Tb_entry_id_nearest) > Tb_entry_size)
    _wrong_sizeof_tb_entry_id_nearest();
  if (sizeof(Tb_entry_jean1)      > Tb_entry_size)
    _wrong_sizeof_tb_entry_jean1();
  if (sizeof(Tb_entry_task_new)   > Tb_entry_size)
    _wrong_sizeof_tb_entry_task_new();
  init_arch();
}

PUBLIC static
void
Tb_entry::set_rdcnt(int num, Mword (*f)())
{
  if (!f)
    f = dummy_read_pmc;

  switch (num)
    {
    case 0: rdcnt1 = f; break;
    case 1: rdcnt2 = f; break;
    }
}

PUBLIC inline
void
Tb_entry::clear()
{ _type = Tbuf_unused; }

PROTECTED inline NEEDS["kip.h"]
void
Tb_entry::set_global(char type, Context *ctx, Address ip)
{
  _type   = type;
  _ctx    = ctx;
  _ip     = ip;
  _kclock = (Unsigned32)Kip::k()->clock;
}

PUBLIC inline
void
Tb_entry::hide()
{ _type |= Tbuf_hidden; }

PUBLIC inline
void
Tb_entry::unhide()
{ _type &= ~Tbuf_hidden; }

PUBLIC inline
Address
Tb_entry::ip() const
{ return _ip; }

PUBLIC inline
Context*
Tb_entry::ctx() const
{ return _ctx; }

PUBLIC inline
Unsigned8
Tb_entry::type() const
{ return _type & (Tbuf_max-1); }

PUBLIC inline
int
Tb_entry::hidden() const
{ return _type & Tbuf_hidden; }

PUBLIC inline
Mword
Tb_entry::number() const
{ return _number; }

PUBLIC inline
void
Tb_entry::number(Mword number)
{ _number = number; }

PUBLIC inline
void
Tb_entry::rdpmc1()
{ _pmc1 = rdcnt1(); }

PUBLIC inline
void
Tb_entry::rdpmc2()
{ _pmc2 = rdcnt2(); }

PUBLIC inline
Unsigned32
Tb_entry::kclock() const
{ return _kclock; }

PUBLIC inline
Unsigned64
Tb_entry::tsc() const
{ return _tsc; }

PUBLIC inline
Unsigned32
Tb_entry::pmc1() const
{ return _pmc1; }

PUBLIC inline
Unsigned32
Tb_entry::pmc2() const
{ return _pmc2; }


PUBLIC inline NEEDS ["entry_frame.h"]
void
Tb_entry_ipc::set(Context *ctx, Mword ip, Sys_ipc_frame *ipc_regs,
		  Unsigned64 left)
{
  set_global(Tbuf_ipc, ctx, ip);
  _snd_desc  = ipc_regs->snd_desc();
  _rcv_desc  = ipc_regs->rcv_desc();
  _dst       = ipc_regs->snd_dst();
  _timeout   = ipc_regs->timeout();
  if (ipc_regs->next_period())
    {
      _dword[0]  = (Unsigned32)(left & 0xffffffff);
      _dword[1]  = (Unsigned32)(left >> 32);
    }
  else
    {
      // hint for gcc
      register Mword tmp0 = ipc_regs->msg_word(0);
      register Mword tmp1 = ipc_regs->msg_word(1);
      _dword[0]  = tmp0;
      _dword[1]  = tmp1;
    }
}

PUBLIC inline
void
Tb_entry_ipc::set_irq(Context *ctx, Mword ip, Mword irq)
{
  set_global(Tbuf_ipc, ctx, ip);
  _snd_desc = L4_snd_desc(0);
  _rcv_desc = L4_rcv_desc(0);
  _dst      = Global_id::irq(irq);
}

PUBLIC inline
void
Tb_entry_ipc::set_sc(Context *ctx, Mword ip, Sys_ipc_frame *ipc_regs,
		     Unsigned64 left)
{
  set_global(Tbuf_shortcut_succeeded, ctx, ip);
  _snd_desc  = ipc_regs->snd_desc();
  _rcv_desc  = ipc_regs->rcv_desc();
  _dst       = ipc_regs->snd_dst();
  _timeout   = ipc_regs->timeout();
  if (ipc_regs->next_period())
    {
      _dword[0]  = (Unsigned32)(left & 0xffffffff);
      _dword[1]  = (Unsigned32)(left >> 32);
    }
  else
    {
      // hint for gcc
      register Mword tmp0 = ipc_regs->msg_word(0);
      register Mword tmp1 = ipc_regs->msg_word(1);
      _dword[0]  = tmp0;
      _dword[1]  = tmp1;
    }
}

PUBLIC inline
L4_snd_desc
Tb_entry_ipc::snd_desc() const
{ return _snd_desc; }

PUBLIC inline
L4_rcv_desc
Tb_entry_ipc::rcv_desc() const
{ return _rcv_desc; }

PUBLIC inline
Global_id
Tb_entry_ipc::dst() const
{ return _dst; }

PUBLIC inline
L4_timeout_pair
Tb_entry_ipc::timeout() const
{ return _timeout; }

PUBLIC inline
Mword
Tb_entry_ipc::dword(unsigned index) const
{ return _dword[index]; }


PUBLIC inline NEEDS ["entry_frame.h"]
void
Tb_entry_ipc_res::set(Context *ctx, Mword ip, Sys_ipc_frame *ipc_regs,
		      Mword result, Mword pair_event, Unsigned8 have_snd,
		      Unsigned8 is_np)
{
  set_global(Tbuf_ipc_res, ctx, ip);
  // hint for gcc
  register Mword tmp0 = ipc_regs->msg_word(0);
  register Mword tmp1 = ipc_regs->msg_word(1);
  _dword[0]   = tmp0;
  _dword[1]   = tmp1;
  _pair_event = pair_event;
  _result     = result;
  _rcv_desc   = ipc_regs->rcv_desc();
  _rcv_src    = ipc_regs->rcv_src();
  _have_snd   = have_snd;
  _is_np      = is_np;
}

PUBLIC inline
int
Tb_entry_ipc_res::have_snd() const
{ return _have_snd; }

PUBLIC inline
int
Tb_entry_ipc_res::is_np() const
{ return _is_np; }

PUBLIC inline
L4_rcv_desc
Tb_entry_ipc_res::rcv_desc() const
{ return _rcv_desc; }

PUBLIC inline
L4_msgdope
Tb_entry_ipc_res::result() const
{ return _result; }

PUBLIC inline
Global_id
Tb_entry_ipc_res::rcv_src() const
{ return _rcv_src; }

PUBLIC inline
Mword
Tb_entry_ipc_res::dword(unsigned index) const
{ return _dword[index]; }

PUBLIC inline
Mword
Tb_entry_ipc_res::pair_event() const
{ return _pair_event; }


PUBLIC inline
void
Tb_entry_ipc_trace::set(Context *ctx, Mword ip, Unsigned64 snd_tsc,
			Global_id snd_dst, Global_id rcv_dst,
			L4_msgdope result, Unsigned8 snd_desc,
			Unsigned8 rcv_desc)
{
  set_global(Tbuf_ipc_trace, ctx, ip);
  _snd_tsc  = snd_tsc;
  _snd_dst  = snd_dst;
  _rcv_dst  = rcv_dst;
  _result   = result;
  _snd_desc = snd_desc;
  _rcv_desc = rcv_desc;
}


PUBLIC inline
void
Tb_entry_ipc_sfl::set(Context *ctx, Mword ip,
		      L4_snd_desc snd_desc, L4_rcv_desc rcv_desc,
		      L4_timeout_pair timeout, Global_id dst,
		      Unsigned8 is_irq, Unsigned8 snd_lst,
		      Unsigned8 dst_ok, Unsigned8 dst_lck,
		      Unsigned8 preempt)
{
  set_global(Tbuf_shortcut_failed, ctx, ip);
  _snd_desc  = snd_desc;
  _rcv_desc  = rcv_desc;
  _timeout   = timeout;
  _dst       = dst;
  _is_irq    = is_irq;
  _snd_lst   = snd_lst;
  _dst_ok    = dst_ok;
  _dst_lck   = dst_lck;
  _preempt   = preempt;
}

PUBLIC inline
L4_timeout_pair
Tb_entry_ipc_sfl::timeout() const
{ return _timeout; }

PUBLIC inline
L4_snd_desc
Tb_entry_ipc_sfl::snd_desc() const
{ return _snd_desc; }

PUBLIC inline
L4_rcv_desc
Tb_entry_ipc_sfl::rcv_desc() const
{ return _rcv_desc; }

PUBLIC inline
Global_id
Tb_entry_ipc_sfl::dst() const
{ return _dst; }

PUBLIC inline
Unsigned8
Tb_entry_ipc_sfl::is_irq() const
{ return _is_irq; }

PUBLIC inline
Unsigned8
Tb_entry_ipc_sfl::snd_lst() const
{ return _snd_lst; }

PUBLIC inline
Unsigned8
Tb_entry_ipc_sfl::dst_ok() const
{ return _dst_ok; }

PUBLIC inline
Unsigned8
Tb_entry_ipc_sfl::dst_lck() const
{ return _dst_lck; }

PUBLIC inline
Unsigned8
Tb_entry_ipc_sfl::preempt() const
{ return _preempt; }


PUBLIC inline
void
Tb_entry_pf::set(Context *ctx, Address ip, Address pfa,
		 Mword error, Space *spc)
{
  set_global(Tbuf_pf, ctx, ip);
  _pfa   = pfa;
  _error = error;
  _space = spc;
}

PUBLIC inline
Mword
Tb_entry_pf::error() const
{ return _error; }

PUBLIC inline
Address
Tb_entry_pf::pfa() const
{ return _pfa; }

PUBLIC inline
Space*
Tb_entry_pf::space() const
{ return _space; }


PUBLIC inline
void
Tb_entry_pf_res::set(Context *ctx, Address ip, Address pfa, 
		     Ipc_err err, Ipc_err ret)
{
  set_global(Tbuf_pf_res, ctx, ip);
  _pfa = pfa;
  _err = err;
  _ret = ret;
}

PUBLIC inline
Address
Tb_entry_pf_res::pfa() const
{ return _pfa; }

PUBLIC inline
Ipc_err
Tb_entry_pf_res::err() const
{ return _err; }

PUBLIC inline
Ipc_err
Tb_entry_pf_res::ret() const
{ return _ret; }


PUBLIC inline
void
Tb_entry_bp::set(Context *ctx, Address ip,
		 int mode, int len, Mword value, Address address)
{
  set_global(Tbuf_breakpoint, ctx, ip);
  _mode    = mode;
  _len     = len;
  _value   = value;
  _address = address;
}



PUBLIC inline
int
Tb_entry_bp::mode() const
{ return _mode; }

PUBLIC inline
int
Tb_entry_bp::len() const
{ return _len; }

PUBLIC inline
Mword
Tb_entry_bp::value() const
{ return _value; }

PUBLIC inline
Address
Tb_entry_bp::addr() const
{ return _address; }



PUBLIC inline
void
Tb_entry_ke::set(Context *ctx, Address ip)
{ set_global(Tbuf_ke, ctx, ip); }

PUBLIC inline
void
Tb_entry_ke::set_const(Context *ctx, Address ip, const char * const msg)
{
  set_global(Tbuf_ke, ctx, ip);
  _msg[0] = 0; _msg[1] = 1;
  *(char const ** const)(_msg + 3) = msg;
}

PUBLIC inline
void
Tb_entry_ke::set_buf(unsigned i, char c)
{
  if (i < sizeof(_msg)-1)
    _msg[i] = c >= ' ' ? c : '.';
}

PUBLIC inline
void
Tb_entry_ke::term_buf(unsigned i)
{ _msg[i < sizeof(_msg)-1 ? i : sizeof(_msg)-1] = '\0'; }

PUBLIC inline
const char *
Tb_entry_ke::msg() const
{
  return _msg[0] == 0 && _msg[1] == 1
    ? *(char const ** const)(_msg + 3) : _msg;
}


PUBLIC inline
void
Tb_entry_ctx_sw::set(Context *ctx, Space *from_space, Address ip,
		     Context *dst, Context *dst_orig, Mword lock_cnt,
		     Sched_context *from_sched, Mword from_prio,
		     Address kernel_ip)
{
  set_global(Tbuf_context_switch, ctx, ip);
  _kernel_ip = kernel_ip;
  _dst        = dst;
  _dst_orig   = dst_orig;
  _lock_cnt   = lock_cnt;
  _from_space = from_space;
  _from_sched = from_sched;
  _from_prio  = from_prio;
}

PUBLIC inline
Space*
Tb_entry_ctx_sw::from_space() const
{ return _from_space; }

PUBLIC inline
Address
Tb_entry_ctx_sw::kernel_ip() const
{ return _kernel_ip; }

PUBLIC inline
Mword
Tb_entry_ctx_sw::lock_cnt() const
{ return _lock_cnt; }

PUBLIC inline
Context*
Tb_entry_ctx_sw::dst() const
{ return _dst; }

PUBLIC inline
Context*
Tb_entry_ctx_sw::dst_orig() const
{ return _dst_orig; }

PUBLIC inline
Mword
Tb_entry_ctx_sw::from_prio() const
{ return _from_prio; }

PUBLIC inline
Sched_context*
Tb_entry_ctx_sw::from_sched() const
{ return _from_sched; }


PUBLIC inline
void
Tb_entry_unmap::set(Context *ctx, Address ip, 
		    L4_fpage fpage, Mword mask, bool result)
{
  set_global(Tbuf_unmap, ctx, ip);
  _fpage  = fpage;
  _mask   = mask;
  _result = result;
}

PUBLIC inline
L4_fpage
Tb_entry_unmap::fpage() const
{ return _fpage; }

PUBLIC inline
Mword
Tb_entry_unmap::mask() const
{ return _mask; }

PUBLIC inline
bool
Tb_entry_unmap::result() const
{ return _result; }

PUBLIC inline NEEDS ["entry_frame.h"]
void
Tb_entry_ex_regs::set(Context *ctx, Address ip, Sys_ex_regs_frame *regs,
		      Address old_sp, Address old_ip, Mword failed)
{
  set_global(Tbuf_exregs, ctx, ip);
  _lthread = regs->lthread();
  _task    = regs->task();
  _old_sp  = old_sp; _new_sp = regs->sp();
  _old_ip  = old_ip; _new_ip = regs->ip();
  _failed  = failed;
}

PUBLIC inline
Mword
Tb_entry_ex_regs::lthread() const
{ return _lthread; }

PUBLIC inline
Mword
Tb_entry_ex_regs::task() const
{ return _task; }

PUBLIC inline
Address
Tb_entry_ex_regs::old_sp() const
{ return _old_sp; }

PUBLIC inline
Address
Tb_entry_ex_regs::new_sp() const
{ return _new_sp; }

PUBLIC inline
Address
Tb_entry_ex_regs::old_ip() const
{ return _old_ip; }

PUBLIC inline
Address
Tb_entry_ex_regs::new_ip() const
{ return _new_ip; }

PUBLIC inline
Mword
Tb_entry_ex_regs::failed() const
{ return _failed; }

PUBLIC inline
void
Tb_entry_id_nearest::set (Context *ctx, Address ip, Global_id dst)
{
  set_global (Tbuf_id_nearest, ctx, ip);
  _dst = dst;
}

PUBLIC inline
Global_id
Tb_entry_id_nearest::dst()
{ return _dst; }

PUBLIC inline
void
Tb_entry_task_new::set (Context *ctx, Address ip, Sys_task_new_frame *regs)
{
  set_global (Tbuf_task_new, ctx, ip);
  _task         = regs->dst();
  _pager        = regs->pager();
  _new_sp      = regs->sp();
  _new_ip      = regs->ip();
  _mcp_or_chief = regs->mcp();
}

PUBLIC inline
Global_id
Tb_entry_task_new::task()
{ return _task; }

PUBLIC inline
Global_id
Tb_entry_task_new::pager()
{ return _pager; }

PUBLIC inline
Address
Tb_entry_task_new::new_sp()
{ return _new_sp; }

PUBLIC inline
Address
Tb_entry_task_new::new_ip()
{ return _new_ip; }

PUBLIC inline
Mword
Tb_entry_task_new::mcp_or_chief()
{ return _mcp_or_chief; }


PUBLIC inline
void
Tb_entry_sched::set (Context *ctx, Address ip, unsigned short mode,
                     Context *owner, unsigned short id, unsigned short prio,
                     signed long left, unsigned long quantum)
{
  set_global (Tbuf_sched, ctx, ip);
  _mode    = mode;
  _owner   = owner;
  _id      = id;
  _prio    = prio;
  _left    = left;
  _quantum = quantum;
}

PUBLIC inline
unsigned short
Tb_entry_sched::mode() const
{ return _mode; }

PUBLIC inline
Context *
Tb_entry_sched::owner() const
{ return _owner; }

PUBLIC inline
unsigned short
Tb_entry_sched::id() const
{ return _id; }

PUBLIC inline
unsigned short
Tb_entry_sched::prio() const
{ return _prio; }

PUBLIC inline
unsigned long
Tb_entry_sched::quantum() const
{ return _quantum; }

PUBLIC inline
signed long
Tb_entry_sched::left() const
{ return _left; }


PUBLIC inline
void
Tb_entry_preemption::set (Context *ctx, Context *preempter, Address ip)
{
  set_global (Tbuf_preemption, ctx, ip);
  _preempter = preempter;
};

PUBLIC inline
Context*
Tb_entry_preemption::preempter() const
{ return _preempter; }


PUBLIC inline
void
Tb_entry_jean1::set (Context *tid, Address ip,
		     Context *sched_owner1, Context *sched_owner2)
{
  set_global(Tbuf_jean1, tid, ip);
  _sched_owner1 = sched_owner1;
  _sched_owner2 = sched_owner2;
}

PUBLIC inline
Context*
Tb_entry_jean1::sched_owner1()
{ return _sched_owner1; }

PUBLIC inline
Context*
Tb_entry_jean1::sched_owner2()
{ return _sched_owner2; }
