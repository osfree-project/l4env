#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/ktrace.h>
#include <l4/cxx/iostream.h>
#include <l4/cxx/l4iostream.h>

#include "globals.h"
#include "page_alloc.h"
#include "mem_man.h"
#include "memmap.h"
#include "ioports.h"

enum {
  L4_fpage_cached   = 0x600,
  L4_fpage_uncached = 0x200,
};


l4_addr_t	 mem_high;
l4_kernel_info_t *l4_info;
l4_addr_t        tbuf_status;

Mem_man iomem;

struct Answer 
{ 
  void *msg;
  unsigned long d1, d2;

  Answer() : msg(0), d1(0), d2(0) {}

  Answer(unsigned long addr, unsigned size, bool ro = false)
  : msg(L4_IPC_SHORT_FPAGE),  d1(0),
    d2(l4_fpage(addr, size, ro?L4_FPAGE_RO:L4_FPAGE_RW, L4_FPAGE_MAP).fpage 
	| L4_fpage_cached)
  {}

  unsigned long snd_base() const { return d1; }
  void snd_base(unsigned long base)
  { d1 = base; }

  void snd_fpage(unsigned long addr, unsigned size, bool ro = false)
  {
    d2 = l4_fpage(addr, size, ro?L4_FPAGE_RO:L4_FPAGE_RW, L4_FPAGE_MAP).fpage;
    d2 |= L4_fpage_cached;
    msg = L4_IPC_SHORT_FPAGE;
  }

  void no_cache()
  { d2 = (d2 & ~0xe00UL) | L4_fpage_uncached; }

  bool failed() const { return msg == L4_IPC_SHORT_MSG; }
};

enum Requests
{
  None,
  Map_free_page,
  Map_free_superpage,
  Map_kip,
  Map_tbuf,
  Map_ram,
  Map_iomem,
  Map_iomem_cached,
  Debug_dump,
};

enum Memory_type { Ram, Io_mem, Io_mem_cached };

static
void
print_request(enum Requests r)
{
  static char const * const names[] = { "None", "Map_free_page",
      "Map_free_superpage", "Map_kip", "Map_tbuf", "Map_ram", 
      "Map_iomem", "Map_iomem_cached", "Debug_dump"};

  L4::cout << "Request: " << names[r] << '\n';
}

/* parser for incoming messages */
static 
Requests get_request(unsigned long d0, unsigned long d1)
{
  if (d0 == 1 && (d1 & 0x0f) == 0x0f)
    return Map_kip;
  else if (d0 == ~3UL)
    return Map_free_page;
  else if ((d0 & ~0xffUL) == ~0xffUL)
    {
      switch (d0 & 0x0f0)
	{
	case 0xc0: return Debug_dump;
	case 0xb0: return Map_tbuf;
	case 0x60: return Map_ram;
	case 0x70: return Map_iomem;
	case 0x80: return Map_iomem_cached;
	case 0xa0: return Map_kip;
	case 0x90: 
	  {
	    l4_fpage_t fp = (l4_fpage_t&)d1;
	    if (fp.fp.size == L4_LOG2_PAGESIZE)
	      return Map_free_page;
	    else if (fp.fp.size == L4_LOG2_SUPERPAGESIZE)
	      return Map_free_superpage;
	    else
	      return None;
	  }
	default: return None;

	}
    }
  else
    return None;
}


static
Answer map_kip(void)
{ return Answer((l4_umword_t) l4_info, L4_LOG2_PAGESIZE, true); }

static
Answer map_tbuf(void)
{
  Answer a;
  if (tbuf_status != 0x00000000 && tbuf_status != ~0UL)
    a.snd_fpage(tbuf_status, L4_LOG2_PAGESIZE, false);
  return a;
}

static
Answer map_free_page(unsigned superpage, l4_threadid_t t)
{
  Answer a;
  unsigned long addr;
  addr = Mem_man::ram()->alloc_first(superpage?L4_SUPERPAGESIZE:L4_PAGESIZE, t.id.task);
  if (addr != ~0UL)
    {
      a.snd_base(addr);
      a.snd_fpage(addr, superpage?L4_LOG2_SUPERPAGESIZE:L4_LOG2_PAGESIZE);

      if (t.id.task < root_taskno) /* sender == kernel? */
	a.d2 |= 1; /* kernel wants page granted */
    }
  return a;
}


static
Answer map_mem(l4_fpage_t fp, Memory_type fn, l4_threadid_t t)
{
  Answer an;

  Mem_man *m;
  switch (fn)
    {
    case Ram: 
      m = Mem_man::ram(); 
      break;
    case Io_mem:
    case Io_mem_cached: 
      m = &iomem;
      break;
    default:
      an.msg = L4_IPC_SHORT_MSG;
      return an;
    }

  unsigned super = fp.fp.size >= L4_LOG2_SUPERPAGESIZE;
  unsigned long addr = m->alloc(Region::bs(fp.raw & ~((1UL << 12) - 1), 
	super?L4_SUPERPAGESIZE:L4_PAGESIZE, t.id.task));

  if (addr == ~0UL)
    return an;

  /* the Fiasco kernel makes the page non-cachable if the frame
   * address is greater than mem_high */
  an.snd_base(addr);
  an.snd_fpage(addr, super?L4_LOG2_SUPERPAGESIZE:L4_LOG2_PAGESIZE);
  if (fn == Io_mem)
    an.no_cache();
  
  return an;
}

