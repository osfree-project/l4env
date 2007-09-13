INTERFACE [arm-debug]:

EXTENSION class Thread
{
public:
  typedef void (*Dbg_extension_entry)(Thread *t, Entry_frame *r);
  static Dbg_extension_entry dbg_extension[64];
};


IMPLEMENTATION [arm-debug]:

#include <cstring>

Thread::Dbg_extension_entry Thread::dbg_extension[64];

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
     "    str   sp, [r1,#-8]      \n" // store sp
     "    str   %0, [r1,#-4]      \n" // store ip
     "    str   %1, [r1,#-16]     \n" // store psr
     "    str   %2, [r1,#-20]     \n" // store ulr
     "    str   %3, [r1,#-24]     \n" // store usp
     "    ldr   r0, [%4,#(0*4)]   \n" // get r0
     "    str   r0, [r1,#-76]     \n" // store r0
     "    ldr   r0, [%4,#(1*4)]   \n" // get r1
     "    str   r0, [r1,#-72]     \n" // store r1
     "    ldr   r0, [%4,#(2*4)]   \n" // get r2
     "    str   r0, [r1,#-68]     \n" // store r2
     "    ldr   r0, [%4,#(3*4)]   \n" // get r3
     "    str   r0, [r1,#-64]     \n" // store r3
     "    ldr   r0, [%4,#(4*4)]   \n" // get r4
     "    str   r0, [r1,#-60]     \n" // store r4
     "    ldr   r0, [%4,#(5*4)]   \n" // get r5
     "    str   r0, [r1,#-56]     \n" // store r5
     "    ldr   r0, [%4,#(6*4)]   \n" // get r6
     "    str   r0, [r1,#-52]     \n" // store r6
     "    ldr   r0, [%4,#(7*4)]   \n" // get r7
     "    str   r0, [r1,#-48]     \n" // store r7
     "    ldr   r0, [%4,#(8*4)]   \n" // get r8
     "    str   r0, [r1,#-44]     \n" // store r8
     "    ldr   r0, [%4,#(9*4)]   \n" // get r9
     "    str   r0, [r1,#-40]     \n" // store r9
     "    ldr   r0, [%4,#(10*4)]  \n" // get r10
     "    str   r0, [r1,#-36]     \n" // store r10
     "    ldr   r0, [%4,#(11*4)]  \n" // get r11
     "    str   r0, [r1,#-32]     \n" // store r11
     "    ldr   r0, [%4,#(12*4)]  \n" // get r12
     "    str   r0, [r1,#-28]     \n" // store r12
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
    /*[reg]*/  "r" (t->regs()->r),
    "r" (str)
      :
      "r0","r1","lr");
}

IMPLEMENTATION [arm-!debug]:

extern "C" void sys_kdb_ke()
{}

extern "C" void enter_jdb()
{}
