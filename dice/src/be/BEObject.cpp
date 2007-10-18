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

#include "BEObject.h"
#include "BERoot.h"
#include "BEFunction.h"
#include "BEStructType.h"
#include "BEUnionType.h"
#include "BEContext.h"
#include "BEHeaderFile.h"
#include "BEImplementationFile.h"
#include "BEClient.h"
#include "BEComponent.h"
#include "BENameFactory.h"
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
	CBENameFactory *pNF = CBENameFactory::Instance();
	m_sTargetImplementation = pNF->GetFileName(pFEObject,
		FILETYPE_CLIENTIMPLEMENTATION);
	// get the FEObject's file, because the header file is always for the
	// whole IDL file. Only implementation files are specific for libs,
	// interfaces or operations
	if (pFEObject && !(dynamic_cast<CFEFile*>(pFEObject)))
		pFEObject = pFEObject->GetSpecificParent<CFEFile>(0);
	m_sTargetHeader = pNF->GetFileName(pFEObject, FILETYPE_CLIENTHEADER);
}

/** \brief checks if the target implementation file is the calculated target file
 *  \param pFile the target implementation file
 *  \return true if this element can be added to this file
 *
 * If the target file is generated from an IDL file, then it should end on
 * "-client.c" or "-server.c". If it doesn't it's no IDL file, and we return
 * false. If it does, we truncate this and compare what is left. It should be
 * exactly the same. The locally stored target file-name always ends on
 * "-client.c" (see above) if derived from a IDL file.
 */
bool CBEObject::IsTargetFile(CBEFile* pFile)
{
	string sTargetFile;
	if (pFile->IsOfFileType(FILETYPE_IMPLEMENTATION))
		sTargetFile = m_sTargetImplementation;
	else
		sTargetFile = m_sTargetHeader;
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEObject::%s(%s) called; sTargetFile=%s\n",
		__func__, pFile->GetFileName().c_str(), sTargetFile.c_str());

	// first check if the target file was generated from an IDL file
	unsigned long length = sTargetFile.length();
	if (length <= 9)
		return false;
	string sSuffix("-client.");
	if (pFile->IsOfFileType(FILETYPE_IMPLEMENTATION))
	{
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
			sSuffix += "cc";
		else
			sSuffix += "c";
	}
	else if (pFile->IsOfFileType(FILETYPE_HEADER))
	{
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
			sSuffix += "hh";
		else
			sSuffix += "h";
	}
	if (sTargetFile.substr(length - sSuffix.length()) != sSuffix)
		return false;

	// now check if given file has the same stem as the target file
	if (pFile->IsOfFileType(FILETYPE_CLIENT))
		sSuffix = "-client.";
	else if (pFile->IsOfFileType(FILETYPE_COMPONENT))
		sSuffix = "-server.";
	else if (pFile->IsOfFileType(FILETYPE_OPCODE))
		sSuffix = "-sys.";
	if (pFile->IsOfFileType(FILETYPE_IMPLEMENTATION))
	{
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
			sSuffix += "cc";
		else
			sSuffix += "c";
	}
	else if (pFile->IsOfFileType(FILETYPE_HEADER))
	{
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
			sSuffix += "hh";
		else
			sSuffix += "h";
	}

	string sBaseTarget = pFile->GetFileName();
	string sOutputDir;
	CCompiler::GetBackEndOption(string("output-dir"), sOutputDir);
	if (sBaseTarget.find(sOutputDir) == 0)
		sBaseTarget = sBaseTarget.substr(sOutputDir.length());
	length = sBaseTarget.length();
	if (length > sSuffix.length() &&
		sBaseTarget.substr(length - sSuffix.length()) != sSuffix)
		return false;
	sBaseTarget = sBaseTarget.substr(0, length - sSuffix.length());
	string sBaseLocal = sTargetFile.substr(0, sBaseTarget.length());

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEObject::%s(%s) sBaseTarget=%s, sBaseLocal=%s\n", __func__,
		pFile->GetFileName().c_str(), sBaseTarget.c_str(), sBaseLocal.c_str());

	return sBaseTarget == sBaseLocal;
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
