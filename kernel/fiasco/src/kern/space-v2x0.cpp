INTERFACE:

#include "space_index.h"

IMPLEMENTATION[v2x0]:

/** Task number.
    @return Number of the task to which this Space instance 
             belongs
 */
PUBLIC inline Space_index
Space::space() const		// returns task number
{
  return Space_index(_dir[number_index] >> 8);
}

/** Task number of this task's chief.
    @return Task number of this task's chief
 */
PUBLIC inline Space_index 
Space::chief() const		// returns chief number
{
  return Space_index(_dir[chief_index] >> 8);
}
