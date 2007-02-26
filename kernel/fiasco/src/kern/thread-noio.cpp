INTERFACE:

IMPLEMENTATION [noio]:


inline
bool
Thread::get_ioport(Address /*eip*/, trap_state * /*ts*/,
		   unsigned * /*port*/, unsigned * /*size*/)
{
  return false;
}

