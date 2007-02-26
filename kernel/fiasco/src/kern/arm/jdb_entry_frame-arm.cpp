INTERFACE [arm]:

#include "cpu.h"
#include "jdb.h"
#include "trap_state.h"
#include "tb_entry.h"

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
IMPLEMENTATION[arm]:

PUBLIC inline
Unsigned8*
Jdb_log_frame::str() const
{ return (Unsigned8*)r[1]; }

PUBLIC inline NEEDS["tb_entry.h"]
void
Jdb_log_frame::set_tb_entry(Tb_entry* tb_entry)
{ r[0] = (Mword)tb_entry; }

//---------------------------------------------------------------------------
PUBLIC inline
Mword
Jdb_log_3val_frame::val1() const
{ return r[2]; }

PUBLIC inline
Mword
Jdb_log_3val_frame::val2() const
{ return r[3]; }

PUBLIC inline
Mword
Jdb_log_3val_frame::val3() const
{ return r[4]; }

//---------------------------------------------------------------------------
PUBLIC inline
void
Jdb_status_page_frame::set(Address status_page)
{ r[0] = (Mword)status_page; }
