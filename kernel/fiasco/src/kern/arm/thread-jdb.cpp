INTERFACE [arm-debug]:

EXTENSION class Thread
{
public:
  typedef void (*Dbg_extension_entry)(Thread *t, Entry_frame *r);
  static Dbg_extension_entry dbg_extension[64];
};


IMPLEMENTATION [arm-debug]:

#include <cstdio>
#include <simpleio.h>
#include "kdb_ke.h"
#include "cpu_lock.h"
#include "vkey.h"
#include "static_init.h"

Thread::Dbg_extension_entry Thread::dbg_extension[64];

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

static void do_cli(Thread *, Entry_frame *r)
{ r->psr |= 128; }

static void do_sti(Thread *, Entry_frame *r)
{ r->psr &= ~128; }

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
  Thread::dbg_extension[0x32] = &do_cli;
  Thread::dbg_extension[0x33] = &do_sti;
}

STATIC_INITIALIZER(init_dbg_extensions);

extern "C" void sys_kdb_ke()
{
  cpu_lock.lock();
  Thread *t = current_thread();
  unsigned *x = (unsigned*)t->regs()->ip();

  if ((*x & 0xffff0000) == 0xe35e0000)
    {
      unsigned func = (*x) & 0x3f;
      if (Thread::dbg_extension[func])
	{
	  Thread::dbg_extension[func](t, t->regs());
	  return;
	}
    }
  
  char str[32] = "USER ENTRY";
  if ((*x & 0xfffffff0) == 0xea000000)
    // check for always branch, no return, maximum 32 bytes forward
    {
      strncpy(str, (char *)(x + 1), sizeof(str));
      str[sizeof(str)-1] = 0;
    }

  asm volatile 
    ("    ldr   r1, 4f            \n"
     "    str   sp, [r1,#-8]      \n"
     "    str   %0, [r1,#-4]      \n"
     "    str   %1, [r1,#-16]     \n"
     "    str   %2, [r1,#-20]     \n"
     "    str   %3, [r1,#-24]     \n"
     "    ldr   r0, [%4,#(5*4)]   \n"
     "    str   r0, [r1,#-28]     \n"
     "    ldr   r0, [%4,#(6*4)]   \n"
     "    str   r0, [r1,#-32]     \n"
     "    ldr   r0, [%4,#(7*4)]   \n"
     "    str   r0, [r1,#-36]     \n"
     "    ldr   r0, [%4,#(8*4)]   \n"
     "    str   r0, [r1,#-40]     \n"
     "    ldr   r0, [%4,#(9*4)]   \n"
     "    str   r0, [r1,#-44]     \n"
     "    ldr   r0, [%4,#(10*4)]  \n"
     "    str   r0, [r1,#-48]     \n"
     "    ldr   r0, [%4,#(11*4)]  \n"
     "    str   r0, [r1,#-52]     \n"
     "    ldr   r0, [%4,#(12*4)]  \n"
     "    str   r0, [r1,#-56]     \n"
     "    ldr   r0, [%4,#(13*4)]  \n"
     "    str   r0, [r1,#-60]     \n"
     "    ldr   r0, [%4,#(14*4)]  \n"
     "    str   r0, [r1,#-64]     \n"
     "    ldr   r0, [%4,#(15*4)]  \n"
     "    str   r0, [r1,#-68]     \n"
     "    ldr   r0, [%4,#(16*4)]  \n"
     "    str   r0, [r1,#-72]     \n"
     "    sub   sp, r1,#(15*4 + 16) \n"
     "    mov   r0, sp            \n"
     "    mov   r1, %5            \n"
     "    adr   lr, 1f            \n"
     "    ldr   pc, 2f            \n"
     "4:  .word _kdebug_stack_top \n"
     "2:  .word enter_jdb         \n"
     "1:  ldr   sp, [sp,#68]      \n"
     : 
     : 
    /*[pc]*/   "r" (t->regs()->ip()),
    /*[spsr]*/ "r" (t->regs()->psr),
    /*[lr]*/   "r" (t->regs()->ulr),
    /*[sp]*/   "r" (t->regs()->sp()),
    /*[reg]*/  "r" (t->regs()),
    "r" (str)
      :
      "r0","r1","lr");
}

IMPLEMENTATION [arm-!debug]:

extern "C" void sys_kdb_ke()
{}

extern "C" void enter_jdb()
{}
