extern int *count_mand;	// id==1
extern int *count_opt;		// id==2
extern int show_mand_preempts;
extern int show_opt_preempts;

void preempter_thread (void*arg);
