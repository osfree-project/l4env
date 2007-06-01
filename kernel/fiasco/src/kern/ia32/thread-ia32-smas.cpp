INTERFACE:

/** Various helper functions for thread that differ for
 *  the implementation with small address spaces.
 */
IMPLEMENTATION[ia32-smas]:

#include <feature.h>
KIP_KERNEL_FEATURE("smallspaces");

#include "context.h"
#include "kdb_ke.h"
#include "smas.h"
#include "l4_types.h"

/**
 * Additional things to do before killing, when using small spaces.
 * Switch to idle thread in case the killer thread runs in a
 * small address space while the one to be killed happens to be
 * the current big one. If we just deleted it the small killer 
 * would be floating in the great big nothing.
 */
IMPLEMENT inline NEEDS ["context.h"]
void
Thread::kill_small_space()
{
  if (mem_space() == current_mem_space())
    current()->switch_to (kernel_thread);

  smas.move (mem_space(), 0);
}

/**
 * Return small address space the task is in.
 */
IMPLEMENT inline NEEDS["smas.h"]
Mword Thread::small_space( void )
{
  return smas.space_nr (mem_space());
}

/**
 * Move the task this thread belongs to to the given small address space
 */
IMPLEMENT inline NEEDS["smas.h"]
void
Thread::set_small_space(Mword nr)
{
  if (nr < 255) 
    {
      if (nr == 0) 
	{
	  // XXX Seems like everybody assumes that 0  here means: do nothing.
	  // Therefore moving a task out of a small space this way
	  // (what it should be doing according to the manual!) is 
	  // disabled by now. */
	  // smas.move (space(), 0);
	} 
      else 
	{
	  int spacesize = 1;
	  while (!(nr & 1))
	    {
	      spacesize <<= 1;
	      nr >>= 1;
	    }
	  smas.set_space_size(spacesize);
	  smas.move(mem_space(), nr >> 1);
	}
    }
}


IMPLEMENT inline NEEDS ["smas.h"]
bool
Thread::handle_smas_page_fault (Address pfa, Mword error_code,
				Ipc_err &ipc_code)
{
  Address smaddr;
  Mem_space *smspace;

  if (!smas.linear_to_small(pfa, &smspace, &smaddr))
    // give up
    return false;

  // doesn't work just yet
  if (EXPECT_FALSE (mem_space()->is_sigma0()))
    panic("Sigma0 cannot (yet) run in a small space.");

  // only interested in ourselves
  if (mem_space() != smspace)
    return false;

  // lazy updating...
  if (EXPECT_TRUE (smspace->mapped (smaddr, error_code & PF_ERR_WRITE)))
    {
      current_mem_space()->remote_update (pfa, smspace, smaddr, 1);
      return true;
    }

  // Didn't work? Seems the pager is needed.
  if (!(ipc_code 
	= handle_page_fault_pager(_pager, smaddr, error_code,
                                  L4_msg_tag::Label_page_fault)).has_error())
    {
      // now copy it in again
      // strange but right: may not be the same space as before
      current_mem_space()->remote_update (pfa, smspace, smaddr, 1);
    }
      
  return true;
}

IMPLEMENT inline NEEDS["smas.h","l4_types.h","kdb_ke.h"]
bool
Thread::handle_smas_gp_fault()
{
  if (!mem_space()->is_small_space())
    return false;

  WARN("Space exceeded? Moving task %02x out of small space.\n",
	unsigned(space()->id()));
#if 0
  kdb_ke("stop");
#endif

  smas.move(mem_space(), 0);
  return true;
}
