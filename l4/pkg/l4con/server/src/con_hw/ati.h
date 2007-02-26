/*!
 * \file	ati.h
 * \brief	ATI Mach64 driver
 *
 * \date	07/2002
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef __ATI_H_
#define __ATI_H_

void ati_register(void);
void ati_vid_init(con_accel_t *accel);
void ati_test_for_card_disappeared(void);

#endif

