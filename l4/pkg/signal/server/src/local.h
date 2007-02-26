#ifndef __LOCAL_SIGNAL_SERVER_H
#define __LOCAL_SIGNAL_SERVER_H

#include <signal.h>
#include <l4/sys/types.h>
#include <l4/semaphore/semaphore.h>
#include <l4/util/rdtsc.h>

#define ALARM_TIME_INVALID 0

/* Signal server stores the registered tasks in a list of
 * task_signal_structs. These structs contain the signal
 * handler of the specific task and a list of signal_structs
 * describing a signal
 */

typedef struct signal_struct
{
    l4_threadid_t   recipient;              // threadid of the recipient
    int             recipient_is_thread;    // 1 = recipient is thread, else rec. is task
    siginfo_t       signal;                 // signal data
    struct signal_struct *next;             // pointer to the next signal pending
} signal_struct_t;

typedef struct task_signal_struct
{
    int             taskno;                 // task
    l4_threadid_t   signal_handler;         // signal handler thread for the task
    signal_struct_t *first_signal;          // pointer to the first pending signal
    int             waiting;                // 1 if signal handler is doing a
                                            // receive_signal() at the moment
    l4_cpu_time_t   t_alarm;                  // timestamp for SIGALRM to be sent
    l4semaphore_t   taskstruct_semaphore;   // semaphore for concurrent access
    struct task_signal_struct *next;        // next element in task list
                                    
} task_signal_struct_t;

/* TASK LIST MANAGEMENT / ACCESS */

/* These functions are meant to manage the task and signal data 
 * stored by the signal server. At the moment there is a list of registered tasks
 * ordered by ascending task numbers. The signal list for each task is a fifo queue.
 * This could be changed in future versions.
 */

// the head of the task list
task_signal_struct_t *task_list_head;
// enqueue signal handler into task list
int enqueue_handler( l4_threadid_t *sigthread );
// dequeue signal handler from task list
void dequeue_handler( l4_threadid_t *sigthread );
// get the task signal struct belonging to task taskno
task_signal_struct_t *get_task_struct( int taskno );
// enqueue signal to signal list
int enqueue_signal( const l4_threadid_t *thread, const siginfo_t *signal, int rec_is_thread );
// reply to client
void send_signal( task_signal_struct_t *task );


/* DEBUG STUFF 
 * 
 * These functions provide some debugging capabilities.
 */

// dump task list to log
void dump_task_list(void);
// dump signal list
void dump_signal_list( signal_struct_t *first );

// alarm scheduling threadfunc
void alarm_scheduler(void *);

// the main thread
l4_threadid_t main_thread;

#endif
