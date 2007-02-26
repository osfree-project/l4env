#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>

#include <stdlib.h>
#include <l4/signal/l4signal.h>

char LOG_tag[9]   = "sig_ex";
const char *me    = "sig_ex1";

#ifdef DEBUG
    int _DEBUG = 1;
#else
    int _DEBUG = 0;
#endif

void the_thread(void *);

void my_signal_handler( int signal )
{
    LOG("I received signal %d and will do nothing about it...", signal);
}

void my_signal_handler3( int sig, siginfo_t *signal, void *context )
{
    LOG("signal: %d = %d", sig, signal->si_signo);
}

int main(int argc, char **argv)
{
    int i;
    struct sigaction sigact;
    sigset_t sigset;
    
    // register one argument signal handler
    signal( SIGCHLD, my_signal_handler );

    // set up three argument signal handler
    sigfillset(&sigset);
    sigact.sa_sigaction = my_signal_handler3;
    sigact.sa_mask      = sigset;
    sigact.sa_flags     = SA_SIGINFO;
    sigaction(SIGUSR1, &sigact, NULL);
    
    // run second thread
    l4thread_create( the_thread, NULL, L4THREAD_CREATE_SYNC );

    // register at names
    names_register(me);

    // raise signals
    i = raise(SIGCHLD);
    LOG("signal SIGCHLD sent: %d", i);
    i = raise(SIGUSR1);
    LOG("signal SIGUSR1 sent: %d", i);

    i=0;
    while(1)
    {
        int j;
        LOG("Thread 1 is running: %d", i++);
        if (i == 10)
        {
            LOG("raising SIGSTOP");
            j = raise(SIGSTOP);
            LOG("raised SIGSTOP: %d", j);
        }
        l4_sleep(1000);
    }
}

void the_thread(void * arg)
{
    l4thread_started(0);
    while(1)
    {
        LOG("Thread 2 is running.");
        l4_sleep(2000);
    }
}
