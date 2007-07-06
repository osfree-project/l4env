
/*** RETRIEVE TIMING VALUES FROM FIASCO ***/
void minitop_get_values(void);

/*** SORT ENTRIES SO THAT BUSIEST THREADS ARE THE FIRST ***/
void minitop_sort_values(void);

/*** REQUEST CURRENT NUMBER OF TABLE ENTRIES ***/
int minitop_get_num(void);

/*** REQUEST PERCENT CPU LOAD ***/
float minitop_get_percent(unsigned int idx);

/*** REQUEST MICROSECONDS VALUE ***/
long minitop_get_usec(unsigned int idx);

/*** REQUEST TASK ID ***/
int minitop_get_taskid(unsigned int idx);

/*** REQUEST THREAD ID ***/
int minitop_get_threadid(unsigned int idx);
