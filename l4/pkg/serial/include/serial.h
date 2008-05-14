/*!
 * \file   serial/include/serial.h
 * \brief  Serial functions
 *
 * \date   10/21/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __SERIAL_INCLUDE_SERIAL_H_
#define __SERIAL_INCLUDE_SERIAL_H_

#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

/*!\brief Attach to serial port.
 *
 * \param  comport	1: com1 (0x3f8 IRQ 4), 2: com2 (0x2f8, IRQ 3)
 *
 * \retval 0		OK
 * \retval -1		invalid comport
 * \retval -2		IRQ attachment failed
 * \retval >0		IPC error of initial communication with IRQ thread
 */
L4_CV int l4serial_init(int tid, int comport);

/*!\brief Send a string to the selected serial port
 *
 * \param  addr		base address of the string
 * \param  len		length of the string
 *
 * \retval 0		OK
 * \retval -1		lib not initialized
 * \retval -2		someone else is sending a string
 * \retval -3		someone else is sending a string
 * \retval >0		IPC error
 */
L4_CV int l4serial_outstring(const char*addr, unsigned len);

/*!\brief Read a character from the serial line, blocking
 *
 * \retval >=0		OK, character read
 * \retval -1		lib not initialized
 * \retval -2		IPC error
 */
L4_CV int l4serial_readbyte(void);

EXTERN_C_END
#endif
