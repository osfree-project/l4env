#if !defined(SHAREDMEM)
#define SHAREDMEM struct shared_memory_struct
#endif

#if !defined(THREAD)
#define THREAD void 
#endif

struct shared_memory_struct;

struct sharedmem_services {
	SHAREDMEM *(*alloc)       (long size);
	void       (*free)        (SHAREDMEM *sm);
	void      *(*get_address) (SHAREDMEM *sm);
	void       (*get_ident)   (SHAREDMEM *sm, u8 *dst_ident_buf);
	void       (*share)       (SHAREDMEM *sm, THREAD *dst_thread);
};
