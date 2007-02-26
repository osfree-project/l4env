/**
 *  \file    dice/src/defines.h
 *  \brief   contains basic macros and definitions for all classes
 *
 *  \date    01/31/2001
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

/** \defgroup Helper Macros */
//@{

/* helper macros */
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
#include <sys/types.h>

/** \def CLONE_MEM(class, member)
 *  \brief clones a member of a source class
 */
#define CLONE_MEM(class, member) \
    if (src.member) \
    { \
	member = (class*)src.member->Clone(); \
	member->SetParent(this); \
    } \
    else \
        member = (class*)0;

//@}

#endif                /* __DICE_DEFINES_H__ */

