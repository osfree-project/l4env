/**
 *	\file	dice/src/be/cdr/CCDRMarshalFunction.cpp
 *	\brief	contains the implementation of the class CCDRMarshalFunction
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

#include "be/cdr/CCDRMarshalFunction.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEContext.h"

#include "fe/FEAttribute.h"

IMPLEMENT_DYNAMIC(CCDRMarshalFunction);

CCDRMarshalFunction::CCDRMarshalFunction()
 : CBEMarshalFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CCDRMarshalFunction, CBEMarshalFunction);
}

/** destroys the object */
CCDRMarshalFunction::~CCDRMarshalFunction()
{
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if this function should be written to the given file
 */
bool CCDRMarshalFunction::DoWriteFunction(CBEFile* pFile,  CBEContext* pContext)
{
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)))
		if (!IsTargetFile((CBEHeaderFile*)pFile))
			return false;
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEImplementationFile)))
		if (!IsTargetFile((CBEImplementationFile*)pFile))
			return false;
	if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
		(!FindAttribute(ATTR_OUT)) &&
		!IsComponentSide())
		return true;
	if (pFile->IsOfFileType(FILETYPE_COMPONENT) &&
		(!FindAttribute(ATTR_IN)) &&
		IsComponentSide())
		return true;
	return false;
}

/** \brief writes the marshalling code
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start marshalling
 *  \param bUseConstOffset true if nStartOffset should be used
 *  \param pContext the context of the write operation
 *
 * No opcode marshalled.
 */
void CCDRMarshalFunction::WriteMarshalling(CBEFile* pFile,  int nStartOffset,  bool& bUseConstOffset,  CBEContext* pContext)
{
    if (IsComponentSide())
		nStartOffset += WriteMarshalException(pFile, nStartOffset, bUseConstOffset, pContext);
    // now unmarshal rest
    CBEOperationFunction::WriteMarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
}
