/**
 * \file	loader/server/src/integrity-types.h
 * \brief	Define implementation-independent types for integrity
 *              measurements.
 *
 * \date	24/07/2007
 * \author	Carsten Weinhold <weinhold@os.inf.tu-dresden.de> */

/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __INGERITY_TYPES_H
#define __INGERITY_TYPES_H

#include <l4/crypto/sha1.h>
#include <l4/lyon/lyon.h>

typedef lyon_id_t  integrity_id_t;
typedef char       integrity_hash_t[SHA1_DIGEST_SIZE];

#endif /* __INTEGRITY_TYPES_H */

