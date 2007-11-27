INTERFACE[amd64]:

EXTENSION class Thread
{
private:
  Mword _exc_sp;
  Mword _exc_ss;
};


//----------------------------------------------------------------------------
IMPLEMENTATION [amd64]:

IMPLEMENT inline
Mword 
Thread::user_sp() const
{ return exception_triggered()?_exc_sp:regs()->sp(); }

IMPLEMENT inline
void
Thread::user_sp(Mword sp)
{
  if (exception_triggered())
    _exc_sp = sp;
  else
    regs()->sp(sp);
}

PRIVATE inline
int
Thread::do_trigger_exception(Entry_frame *r)
{
  if (!exception_triggered())
    {
      extern Mword leave_by_trigger_exception;
      _exc_sp = r->sp();
      _exc_ss = r->ss();
      _exc_ip = r->ip();
      _exc_flags = r->flags();
      r->cs (Gdt::gdt_code_kernel | Gdt::Selector_kernel);
      r->ip (reinterpret_cast<Address>(&leave_by_trigger_exception));
      r->sp ((Address)r+ sizeof(*r));
      r->ss (Gdt::gdt_data_kernel | Gdt::Selector_kernel);
      r->flags (r->flags() & ~EFLAGS_TF); // do not singlestep inkernel code

      return 1;
    }
  // else ignore change of IP because triggered exception already pending
  return 0;
}

PUBLIC inline
void
Thread::restore_exc_state()
{
  Entry_frame *r = regs();

  r->cs (exception_cs());
  r->ip (_exc_ip);
  r->sp (_exc_sp);
  r->ss (_exc_ss);
  r->flags (_exc_flags);
  _exc_ip = ~0UL;
}

PRIVATE static inline
void
Thread::copy_utcb_to_ts(L4_msg_tag const &tag, Thread *snd, Thread *rcv)
{
  Trap_state *ts = (Trap_state*)rcv->_utcb_handler;
  Mword       s  = tag.words();
  Unsigned32  cs = ts->cs();

  if (EXPECT_FALSE(rcv->exception_triggered()))
    {
      // triggered exception pending
      Cpu::memcpy_mwords (ts, snd->utcb()->values, s > 19 ? 19 : s);
      if (EXPECT_TRUE(s > 19))
	rcv->_exc_ip = snd->utcb()->values[19];
      if (EXPECT_TRUE(s > 21))
	ts->flags(snd->utcb()->values[21]);
      if (EXPECT_TRUE(s > 22))
	rcv->_exc_sp = snd->utcb()->values[22];
    }
  else
    Cpu::memcpy_mwords (ts, snd->utcb()->values, s > 23 ? 23 : s);

  // sanitize eflags
  // XXX: ia32 in here!
  if (!rcv->trap_is_privileged(0))
    ts->flags((ts->flags() & ~(EFLAGS_IOPL | EFLAGS_NT)) | EFLAGS_IF);

  // don't allow to overwrite the code selector!
  ts->cs(cs);

  rcv->state_del(Thread_in_exception);
}

PRIVATE static inline
void
Thread::copy_ts_to_utcb(L4_msg_tag const &, Thread *snd, Thread *rcv)
{
  Trap_state *ts = (Trap_state*)snd->_utcb_handler;

  {
    Lock_guard <Cpu_lock> guard (&cpu_lock);
    if (EXPECT_FALSE(snd->exception_triggered()))
      {
	Cpu::memcpy_mwords (rcv->utcb()->values, ts, 19);
	rcv->utcb()->values[19] = snd->_exc_ip;
	rcv->utcb()->values[21] = ts->flags();
	rcv->utcb()->values[22] = snd->_exc_sp;
      }
    else
      Cpu::memcpy_mwords (rcv->utcb()->values, ts, 23);
  }
}



