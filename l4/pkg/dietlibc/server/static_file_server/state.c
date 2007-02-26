#include "state.h"
#include <l4/log/l4log.h>

extern int _DEBUG;

static_file_t files[MAX_STATIC_FILES];

extern int _binary____hosts_start;
extern int _binary____hosts_size;

void state_init(void)
{
    int i;
    for (i = 0; i < MAX_STATIC_FILES; i++)
    {
        files[i].name = NULL;
        files[i].data = NULL;
        files[i].length = 0;
    }

    files[0].name = "hosts";
     /* get adr. of linked file */
    files[0].data = (char *)&_binary____hosts_start;
    files[0].length = (int)&_binary____hosts_size; /* size of linked file */
}
