/*!
 * \file	matrox.h
 * \brief	Matrox Gxx driver
 *
 * \date	07/2002
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef __MATROX_H_
#define __MATROX_H_

#include "pci.h"

void matrox_register(void);
void matrox_vid_init(con_accel_t *accel);

#endif

