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
#include "BENameSpace.h"
#include "BEClass.h"
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
#include <cassert>

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
CBEObject* CBEObject::Clone()
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
	unsigned long targetLength = sTargetFile.length();
	if (targetLength <= 9)
		return false;
	string sTargetSuffix("-client.");
	if (pFile->IsOfFileType(FILETYPE_IMPLEMENTATION))
	{
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
			sTargetSuffix += "cc";
		else
			sTargetSuffix += "c";
	}
	else if (pFile->IsOfFileType(FILETYPE_HEADER))
	{
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
			sTargetSuffix += "hh";
		else
			sTargetSuffix += "h";
	}
	if (sTargetFile.substr(targetLength - sTargetSuffix.length()) != sTargetSuffix)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEObject::IsTargetFile: suffix mismatch of target, return false\n");
		return false;
	}

	// now check if given file has the same stem as the target file
	string sArgSuffix;
	if (pFile->IsOfFileType(FILETYPE_CLIENT))
		sArgSuffix = "-client.";
	else if (pFile->IsOfFileType(FILETYPE_COMPONENT))
		sArgSuffix = "-server.";
	else if (pFile->IsOfFileType(FILETYPE_OPCODE))
		sArgSuffix = "-sys.";
	if (pFile->IsOfFileType(FILETYPE_IMPLEMENTATION))
	{
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
			sArgSuffix += "cc";
		else
			sArgSuffix += "c";
	}
	else if (pFile->IsOfFileType(FILETYPE_HEADER))
	{
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
			sArgSuffix += "hh";
		else
			sArgSuffix += "h";
	}

	string sArgFile = pFile->GetFileName();
	string sOutputDir;
	CCompiler::GetBackEndOption(string("output-dir"), sOutputDir);
	if (sArgFile.find(sOutputDir) == 0)
		sArgFile = sArgFile.substr(sOutputDir.length());

	unsigned long argLength = sArgFile.length();
	if (argLength > sArgSuffix.length() &&
		sArgFile.substr(argLength - sArgSuffix.length()) != sArgSuffix)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEObject::IsTargetFile: suffix mismatch in argument, return false\n");
		return false;
	}

	// we need seperate length variables and suffixes, because sArgFile can be
	// a subset of sTargetFile, or the other way around: foo-client.h is
	// subset of foo_bar-client.h
	sArgFile = sArgFile.substr(0, argLength - sArgSuffix.length());
	sTargetFile = sTargetFile.substr(0, targetLength - sTargetSuffix.length());

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEObject::%s(%s) sTargetFile=%s, sArgFile=%s\n", __func__,
		pFile->GetFileName().c_str(), sTargetFile.c_str(), sArgFile.c_str());

	return sTargetFile == sArgFile;
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


/** \brief look for a typedef
 *  \param sTypeName the name of the type to look for
 *  \param pPrev the previous type found
 *  \return reference to the found type or 0 if nothing found
 *
 * A typedef is usually found by looking into the current:
 * # function
 * # class
 * # namespace
 * # root
 * in that order. The respective classes may overload the FindTypedef method
 * to search in their respective typedefs.
 */
CBETypedef* CBEObject::FindTypedef(std::string sTypeName, CBETypedef *pPrev)
{
	CBETypedef *pRet = 0;

	CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
	if (pFunction && (pRet = pFunction->FindTypedef(sTypeName, pPrev)))
		return pRet;
	CBEClass *pClass = GetSpecificParent<CBEClass>();
	if (pClass && (pRet = pClass->FindTypedef(sTypeName, pPrev)))
		return pRet;
	CBENameSpace *pNamespace = GetSpecificParent<CBENameSpace>();
	while (pNamespace)
	{
		if ((pRet = pNamespace->FindTypedef(sTypeName, pPrev)))
			return pRet;
		pNamespace = pNamespace->GetSpecificParent<CBENameSpace>();
	}
	CBERoot *pRoot = GetSpecificParent<CBERoot>();
	if (pRoot && (pRet = pRoot->FindTypedef(sTypeName, pPrev)))
		return pRet;
	return 0;
}

/** \brief look for a constant
 *  \param sConstantName
 *  \return reference to the constant or 0 if not found
 *
 * A constant is usually found by looking into the current:
 * # class
 * # namespace
 * # root
 * in that order.
 */
CBEConstant* CBEObject::FindConstant(std::string sConstantName)
{
	CBEConstant *pRet = 0;

	CBEClass *pClass = GetSpecificParent<CBEClass>();
	if (pClass && (pRet = pClass->FindConstant(sConstantName)))
		return pRet;
	CBENameSpace *pNamespace = GetSpecificParent<CBENameSpace>();
	while (pNamespace)
	{
		if ((pRet = pNamespace->FindConstant(sConstantName)))
			return pRet;
		pNamespace = pNamespace->GetSpecificParent<CBENameSpace>();
	}
	CBERoot *pRoot = GetSpecificParent<CBERoot>();
	if (pRoot && (pRet = pRoot->FindConstant(sConstantName)))
		return pRet;

	return 0;
}

/** \brief look for a namespace
 *  \param sNameSpaceName the name of the namespace
 *  \return a reference to that namespace or 0 if not found
 *
 * A namespace is usually found by looking into the current:
 * # namespace
 * # root
 * in that order. If the namespace contains a scope we have to do a top-down
 * search with the fully qualified name.
 */
