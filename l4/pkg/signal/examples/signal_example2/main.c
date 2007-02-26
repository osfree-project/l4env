#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/names/libnames.h>

#include <stdlib.h>

#include <l4/signal/l4signal.h>

char LOG_tag[9]   = "sig_exB";

#ifdef _DEBUG
  int _DEBUG = 1;
#else
  int _DEBUG = 0;
#endif

int main(int argc, char **argv)
{
    int i;
    l4_threadid_t the_other;

    l4_sleep(2000);
    i = names_query_name("sig_ex1", &the_other);
    LOG("queried other: %d", i);

    i = 0;
    while(1)
    {
        LOG("Task B is running: %d", i++);
        if (i == 12)
        {
            LOG("sending SIGCONT");
            kill(PID_FROM_THREADID(the_other), SIGCONT);
        }
        l4_sleep(2000);
    }
}
