#include "color.h"

/** Translate a value and a scale into a rgb color value to be displayed.
 */
void get_color_for_val(int val, double scale, int *r, int *g, int *b)
{
    if (val <= 0)
    {
        *r = *g = *b = 0;
        return;
    }
    val /= scale;
    // currently we use a fading from red to green and black for 0
    if (val > 255)
    {
        *r = *b = 0; // max color is green
        *g = 255;
    }
    else
    {
        *g = val;
        *r = 255 - val;
        *b = 0;
    }
}
