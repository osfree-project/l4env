#include <stdio.h>
#include "dice/dice-tracing.h"
// our derived tracing class
#include "sensor.h"

/* for Dice includes be sure to set the correct include path in your Makefile
 * ($(DICEDIR)/src).
 */

/* When defining global variables, please use static variables. */

void dice_tracing_init(int argc, char *argv[])
{
    /* This is the right place to parse the Dice arguments and check if there
     * is something for me in it.
     */
}

CTrace* dice_tracing_new_class(void)
{
    return new Sensor();
}
