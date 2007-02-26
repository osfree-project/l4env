#ifndef __main_h__
#define __main_h__

#define READ_MODE  0
#define WRITE_MODE 1

#define MAX(a,b) ((a>b)?(a):(b)) 
#define MIN(a,b) ((a<b)?(a):(b)) 

typedef struct disc_request_s
{
	int start;
	int size;
	int mode;
	double time;
	struct disc_request_s *next;
	double est_time;
} disc_request_t;

#endif /* __disc_types_h__ */
