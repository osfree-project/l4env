
IMPLEMENTATION[ia32,amd64]:

IMPLEMENT inline
void
Vmem_alloc::page_map (void * /*address*/, int /*order*/, Zero_fill /*zf*/,
		      Address /*phys*/)
{}

IMPLEMENT inline
void
Vmem_alloc::page_unmap (void * /*address*/, int /*order*/)
{}
