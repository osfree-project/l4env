IMPLEMENTATION[ia32-ux-v4]:

#include "cstdio"

PROTECTED inline NEEDS ["cstdio"]
void Context::update_utcb_ptr() {

  assert (local_id().is_nil()		// for kernel thread
	  || local_id().is_local());	// for user threads

  printf ("Utcb ptr %x set to: %x\n", 
	  (Mword) global_utcb_ptr, local_id().lthread());

  *global_utcb_ptr = local_id().lthread();
}
