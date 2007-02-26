/*****************************************************************************/
/**
 * \file   l4ide/src/include/driver/command.h 
 * \brief  L4IDE command thread.
 *
 * \date   01/27/2004
 * \author Marek Menzer <mm19@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef __L4IDE_COMMAND_H
#define __L4IDE_COMMAND_H

/* driver includes */
#include <driver/types.h>

extern int get_disk_name(int num);

int
l4ide_start_command_thread(l4ide_driver_t * driver);

void
l4ide_shutdown_command_thread(l4ide_driver_t * driver);

#endif /* __L4IDE_COMMAND_H */
