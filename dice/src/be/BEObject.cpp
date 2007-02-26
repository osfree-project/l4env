/**
 *	\file	dice/src/be/BEObject.cpp
 *	\brief	contains the implementation of the class CBEObject
 *
 *	\date	02/13/2001
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

#include "be/BEObject.h"
#include "be/BERoot.h"
#include "be/BEFunction.h"
#include "be/BEStructType.h"
#include "be/BEUnionType.h"
#include "be/BEContext.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BETestsuite.h"
#include "be/BEClient.h"
#include "be/BEComponent.h"

#include "fe/FEBase.h"
#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"

/////////////////////////////////////////////////////////////////////////////
// Base class

IMPLEMENT_DYNAMIC(CBEObject)

CBEObject::CBEObject(CObject * pParent)
: CObject(pParent),
  m_sTargetHeader(),
  m_sTargetImplementation(),
  m_sTargetTestsuite()
{
    IMPLEMENT_DYNAMIC_BASE(CBEObject, CObject);
}

CBEObject::CBEObject(CBEObject & src)
: CObject(src),
  m_sTargetHeader(src.m_sTargetHeader),
  m_sTargetImplementation(src.m_sTargetImplementation),
  m_sTargetTestsuite(src.m_sTargetTestsuite)
{
    IMPLEMENT_DYNAMIC_BASE(CBEObject, CObject);
}

/** cleans up the base object */
CBEObject::~CBEObject()
{

}

/**	\brief tries to find the root class
 *	\return a reference to the root class or 0
 *
 * The root class should be at the top of the parent-relationship tree. So iterating over the
 * parents of this class and checking their type should be sufficient.
 */
CBERoot *CBEObject::GetRoot()
{
	CObject *pCurr = this;
	while (pCurr)
	{
		if (pCurr->IsKindOf(RUNTIME_CLASS(CBERoot)))
			return (CBERoot *) pCurr;
		pCurr = pCurr->GetParent();
	}
	return 0;
}

/**	\brief tries to find the function an object belongs to
 *	\return a reference to the function or 0 if search failed
 */
CBEFunction *CBEObject::GetFunction()
{
    CObject *pCur = this;
    while (pCur)
	{
		if (pCur->IsKindOf(RUNTIME_CLASS(CBEFunction)))
			return (CBEFunction *) pCur;
		pCur = pCur->GetParent();
	}
    return 0;
}

/**	\brief tries to find a parent of type CBEStructType
 *	\return a reference to the struct type or 0 is search failed
 */
CBEStructType *CBEObject::GetStructType()
{
    CObject *pCur = this;
    while (pCur)
      {
	  if (pCur->IsKindOf(RUNTIME_CLASS(CBEStructType)))
	      return (CBEStructType *) pCur;
	  pCur = pCur->GetParent();
      }
    return 0;
}

/**	\brief tries to find a parent of type CBEUnioneType
 *	\return a reference to the union type or 0 if search failed
 */
CBEUnionType *CBEObject::GetUnionType()
{
    CObject *pCur = this;
    while (pCur)
      {
	  if (pCur->IsKindOf(RUNTIME_CLASS(CBEUnionType)))
	      return (CBEUnionType *) pCur;
	  pCur = pCur->GetParent();
      }
    return 0;
}

/** \brief sets the target file name
 *  \param pFEObject the front-end object to use for the target file generation
 *  \param pContext the context of this operation (contains the compiler options)
 */
