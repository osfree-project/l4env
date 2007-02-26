
/* Jiffies may be defined in libio. So define it weak here and start jiffies
 * thread only if not using libio. */
volatile unsigned long jiffies __attribute__ ((weak)) = 0;
