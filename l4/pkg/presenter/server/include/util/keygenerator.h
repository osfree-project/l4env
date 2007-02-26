#if !defined(KEYGENERATOR)
#define KEYGENERATOR void
#endif

#define KEYGENERATOR_INFINITE_KEYS -1

struct keygenerator_services {
	KEYGENERATOR 	*(*create) 	(int number_of_keys);
	int		(*get_key) 	(KEYGENERATOR *k);
	void	        (*release_key)	(KEYGENERATOR *k, int key);
};
