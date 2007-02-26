/**
 * \file   server/src/local.h
 * \brief  Internal interfaces
 *
 * \date   10/01/2004
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __SERVER_SRC_LOCAL_H_
#define __SERVER_SRC_LOCAL_H_

#define APP_NAME "\\m/"

extern int dmon_logsrv_init(void);
extern int dmon_logger_init(long app_id);
extern int dmon_dumper_init(long app_id);

#endif
