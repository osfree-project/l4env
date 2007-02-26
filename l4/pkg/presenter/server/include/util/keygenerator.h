#if !defined(KEYGENERATOR)
#define KEYGENERATOR void
#endif

struct keygenerator_services {
	KEYGENERATOR 	*(*create) (void);
	int		(*get_key) (KEYGENERATOR *k);
};
