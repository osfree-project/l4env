INTERFACE [amd64]:

EXTENSION class Tb_entry
{
public:
  enum
  {
    Tb_entry_size = 128,
  };
};

/** logged kernel event plus register content. */
class Tb_entry_ke_reg : public Tb_entry
{
private:
  char		_msg[19];	///< debug message
  Mword		_rax, _rcx, _rdx; ///< registers
};

/** logged trap. */
class Tb_entry_trap : public Tb_entry
{
private:
  char		_trapno;
  Unsigned16	_error;
  Mword		_rbp, _rdx, _cr2, _rax, _rflags, _rsp;
  Unsigned16	_cs,  _ds;
};

IMPLEMENTATION [amd64]:

#include "cpu.h"

extern "C" void _wrong_sizeof_tb_entry_trap	(void);
extern "C" void _wrong_sizeof_tb_entry_ke_reg	(void);

PUBLIC static FIASCO_INIT
void
Tb_entry_fit::init_arch()
{
  // ensure several assertions
  if (sizeof(Tb_entry_ke_reg)	 > Tb_entry_size)
    _wrong_sizeof_tb_entry_ke_reg();
  if (sizeof(Tb_entry_trap)	 > Tb_entry_size)
    _wrong_sizeof_tb_entry_trap();
}


PUBLIC inline NEEDS ["cpu.h"]
void
Tb_entry::rdtsc()
{ _tsc = Cpu::rdtsc(); }



PUBLIC inline
void
Tb_entry_ke_reg::set(Context *ctx, Mword rip, Mword v1, Mword v2, Mword v3)
{
  set_global(Tbuf_ke_reg, ctx, rip);
  _rax = v1; _rcx = v2; _rdx = v3;
}

PUBLIC inline NEEDS [<cstring>]
void
Tb_entry_ke_reg::set(Context *ctx, Mword rip, Trap_state *ts)
{ set(ctx, rip, ts->_rax, ts->_rcx, ts->_rdx); }

PUBLIC inline
void
Tb_entry_ke_reg::set_const(Context *ctx, Mword rip, const char * const msg,
			   Mword rax, Mword rcx, Mword rdx)
{
  set(ctx, rip, rax, rcx, rdx);
  _msg[0] = 0; _msg[1] = 1;
  *(char const ** const)(_msg + 3) = msg;
}

PUBLIC inline
void
Tb_entry_ke_reg::set_buf(unsigned i, char c)
{
  if (i < sizeof(_msg)-1)
    _msg[i] = c >= ' ' ? c : '.';
}

PUBLIC inline
void
Tb_entry_ke_reg::term_buf(unsigned i)
{ _msg[i < sizeof(_msg)-1 ? i : sizeof(_msg)-1] = '\0'; }

PUBLIC inline
const char *
Tb_entry_ke_reg::msg() const
{ 
  return _msg[0] == 0 && _msg[1] == 1
    ? *(char const ** const)(_msg + 3) : _msg;
}

PUBLIC inline
Mword
Tb_entry_ke_reg::val1() const
{ return _rax; }

PUBLIC inline
Mword
Tb_entry_ke_reg::val2() const
{ return _rcx; }

PUBLIC inline
Mword
Tb_entry_ke_reg::val3() const
{ return _rdx; }


PUBLIC inline NEEDS ["trap_state.h"]
void
Tb_entry_trap::set(Context *ctx, Mword rip, Trap_state *ts)
{
  set_global(Tbuf_trap, ctx, rip);
  _trapno = ts->_trapno;
  _error  = ts->_err;
  _rdx    = ts->_rdx;
  _cr2    = ts->_cr2;
  _rax    = ts->_rax; 
  _cs     = (Unsigned16)ts->cs();
  _rsp    = ts->sp();
  _rflags = ts->flags();
}

PUBLIC inline
void
Tb_entry_trap::set(Context *ctx, Mword eip, Mword trapno)
{
  set_global(Tbuf_trap, ctx, eip);
  _trapno = trapno | 0x80;
}

PUBLIC inline
char
Tb_entry_trap::trapno() const
{ return _trapno; }

PUBLIC inline
Unsigned16
Tb_entry_trap::error() const
{ return _error; }

PUBLIC inline
Mword
Tb_entry_trap::eax() const
{ return _rax; }

PUBLIC inline
Mword
Tb_entry_trap::cr2() const
{ return _cr2; }

PUBLIC inline
Mword
Tb_entry_trap::edx() const
{ return _rdx; }

PUBLIC inline
Mword
Tb_entry_trap::ebp() const
{ return _rbp; }

PUBLIC inline
Unsigned16
Tb_entry_trap::cs() const
{ return _cs; }

PUBLIC inline
Unsigned16
Tb_entry_trap::ds() const
{ return _ds; }

PUBLIC inline
Mword
Tb_entry_trap::sp() const
{ return _rsp; }

PUBLIC inline
Mword
Tb_entry_trap::flags() const
{ return _rflags; }


