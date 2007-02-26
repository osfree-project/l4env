#if !defined(BUTTON)
#define BUTTON struct public_button
#endif

#if !defined(WIDGETARG)
#define WIDGETARG void
#endif

struct button_methods;

struct public_button {
	struct widget_methods 	*gen;
	struct button_methods 	*but;
};

struct button_methods {
	void	(*set_text)		(BUTTON *,char *new_txt);
	char   *(*get_text)		(BUTTON *);
	void	(*set_font)		(BUTTON *,s32 new_font_id);
	s32  (*get_font)		(BUTTON *);
	void	(*set_style)	(BUTTON *,s32 new_style);
	s32  (*get_style)	(BUTTON *);
	void	(*set_click)	(BUTTON *,void (*)(BUTTON *));
	void	(*set_release)	(BUTTON *,void (*)(BUTTON *));
};

struct button_services {
	BUTTON	*(*create)	(void);
};
