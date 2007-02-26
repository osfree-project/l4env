INTERFACE [ia32,ux]:

EXTENSION class Tb_entry
{
public:
  enum
  {
    Tb_entry_size = 64,
  };
};

/** logged kernel event plus register content. */
class Tb_entry_ke_reg : public Tb_entry
{
private:
  char		_msg[19];	///< debug message
  Mword		_eax, _ecx, _edx; ///< registers
};

/** logged trap. */
class Tb_entry_trap : public Tb_entry
{
private:
  char		_trapno;
  Unsigned16	_errno;
  Mword		_ebp, _edx, _cr2, _eax, _eflags, _esp;
  Unsigned16	_cs,  _ds;
};

IMPLEMENTATION [ia32,ux]:

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
Tb_entry_ke_reg::set(Context *ctx, Mword eip, Mword v1, Mword v2, Mword v3)
{
  set_global(Tbuf_ke_reg, ctx, eip);
  _eax = v1; _ecx = v2; _edx = v3;
}

PUBLIC inline NEEDS [<cstring>]
void
Tb_entry_ke_reg::set(Context *ctx, Mword eip, Trap_state *ts)
{ set(ctx, eip, ts->eax, ts->ecx, ts->edx); }

PUBLIC inline
void
Tb_entry_ke_reg::set_const(Context *ctx, Mword eip, const char * const msg,
			   Mword eax, Mword ecx, Mword edx)
{
  set(ctx, eip, eax, ecx, edx);
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
{ return _eax; }

PUBLIC inline
Mword
Tb_entry_ke_reg::val2() const
{ return _ecx; }

PUBLIC inline
Mword
Tb_entry_ke_reg::val3() const
{ return _edx; }


PUBLIC inline NEEDS ["trap_state.h"]
void
Tb_entry_trap::set(Context *ctx, Mword eip, Trap_state *ts)
{
  set_global(Tbuf_trap, ctx, eip);
  _trapno = ts->trapno;
  _errno  = ts->err;
  _edx    = ts->edx;
  _cr2    = ts->cr2;
  _eax    = ts->eax; 
  _cs     = (Unsigned16)ts->cs;
  _ds     = (Unsigned16)ts->ds;  
  _esp    = ts->esp;
  _eflags = ts->eflags;
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
Tb_entry_trap::errno() const
{ return _errno; }

PUBLIC inline
Mword
Tb_entry_trap::eax() const
{ return _eax; }

PUBLIC inline
Mword
Tb_entry_trap::cr2() const
{ return _cr2; }

PUBLIC inline
Mword
Tb_entry_trap::edx() const
{ return _edx; }

PUBLIC inline
Mword
Tb_entry_trap::ebp() const
{ return _ebp; }

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
{ return _esp; }

PUBLIC inline
Mword
Tb_entry_trap::flags() const
{ return _eflags; }


