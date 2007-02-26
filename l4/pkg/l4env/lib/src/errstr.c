/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4env/lib/src/errstr.c
 * \brief  Error string handling for l4env
 * \ingroup errno
 *
 * \date   08/09/2001
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
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

/* standard includes */
#include <string.h>     // we need strncmp
#include <stdlib.h>     // we need malloc to register additional error messages

/* L4 includes */
#include <l4/env/errno.h>

/*****************************************************************************
 *** global data
 *****************************************************************************/

/**
 * Error string for standard error codes
 */
static l4env_err_msg_t err_msg[] =
{
  /* L4Env Errors */
  { L4_EUNKNOWN,        "unknown error" },
  { L4_ENOMEM,          "out of memory" },
  { L4_EINVAL,          "invalid argument" },
  { L4_EINVAL_OFFS,     "invalid offset in dataspace" },
  { L4_EIPC,            "ipc error" },
  { L4_ENOMAP,          "no map area available" },
  { L4_ENOTHREAD,       "no thread available" },
  { L4_ENOTFOUND,       "item not found" },
  { L4_EIO,             "I/O error" },
  { L4_ENODATA,         "no data available" },
  { L4_ENOTOWNER,       "not owner" },
  { L4_ENOTASK,         "no task available" },
  { L4_ENODM,           "no dataspace manager found" },
  { L4_EUSED,           "item already used" },
  { L4_EUNUSED,         "item not used" },
  { L4_EPERM,           "permission denied" },
  { L4_EBUSY,           "resource busy" },
  { L4_ESKIPPED,        "operation skipped" },
  { L4_ENOHANDLE,       "no handle available" },
  { L4_ENOKEY,          "no key available" },
  { L4_ENOTSUPP,        "operation not supported" },
  { L4_EEXISTS,         "item already exists" },
  { L4_ENOENT,          "no such file or directory" },
  { L4_EOPEN,           "open failed" },
  { L4_EIOCTL,          "ioctl failed" },
  { L4_ENOTAVAIL,       "item not available" },
  { L4_ENODEV,          "no such device" },
  { L4_EMFILE,          "too many open files" },
  { L4_ENOSPC,          "no space left on device"}, 
  { L4_ETIME,           "timer expired"},
  { L4_EBADF,           "invalid file descriptor"},
  { L4_ENFILE,          "file table overflow"},
  { L4_EROFS,           "read-only file system"},
  { L4_EINVOFFS,        "invalid file offset"},
  { L4_EINVSB,          "invalid file system superblock"},
  { L4_ERES,            "resource reservation failed"}
};

/**
 * String returned for an unknown error-code
 */
const char * l4env_err_unknown = "Unknown (unregistered) error";

/**
 * List of error message tables
 */
static struct l4env_err_desc descs = 
{
  NULL,
  sizeof(err_msg) / sizeof(l4env_err_msg_t),
  err_msg
};

/**
 * List of user error string functions
 */
static l4env_err_fn_desc_t * fn_descs = NULL;

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
int 
l4env_err_register_fn(l4env_err_fn_desc_t * fn_desc)
{
  if (fn_desc == 0 || fn_desc->fn == 0) 
    return -L4_EINVAL;

  if (fn_desc->unknown_len == -1) 
    fn_desc->unknown_len = strlen(fn_desc->unknown);

  fn_desc->next = fn_descs;
  fn_descs = fn_desc;

  return 0;
}

/*****************************************************************************/
/** 
 * \brief  Check if the entries in the given array are unique
 *
 * \retval 0 if all elements in \a arr are unique (unregistered so far)
 * \retval 1 if at least one of the codes in \a arr are already registered
 */
/*****************************************************************************/
static int 
check_unique(int entries, 
	     l4env_err_msg_t * arr)
{
  int i;
  
  for (i = 0; i < entries; i++)
    if (l4env_strerror(arr[i].no) != l4env_err_unknown) 
      return 1;

  return 0;
}

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
int 
l4env_err_register_desc(l4env_err_desc_t * desc)
{
  if (check_unique(desc->entries, desc->arr)) 
    return -L4_EUSED;

  desc->next = descs.next;
  descs.next = desc;

  return 0;
}

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
const char *
l4env_strerror(int code)
{
  int i;
  l4env_err_desc_t * a;
  l4env_err_fn_desc_t * f;
  const char * msg;
  
  if (!code) 
    return "Success";

  for (a = &descs; a; a = a->next)
    {
      for (i = 0; i < a->entries; i++)
	{
	  if (a->arr[i].no == code) 
	    return a->arr[i].str;
	}
    }
  
  for (f = fn_descs; f; f = f->next)
    {
      msg = f->fn(code);
      if (msg && 
	  (f->unknown == NULL || strncmp(msg, f->unknown, f->unknown_len)))
	return msg;
    }

  return l4env_err_unknown;
}
