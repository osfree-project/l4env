
/*** START L4 THREAD ***/
void minitop_thread_create(void (*entry)(void *),void *arg);

/*** WAIT SPECIFIED NUMBER OF MICROSECONDS ***/
void minitop_usleep(unsigned long usec);
