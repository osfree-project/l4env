/*
 * \brief   Header for normal server functionality.
 * \date    2006-07-17
 * \author  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Lyon package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef __LYON_SERVER_H
#define __LYON_SERVER_H

/*
 * ***************************************************************************
 */

int
lyon_start(char *srk_auth, char *secrets_auth);

#endif /* __LYON_SERVER_H */

