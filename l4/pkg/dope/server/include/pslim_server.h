#if !defined(THREAD)
#define THREAD void 
#endif

#if !defined(PSLIM)
#define PSLIM void
#endif

struct pslim_server_services {
	THREAD *(*start) (PSLIM *);
};
