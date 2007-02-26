/*
 * \brief   Player User interface for  VERNER's control component
 * \date    2004-05-14
 * \author  Carsten Rietzschel <cr7@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2004  Carsten Rietzschel  <cr7@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef __EXTCTRL_H_
#define __EXTCTRL_H_

/* dope */
#include <dopelib.h>

/* init of ec windows */
void ec_init (void);

/* callback for extra controls' window */
void ec_callback (dope_event * e, void *arg);

#endif
