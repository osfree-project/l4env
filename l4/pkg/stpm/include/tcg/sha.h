/*
 * \brief   Header for SHA1 contrib code.
 * \date    2004-09-14
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/*
 * Copyright (C) 2004  Bernhard Kauer <kauer@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef SHA_H
#define SHA_H

unsigned long
SHA1Start (unsigned long *value);

unsigned long
SHA1Update (unsigned char *data, unsigned long datalen);

unsigned long
SHA1Complete (unsigned char *data, unsigned long datalen, unsigned char *hash);

#endif
