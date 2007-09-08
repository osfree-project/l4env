/**
 *  \file    dice/src/be/BEObject.cpp
 *  \brief   contains the implementation of the class CBEObject
 *
 *  \date    02/13/2001
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

#include "be/BEObject.h"
#include "be/BERoot.h"
#include "be/BEFunction.h"
#include "be/BEStructType.h"
#include "be/BEUnionType.h"
#include "be/BEContext.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEClient.h"
#include "be/BEComponent.h"
#include "Compiler.h"
#include "fe/FEBase.h"
#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include <typeinfo>

/////////////////////////////////////////////////////////////////////////////
// Base class


CBEObject::CBEObject(CObject * pParent)
: CObject(pParent),
  m_sTargetHeader(),
  m_sTargetImplementation()
{ }

CBEObject::CBEObject(CBEObject* src)
: CObject(*src),
  m_sTargetHeader(src->m_sTargetHeader),
  m_sTargetImplementation(src->m_sTargetImplementation)
{ }

/** cleans up the base object */
CBEObject::~CBEObject()
{ }

/** \brief create a copy of this object
 *  \return reference to clone
 */
CObject* CBEObject::Clone()
{
	return new CBEObject(this);
}

/** \brief sets the target file name
 *  \param pFEObject the front-end object to use for the target file generation
s)
 */
