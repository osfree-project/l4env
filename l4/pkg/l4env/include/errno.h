/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4env/inlude/errno.h
 * \brief   L4ENV, error codes and erroor-string functions.
 * \ingroup errno
 *
 * \date    08/19/2000
 * \author  Lars Reuther <reuther@os.inf.tu-dresden.de>
 * \author  Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2 as
 * published by the Free Software Foundation (see the file COPYING).
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For different licensing schemes please contact
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/
#ifndef __L4ENV_INCLUDE_ERRNO_H
#define __L4ENV_INCLUDE_ERRNO_H

/* L4/L4Env includes */
#include <l4/env/cdefs.h>

/*****************************************************************************
 *** Error codes
 *****************************************************************************/

#define L4_EUNKNOWN        1  /* unknown error */
#define L4_ENOMEM          2  /* cannot allocate memory */
#define L4_EINVAL          3  /* invalid argument */
#define L4_EINVAL_OFFS     4  /* invalid offset in dataspace */
#define L4_EIPC            5  /* IPC error */
#define L4_ENOMAP          6  /* no map area available */
#define L4_ENOTHREAD       7  /* no thread available */
#define L4_ENOTFOUND       8  /* item not found */
#define L4_EIO             9  /* file I/O error */
#define L4_ENODATA        10  /* no data available */
#define L4_ENOTOWNER      11  /* not owner */
#define L4_ENOTASK        12  /* no task available */
#define L4_ENODM          13  /* no dataspace manager found */
#define L4_EUSED          14  /* item already used */
#define L4_EUNUSED        15  /* item not used */
/* 16 is reserved for L4 IPC error */
#define L4_EPERM          17  /* permission denied */
#define L4_EBUSY          18  /* resource busy */
#define L4_ESKIPPED       19  /* operation skipped */
#define L4_ENOHANDLE      20  /* no handle available */
#define L4_ENOKEY         21  /* no key available */
#define L4_ENOTSUPP       22  /* operation not supported */
#define L4_EEXISTS        23  /* item already exists */
#define L4_ENOENT         24  /* no such file or directory */
#define L4_EOPEN          25  /* open failed */
#define L4_EIOCTL         26  /* ioctl failed */
#define L4_ENOTAVAIL      27  /* item not available */
#define L4_ENODEV         28  /* no such device */
#define L4_EMFILE         29  /* too many open files */
#define L4_ENOSPC         30  /* no space left on device */
#define L4_ETIME          31  /* timer expired */
/* 32 is reserved for L4 IPC error */
#define L4_EBADF          33  /* invalid file descriptor */
#define L4_ENFILE         34  /* file table overflow */
#define L4_EROFS          35  /* read-only file system */
#define L4_EINVOFFS       36  /* invalid file offset */
#define L4_EINVSB         37  /* invalid file system superblock */
#define L4_ERES           38  /* resource reservation failed */

/* 48, 64 ... 240 are reserved for L4 IPC errors */

/* 0x100 (256) .. 0x10F (271) are reserved for event-package */

/*****************************************************************************
 *** typedefs
 *****************************************************************************/

/**
 * Error message table entry
 * \ingroup errno
 */
typedef struct
{
  int		no;    ///< Error code
  const char *  str;   ///< Error message string
} l4env_err_msg_t;

/**
 * Structure for registering multiple error message tables
 * \ingroup errno
 */
typedef struct l4env_err_desc
{
  struct l4env_err_desc * next;       ///< next table
  int                     entries;    ///< number of elements in table
  l4env_err_msg_t *       arr;        ///< error message table
} l4env_err_desc_t;

/**
 * Declare error message table
 * \ingroup errno
 *
 * \param  vis            Error table scope (global or static)
 * \param  name           Error table name
 * \param  arr            Error table definition (l4env_err_msg_t array)
 */
