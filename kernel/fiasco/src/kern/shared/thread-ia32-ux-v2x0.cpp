IMPLEMENTATION[ia32-ux-v2x0]:

/** Task number for debugging purposes.
 * May be changed to show sth. more useful for the debugger.
 * Do not rely on this method in kernel code.
 * This method is ABI specific.  For v2/x0 it returns the task number.
 */
PUBLIC inline Mword Thread::debug_taskno() 
{ 
  return space_index(); 
}

/** Thread number for debugging purposes. 
 * see also: debug_taskno()
 * This method is ABI specific.  For v2/x0 it returns the lthread number.
 */
PUBLIC inline Mword Thread::debug_threadno()
{
  return id().lthread();
}

