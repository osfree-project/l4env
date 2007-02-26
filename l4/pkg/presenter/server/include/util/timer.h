struct timer_services  {
  void			 (*timer_setabstime)	(double time);
  void		   (*timer_tick)	(void);
  double		 (*timer_getpercent)	(void);
  void       (*timer_start) (void);
  void       (*timer_stop)  (void);
  int        (*timer_pause) (void);
};

