/*
 * \brief   dataprovider class module
 * \date    2006-06-19
 * \author  Steffen Liebergeld <s1010824@inf.tu-dresden.de>
 *
 * Implementation of all that is needed to have an internal timer.
 */

#include "presenter_conf.h"
#include "module_names.h"
#include "timer.h"

/* values in seconds */
/* abstime is the estimated duration of the talk */
static double abstime;
/* curtime is the time the task lasted up to now */
static double curtime;
/* 1 if running, 0 if not */
static int running;

int init_timer(struct presenter_services *p);

/* set duration of the task in minutes */
static void timer_setabstime(double time);
static void timer_tick(void);
/* get the percentage of duration the task took in spite of the
 * estimated duration */
static double timer_getpercent(void);

static void timer_start(void);
static void timer_stop(void);
static int timer_pause(void);

/************************/
/**  SPECIFIC METHODS  **/
/************************/

static void timer_setabstime(double time) {
    /* a presentation shall never last longer than a day, and never be
     * shorter than a minute */
    if (time < 1 || time > 1224) {
        LOG ("Error, the given duration (%d) makes no sense.", (int)time);
        return;
    }
    abstime = time*60;
}

static void timer_tick(void) {
    if (!running)
        return;
    if (curtime < abstime)
        curtime++;
}

static double timer_getpercent(void) {
     if (abstime == 0) return -1;
     return curtime/abstime;
}

static void timer_start(void) {
    running = 1;
    curtime = 0;
}

static void timer_stop(void) {
    running = 0;
    curtime = 0;
}

static int timer_pause(void) {
    if (running)
        running = 0;
    else
        running = 1;
    return running;
}

/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct timer_services services = {
    timer_setabstime,
    timer_tick,
    timer_getpercent,
    timer_start,
    timer_stop,
    timer_pause,
};

int init_timer(struct presenter_services *p) {

    abstime = 0;
    curtime = 0;
  
    p->register_module(TIMER_MODULE,&services);
  
    return 1;
}
