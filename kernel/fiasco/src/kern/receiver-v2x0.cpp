IMPLEMENTATION[v2x0]:

/** Constructor.
    @param thread_lock the lock used for synchronizing access to this receiver
    @param space_context the space context 
 */
PROTECTED
inline
Receiver::Receiver(Thread_lock *thread_lock, 
		   Space_context* space_context,
		   unsigned short prio, unsigned short mcp,
		   unsigned short timeslice)
  : Context (thread_lock, space_context, prio, mcp, timeslice)
{}

