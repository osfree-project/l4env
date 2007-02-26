/*!
 * \file	check.h
 * \brief	
 *
 * \date	2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef __CHECK_H_
#define __CHECK_H_

int check(int error, const char *format, ...)
  __attribute__ ((format (printf, 2, 3)));

#endif

