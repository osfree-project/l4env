/*****************************************************************************/
/**
 * \file   l4ide/src/include/driver/driver.h
 * \brief  Driver instance handling / global stuff
 *
 * \date   01/27/2004
 * \author Marek Menzer <mm19@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef __L4IDE_DRIVER_H
#define __L4IDE_DRIVER_H

/* L4/DROPS includes */
#include <l4/sys/types.h>

/* driver includes */
#include <driver/types.h>

/* prototypes */
void
l4ide_init_driver_instances(void);

void
l4ide_driver_service_loop(void);

l4ide_driver_t *
l4ide_get_driver(l4blk_driver_id_t driver);

/* macros */
#define DRIVER_EQUAL(a, b) \
  (l4_thread_equal(a->client, b->client) && (a->handle == b->handle))

#endif /* __L4IDE_DRIVER_H */
