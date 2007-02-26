/*!
 * \file   omega0/examples/pit/config.h
 * \brief  PIT configuration
 *
 * \date   28/02/2001
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __OMEGA0_EXAMPLES_PIT_CONFIG_H_
#define __OMEGA0_EXAMPLES_PIT_CONFIG_H_

/* define USE_LOCKING if you want to use locking on pic-manipulation.
*/
//#define USE_LOCKING

/* define USE_CLISTI if you want cli/sti-pairs arout pic-manipulation.
*/
#define USE_CLISTI

/* define USE_OMEGA0 if you want to use omega0-server. Dont define it to
   compare against native implementation.
*/
#define USE_OMEGA0



#endif

