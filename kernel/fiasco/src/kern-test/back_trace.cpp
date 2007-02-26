INTERFACE:
#include "space.h"
#include "kmem.h"
#include "globalconfig.h"

class back_trace_t
{
protected:
  bool done;
  unsigned *_ebp, *_esp, *max_esp;
  space_context_t *_space_ctxt;
};

IMPLEMENTATION:

#include <stdio.h>
#include "config.h"
#include "helping_lock.h"   // contains threading_system_active

//#include "printf.h"

PUBLIC
back_trace_t::back_trace_t(unsigned ebp=0,  unsigned esp=0,
			   unsigned m_esp=0, space_context_t *space_ctxt = 0)
  :_ebp((unsigned *) ebp),_esp((unsigned *)esp),_space_ctxt(space_ctxt)
{
  
  done = false;
  if(!_ebp)
    {
      asm volatile 
	("movl %%ebp, %0\n"
	 :"=q"(_ebp));
    }
  if(!_esp)
    {
      asm volatile 
	("movl %%esp, %0\n"
	 :"=q"(_esp));
    }
  if(!m_esp)
    {
      if(helping_lock_t::threading_system_active)
	{
	  max_esp = reinterpret_cast<unsigned *>
	    ((reinterpret_cast<unsigned long>
	      (_esp) | (config::thread_block_size - 1)));
	} 
      else
	{
	  max_esp = reinterpret_cast<unsigned *>
	    (reinterpret_cast<unsigned long>
	      (_esp) + 1024);
	}
    }
  else
    {
      max_esp = (unsigned*)m_esp;
    }

}

PUBLIC unsigned
back_trace_t::next()
{
  unsigned *next_ebp;

#ifdef CONFIG_NO_FRAME_PTR
  return 0;
#endif

  if (done)
    {
      //      printf("bt done\n");
      return 0;
    }

  if((((unsigned) _ebp) < ((unsigned)max_esp)) && 
     (((unsigned) _ebp) >= (((unsigned) _esp))))
    {
      unsigned result;
      if(!_space_ctxt)
	{
	  result = *(_ebp + 1);
	  next_ebp = *(unsigned **) _ebp;
	}
      else
	{
	  vm_offset_t phys_ebp;
	  unsigned *virt_ebp;
	  phys_ebp = _space_ctxt->lookup((vm_offset_t)_ebp);
	  if(phys_ebp == 0xffffffff)
	    {
	      //	      printf("no phys_ebp, _ebp: %08x\n", _ebp);
	      done = true;
	      return 0;
	    }
	  virt_ebp = (unsigned*) kmem::phys_to_virt(phys_ebp);

	  result = *(virt_ebp + 1);
	  next_ebp   = *(unsigned **) virt_ebp;
	}
      if( next_ebp <= _ebp)
	{
	  //	  printf("exhausted\n");
	  done = true;
	}
      _ebp = next_ebp;
      return result;
    }
  else
    {
      //      printf("invalid: _ebp: %08x, _esp: %08x, max_esp: %08x\n",
      //	     _ebp, _esp, max_esp);
      done = true;
      return 0;
    }
}
PUBLIC long long
back_trace_t::next_longlong()
{
  unsigned ra, this_ref;
  if((((unsigned) _ebp) < ((unsigned)max_esp)) && 
     (((unsigned) _ebp) >= (((unsigned) _esp))))
    {
      ra = *(_ebp + 1);
      this_ref = *(_ebp + 2) | 0xf0001234;
      _ebp = *(unsigned **) _ebp;
      return (((long long) this_ref) << 32) | (long long) ra;
    }
  else
    return 0;
}
PUBLIC unsigned
back_trace_t::ebp() const
{
  return (unsigned) _ebp;
}

#define virt_to_kern(addr,space) ({vm_offset_t phys_addr = (space)->lookup(addr); \
                                   (phys_addr == 0xffffffff) ? 0 :                \
                                        (unsigned *) kmem::phys_to_virt(phys_addr);})

PUBLIC static unsigned
back_trace_t::find_ebp(unsigned esp,space_context_t *sc=0)
{
  for(int cnt = 0; cnt < 0x40; cnt ++, esp +=4)
    {
      if(check_ebp(esp,sc))
	{
	  return esp;
	}
    }
  return 0;
}

PUBLIC static bool
back_trace_t::check_ebp(unsigned ebp, space_context_t *sc=0)
{
  unsigned *next_ebp; 
  for(;;)
    {
      if(sc)
	{
	  next_ebp = virt_to_kern(ebp,sc);
	  if(!next_ebp)
	    {
	      return false;
	    }
	}
      else
	{
	  next_ebp = (unsigned *) ebp;
	}
      if(*next_ebp  == 0)
	{
	  return true;
	}
      if(*next_ebp < ebp)
	{
	  return false;
	}
      ebp = *next_ebp;
    }
}
