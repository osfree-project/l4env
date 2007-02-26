/**
 *	\file	dice/src/be/cdr/CCDRUnmarshalFunction.cpp
 *	\brief	contains the implementation of the class CCDRUnmarshalFunction
 *
 *	\date	10/29/2003
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

#include "be/cdr/CCDRUnmarshalFunction.h"
#include "be/BEContext.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"

#include "fe/FEAttribute.h"

IMPLEMENT_DYNAMIC(CCDRUnmarshalFunction);

CCDRUnmarshalFunction::CCDRUnmarshalFunction()
 : CBEUnmarshalFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CCDRUnmarshalFunction, CBEUnmarshalFunction);
}

/** destroys object */
CCDRUnmarshalFunction::~CCDRUnmarshalFunction()
{
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if this function should be written to the given file
 */
bool CCDRUnmarshalFunction::DoWriteFunction(CBEFile* pFile,  CBEContext* pContext)
{
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)))
		if (!IsTargetFile((CBEHeaderFile*)pFile))
			return false;
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEImplementationFile)))
		if (!IsTargetFile((CBEImplementationFile*)pFile))
			return false;
	if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
		(!FindAttribute(ATTR_IN)) &&
		!IsComponentSide())
		return true;
	if (pFile->IsOfFileType(FILETYPE_COMPONENT) &&
		(!FindAttribute(ATTR_OUT)) &&
		IsComponentSide())
		return true;
	return false;
}

/** \brief writes the unmarshalling instructions
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start unmarshalling
 *  \param bUseConstOffset true if const offset should be used
 *  \param pContext the context of the write operation
 *
 * No opcode unmarshalling.
 */
void CCDRUnmarshalFunction::WriteUnmarshalling(CBEFile* pFile,  int nStartOffset,  bool& bUseConstOffset,  CBEContext* pContext)
{
    if (!IsComponentSide())
	{
	    // unmarshal exception
		nStartOffset += WriteUnmarshalException(pFile, nStartOffset, bUseConstOffset, pContext);
        // first unmarshl return value
        nStartOffset += WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
	}
    // now unmarshal rest
    CBEOperationFunction::WriteUnmarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
}
