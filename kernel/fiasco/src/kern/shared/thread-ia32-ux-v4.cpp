IMPLEMENTATION[ia32-ux-v4]:

/** Task number for debugging purposes.
 * May be changed to show sth. more useful for the debugger.
 * Do not rely on this method in kernel code.
 * This method is ABI specific.  For v4 it returns the space pointer
 * casted to an Mword.
 */
PUBLIC inline Mword Thread::debug_taskno() 
{ 
  return (Mword) space();
}

/** Thread number for debugging purposes. 
 * This method is ABI specific.  For v4 it returns the global thread ID.
 * see also: debug_taskno()
 */
PUBLIC inline Mword Thread::debug_threadno()
{
  return id().gthread();
}

