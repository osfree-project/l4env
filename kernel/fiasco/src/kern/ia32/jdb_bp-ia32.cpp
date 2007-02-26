INTERFACE: 

#include "l4_types.h"

struct trap_state;
class Thread;

class Jdb_bp
{
public:
  typedef struct 
    {
      Address addr;
      unsigned char len, mode;
    } breakpoint_t;
  
  typedef struct 
    {
      int other;
      int thread; 
    } bp_thread_res_t;
  
  typedef struct 
    {
      char reg;
      Address y, z;
    } bp_reg_res_t;
  
  typedef struct 
    {
      unsigned char len;
      Address addr;
      Address y, z;
    } bp_var_res_t;
  
  typedef struct 
    {
      bp_thread_res_t t_res;
      bp_reg_res_t r_res;
      bp_var_res_t v_res;
    } bp_res_t;

private:
  static Mword debug_control;
  static breakpoint_t bps[4];
  static bp_res_t bpres[4];
};


IMPLEMENTATION[ia32]:

#include <cstdio>
#include <flux/x86/base_trap.h>

#include "kmem.h"
#include "jdb_bp.h"
#include "thread.h"

#define load_debug_register(val, num) \
    asm("movl %0, %%db" #num ";" : /* no output */ : "r" (val)); 

Jdb_bp::breakpoint_t Jdb_bp::bps[4];
Jdb_bp::bp_res_t Jdb_bp::bpres[4];
Mword Jdb_bp::debug_control;

PUBLIC static
void
Jdb_bp::init(void)
{
  for (int i=0; i<4; i++)
    bpres[i].t_res.thread = -1;
}


PUBLIC static inline
void 
Jdb_bp::reset_debug_status_register() 
{ 
  __asm__("movl %0, %%db6" : /* no output */ : "r" (0));
}

PUBLIC static inline
Mword 
Jdb_bp::get_debug_status_register() 
{
  Mword st; __asm__("movl %%db6, %0" : "=r" (st)); return st;
}

PUBLIC static inline
Mword 
Jdb_bp::get_debug_control_register()
{ 
  Mword ctrl; __asm__("movl %%db7, %0" : "=r" (ctrl)); return ctrl;
}

PUBLIC static inline
void 
Jdb_bp::set_debug_control_register(Mword ctrl)
{ 
  __asm__("movl %0, %%db7" : /* no output */ : "r" (ctrl)); 
}

static 
void 
Jdb_bp::set_debug_address_register(Mword addr, int reg_num)
{
  switch (reg_num) 
    {
    case 0: load_debug_register(addr, 0); break;
    case 1: load_debug_register(addr, 1); break;
    case 2: load_debug_register(addr, 2); break;
    case 3: load_debug_register(addr, 3); break;
    default: printf("set_debug_register: invalid number\n");
    }
}

PUBLIC static
void
Jdb_bp::save_state()
{
  debug_control = get_debug_control_register();
  set_debug_control_register(debug_control & ~0xffff00ff);
}


PUBLIC static
void
Jdb_bp::restore_state()
{
  reset_debug_status_register();
  set_debug_control_register(debug_control);
}

PUBLIC static 
void 
Jdb_bp::delete_bp(int used)
{
  debug_control &= ~(((3 + (3 << 2)) << (16 + 4*used)) + (3 << (2*used)));
  bps[used].addr = 0;
}


PUBLIC static
void
Jdb_bp::bps_used(int *n, int *u)
{
  int i, num, used;
  for (i=0, num=0, used=0; i<4; i++)
    {
      if (bps[i].addr != 0)
	{
	  num++;
	  used |= 1<<i;
	}
    }

  *n = num;
  *u = used;
}


PUBLIC static
int
Jdb_bp::first_unused()
{
  int i;
  for(i=0; ((i<4) && (bps[i].addr != 0)); i++);
  return i;
}


PUBLIC static
void
Jdb_bp::get_bp(int i, Mword *addr, Mword *mode, Mword *len)
{
  *addr = bps[i].addr;
  *mode = bps[i].mode;
  *len  = bps[i].len;
}


PUBLIC static
void
Jdb_bp::set_bp(int i, Mword addr, Mword mode, Mword len)
{
  bps[i].addr = addr;
  bps[i].mode = mode;
  bps[i].len  = len;
    
  debug_control &= ~(((3        + (3      <<2)) << (16 + 4*i)) + (3 << (2*i)));
  debug_control |=  ((((mode&3) + ((len-1)<<2)) << (16 + 4*i)) + (2 << (2*i)));

  set_debug_address_register(addr, i);
}


PUBLIC static
void
Jdb_bp::get_bpres(int bpn, 
		  int *other, int *thread,
		  char *reg, Mword *ry, Mword *rz,
		  Mword *len, Mword *addr, Mword *vy, Mword *vz)
{
  bp_res_t *bpr = bpres + bpn;
  
  *other   = bpr->t_res.other;
  *thread  = bpr->t_res.thread;
  *reg     = bpr->r_res.reg;
  *ry      = bpr->r_res.y;
  *rz      = bpr->r_res.z;
  *len     = bpr->v_res.len;
  *addr    = bpr->v_res.addr;
  *vy      = bpr->v_res.y;
  *vz      = bpr->v_res.z;
}


PUBLIC static
void
Jdb_bp::set_bpres(int bpn, int other, int thread, 
		  char reg, Mword ry, Mword rz,
		  Mword len, Mword addr, Mword vy, Mword vz)
{
  bp_res_t *bpr = bpres + bpn;
  
  bpr->t_res.other = other;
  bpr->t_res.thread = thread;
  bpr->r_res.reg = reg;
  bpr->r_res.y = ry;
  bpr->r_res.z = rz;
  bpr->v_res.len = len;
  bpr->v_res.addr = addr;
  bpr->v_res.y = vy;
  bpr->v_res.z = vz;
}


PUBLIC static 
void 
Jdb_bp::clear_breakpoint_restriction(int bp)
{
  int a,z;

  if (bp > 3) 
    {
      // 0..3 bp number , 4.. all bps
      a=0;
      z=3;
    } 
  else 
    a = z = bp; 

  for (int i=a; i<=z; i++)
    {
      bp_res_t *bpr = bpres + i;
      
      bpr->t_res.other = 0;
      bpr->t_res.thread = -1; // INVALID
      
      bpr->r_res.reg = 0;
      bpr->r_res.y = 0;
      bpr->r_res.z = 0;
      
      bpr->v_res.len = 0;
      bpr->v_res.addr = 0;
      bpr->v_res.y = 0;
      bpr->v_res.z = 0;
    }
}

// return TRUE if the breakpoint does NOT match
PUBLIC static
int
Jdb_bp::check_bpres(trap_state *st, int i, Thread *t)
{
  bp_res_t *bpr = bpres + i;
    
  // investigate for thread restriction
  if (bpr->t_res.thread != -1)
    {
      int global = t->id().gthread();
      if (bpr->t_res.other ^ (bpr->t_res.thread != global))
	return 1;
    }

  // investigate for register restriction
  if (bpr->r_res.reg)
    { 
      Mword val = 0;
      int reg = bpr->r_res.reg;
      Mword y = bpr->r_res.y;
      Mword z = bpr->r_res.z;

      if (reg == 1) val = st->eax;
      if (reg == 2) val = st->ebx;
      if (reg == 3) val = st->ecx;
      if (reg == 4) val = st->edx;
      if (reg == 5) val = st->ebp;
      if (reg == 6) val = st->esi;
      if (reg == 7) val = st->edi;
      if (reg == 8) val = st->eip;
      if (reg == 9) val = st->esp;
      if (reg == 10) val = st->eflags;

      // return true if rules does NOT match
      if (  ((y <= z) && ((val <  y) || (val >  z)))
	  ||((y >  z) && ((val >= z) || (val <= y))))
	return 1;
    }

  // investigate for variable restriction
  if (bpr->v_res.len)
    { 
      Mword val = 0;
      Mword y = bpr->v_res.y;
      Mword z = bpr->v_res.z;

      if (bpr->v_res.len == 1)
	val = *(Unsigned8 *)Kmem::phys_to_virt(bpr->v_res.addr);
      else if (bpr->v_res.len == 2)
	val = *(Unsigned16 *)Kmem::phys_to_virt(bpr->v_res.addr);
      else if (bpr->v_res.len == 4)
	val = *(Unsigned32 *)Kmem::phys_to_virt(bpr->v_res.addr);

      // return true if rules does NOT match
      if ((y <= z) ^ ((val >= y) && (val <= z)))
	return 1;
    }

  return 0;
}

PUBLIC static
Mword
Jdb_bp::check_bp_set(Address addr)
{
  for (int i=0; i<4; i++)
    if (bps[i].addr == addr)
      return i+1;

  return 0;
}

// Create log entry if breakpoint matches
PUBLIC static
int
Jdb_bp::check_bp_log(trap_state *ts, Mword db7, Thread *t)
{
  if (db7 & 0xf)
    {
      for (int i=0; i<4; i++)
	{
	  if (db7 & (1<<i))
	    {
	      Mword mode = bps[i].mode;

	      if (!check_bpres(ts, i, t) && (mode & 0x80))
		{
		  // log breakpoint
		  Mword value = 0;

		  mode &= ~0x80;
		  if (mode == 1 || mode == 3)
		    {
		      // If it's a write or access (read) breakpoint,
	      	      // we look at the appropriate place and print
      		      // the bytes we find there. We do not need to
		      // look if the page is present because the x86
		      // enters the debug exception _after_ the memory
		      // access was performed.
		      switch (bps[i].len)
			{
			case 1: value = *(Unsigned8*) bps[i].addr;
			case 2: value = *(Unsigned16*)bps[i].addr;
			case 4: value = *(Unsigned32*)bps[i].addr;
			}
		    }
		  Tb_entry_bp *tb = 
		    static_cast<Tb_entry_bp*>(Jdb_tbuf::new_entry());
		  tb->set(t, ts->eip, mode, bps[i].len, value, bps[i].addr);
		  Jdb_tbuf::commit_entry();

		  return 1;
		}
	      if ((mode & 3) == 0)
		ts->eflags = EFLAGS_RF;

	      reset_debug_status_register();
	    }
	}
    }

  return 0;
}