void CBEObject::SetTargetFileName(CFEBase *pFEObject, CBEContext *pContext)
{
	if (pContext->IsOptionSet(PROGRAM_FILE_IDLFILE) ||
		pContext->IsOptionSet(PROGRAM_FILE_ALL))
	{
		if (!(pFEObject->IsKindOf(RUNTIME_CLASS(CFEFile))))
			pFEObject = pFEObject->GetFile();
	}
	else if (pContext->IsOptionSet(PROGRAM_FILE_MODULE))
	{
	    if (!(pFEObject->IsKindOf(RUNTIME_CLASS(CFELibrary))) &&
			!(pFEObject->IsKindOf(RUNTIME_CLASS(CFEInterface))) &&
			(pFEObject->GetParentLibrary()))
			pFEObject = pFEObject->GetParentLibrary();
	}
	else if (pContext->IsOptionSet(PROGRAM_FILE_INTERFACE))
	{
		if (!(pFEObject->IsKindOf(RUNTIME_CLASS(CFEInterface))) &&
			!(pFEObject->IsKindOf(RUNTIME_CLASS(CFELibrary))) &&
			(pFEObject->GetParentInterface()))
			pFEObject = pFEObject->GetParentInterface();
	}
	else if (pContext->IsOptionSet(PROGRAM_FILE_FUNCTION))
	{
		if (!(pFEObject->IsKindOf(RUNTIME_CLASS(CFEOperation))) &&
			!(pFEObject->IsKindOf(RUNTIME_CLASS(CFEInterface))) &&
			!(pFEObject->IsKindOf(RUNTIME_CLASS(CFELibrary))) &&
			(pFEObject->GetParentOperation()))
			pFEObject = pFEObject->GetParentOperation();
	}
	pContext->SetFileType(FILETYPE_CLIENTIMPLEMENTATION);
	m_sTargetImplementation = pContext->GetNameFactory()->GetFileName(pFEObject, pContext);
	pContext->SetFileType(FILETYPE_CLIENTHEADER);
	if (pFEObject)
	{
		if (!(pFEObject->IsKindOf(RUNTIME_CLASS(CFEFile))))
			pFEObject = pFEObject->GetFile();
	}
	m_sTargetHeader = pContext->GetNameFactory()->GetFileName(pFEObject, pContext);
}

/** \brief checks if the target header file is the calculated target file
 *  \param pFile the target header file
 *  \return true if this element can be added to this file
 *
 * If the target header file is generated from an IDL file, then it should end on
 * "-client.h", "-server.h", or "-sys.h". If it doesn't
 * it's no IDL file, and we return false. If it does, we truncate this and
 * compare what is left. It should be exactly the same. The locally stored
 * target file-name always ends on "-client.h" (see above) if derived from a
 * IDL file.
 */
bool CBEObject::IsTargetFile(CBEHeaderFile *pFile)
{
    DTRACE("IsTargetFile(%s) m_sTargetHeader=%s\n", (const char*)pFile->GetFileName(), (const char*)m_sTargetHeader);
	if (m_sTargetHeader.Right(9) != "-client.h")
		return false;
	String sBaseLocal = m_sTargetHeader.Left(m_sTargetHeader.GetLength()-9);
	String sBaseTarget = pFile->GetFileName();
	int nPos = 0;
	if ((sBaseTarget.Right(9) == "-client.h") || (sBaseTarget.Right(9) == "-server.h"))
		nPos = 9;
	if (sBaseTarget.Right(6) == "-sys.h")
		nPos = 6;
	if (nPos == 0)
		return false;
	sBaseTarget = sBaseTarget.Left(sBaseTarget.GetLength()-nPos);
	if (sBaseTarget == sBaseLocal)
		return true;
	return false;
}

/** \brief checks if the target implementation file is the calculated target file
 *  \param pFile the target implementation file
 *  \return true if this element can be added to this file
 *
 * If the target file is generated from an IDL file, then it should end on
 * "-client.c" or "-server.c". If it doesn't
 * it's no IDL file, and we return false. If it does, we truncate this and
 * compare what is left. It should be exactly the same. The locally stored
 * target file-name always ends on "-client.c" (see above) if derived from a
 * IDL file.
 */
bool CBEObject::IsTargetFile(CBEImplementationFile *pFile)
{
	if (m_sTargetImplementation.Right(9) != "-client.c")
		return false;
	String sBaseTarget = pFile->GetFileName();
	if (pFile->IsOfFileType(FILETYPE_CLIENT) && (sBaseTarget.Right(9) != "-client.c"))
		return false;
	if (pFile->IsOfFileType(FILETYPE_COMPONENT) && (sBaseTarget.Right(9) != "-server.c"))
		return false;
	sBaseTarget = sBaseTarget.Left(sBaseTarget.GetLength()-9);
//	String sBaseLocal = m_sTargetImplementation.Left(m_sTargetImplementation.GetLength()-9);
	String sBaseLocal = m_sTargetImplementation.Left(sBaseTarget.GetLength());
	if (sBaseTarget == sBaseLocal)
		return true;
	return false;
}

/** \brief creates a new instance of itself */
CObject * CBEObject::Clone()
{
    TRACE("Clone() not implemented for %s. Fallback to CBEObject::Clone().\n", (const char*)GetClassName());
    return new CBEObject(*this);
}

/** \brief returns the name of the target header file
 *  \return the name of the target header file name
 */
String CBEObject::GetTargetHeaderFileName()
{
    return m_sTargetHeader;
}

/** \brief return the name of the target implementation file
 *  \return the name of the target implementation file
 */
String CBEObject::GetTargetImplementationFileName()
{
    return m_sTargetImplementation;
}
