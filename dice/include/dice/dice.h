/*
 * This file is distributed along with the 'dice' package. It contains
 * constants, types and functions needed by the code generated by dice.
 *
 */

#ifndef __DICE_DICE_H__
#define __DICE_DICE_H__

#if !defined(L4API_l4v2) && !defined(L4API_l4x0) && !defined(L4API_l4x2) && !defined(L4API_l4v4) && !defined(L4API_linux)
#warning no L4 API set
#define L4API_l4v2
#endif

/* common macros */

// needed for str* functions
#if !defined(LINUX_ON_L4) && !defined(CONFIG_L4_LINUX)
#include <string.h>
#endif

#undef _dice_alloca
#ifdef __GNUC__
#define _dice_alloca(size)	                        \
  ((void*)((((unsigned long)__builtin_alloca ((size)    \
    + sizeof (unsigned long))) + sizeof (unsigned long) \
    -1) & ~(sizeof (unsigned long) -1)))
#endif

#define DICE_REPLY    1
#define DICE_NO_REPLY 2

#ifndef DICE_IID_BITS
#define DICE_IID_BITS 20
#endif

#ifndef DICE_FID_MASK
#define DICE_FID_MASK 0xfffff
#endif

#ifndef DICE_STR
#define __DICE_STR(x) #x
#define DICE_STR(x) __DICE_STR(x)
#endif

#include "dice/dice-corba-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * declares CORBA_alloc and CORBA_free - implementation has to be user provided
 */
void* CORBA_alloc(unsigned long size);
void CORBA_free(void *ptr);

#ifdef __cplusplus
}
#endif


/* testsuite functions and macros */
#if defined(DICE_TESTSUITE)
#include "dice/dice-testsuite.h"
#endif

#if defined(L4API_linux)
#include "dice/dice-sockets.h"
#else
#if defined(L4API_l4v2)
#include "dice/dice-l4-v2.h"
#else
#if defined(L4API_l4x0)
#include "dice/dice-l4-x0.h"
#else
#if defined(L4API_l4x2) || defined(L4API_l4v4)
#include "dice/dice-l4-v4.h"
#endif // L4X2
#endif // L4X0
#endif // L4V2
#endif // SOCKET

/* Convenience macros.
 *
 * They're here, because we need dice_default_environment and
 * dice_default_server_environment to be declared, which are API dependant. The
 * macros however are independent.
 */
#ifndef DICE_DECLARE_ENV
#ifdef __cplusplus
#define DICE_DECLARE_ENV(env) CORBA_Environment env
#else
#define DICE_DECLARE_ENV(env) CORBA_Environment env = dice_default_environment
#endif /* __cplusplus */
#endif /* !DICE_DECLARE_ENV */

#ifndef DICE_DECLARE_SERVER_ENV
#ifdef __cplusplus
#define DICE_DECLARE_SERVER_ENV(env) CORBA_Server_Environment env
#else
#define DICE_DECLARE_SERVER_ENV(env) \
             CORBA_Server_Environment env = dice_default_server_environment
#endif /* __cplusplus */
#endif /* !DICE_DECLARE_SERVER_ENV */

/* common functions for the CORBA environement */
#define CORBA_NO_EXCEPTION      0
#define CORBA_USER_EXCEPTION    1
#define CORBA_SYSTEM_EXCEPTION  2

#define CORBA_DICE_EXCEPTION_NONE 0
#define CORBA_DICE_EXCEPTION_WRONG_OPCODE 1
#define CORBA_DICE_EXCEPTION_IPC_ERROR 2
#define CORBA_DICE_INTERNAL_IPC_ERROR 3
#define CORBA_DICE_EXCEPTION_COUNT 4

static CORBA_char* __CORBA_Exception_Repository[CORBA_DICE_EXCEPTION_COUNT+1] = { "none", "wrong opcode", "ipc error", "internal ipc error", 0 };

