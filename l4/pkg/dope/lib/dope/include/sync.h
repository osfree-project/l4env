struct dopelib_mutex;

struct dopelib_mutex *dopelib_create_mutex(int init_state);
void dopelib_destroy_mutex(struct dopelib_mutex *);
void dopelib_lock_mutex(struct dopelib_mutex *);
void dopelib_unlock_mutex(struct dopelib_mutex *);
