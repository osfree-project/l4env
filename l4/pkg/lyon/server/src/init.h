/*
 * \brief   Header for TPM init functionality.
 * \date    2006-07-14
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

#ifndef __LYON_INIT_H
#define __LYON_INIT_H

/*
 * ***************************************************************************
 */

int
lyon_init(char *old_owner_auth, char *owner_auth, char *srk_auth,
          char *secrets_auth, char *counter_auth);

#endif /* __LYON_INIT_H */

