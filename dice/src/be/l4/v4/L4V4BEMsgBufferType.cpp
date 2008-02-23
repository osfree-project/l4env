/**
 *  \file    dice/src/be/l4/v4/L4V4BEMsgBufferType.cpp
 *  \brief   contains the implementation of the class CL4V4BEMsgBufferType
 *
 *  \date    02/19/2008
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2008
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

#include "L4V4BEMsgBufferType.h"

CL4V4BEMsgBufferType::CL4V4BEMsgBufferType()
: CL4BEMsgBufferType()
{ }

CL4V4BEMsgBufferType::CL4V4BEMsgBufferType(CL4V4BEMsgBufferType* src)
: CL4BEMsgBufferType(src)
{ }

/** \brief destructor of this instance */
CL4V4BEMsgBufferType::~CL4V4BEMsgBufferType()
{ }

/** \brief create a copy of this object
 *  \return reference to clone
 */
CL4V4BEMsgBufferType* CL4V4BEMsgBufferType::Clone()
{
	return new CL4V4BEMsgBufferType(this);
}

/** \brief adds a zero flexpage if neccessary
 *  \param pFEOperation the front-end operation containing the parameters
 *  \param nType the type of the struct
 *
 *  For V4 we need no zero fpage members
 */
void CL4V4BEMsgBufferType::AddZeroFlexpage(CFEOperation* /*pFEOperation*/,
	CMsgStructType /*nType*/)
{ }

