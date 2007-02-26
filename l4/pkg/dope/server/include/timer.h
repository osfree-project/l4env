struct timer_services {
	u32 	(*get_time)	(void);
	u32		(*get_diff) (u32 time1,u32 time2);
	void	(*usleep)	(u32 num_usec);
};