/* handler for page fault and io page fault requests */
static
Answer
handle_page_fault(unsigned long d1, unsigned long d2, l4_threadid_t t)
{
  unsigned long pfa = d1 & ~3UL;
  Answer answer;

  d1 &= ~2UL;	/* the L4 kernel seems to get this bit wrong */
  if (!handle_io_page_fault(d1, d2, t.id.task, 
	answer.msg, answer.d1, answer.d2))
    {
#if defined (ARCH_x86)
      if (pfa >= 0x40000000)
	L4::cout 
	  << PROG_NAME": changed behavior for page faults above 0x40000000\n";
#endif
      unsigned long addr 
	= Mem_man::ram()->alloc(Region::bs(l4_trunc_page(pfa), L4_PAGESIZE, t.id.task));

      if (addr != ~0UL)
	{
	  answer.snd_base(addr);
	  answer.snd_fpage(pfa, L4_LOG2_PAGESIZE);
	}
      else
	{
	  if (debug_warnings)
	    L4::cout << PROG_NAME": no appropriate page found\n";
	}
    }
  return answer;
}


/* PAGER dispatch loop */
void
pager(void)
{
  l4_umword_t d1, d2;
  int err;
  l4_threadid_t t;
  l4_msgdope_t result;

  /* now start serving the subtasks */
  for (;;)
    {
      err = l4_ipc_wait(&t, 0, &d1, &d2, L4_IPC_NEVER, &result);

      while (!err)
	{
	  Answer answer;
	  /* we received a paging request here */
	  /* handle the sigma0 protocol */

	  if (debug_ipc)
	    L4::cout << PROG_NAME": received d1=" << L4::hex << d1
	      << " d2=" << d2 << L4::dec << " from thread=" << t << '\n';


	  Requests req = get_request(d1,d2);
	  if (debug_ipc)
	    print_request(req);
	  switch(req)
	    {
	    case Debug_dump:
		{
	          Mem_man::Tree::Node_allocator alloc;
		  L4::cout << PROG_NAME": Memory usage: a total of " 
		    << Page_alloc_base::total() 
		    << " byte are in the memory pool\n"
		    << "  allocated " 
		    << alloc.total_objects() - alloc.free_objects()
		    << " of " << alloc.total_objects() << " objects\n"
		    << "  this are " 
		    << (alloc.total_objects() - alloc.free_objects()) 
		       * alloc.object_size
		    << " of " << alloc.total_objects() * alloc.object_size
		    << " byte\n";
		  L4::cout 
		    << PROG_NAME": Dump of all resource maps\n"
		    << "RAM:------------------------\n";
		  Mem_man::ram()->dump();
		  L4::cout << "IOMEM:----------------------\n";
		  iomem.dump();
		  dump_io_ports();

		}
	      break;
	    case Map_kip:
	      answer = map_kip();
	      break;
	    case Map_tbuf:
	      answer = map_tbuf();
	      break;
	    case Map_free_page:
	      answer = map_free_page(0, t);
	      break;
	    case Map_free_superpage:
	      answer = map_free_page(1, t);
	      break;
	    case Map_ram:
	      answer = map_mem((l4_fpage_t&)d2, Ram, t);
	      break;
	    case Map_iomem:
	      answer = map_mem((l4_fpage_t&)d2, Io_mem, t);
	      break;
	    case Map_iomem_cached:
	      answer = map_mem((l4_fpage_t&)d2, Io_mem_cached, t);
	      break;
	    default:
	      answer = handle_page_fault(d1, d2, t);
	      break;
	    }

	  if (answer.failed())
	    {
	      if (debug_warnings)
		L4::cout << PROG_NAME": can't handle d1=" << L4::hex << d1
		  << " d2=" << d2 << " from thread=" << L4::dec << t << '\n',
	      answer.d1 = answer.d2 = 0;
	    }

	  if (debug_ipc)
	    L4::cout << PROG_NAME": sending d1=" << L4::hex << answer.d1
	      << " d2=" << answer.d2 << " msg=" << answer.msg << L4::dec
	      << " to thread=" << t << '\n';

	  /* send reply and wait for next message */
	  err = l4_ipc_reply_and_wait(t, answer.msg, answer.d1, answer.d2,
				      &t, 0, &d1, &d2,
				      L4_IPC_SEND_TIMEOUT_0, &result);

	}
    }
}

