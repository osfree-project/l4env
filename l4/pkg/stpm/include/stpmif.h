/*
 * \brief   Header for STPM interface.
 * \date    2004-06-02
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

#ifndef _STPM_STPMIF_H
#define _STPM_STPMIF_H

#define stpmif_name "stpm"

#ifdef __cplusplus
extern "C" {
#endif 

int stpm_transmit(const char *write_buf, unsigned int write_count,
                  char **read_buf, unsigned int *read_count);
int stpm_abort(void);

#ifdef __cplusplus
}
#endif 

#endif
