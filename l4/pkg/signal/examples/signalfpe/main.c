#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/thread/thread.h>

#include <l4/signal/l4signal.h>
#include <signal.h>

char LOG_tag[9]   = "except";

#ifdef DEBUG
int _DEBUG = 1;
#else
int _DEBUG = 0;
#endif

void fp_handler(int sig)
{
    LOG("Signal FPE received - POSIX does not define any ");
    LOG("behaviour after return from this signal handler...");
    LOG("Therefore we will loop here forever.");

    l4_sleep_forever();
}

void the_thread(void *t)
{
    volatile int i=0;
    l4thread_started(0);
    l4signal_init_idt();
    l4_sleep(5000);
    LOG("thread 2 running.");
    l4_sleep(2000);
    LOG("thread 2 still running.");
    l4_sleep(2000);

    // divide by zero
    LOG("dividing by zero for the next time");
    i /= 0;

    l4_sleep_forever();
}

int main(int argc, char **argv)
{
    volatile int i;

    // prepare idt to receive kernel signals
    l4signal_init_idt();
    LOG("idt initialized.");

    signal(SIGFPE, fp_handler);

    l4thread_create(the_thread, NULL, L4THREAD_CREATE_SYNC);

    l4_sleep(2000);
    // divide by zero
    LOG("dividing by zero.");
    i /= 0;

    l4_sleep_forever();
    
    return 0;
}
