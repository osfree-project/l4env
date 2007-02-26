/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/lib/src/irq_stat.c
 * \brief  irq_stat symbol
 *
 * \date   08/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <linux/interrupt.h>

irq_cpustat_t irq_stat[NR_CPUS];
