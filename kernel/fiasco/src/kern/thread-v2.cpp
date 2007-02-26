IMPLEMENTATION[v2]:

/** Nesting level.
    @return nesting level of thread's clan
 */
PUBLIC inline 
unsigned Thread::nest() const
{ return id().nest(); }

