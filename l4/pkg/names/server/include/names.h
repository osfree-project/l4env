#include <l4/sys/types.h>
#include <l4/names/libnames.h>

#define	NAMES_REGISTER		1
#define	NAMES_UNREGISTER	2
#define	NAMES_QUERY_NAME	3
#define	NAMES_QUERY_ID		4
#define	NAMES_QUERY_NR		5
#define NAMES_UNREGISTER_TASK	6

#define MAX_NAME_LEN	255
#define MAX_ENTRIES	32

typedef struct {
  l4_fpage_t	fpage;
  l4_msgdope_t	size_dope;
  l4_msgdope_t	send_dope;
  l4_threadid_t	id;		/* 2x l4_umword_t !!! */
  l4_umword_t	cmd;
  l4_strdope_t	string;
} message_t;

/* Internal prototypes */
void names_init_message(message_t* msg, void* buffer);
int  names_send_message(message_t* msg);
