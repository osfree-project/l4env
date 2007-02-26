#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <dice/dice.h>

#include <unistd.h>

#include <l4/signal/l4signal.h>

#ifdef DEBUG
    int _DEBUG = 1;
#else
    int _DEBUG = 0;
#endif

char LOG_tag[9]   = "sig_alarm";

void alarmfunc(int s)
{
    LOG("SIGALARM...");
}

int main(int argc, char **argv)
{
    int i=0;

    signal(SIGALRM, alarmfunc);

    while(1)
    {
        LOG("Task running: %d", i++);
        l4_sleep(1000);

        if (i%5 == 0)
        {
            LOG("Alarm in 3 seconds.");
            signal(SIGALRM, alarmfunc);
            alarm(3);
        }
    }
}
