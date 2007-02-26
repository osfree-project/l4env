IMPLEMENTATION[nosmas]:

IMPLEMENT inline
bool Thread::handle_smas_page_fault ( Address, Mword, L4_msgdope & )
{
  return false;
}

/**
 * Additional things to do before killing, when using small spaces.
 */
IMPLEMENT inline
void Thread::kill_small_space (void)
{
  //nothing
}


/**
 * Return small address space the task is in.
 */
IMPLEMENT inline
Mword Thread::small_space( void )
{
  return 0;   // unimplemented
}

/**
 * Move the task this thread belongs to to the given small address space
 */
IMPLEMENT inline
void Thread::set_small_space( Mword /*nr*/)
{
  // nothing
}
