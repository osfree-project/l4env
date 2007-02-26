
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/l4rm/l4rm.h>
#include <l4/util/macros.h>
#include <l4/generic_io/libio.h>

#include "init.h"
#include "iomem.h"

static l4_threadid_t my_task_pager_id = L4_INVALID_ID;

static void
get_my_task_pager_id(void)
{
  l4_umword_t dummy;

  l4_threadid_t my_task_preempter_id;

  /* get region manager's pager */
  my_task_preempter_id = my_task_pager_id = L4_INVALID_ID;
  l4_thread_ex_regs(l4rm_region_mapper_id(), (l4_umword_t)-1, (l4_umword_t)-1,
		    &my_task_preempter_id, &my_task_pager_id, 
		    &dummy, &dummy, &dummy);
}

int
map_io_mem(l4_addr_t addr, l4_size_t size, const char *id, l4_addr_t *vaddr)
{
  int error;
  l4_addr_t m_addr;
  l4_uint32_t rg;
  l4_umword_t dummy;
  l4_msgdope_t result;
  l4_offs_t offset;

  if (!use_l4io)
    {
      offset = addr - (addr & L4_SUPERPAGEMASK);
      size   = (size + offset + L4_SUPERPAGESIZE-1) & L4_SUPERPAGEMASK;
      addr  &= L4_SUPERPAGEMASK;

      if ((error = l4rm_area_reserve(size, L4RM_LOG2_ALIGNED, vaddr, &rg)))
	Panic("Error %d reserving region size=%dMB for video memory",
	    error, size>>20);

      printf("Mapping I/O %s memory at %08x to %08x+%06x [%dkB]\n",
	  id, addr+offset, *vaddr, offset, size>>10);

      /* check here for curious video buffer, one candidate is VMware */
      if (addr < 0x80000000)
	Panic("I/O memory address is below 2GB (0x80000000),\n"
	      "don't know how to map it as device super I/O page.");

      if (l4_is_invalid_id(my_task_pager_id))
	get_my_task_pager_id();

      for (m_addr=*vaddr; size>0; size-=L4_SUPERPAGESIZE,
				  addr+=L4_SUPERPAGESIZE, 
				  m_addr+=L4_SUPERPAGESIZE)
	{
	  for (;;)
	    {
	      /* we could get l4_thread_ex_regs'd ... */
	      error =
		l4_i386_ipc_call(my_task_pager_id,
				 L4_IPC_SHORT_MSG, (addr-0x40000000) | 2, 0,
	  			 L4_IPC_MAPMSG(m_addr, L4_LOG2_SUPERPAGESIZE),
  				 &dummy, &dummy,
				 L4_IPC_NEVER, &result);
	      if (error != L4_IPC_SECANCELED && error != L4_IPC_SEABORTED)
		break;
	    }

	  if (error)
	    Panic("Error 0x%02x mapping I/O %s memory", error, id);

	  if (!l4_ipc_fpage_received(result))
	    Panic("No fpage received, result=0x%04x", result.msgdope);
	}
    }
  else /* use l4io */
    {
      if ((*vaddr = l4io_request_mem_region(addr, size, &offset)) == 0)
	Panic("Can't request memory region from l4io.");

      printf("Mapped %s memory at %08x to %08x+%06x [%dkB] via L4IO\n",
	  id, addr + offset, *vaddr, offset, size >> 10);
    }

  *vaddr += offset;

  return 0;
}

