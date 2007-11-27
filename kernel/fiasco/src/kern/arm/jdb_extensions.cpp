IMPLEMENTATION [arm-debug]:

#include <cstdio>
#include <simpleio.h>
#include "jdb_tbuf.h"
#include "jdb_entry_frame.h"
#include "kdb_ke.h"
#include "cpu_lock.h"
#include "vkey.h"
#include "static_init.h"
#include "thread.h"

static void outchar(Thread *, Entry_frame *r)
{ putchar(r->r[0] & 0xff); }

static void outstring(Thread *, Entry_frame *r)
{ putstr((char*)r->r[0]); }

static void outnstring(Thread *, Entry_frame *r)
{ putnstr((char*)r->r[0], r->r[1]); }

static void outdec(Thread *, Entry_frame *r)
{ printf("%d", r->r[0]); }

static void outhex(Thread *, Entry_frame *r)
{ printf("%08x", r->r[0]); }

static void outhex20(Thread *, Entry_frame *r)
{ printf("%05x", r->r[0] & 0xfffff); }

static void outhex16(Thread *, Entry_frame *r)
{ printf("%04x", r->r[0] & 0xffff); }

static void outhex12(Thread *, Entry_frame *r)
{ printf("%03x", r->r[0] & 0xfff); }

static void outhex8(Thread *, Entry_frame *r)
{ printf("%02x", r->r[0] & 0xff); }

static void inchar(Thread *, Entry_frame *r)
{
  r->r[0] = Vkey::get();
  Vkey::clear();
}

static void tbuf(Thread *, Entry_frame *r)
{
  Thread *t    = current_thread();
  Mem_space *s = t->mem_space();
  Address ip = r->ip();
  Address_type user;
  Unsigned8 *str;
  int len;
  char c;

  Jdb_entry_frame *entry_frame = reinterpret_cast<Jdb_entry_frame*>(r);
  user = entry_frame->from_user();

  switch (entry_frame->param())
    {
    case 0: // fiasco_tbuf_get_status()
	{
	  Jdb_status_page_frame *regs =
	    reinterpret_cast<Jdb_status_page_frame*>(r);
	  regs->set(Mem_layout::Tbuf_ustatus_page);
	}
      break;
    case 1: // fiasco_tbuf_log()
	{
	  Jdb_log_frame *regs = reinterpret_cast<Jdb_log_frame*>(r);
	  Tb_entry_ke *tb =
            static_cast<Tb_entry_ke*>(Jdb_tbuf::new_entry());
          str = regs->str();
	  tb->set(t, ip-1);
	  for (len=0; (c = s->peek(str++, user)); len++)
            tb->set_buf(len, c);
          tb->term_buf(len);
          regs->set_tb_entry(tb);
          Jdb_tbuf::commit_entry();
	}
      break;
    case 2: // fiasco_tbuf_clear()
      Jdb_tbuf::clear_tbuf();
      break;
    case 3: // fiasco_tbuf_dump()
      return; // => Jdb
    case 4: // fiasco_tbuf_log_3val()
        {
          // interrupts are disabled in handle_slow_trap()
          Jdb_log_3val_frame *regs =
            reinterpret_cast<Jdb_log_3val_frame*>(r);
          Tb_entry_ke_reg *tb =
            static_cast<Tb_entry_ke_reg*>(Jdb_tbuf::new_entry());
          str = regs->str();
          tb->set(t, ip-1, regs->val1(), regs->val2(), regs->val3());
          for (len=0; (c = s->peek(str++, user)); len++)
            tb->set_buf(len, c);
          tb->term_buf(len);
          regs->set_tb_entry(tb);
          Jdb_tbuf::commit_entry();
        }
      break;
    case 5: // fiasco_tbuf_get_status_phys()
        {
          Jdb_status_page_frame *regs =
            reinterpret_cast<Jdb_status_page_frame*>(r);
          regs->set(s->virt_to_phys(Mem_layout::Tbuf_ustatus_page));
        }
      break;
    case 6: // fiasco_timer_disable
      Timer::enable();
      break;
    case 7: // fiasco_timer_enable
      Timer::enable();
      break;
    }
}

static void do_cli(Thread *, Entry_frame *r)
{ r->psr |= 128; }

static void do_sti(Thread *, Entry_frame *r)
{ r->psr &= ~128; }

/* Instruction Memory Barrier */
static void imb(Thread *, Entry_frame *e)
{
  // r0: op,  r1: start,  r2: end

  // this also tells userland which area we really took
  if (e->r[1] >= Mem_layout::User_max)
    e->r[1] = Mem_layout::User_max - 1;
  if (e->r[2] >= Mem_layout::User_max)
    e->r[2] = Mem_layout::User_max - 1;

  switch (e->r[0])
    {
    case 1: // clean
      Mem_unit::clean_dcache((void *)e->r[1], (void *)e->r[2]);
      break;
    case 2: // flush
      Mem_unit::flush_dcache((void *)e->r[1], (void *)e->r[2]);
      break;
    case 3: // inv
      Mem_unit::inv_dcache((void *)e->r[1], (void *)e->r[2]);
      break;
    };
}

static void init_dbg_extensions()
{
  Thread::dbg_extension[0x01] = &outchar;
  Thread::dbg_extension[0x02] = &outstring;
  Thread::dbg_extension[0x03] = &outnstring;
  Thread::dbg_extension[0x04] = &outdec;
  Thread::dbg_extension[0x05] = &outhex;
  Thread::dbg_extension[0x06] = &outhex20;
  Thread::dbg_extension[0x07] = &outhex16;
  Thread::dbg_extension[0x08] = &outhex12;
  Thread::dbg_extension[0x09] = &outhex8;
  Thread::dbg_extension[0x0d] = &inchar;
  Thread::dbg_extension[0x1d] = &tbuf;
  Thread::dbg_extension[0x32] = &do_cli;
  Thread::dbg_extension[0x33] = &do_sti;

  Thread::dbg_extension[0x3f] = &imb;
}

STATIC_INITIALIZER(init_dbg_extensions);

