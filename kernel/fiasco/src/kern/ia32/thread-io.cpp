IMPLEMENTATION [io]:

#include <feature.h>
KIP_KERNEL_FEATURE("io_prot");

// 
// disassamble IO statements to compute the port address and 
// the number of ports accessed
// 

/** Compute port number and size for an IO instruction.
    @param eip address of the instruction 
    @param ts thread state with registers
    @param port return port address
    @param size return number of ports accessed
    @return true if the instruction was handled successfully
      false otherwise
*/

bool
Thread::get_ioport(Address eip, Trap_state * ts, 
		     unsigned * port, unsigned * size)
{
  int from_user = ts->cs & 3;

  // handle 1 Byte IO
  switch(space()->peek((Unsigned8*)eip, from_user))
    {
    case 0xec:			// in dx, al
    case 0x6c:			// insb
    case 0xee:			// out dx, al
    case 0x6e:			// outb
      *size = 0;
      *port = ts->edx & 0xffff;
      return true;
    case 0xed:			// in dx, eax
    case 0x6d:			// insd
    case 0xef:			// out eax, dx
    case 0x6f:			// outd
      *size = 2;
      *port = ts->edx & 0xffff;
      if (*port +4 <= L4_fpage::Io_port_max)
	return true;
      else		   // Access beyond L4_IOPORT_MAX
	return false;
    case 0xfa:			// cli
    case 0xfb:			// sti
      *size = L4_fpage::Whole_io_space;
      *port = 0;
      return true;
    }
  
  // handle 2 Byte IO
  if(! (eip < Kmem::mem_user_max -1))
    return false;

  switch(space()->peek((Unsigned8*)eip, from_user))
    {
    case 0xe4:			// in imm8, al
    case 0xe6:			// out al, imm8 
      *size = 0;
      *port = space()->peek((Unsigned8*)(eip+1), from_user);
      return true;
    case 0xe5:			// in imm8, eax
    case 0xe7:			// out eax, imm8
      *size = 2;
      *port = space()->peek((Unsigned8*)(eip+1), from_user);
      return *port +4 <= L4_fpage::Io_port_max ? true : false;

    case 0x66:			// operand size override
      switch(space()->peek((Unsigned8*)(eip+1), from_user))
	{
	case 0xed:			// in dx, ax
	case 0xef:			// out ax, dx
	case 0x6d:			// insw
	case 0x6f:			// outw
	  *size = 1;
	  *port = ts->edx & 0xffff;
	  if (*port +2 <= L4_fpage::Io_port_max)
	    return true;
	  else		   // Access beyond L4_IOPORT_MAX
	    return false;
	case 0xe5:			// in imm8, ax
	case 0xe7:			// out ax,imm8
	  *size = 1;
	  *port = space()->peek((Unsigned8*)(eip + 2), from_user);
	  if (*port +2 <= L4_fpage::Io_port_max)
	    return true;
	  else
	    return false;
	}

    case 0xf3:			// REP
      switch(space()->peek((Unsigned8*)(eip +1), from_user))
	{
	case 0x6c:			// REP insb
	case 0x6e:			// REP outb
	  *size = 0;
	  *port = ts->edx & 0xffff;
	  return true;
	case 0x6d:			// REP insd
	case 0x6f:			// REP outd
	  *size = 2;
	  *port = ts->edx & 0xffff;
	  if(*port +4 <= L4_fpage::Io_port_max)
	    return true;
	  else		   // Access beyond L4_IOPORT_MAX
	    return false;
	}
    }

  // handle 3 Byte IO
  if(! (eip < Kmem::mem_user_max -2))
    return false;

  Unsigned16 w = space()->peek((Unsigned16*)eip, from_user);
  if (w == 0x66f3 || // sizeoverride REP
      w == 0xf366)   // REP sizeoverride
    {
      switch(space()->peek((Unsigned8*)(eip +2), from_user))
	{
	case 0x6d:			// REP insw
	case 0x6f:			// REP outw
	  *size = 1;
	  *port = ts->edx & 0xffff;
	  if(*port +2 <= L4_fpage::Io_port_max)
	    return true;
	  else		   // Access beyond L4_IOPORT_MAX
	    return false;
	}
    }


  // nothing appropriate found
  return false;
}

PUBLIC inline
bool
Thread::has_privileged_iopl()
{
  return (regs()->flags() & EFLAGS_IOPL) == EFLAGS_IOPL_U;
}
