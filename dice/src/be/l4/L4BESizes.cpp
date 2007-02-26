/**
 *	\file	dice/src/be/l4/L4BESizes.cpp
 *	\brief	contains the implementation of the class CL4BESizes
 *
 *	\date	Thu Oct 10 2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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

#include "be/l4/L4BESizes.h"

IMPLEMENT_DYNAMIC(CL4BESizes);

CL4BESizes::CL4BESizes()
{
    IMPLEMENT_DYNAMIC_BASE(CL4BESizes, CBESizes);
}

/** \brief destroys an object of this class */
CL4BESizes::~CL4BESizes()
{
}
