#if !defined(THREAD)
#define THREAD void
#endif
  
#if !defined(VSCREEN)
#define VSCREEN void
#endif

struct vscr_server_services {
	THREAD *(*start) (VSCREEN *);
};
