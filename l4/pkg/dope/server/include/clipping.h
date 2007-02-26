struct clipping_services {
	void	 (*push)		(long x1, long y1, long x2, long y2);
	void	 (*pop)			(void);
	void	 (*reset)		(void);
	void	 (*set_range)	(long x1, long y1, long x2, long y2);
	long	 (*get_x1)		(void);
	long	 (*get_y1)		(void);
	long	 (*get_x2)		(void);
	long	 (*get_y2)		(void);
};
