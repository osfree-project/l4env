#if !defined(HASHTAB)
#define HASHTAB void
#endif

#if !defined(THREAD)
#define THREAD void
#endif


struct appman_services {
	s32      (*reg_app)        (char *app_name);
	s32      (*unreg_app)      (u32 app_id);
	HASHTAB *(*get_widgets)    (u32 app_id);
	HASHTAB *(*get_variables)  (u32 app_id);
	void     (*reg_listener)   (s32 app_id,THREAD *listener);
	THREAD  *(*get_listener)   (s32 app_id);
	char    *(*get_app_name)   (s32 app_id);
	void     (*reg_app_thread) (s32 app_id,THREAD *app_thread);
	THREAD  *(*get_app_thread) (s32 app_id);
};
