INTERFACE [arm]:

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
  char          _msg[19];       ///< debug message
  Mword         _r0, _r1, _r2;  ///< registers
};

/** logged trap. */
class Tb_entry_trap : public Tb_entry
{
private:
  Unsigned8     _trapno;
  Unsigned16    _error;
  Mword         _cpsr, _sp;
};

// --------------------------------------------------------------------
IMPLEMENTATION [arm]:

PUBLIC static FIASCO_INIT
void
Tb_entry_fit::init_arch()
{
}

PUBLIC inline
void
Tb_entry::rdtsc()
{}

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
{ return _r0; }

PUBLIC inline
Mword
Tb_entry_ke_reg::val2() const
{ return _r1; }

PUBLIC inline
Mword
Tb_entry_ke_reg::val3() const
{ return _r2; }

PUBLIC inline
void
Tb_entry_ke_reg::set(Context *ctx, Mword eip, Mword v1, Mword v2, Mword v3)
{
  set_global(Tbuf_ke_reg, ctx, eip);
  _r0 = v1; _r1 = v2; _r2 = v3;
}

PUBLIC inline
void
Tb_entry_ke_reg::set_const(Context *ctx, Mword eip, const char * const msg,
                           Mword v1, Mword v2, Mword v3)
{
  set(ctx, eip, v1, v2, v3);
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

// ------------------
PUBLIC inline
Unsigned16
Tb_entry_trap::cs() const
{ return 0; }

PUBLIC inline
Unsigned8
Tb_entry_trap::trapno() const
{ return _trapno; }

PUBLIC inline
Unsigned16
Tb_entry_trap::error() const
{ return _error; }

PUBLIC inline
Mword
Tb_entry_trap::sp() const
{ return _sp; }

PUBLIC inline
Mword
Tb_entry_trap::cr2() const
{ return 0; }

PUBLIC inline
Mword
Tb_entry_trap::eax() const
{ return 0; }
