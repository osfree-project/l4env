#ifndef __RT_MON_EXAMPLES_COMMON_WINS_H_
#define __RT_MON_EXAMPLES_COMMON_WINS_H_

#include <l4/semaphore/semaphore.h>

#include <l4/rt_mon/types.h>

#define MAX_WINS 80

#define WINDOW_H      100    // height for all visalization windows
#define WINDOW_LIST_W 300    // width for event list windows
#define BUFFER_LIST   500    // buffer size for local event buffers

#define BIND_BUTTON_OFFSET 100
#define BIND_BUTTON_SCALER 100


/* Represents one window to be displayed
 */
typedef struct wins_s
{
    int              visible;// window visible? refresh necessary?
    int              fd;     // stores the open fd
    int              id;     // contains the object id of the opened file
    int              type;   // type of monitored data (histogram, list, ...)
    l4_int16_t     * vscr;   // pointer to vscreen
    int              w, h;   // window dimensions
    void           * data;   // pointer to data shared via dataspace
    void           * local;  // local pointer to specific data (e.g. for lists)
    l4semaphore_t    sem;    // well, its a semaphore
} wins_t;

// fixme: factore out some access methods for the ring buffer

/* Ringbuffer to locally store events from event list
 */
typedef struct ring_buf_s
{
    int index;               // index to position to be written next
    int max;                 // size of the following ring buffer data
    rt_mon_time_t data[0];   // ring buffer data
} ring_buf_t;


/* fancy event list windows need some data
 */
typedef struct fancy_list_s
{
    ring_buf_t *  vals;
    ring_buf_t *  avgs;
    rt_mon_time_t low;
    rt_mon_time_t high;
    rt_mon_time_t min;
    rt_mon_time_t max;
    double        avg;
    int           mouse_posx;
    int           mouse_posy;
    int           mouse_but;
} fancy_list_t;

extern wins_t wins[MAX_WINS];


int wins_index_for_id(int id);
int wins_index_for_fd(int fd);
int wins_get_new(void);
void wins_init(void);
void wins_init_win(int w);

#endif