#ifdef __cplusplus
extern "C" {
#endif

static inline
void CORBA_exception_free(CORBA_Environment *ev)
{
  if (ev->major != CORBA_NO_EXCEPTION)
    {
#ifdef L4API_linux
      //ev->free(ev->param);
      ev->param = 0;
#else
      //ev->free(ev->_p.param);    
      ev->_p.param = 0;
#endif
    }
  ev->major = CORBA_NO_EXCEPTION;
  ev->repos_id = CORBA_DICE_EXCEPTION_NONE;
}


static inline
void CORBA_exception_set(
    CORBA_Environment *ev,
    CORBA_exception_type major,
//    CORBA_char *except_repos_id,
    CORBA_exception_type repos_id,
    void *param)
{
  CORBA_exception_free(ev);
  ev->major = major;
  if (major != CORBA_NO_EXCEPTION)
    {
      ev->repos_id = repos_id;
#ifdef L4API_linux
      ev->param = param;
#else
      ev->_p.param = param;
#endif
    }
}

static inline
CORBA_char* CORBA_exception_id(CORBA_Environment *ev)
{
  // string can be found using repository id (repos_id)
  if ((ev->major != CORBA_NO_EXCEPTION) && (ev->repos_id >= 0) &&
      (ev->repos_id < CORBA_DICE_EXCEPTION_COUNT))
    return __CORBA_Exception_Repository[ev->repos_id];
  else
    return 0;
}

static inline
void* CORBA_exception_value(CORBA_Environment *ev)
{
#ifdef L4API_linux
  return ev->param;
#else
  return ev->_p.param;
#endif
}

static inline
CORBA_any* CORBA_exception_as_any(CORBA_Environment *ev)
{
  // not supported
  return 0;
}

/*************************************************************
 * For the server environement as well
 *************************************************************/
static inline
void CORBA_server_exception_free(CORBA_Server_Environment *ev)
{
  if (ev->major != CORBA_NO_EXCEPTION)
    {
#ifdef L4API_linux
      //ev->free(ev->param);
      ev->param = 0;
#else
      //ev->free(ev->_p.param);    
      ev->_p.param = 0;
#endif
    }
  ev->major = CORBA_NO_EXCEPTION;
  ev->repos_id = CORBA_DICE_EXCEPTION_NONE;
}


static inline
void CORBA_server_exception_set(
    CORBA_Server_Environment *ev,
    CORBA_exception_type major,
//    CORBA_char *except_repos_id,
    CORBA_exception_type repos_id,
    void *param)
{
  CORBA_server_exception_free(ev);
  ev->major = major;
  if (major != CORBA_NO_EXCEPTION)
    {
      ev->repos_id = repos_id;
#ifdef L4API_linux
      ev->param = param;
#else
      ev->_p.param = param;
#endif
    }
}

static inline
CORBA_char* CORBA_server_exception_id(CORBA_Server_Environment *ev)
{
  // string can be found using repository id (repos_id)
  if ((ev->major != CORBA_NO_EXCEPTION) && (ev->repos_id >= 0) &&
      (ev->repos_id < CORBA_DICE_EXCEPTION_COUNT))
    return __CORBA_Exception_Repository[ev->repos_id];
  else
    return 0;
}

static inline
void* CORBA_server_exception_value(CORBA_Server_Environment *ev)
{
#ifdef L4API_linux
  return ev->param;
#else
  return ev->_p.param;
#endif
}

static inline
CORBA_any* CORBA_server_exception_as_any(CORBA_Server_Environment *ev)
{
  // not supported
  return 0;
}

/***********************************************************************
 * DICE specific environment functions 
 ***********************************************************************/
static inline
int dice_set_ptr(CORBA_Server_Environment *ev, void* ptr)
{
  if (!ev || !ptr)
    return 1;
  if (ev->ptrs_cur >= DICE_PTRS_MAX)
    return 1;
  ev->ptrs[ev->ptrs_cur++] = ptr;
  return 0;
}

static inline
void* dice_get_last_ptr(CORBA_Server_Environment *ev)
{
  void *ptr = 0;
  if (!ev)
    return 0;
  if (ev->ptrs_cur > 0 &&
      ev->ptrs_cur <= DICE_PTRS_MAX)
    {
      ptr = ev->ptrs[--ev->ptrs_cur];
      ev->ptrs[ev->ptrs_cur] = 0;
    }
  return ptr;
}

#ifdef __cplusplus
}
#endif

#endif /* __DICE_DICE_H__ */
