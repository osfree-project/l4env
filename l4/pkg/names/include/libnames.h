/*!
 * \file   names/include/libnames.h
 * \brief  names client library header file
 *
 * \date   05/27/2003
 * \author Uwe Dannowski <Uwe.Dannowski@ira.uka.de>
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __NAMES_INCLUDE_LIBNAMES_H_
#define __NAMES_INCLUDE_LIBNAMES_H_

#include <l4/sys/types.h>
#include <l4/sys/compiler.h>
#include <l4/names/__names_defs.h>

EXTERN_C_BEGIN

int names_register(const char* name);
int names_register_thread_weak(const char* name, l4_threadid_t id);
int names_unregister(const char* name);
int names_unregister_thread(const char* name, l4_threadid_t id);
int names_query_name(const char* name, l4_threadid_t* id);
int names_query_id(const l4_threadid_t id, char* name, const int length);
int names_waitfor_name(const char* name, l4_threadid_t* id, const int timeout);
int names_query_nr(int nr, char* name, int length, l4_threadid_t *id);
int names_unregister_task(l4_threadid_t tid);
int names_dump(void);

EXTERN_C_END

#endif /* ! __NAMES_INCLUDE_LIBNAMES_H_ */
