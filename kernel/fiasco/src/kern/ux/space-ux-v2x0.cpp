IMPLEMENTATION[ux-v2x0]:

/** Constructor.  Creates a new address space and registers it with
  * Space_index.
  *
  * Registration may fail (if a task with the given number already
  * exists, or if another thread creates an address space for the same
  * task number concurrently).  In this case, the newly-created
  * address space should be deleted again.
  * 
  * @param new_number Task number of the new address space
  */
PROTECTED
Space::Space(unsigned new_number)
      :Space_context (new_number)
{
  // scribble task/chief numbers into an unused part of the page directory

  _dir[number_index] = new_number << 8;		// must not be INTEL_PDE_VALID
  _dir[chief_index]  = Space_index(new_number).chief() << 8;	// dito

  Space_index::add (this, new_number);		// register in task table
}

