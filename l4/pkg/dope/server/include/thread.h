#if !defined(THREAD)
#define THREAD void 
#endif

#if !defined(MUTEX)
#define MUTEX void 
#endif

struct thread_services {
	THREAD *(*create_thread) (void (*entry)(void *),void *arg);
	MUTEX  *(*create_mutex)  (int init_locked);
	void    (*destroy_mutex) (MUTEX *);
	void    (*mutex_down)    (MUTEX *);
	void    (*mutex_up)      (MUTEX *);
	s8      (*mutex_is_down) (MUTEX *);
	THREAD *(*ident2thread)  (u8 *thread_ident);
};
