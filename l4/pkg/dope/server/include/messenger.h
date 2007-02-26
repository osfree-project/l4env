#if !defined EVENT
#define EVENT void
#endif


struct messenger_services {
	void	(*send_input_event)	(s32 app_id,EVENT *e,char *bindarg);
	void	(*send_action_event)(s32 app_id,char *action,char *bindarg);
};