void CBEObject::SetTargetFileName(CFEBase *pFEObject)
{
    if (CCompiler::IsFileOptionSet(PROGRAM_FILE_IDLFILE) ||
        CCompiler::IsFileOptionSet(PROGRAM_FILE_ALL))
    {
        if (!(dynamic_cast<CFEFile*>(pFEObject)))
            pFEObject = pFEObject->GetSpecificParent<CFEFile>(0);
    }
    else if (CCompiler::IsFileOptionSet(PROGRAM_FILE_MODULE))
    {
        if (!(dynamic_cast<CFELibrary*>(pFEObject)) &&
            !(dynamic_cast<CFEInterface*>(pFEObject)) &&
            (pFEObject->GetSpecificParent<CFELibrary>()))
            pFEObject = pFEObject->GetSpecificParent<CFELibrary>();
    }
    else if (CCompiler::IsFileOptionSet(PROGRAM_FILE_INTERFACE))
    {
        if (!(dynamic_cast<CFEInterface*>(pFEObject)) &&
            !(dynamic_cast<CFELibrary*>(pFEObject)) &&
            (pFEObject->GetSpecificParent<CFEInterface>()))
            pFEObject = pFEObject->GetSpecificParent<CFEInterface>();
    }
    else if (CCompiler::IsFileOptionSet(PROGRAM_FILE_FUNCTION))
    {
        if (!(dynamic_cast<CFEOperation*>(pFEObject)) &&
            !(dynamic_cast<CFEInterface*>(pFEObject)) &&
            !(dynamic_cast<CFELibrary*>(pFEObject)) &&
            (pFEObject->GetSpecificParent<CFEOperation>()))
            pFEObject = pFEObject->GetSpecificParent<CFEOperation>();
    }
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    m_sTargetImplementation = pNF->GetFileName(pFEObject,
	FILETYPE_CLIENTIMPLEMENTATION);
    // get the FEObject's file, because the header file is always for the
    // whole IDL file. Only implementation files are specific for libs,
    // interfaces or operations
    if (pFEObject && !(dynamic_cast<CFEFile*>(pFEObject)))
	pFEObject = pFEObject->GetSpecificParent<CFEFile>(0);
    m_sTargetHeader = pNF->GetFileName(pFEObject, FILETYPE_CLIENTHEADER);
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
bool CBEObject::IsTargetFile(CBEHeaderFile* pFile)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBEObject::%s(header: %s) called; m_sTargetHeader=%s\n",
	__func__, pFile->GetFileName().c_str(), m_sTargetHeader.c_str());

    long length = m_sTargetHeader.length();
    if (length <= 9)
        return false;
    if ((m_sTargetHeader.substr(length-9) != "-client.h") &&
	(m_sTargetHeader.substr(length-10) != "-client.hh"))
        return false;
    string sBaseTarget = pFile->GetFileName();
    int nPos = 0;
    length = sBaseTarget.length();
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"IsTargetFile(head: %s) sBaseTarget=%s\n",
        pFile->GetFileName().c_str(), sBaseTarget.c_str());
    if ((length > 9) &&
        ((sBaseTarget.substr(length - 9) == "-client.h") ||
         (sBaseTarget.substr(length - 9) == "-server.h")))
        nPos = 9;
    if ((length > 10) &&
	((sBaseTarget.substr(length - 10) == "-client.hh") ||
	 (sBaseTarget.substr(length - 10) == "-server.hh")))
	nPos = 10;
    if ((length > 6) &&
        (sBaseTarget.substr(length - 6) == "-sys.h"))
        nPos = 6;
    if ((length > 7) &&
	(sBaseTarget.substr(length - 7) == "-sys.hh"))
	nPos = 7;
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"IsTargetFile(head: %s) pos = %d\n", pFile->GetFileName().c_str(),
	nPos);
    if (nPos == 0)
        return false;
    sBaseTarget = sBaseTarget.substr(0, length-nPos);
    string sBaseLocal = m_sTargetHeader.substr(0, sBaseTarget.length());
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"IsTargetFile(head: %s) sBaseTarget=%s sBaseLocal=%s\n",
        pFile->GetFileName().c_str(), sBaseTarget.c_str(),
        sBaseLocal.c_str());
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
bool CBEObject::IsTargetFile(CBEImplementationFile* pFile)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEObject::%s(impl: %s) called; m_sTargetImplementation=%s\n",
	__func__, pFile->GetFileName().c_str(), m_sTargetImplementation.c_str());

    long length = m_sTargetImplementation.length();
    if (length <= 9)
    {
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBEObject::%s(%s) fails, because length(%s) <= 9\n",
	    __func__, pFile->GetFileName().c_str(), m_sTargetImplementation.c_str());
        return false;
    }
    if ((m_sTargetImplementation.substr(length - 9) != "-client.c") &&
	(m_sTargetImplementation.substr(length - 10) != "-client.cc"))
    {
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBEObject::%s(%s) fails, because %s does not end on -client.c\n",
	    __func__, pFile->GetFileName().c_str(), m_sTargetImplementation.c_str());
        return false;
    }
    string sBaseTarget = pFile->GetFileName();

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEObject::%s(%s) sBaseTarget=%s\n",
	__func__, pFile->GetFileName().c_str(), sBaseTarget.c_str());

    length = sBaseTarget.length();
    if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
        (sBaseTarget.substr(length - 9) != "-client.c") &&
	(sBaseTarget.substr(length -10) != "-client.cc"))
    {
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBEObject::%s(%s) fails, because %s does not contain -client.c\n",
	    __func__, pFile->GetFileName().c_str(), sBaseTarget.c_str());
        return false;
    }
    if (pFile->IsOfFileType(FILETYPE_COMPONENT) &&
        (sBaseTarget.substr(length - 9) != "-server.c") &&
	(sBaseTarget.substr(length - 10) != "-server.cc"))
    {
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBEObject::%s(%s) fails, because %s does not conatain -server.c\n",
	    __func__, pFile->GetFileName().c_str(), sBaseTarget.c_str());
        return false;
    }
    sBaseTarget = sBaseTarget.substr(0, length-9);
    string sBaseLocal = m_sTargetImplementation.substr(0, sBaseTarget.length());

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBEObject::%s(%s) sBaseTarget=%s, sBaseLocal=%s\n", __func__,
	pFile->GetFileName().c_str(), sBaseTarget.c_str(), sBaseLocal.c_str());

    if (sBaseTarget == sBaseLocal)
    {
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBEObject::%s(%s) returns true, stems match\n", __func__,
	    pFile->GetFileName().c_str());
        return true;
    }

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBEObject::%s(%s) fails, stems mismatch\n", __func__,
	pFile->GetFileName().c_str());
    return false;
}

/** \brief returns the name of the target header file
 *  \return the name of the target header file name
 */
string CBEObject::GetTargetHeaderFileName()
{
    return m_sTargetHeader;
}

/** \brief return the name of the target implementation file
 *  \return the name of the target implementation file
 */
string CBEObject::GetTargetImplementationFileName()
{
    return m_sTargetImplementation;
}

/** \brief sets source file information
 *  \param pFEObject the front-end object to use to extract information
 *  \return true on success
 */
void
CBEObject::CreateBackEnd(CFEBase* pFEObject)
{
    m_sourceLoc = pFEObject->m_sourceLoc;
}