#define L4ENV_ERR_DESC_(vis, name, arr...)                          \
  static l4env_err_msg_t l4env_err_description_##name[]={arr};      \
  vis l4env_err_desc_t name = { NULL,                               \
                              sizeof(l4env_err_description_##name)  \
                                  / sizeof(l4env_err_msg_t),        \
                              l4env_err_description_##name}

/**
 * Construct a description of error codes
 * \ingroup errno
 *
 * \param  name           Error table name
 * \param  arr            Error table definition (l4env_err_msg_t array)
 */
#define L4ENV_ERR_DESC(name,arr...)  L4ENV_ERR_DESC_(,name,arr)

/**
 * Construct a description of error code, static version
 * \ingroup errno
 *
 * \param  name           Error table name
 * \param  arr            Error table definition (l4env_err_msg_t array)
 */
#define L4ENV_ERR_DESC_STATIC(name,arr...) L4ENV_ERR_DESC_(static,name,arr)

/**
 * Structure for registering an error-string function
 * \ingroup errno
 */
typedef struct l4env_err_fn_desc
{
  struct l4env_err_fn_desc * next;
  char *                     (*fn)(int);   ///< error-string function
  const char *               unknown;      ///< string for unkown error code
  int                        unknown_len;  ///< length of unknown-string
} l4env_err_fn_desc_t;

/**
 * Declare error-string function
 * \ingroup errno
 *
 * \param  vis            Descriptor scope (global or static)
 * \param  name           Descriptor name
 * \param  fn             Error string function
 * \param  unknown_string Error string for unknown error codes
 */
#define L4ENV_ERR_FN_DESC_(vis,name,fn,unknown_string)	\
	vis l4env_err_fn_desc_t name={			\
		NULL,					\
		fn,					\
		unknown_string, -1}

/**
 * Construct a description for an error string function
 * \ingroup errno
 *
 * \param  name           Descriptor name
 * \param  fn             Error string function
 * \param  unknown_string Error string for unknown error codes
 */
#define L4ENV_ERR_FN_DESC(name,fn,unknown_string) \
                L4ENV_ERR_FN_DESC_(,name,fn,unknown_string)

/**
 * Construct a description for an error string function, static version
 * \ingroup errno
 *
 * \param  name           Descriptor name
 * \param  fn             Error string function
 * \param  unknown_string Error string for unknown error codes
 */
#define L4ENV_ERR_FN_DESC_STATIC(name,fn,unknown_string) \
		L4ENV_ERR_FN_DESC_(static,name,fn,unknown_string)

/**
 * Show error message for return code
 * \ingroup errno
 */
#define l4env_errstr(retcode) l4env_strerror(-retcode)

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

__BEGIN_DECLS;

/**
 * Unknown error message string
 * \ingroup errno
 */
extern const char * l4env_err_unknown;

/*****************************************************************************/
/**
 * \brief Register an error-function
 * \ingroup errno
 *
 * \param fn_desc	description of the function
 * \param fn_desc->fn	the function
 * \param fn_desc->unknown string returned by \a fn in case of an unknown
 *			   error code (or the first characters of it)
 * \param fn_desc->unknown_len -1
 *
 * \retval 0              success
 * \retval \c -L4_EINVAL  fn_desc or the function is invalid (0)
 *
 * The error-function in \a fn_desc will be called from l4env_strerror() to
 * obtain a string describing a given error code. \a fn_desc->fn should
 * return either the string description, or a string whose characters
 * compare against \a fn_desc->unknown. In the latter case, the code is
 * assumed to be unknown to the function in \a fn_desc and l4env_strerror()
 * will use the other registered functions.
 *
 * You can use the macros #L4ENV_ERR_FN_DESC and #L4ENV_ERR_FN_DESC_STATIC
 * to easily construct \a fn_desc.
 *
 * Due to the large searching space, l4env_err_register_fn() does not
 * perform checks if the codes valid to \a fn_desc->fn are already
 * registerd/used.
 */
/*****************************************************************************/
extern int
l4env_err_register_fn(l4env_err_fn_desc_t * fn_desc);

/*****************************************************************************/
/**
 * \brief  Register an error-description structure containing error-codes
 *         and messages
 * \ingroup errno
 *
 * \param  desc		the description
 *
 * \retval 0		success
 * \retval \c -L4_EUSED	at least one of the codes in \a arr is already
 *			registered
 *
 * l4env_err_register_desc() registers error-codes and messages. \a desc
 * can be constructed using the #L4ENV_ERR_DESC macro.
 *
 * Prior to registering, this function checks if all of the codes in \a desc
 * are unregistered so far. This means, they can not be found in the registerd
 * array, nor returns any of the registered functions a valid error-string.
 */
/*****************************************************************************/
extern int
l4env_err_register_desc(l4env_err_desc_t * desc);

/*****************************************************************************/
/**
 * \brief  Return string describing error code
 * \ingroup errno
 *
 * \param  code          the error code
 * \return a string describing the error. If the error-code is unknown
 *	   (unregistered), \a l4env_err_unknown is returned.
 *
 * This function performs a search for the given code in the
 * registered arrays and functions. The search begins within the
 * arrays. The search order within the arrays does not matter, because they
 * are distinct. The search order within the functions is not defined.
 *
 * If the return value is \b not l4env_err_unknown, the returned string
 * can only be used until the next call to l4env_strerror().
 *
 * \note As in strerror(), the given error code is not the (mostly negative)
 *       return value of service-functions, but the error code as defined
 *       in the several errno.h's. This means, the caller must negate the
 *       return-value of these functions, if they return error-codes in
 *       their negated form.
 */
/*****************************************************************************/
extern const char *
l4env_strerror(int code);

/*!\brief  Print an error message containing an l4env error string
 * \ingroup errno
 *
 * \param  string        The main format string. May contain printf-
 *                       format specifiers. AAn ": %s\n" will be appended.
 * \param  code          The error code. It will be passed to
 *                       l4env_strerror() and given as last argument to
 *                       the string formatter.
 * \param  opts          Optional arguments for the format specifiers
 *                       in string.
 *
 * \pre Requires liblog to be linked, which is the case with l4env normally.
 */
#define l4env_perror(string, errcode, opts...) \
	LOG_Error(string ": %s" ,##opts, l4env_strerror(errcode))

__END_DECLS;

#endif /* !__L4ENV_INCLUDE_ERRNO_H */
