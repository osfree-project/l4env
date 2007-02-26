IMPLEMENTATION[ia32-ux-debug]:


PUBLIC
int
Thread::is_valid()
{
  return    Kmem::is_tcb_page_fault((Address)this, 0)
	 && is_mapped()
	 && (((Address)this & (Config::thread_block_size-1)) == 0)
	 && (state() != Thread_invalid);
}

