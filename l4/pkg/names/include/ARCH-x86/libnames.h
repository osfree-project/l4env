/*!
 * \file   names/include/ARCH-x86/libnames.h
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
#ifndef __NAMES_INCLUDE_ARCH_X86_LIBNAMES_H_
#define __NAMES_INCLUDE_ARCH_X86_LIBNAMES_H_

#include <l4/sys/types.h>

/*!\brief Maximum length of a string to register with names */
#define NAMES_MAX_NAME_LEN	255
/*!\brief Maximum number of entries the nameserver handles */
#define NAMES_MAX_ENTRIES	32

#ifdef __cplusplus
extern "C" {
#endif
  
int names_register(const char* name);
int names_unregister(const char* name);
int names_query_name(const char* name, l4_threadid_t* id);
int names_query_id(const l4_threadid_t id, char* name, const int length);
int names_waitfor_name(const char* name, l4_threadid_t* id, const int timeout);
int names_query_nr(int nr, char* name, int length, l4_threadid_t *id);
int names_unregister_task(l4_threadid_t tid);
  
#ifdef __cplusplus
}
#endif

#endif
