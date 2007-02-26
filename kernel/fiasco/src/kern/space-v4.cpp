IMPLEMENTATION[v4]:

#include "globals.h"

/** Space pointer.
    @return Pointer to the task to which this Space instance 
            belongs
    
 */
PUBLIC inline Space*
Space::space()
{
  // A really stupid method.  However we need it to hold code
  // generic between v2/x0 and v4 ABI.
  return this;
}

/** Task number of this task's chief.
    Dummy method for v4 ABI.
    @return 
 */
PUBLIC inline Mword 
Space::chief() const		// returns chief number
{
  return 0;
}

/** Tests if a task is the root task.
    @return true if the task is root, false otherwise.
*/
PUBLIC inline 
bool Space::is_root()
{
  return this == global_root_task;
}

