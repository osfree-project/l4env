IMPLEMENTATION[ux-v4]:

/** Constructor.  Creates a new address space.
  *
  * @param new_number Task number of the new address space
  */
PROTECTED
Space::Space(L4_fpage utcb_area, L4_fpage kip_area)
  : Space_context()
{
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

