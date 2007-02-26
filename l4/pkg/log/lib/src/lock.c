/*!
 * \file   log/lib/src/lock.c
 * \brief  Locking for the LOG-macros
 *
 * \date   02/13/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 * The LOG-macros use a common lock to make a 'LOG()'-call atomar. 
 * Although our printf is atomar, we need the additional locking as
 * the LOG-calls do multiple printf-calls, which should not be
 * intermixed.
 */
#include "internal.h"

/* if we use locks, we can use a global message buffer */
#ifdef __USE_L4WQLOCKS__
wq_lock_queue_base LOG_lock_queue={NULL};
#endif
