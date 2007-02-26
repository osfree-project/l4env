INTERFACE:

#include "cpu.h"
#include "trap_state.h"
#include "tb_entry.h"

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
IMPLEMENTATION[ia32,ux]:

PUBLIC inline
Address_type
Jdb_entry_frame::from_user() const
{ return cs() & 3 ? ADDR_USER : ADDR_KERNEL; }

PUBLIC inline
Address
Jdb_entry_frame::ksp() const
{ return (Address)&_esp; }

PUBLIC inline
Address
Jdb_entry_frame::sp() const
{ return from_user() ? _esp : ksp(); }

PUBLIC inline
Mword
Jdb_entry_frame::param() const
{ return _eax; }

PUBLIC inline
Mword
Jdb_entry_frame::get_reg(unsigned reg) const
{
  Mword val = 0;

  switch (reg)
    {
    case 1:  val = _eax; break;
    case 2:  val = _ebx; break;
    case 3:  val = _ecx; break;
    case 4:  val = _edx; break;
    case 5:  val = _ebp; break;
    case 6:  val = _esi; break;
    case 7:  val = _edi; break;
    case 8:  val = _eip; break;
    case 9:  val = _esp; break;
    case 10: val = _eflags; break;
    }

  return val;
}

//---------------------------------------------------------------------------
IMPLEMENTATION[ia32]:

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
IMPLEMENTATION[ia32,ux]:

PUBLIC inline
Unsigned8*
Jdb_output_frame::str() const
{ return (Unsigned8*)_eax; }

PUBLIC inline
int
Jdb_output_frame::len() const
{ return (unsigned)_ebx; }

//---------------------------------------------------------------------------
PUBLIC inline
void
Jdb_status_page_frame::set(Address status_page)
{ _eax = (Mword)status_page; }

//---------------------------------------------------------------------------
PUBLIC inline
Unsigned8*
Jdb_log_frame::str() const
{ return (Unsigned8*)_edx; }

PUBLIC inline NEEDS["tb_entry.h"]
void
Jdb_log_frame::set_tb_entry(Tb_entry* tb_entry)
{ _eax = (Mword)tb_entry; }

//---------------------------------------------------------------------------
PUBLIC inline
Mword
Jdb_log_3val_frame::val1() const
{ return _ecx; }

PUBLIC inline
Mword
Jdb_log_3val_frame::val2() const
{ return _esi; }

PUBLIC inline
Mword
Jdb_log_3val_frame::val3() const
{ return _edi; }

//---------------------------------------------------------------------------
PUBLIC inline
Task_num
Jdb_debug_frame::task() const
{ return (Task_num)_ebx; }

PUBLIC inline
Mword
Jdb_debug_frame::addr() const
{ return _eax; }

PUBLIC inline
Mword
Jdb_debug_frame::size() const
{ return _edx; }

//---------------------------------------------------------------------------
PUBLIC inline
void
Jdb_get_cputime_frame::invalidate()
{ _ecx = ~0UL; }

//---------------------------------------------------------------------------
PUBLIC inline
const char *
Jdb_thread_name_frame::name() const
{ return (const char*)_eax; }

//---------------------------------------------------------------------------
IMPLEMENTATION[v2]:

PUBLIC inline
void
Jdb_get_cputime_frame::set(L4_uid next, Cpu_time total_us, unsigned short prio)
{
  _eax = total_us;
  _edx = total_us >> 32;
  _ecx = prio;
  _esi = next.raw(); 
  _edi = next.raw() >> 32;
}

PUBLIC inline
L4_uid
Jdb_get_cputime_frame::dst() const
{ return L4_uid((Unsigned64)_edi << 32 | (Unsigned64)_esi); }

PUBLIC inline
L4_uid
Jdb_thread_name_frame::dst() const
{ return L4_uid((Unsigned64)_edi << 32 | (Unsigned64)_esi); }

//---------------------------------------------------------------------------
IMPLEMENTATION[x0]:

PUBLIC inline
void
Jdb_get_cputime_frame::set(L4_uid next, Cpu_time total_us, unsigned short prio)
{
  _eax = total_us;
  _edx = total_us >> 32;
  _ecx = prio;
  _esi = next.raw();
}

PUBLIC inline
L4_uid
Jdb_get_cputime_frame::dst() const
{ return L4_uid(_esi); }

PUBLIC inline
L4_uid
Jdb_thread_name_frame::dst() const
{ return L4_uid(_esi); }
