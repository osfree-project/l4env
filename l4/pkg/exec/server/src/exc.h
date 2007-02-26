/*!
 * \file	exc.h
 * \brief	
 *
 * \date	2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef __EXC_H_
#define __EXC_H_

#include <l4/env/env.h>

void
exc_init_env(int id, l4env_infopage_t *env);

#endif /* __L4_EXEC_SERVER_EXC_H */

