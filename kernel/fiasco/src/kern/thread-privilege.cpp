IMPLEMENTATION [pl0_hack]:

#include "entry_frame.h"
#include "feature.h"
#include "l4_types.h"
#include "space_index.h"

KIP_KERNEL_FEATURE("pl0_hack");

/** L4 system call privilege control.  */
PUBLIC inline NOEXPORT ALWAYS_INLINE
void
Thread::sys_privilege_control()
{
  Thread *dst;
  Sys_thread_privctrl_frame *regs =
    sys_frame_cast<Sys_thread_privctrl_frame>(this->regs());

  if (! Space_index::has_pl0_privilege(current_thread()->space_index()))
    goto error;

  switch(regs->command())
    {
    case Space_index::Prctrl_idt:
      Space_index::set_entry(current_thread()->space_index(),
			     regs->entry_func());
      regs->ret_val(0);
      return;

    case Space_index::Prctrl_transfer:
      dst = lookup(regs->dst());
      if (EXPECT_FALSE (!dst))
	goto error;

      Space_index::grant_privilege(dst->space_index());
      regs->ret_val(0);
      return;
    }

error:
  regs->ret_val((Mword)-1);
}

PUBLIC static inline NOEXPORT ALWAYS_INLINE
Mword
Thread::privilege_entry(Mword *pfn)
{
  unsigned pos = current_thread()->space_index();

  // Range check performed in Space_index::has_pl0_privilege()
  if (EXPECT_FALSE(!Space_index::has_pl0_privilege(pos)))
    return 0;

  *pfn = Space_index::get_entry(pos);
  return 1;
}

extern "C"
void
sys_priv_control_wrapper()
{ current_thread()->sys_privilege_control(); }

extern "C" FIASCO_FASTCALL
Mword
sys_priv_entry_wrapper(Mword *pfn)
{ return Thread::privilege_entry(pfn); }

