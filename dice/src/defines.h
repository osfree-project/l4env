/**
 *    \file    dice/src/defines.h
 *    \brief   contains basic macros and definitions for all classes
 *
 *    \date    01/31/2001
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2004
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For different licensing schemes please contact
 * <contact@os.inf.tu-dresden.de>.
 */

/** preprocessing symbol to check header file */
#ifndef __DICE_DEFINES_H__
#define __DICE_DEFINES_H__

/** defines the maximum number of include paths */
#define MAX_INCLUDE_PATHS    25

// exceptions
#define EXCEPTION_TYPE        int        /**< defines an exception type */
#define EXCEP_BADSIZE        1        /**< defines the bad size exception */
#define EXCEP_OUTOFMEMORY    2        /**< defines the out of memory exception */

//@{
/** helper macros */
#define DWORD_ALIGN_BYTE(byte_var) ((byte_var+3) & ~0x3)
#define DWORD_FROM_BYTE(byte_var) ((byte_var+3) >> 2)
#define BYTE_FROM_DWORD(dw_var) (dw_var << 2)
#define BYTE_FROM_BIT(bit_var) ((bit_var+7) >> 4)
#define BIT_FROM_BYTE(byte_var) (byte_var << 4)
#define MAX(x,y) (((x)>(y))?(x):(y))
#define VERBOSE(s, args...) if (pContext->IsVerbose()) printf(s, ## args);
//@}

#ifdef _WIN_
#pragma warning(disable:4786)
using namespace std;
#define __PRETTY_FUNCTION__ "(no function name available)"
#endif                /* _WIN_ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>

#include "debug.h"

/** \def COPY_VECTOR(type,vec, iter)
 *  \brief helper macro to copy a vector
 */
#define COPY_VECTOR(type, vec, iter) \
    vector<type*>::iterator iter; \
    for (iter = src.vec.begin(); iter != src.vec.end(); iter++) \
    { \
        type *pNew = (type*)((*iter)->Clone()); \
        vec.push_back(pNew); \
        pNew->SetParent(this); \
    }

/** \def COPY_VECTOR_WOP(type,vec, iter)
 *  \brief helper macro to copy a vector (without setting parent)
 */
#define COPY_VECTOR_WOP(type, vec, iter) \
    vector<type*>::iterator iter = src.vec.begin(); \
    for (; iter != src.vec.end(); iter++) \
    { \
        type *pNew = (type*)((*iter)->Clone()); \
        vec.push_back(pNew); \
    }

/** \def SWAP_VECTOR(type,vec, param)
 *  \brief helper macro to copy a vector
 */
#define SWAP_VECTOR(type, vec, param) \
    if (param) \
    { \
        vec.swap(*param); \
        vector<type*>::iterator iter; \
        for (iter = vec.begin(); iter != vec.end(); iter++) \
            (*iter)->SetParent(this); \
    }

/** \def DEL_VECTOR(vec)
 *  \brief helper macro to delete a vector
 */
#define DEL_VECTOR(vec) \
    while (!vec.empty()) \
    { \
        delete vec.back(); \
        vec.pop_back(); \
    }

#endif                /* __DICE_DEFINES_H__ */