CBENameSpace* CBEObject::FindNameSpace(std::string sNameSpaceName)
{
	CBENameSpace *pRet = 0;
	if (sNameSpaceName.find("::") != string::npos)
	{
		CBERoot *pRoot = GetSpecificParent<CBERoot>();
		assert(pRoot);
		return pRoot->SearchNamespace(sNameSpaceName);
	}

	CBENameSpace *pNamespace = GetSpecificParent<CBENameSpace>();
	while (pNamespace)
	{
		if ((pRet = pNamespace->FindNameSpace(sNameSpaceName)))
			return pRet;
		pNamespace = pNamespace->GetSpecificParent<CBENameSpace>();
	}
	CBERoot *pRoot = GetSpecificParent<CBERoot>();
	if (pRoot && (pRet = pRoot->FindNameSpace(sNameSpaceName)))
		return pRet;

	return 0;
}

/** \brief look for a class
 *  \param sClassName the name of the class to find
 *  \return a reference to the found class or 0 if not found
 *
 * A class is usually found by looking into the current:
 * # namespace
 * # root
 * in that order. If the name contains a scope, we have to do a top-down search
 * with the fully qualified name.
 */
CBEClass* CBEObject::FindClass(std::string sClassName)
{
	CBEClass *pRet = 0;
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEObject::FindClass(%s) called\n", sClassName.c_str());
	if (sClassName.find("::") != string::npos)
	{
		CBERoot *pRoot = GetSpecificParent<CBERoot>();
		assert(pRoot);
		return pRoot->SearchClass(sClassName);
	}

	CBENameSpace *pNamespace = GetSpecificParent<CBENameSpace>();
	while (pNamespace)
	{
		if ((pRet = pNamespace->FindClass(sClassName)))
			return pRet;
		pNamespace = pNamespace->GetSpecificParent<CBENameSpace>();
	}
	CBERoot *pRoot = GetSpecificParent<CBERoot>();
	assert(pRoot);
	return pRoot->FindClass(sClassName);
}

/** \brief look for a tagged type
 *  \param nType the type of the type
 *  \param sTag the tag of the type
 *  \return a reference to the found type or 0 if not found
 *
 * A type is usually found by looking into the current:
 * # class
 * # namespace
 * # root
 * in that order.
 */
CBEType* CBEObject::FindTaggedType(unsigned int nType, std::string sTag)
{
	CBEType *pRet = 0;

	CBEClass *pClass = GetSpecificParent<CBEClass>();
	if (pClass && (pRet = pClass->FindTaggedType(nType, sTag)))
		return pRet;
	CBENameSpace *pNamespace = GetSpecificParent<CBENameSpace>();
	while (pNamespace)
	{
		if ((pRet = pNamespace->FindTaggedType(nType, sTag)))
			return pRet;
		pNamespace = pNamespace->GetSpecificParent<CBENameSpace>();
	}
	CBERoot *pRoot = GetSpecificParent<CBERoot>();
	if (pRoot && (pRet = pRoot->FindTaggedType(nType, sTag)))
		return pRet;

	return 0;
}

/** \brief look for function
 *  \param sFunctionName the name of the function
 *  \param nFunctionType the function type
 *  \return a reference to the found function or 0 if not found
 *
 * A function is usually found by looking into the current:
 * # class
 * # namespace
 * # root
 * in that order
 */
CBEFunction* CBEObject::FindFunction(std::string sFunctionName, FUNCTION_TYPE nFunctionType)
{
	CBEFunction *pRet = 0;
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "CBEObject::FindFunction(%s, %d) called\n",
		sFunctionName.c_str(), nFunctionType);

	CBEClass *pClass = GetSpecificParent<CBEClass>();
	if (pClass && (pRet = pClass->FindFunction(sFunctionName, nFunctionType)))
		return pRet;
	CBENameSpace *pNamespace = GetSpecificParent<CBENameSpace>();
	while (pNamespace)
	{
		if ((pRet = pNamespace->FindFunction(sFunctionName, nFunctionType)))
			return pRet;
		pNamespace = pNamespace->GetSpecificParent<CBENameSpace>();
	}
	CBERoot *pRoot = GetSpecificParent<CBERoot>();
	assert(pRoot);
	return pRoot->FindFunction(sFunctionName, nFunctionType);
}

/** \brief look for an enum
 *  \param sEnumerator the name of the enum to look for
 *  \return reference to the enum type containung the enum
 *
 * A enum is usually found by looking into the current:
 * # class
 * # namespace
 * # root
 * in that order.
 */
CBEEnumType* CBEObject::FindEnum(std::string sEnumerator)
{
	CBEEnumType *pRet = 0;

	CBEClass *pClass = GetSpecificParent<CBEClass>();
	if (pClass && (pRet = pClass->FindEnum(sEnumerator)))
		return pRet;
	CBENameSpace *pNamespace = GetSpecificParent<CBENameSpace>();
	while (pNamespace)
	{
		if ((pRet = pNamespace->FindEnum(sEnumerator)))
			return pRet;
		pNamespace = pNamespace->GetSpecificParent<CBENameSpace>();
	}
	CBERoot *pRoot = GetSpecificParent<CBERoot>();
	if (pRoot && (pRet = pRoot->FindEnum(sEnumerator)))
		return pRet;

	return 0;
}

