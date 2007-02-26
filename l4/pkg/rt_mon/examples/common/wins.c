#include <l4/log/l4log.h>
#include <l4/semaphore/semaphore.h>

#include "wins.h"

wins_t wins[MAX_WINS];


int wins_index_for_id(int id)
{
    int i;

    for (i = 0; i < MAX_WINS; i++)
    {
        if (wins[i].id == id)
            return i;
    }
    return -1;
}

int wins_index_for_fd(int fd)
{
    int i;

    for (i = 0; i < MAX_WINS; i++)
    {
        if (wins[i].fd == fd)
            return i;
    }
    return -1;
}

int wins_get_new(void)
{
    int i;

    for (i = 0; i < MAX_WINS; i++)
    {
        if (wins[i].visible == 0)
            return i;
    }
    return -1;
}

void wins_init(void)
{
    int i;

    for (i = 0; i < MAX_WINS; i++)
    {
        wins_init_win(i);
    }
}

void wins_init_win(int w)
{
    wins[w].visible = 0;
    wins[w].local   = NULL;
    wins[w].sem     = L4SEMAPHORE_UNLOCKED;
    wins[w].id      = -1;
    wins[w].fd      = -1;
}
