/**
 * \file	loader/server/src/integrity.h
 * \brief	Measure integrity and report it to Lyon virtual TPM.
 *
 * \date	21/07/2007
 * \author	Carsten Weinhold <weinhold@os.inf.tu-dresden.de> */

/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __INTEGRITY_H
#define __INTEGRITY_H

#include "integrity-types.h"
#include "app.h"

int
integrity_parse_id(const char *id64, integrity_id_t *id);

void
integrity_hash_data(app_t *app, const char *name,
                    const unsigned char *data, size_t size);

int
integrity_report_hash(const cfg_task_t *cfg, app_t *app);

#endif /* __INTEGRITY_H */
