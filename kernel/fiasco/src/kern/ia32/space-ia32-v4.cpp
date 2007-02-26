IMPLEMENTATION[ia32-v4]:

/** Constructor.  Creates a new address space.
  *
  * Registration may fail (if a task with the given number already
  * exists, or if another thread creates an address space for the same
  * task number concurrently).  In this case, the newly-created
  * address space should be deleted again.
  * 
  * XXX may creation fail in v4, too?
  */
PROTECTED
Space::Space (L4_fpage utcb_area, L4_fpage kip_area)
{
  Kmem::dir_init(_dir);		// copy current shared kernel page directory

  // Scribble thread counter, utcb area, and kip area into an unused
  // part of the page directory.
  //
  // Entries must not get INTEL_PDE_VALID (bit 0 on x86).  No problem
  // since the corresponding bit in the L4_fpage is the EXEC bit and
  // is not used here.

  assert ((utcb_area.raw() & INTEL_PDE_VALID) == 0);
  assert (( kip_area.raw() & INTEL_PDE_VALID) == 0);

  _dir[number_index] = 0;		
  _dir[chief_index]  = utcb_area.raw();
  _dir[kip_index]    = kip_area.raw();
}
