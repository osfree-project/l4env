INTERFACE:

#include "cpu.h"
#include "tb_entry.h"
#include "trap_state.h"

class Jdb_entry_frame : public Trap_state
{};

class Jdb_output_frame : public Jdb_entry_frame
{};

class Jdb_status_page_frame : public Jdb_entry_frame
{};

class Jdb_log_frame : public Jdb_entry_frame
{};

class Jdb_log_3val_frame : public Jdb_log_frame
{};

class Jdb_debug_frame : public Jdb_entry_frame
{};

class Jdb_symbols_frame : public Jdb_debug_frame
{};

class Jdb_lines_frame : public Jdb_debug_frame
{};

class Jdb_get_cputime_frame : public Jdb_entry_frame
{};

class Jdb_thread_name_frame : public Jdb_entry_frame
{};

//---------------------------------------------------------------------------
IMPLEMENTATION[amd64]:

PUBLIC inline
Address_type
Jdb_entry_frame::from_user() const
{ return cs() & 3 ? ADDR_USER : ADDR_KERNEL; }

PUBLIC inline
Address
Jdb_entry_frame::ksp() const
{ return (Address)&_rsp; }

PUBLIC inline
Address
Jdb_entry_frame::sp() const
{ return from_user() ? _rsp : ksp(); }

PUBLIC inline
Mword
Jdb_entry_frame::param() const
{ return _rax; }

PUBLIC inline
Mword
Jdb_entry_frame::get_reg(unsigned reg) const
{
  Mword val = 0;

  switch (reg)
    {
    case 1:  val = _rax; break;
    case 2:  val = _rbx; break;
    case 3:  val = _rcx; break;
    case 4:  val = _rdx; break;
    case 5:  val = _rbp; break;
    case 6:  val = _rsi; break;
    case 7:  val = _rdi; break;
    case 8:  val = _r8;  break;
    case 9:  val = _r9;  break;
    case 10: val = _r10; break;
    case 11: val = _r11; break;
    case 12: val = _r12; break;
    case 13: val = _r13; break;
    case 14: val = _r14; break;
    case 15: val = _r15; break;
    case 16: val = _rip; break;
    case 17: val = _rsp; break;
    case 18: val = _rflags; break;
    }

  return val;
}

//---------------------------------------------------------------------------
IMPLEMENTATION[amd64]:

PUBLIC inline NEEDS["cpu.h"]
Mword
Jdb_entry_frame::ss() const
{ return from_user() ? _ss : Cpu::get_ss(); }

//---------------------------------------------------------------------------
IMPLEMENTATION[ux]:

PUBLIC
Mword
Jdb_entry_frame::ss() const
{ return _ss; }

//---------------------------------------------------------------------------
IMPLEMENTATION[amd64]:

PUBLIC inline
Unsigned8*
Jdb_output_frame::str() const
{ return (Unsigned8*)_rax; }

PUBLIC inline
int
Jdb_output_frame::len() const
{ return (unsigned)_rbx; }

//---------------------------------------------------------------------------
PUBLIC inline
void
Jdb_status_page_frame::set(Address status_page)
{ _rax = (Mword)status_page; }

//---------------------------------------------------------------------------
PUBLIC inline
Unsigned8*
Jdb_log_frame::str() const
{ return (Unsigned8*)_rdx; }

PUBLIC inline NEEDS["tb_entry.h"]
void
Jdb_log_frame::set_tb_entry(Tb_entry* tb_entry)
{ _rax = (Mword)tb_entry; }

//---------------------------------------------------------------------------
PUBLIC inline
Mword
Jdb_log_3val_frame::val1() const
{ return _rcx; }

PUBLIC inline
Mword
Jdb_log_3val_frame::val2() const
{ return _rsi; }

PUBLIC inline
Mword
Jdb_log_3val_frame::val3() const
{ return _rdi; }

//---------------------------------------------------------------------------
PUBLIC inline
Task_num
Jdb_debug_frame::task() const
{ return (Task_num)_rbx; }

PUBLIC inline
Mword
Jdb_debug_frame::addr() const
{ return _rax; }

PUBLIC inline
Mword
Jdb_debug_frame::size() const
{ return _rdx; }

//---------------------------------------------------------------------------
PUBLIC inline
void
Jdb_get_cputime_frame::invalidate()
{ _rcx = ~0UL; }

//---------------------------------------------------------------------------
PUBLIC inline
const char *
Jdb_thread_name_frame::name() const
{ return (const char*)_rax; }

//---------------------------------------------------------------------------
IMPLEMENTATION[v2]:

PUBLIC inline
void
Jdb_get_cputime_frame::set(L4_uid next, Cpu_time total_us, unsigned short prio)
{
  _rax = total_us;
  _rcx = prio;
  _rsi = next.raw(); 
}

// XXX dst is now only esi
PUBLIC inline
L4_uid
Jdb_get_cputime_frame::dst() const
{ return L4_uid(_rsi); }

PUBLIC inline
L4_uid
Jdb_thread_name_frame::dst() const
{ return L4_uid(_rsi); }

//---------------------------------------------------------------------------
IMPLEMENTATION[x0]:

PUBLIC inline
void
Jdb_get_cputime_frame::set(L4_uid next, Cpu_time total_us, unsigned short prio)
{
  _rax = total_us;
  _rcx = prio;
  _rsi = next.raw();
}

PUBLIC inline
L4_uid
Jdb_get_cputime_frame::dst() const
{ return L4_uid(_rsi); }

PUBLIC inline
L4_uid
Jdb_thread_name_frame::dst() const
{ return L4_uid(_rsi); }
