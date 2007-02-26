IMPLEMENTATION[v4]:

/** Constructor.
    @param thread_lock the lock used for synchronizing access to this receiver
    @param local_id the local thread ID == pointer to the UTCB
    @param space_context the space context 
 */
PROTECTED
inline
Receiver::Receiver(Thread_lock *thread_lock, 
		   Space_context* space_context,
		   unsigned short prio, 
		   unsigned short mcp,
		   unsigned short timeslice, 
		   const L4_uid& local_id = L4_uid::NIL)
  : Context (thread_lock, space_context, local_id, prio, mcp, timeslice)
{}

