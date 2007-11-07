/**
 *  \file    dice/src/be/BEHeaderFile.cpp
 *  \brief   contains the implementation of the class CBEHeaderFile
 *
 *  \date    01/11/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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

#include "BEHeaderFile.h"
#include "BEContext.h"
#include "BEConstant.h"
#include "BETypedef.h"
#include "BEType.h"
#include "BEFunction.h"
#include "BEDeclarator.h"
#include "BENameSpace.h"
#include "BEClass.h"
#include "BEStructType.h"
#include "BEUnionType.h"
#include "BENameFactory.h"
#include "BEClassFactory.h"
#include "Trace.h"
#include "IncludeStatement.h"
#include "Compiler.h"
#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include <iostream>
#include <cassert>
#include <cstring>

CBEHeaderFile::CBEHeaderFile()
: m_Constants(0, (CObject*)0),
	m_Typedefs(0, (CObject*)0),
	m_TaggedTypes(0, (CObject*)0)
{ }

/** \brief destructor
 */
CBEHeaderFile::~CBEHeaderFile()
{ }

/** \brief prepares the header file for the back-end
 *  \param pFEFile the corresponding front-end file
 *  \param nFileType the type of the file
 *  \return true if the creation was successful
 *
 * This function should only add the file name and the names of the included
 * files to this instance.
 *
 * If the name of the front-end file included a relative path, this path
 * should be stripped of for the file name of this file. But it should be used
 * when writing include statements. E.g. a file included with \#include
 * "l4/test.idl" should get the header file name "test-client.h" or
 * "test-server.h", but should be included using "l4/test-client.h" or
 * "l4/test-server.h".
 */
void CBEHeaderFile::CreateBackEnd(CFEFile * pFEFile, FILE_TYPE nFileType)
{
	assert(pFEFile);

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEHeaderFile::%s(file: %s) called\n",
		__func__, pFEFile->GetFileName().c_str());

	m_nFileType = nFileType;
	CBENameFactory *pNF = CBENameFactory::Instance();
	m_sFilename = pNF->GetFileName(pFEFile, m_nFileType);
	m_sIncludeName = pNF->GetIncludeFileName(pFEFile, m_nFileType);

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEHeaderFile::%s m_sFilename=%s, m_sIncludeName=%s\n", __func__,
		m_sFilename.c_str(), m_sIncludeName.c_str());

	CFEFile *pFERoot = pFEFile->GetRoot();
	assert(pFERoot);
	vector<CFEFile*>::iterator iFile;
	for (iFile = pFEFile->m_ChildFiles.begin();
		iFile != pFEFile->m_ChildFiles.end();
		iFile++)
	{
		string sIncName = pNF->GetIncludeFileName(*iFile, m_nFileType);

		// if the compiler option is FILE_ALL, then we only add non-IDL files
		if (CCompiler::IsFileOptionSet(PROGRAM_FILE_ALL) &&
			(*iFile)->IsIDLFile())
			continue;

		AddIncludedFileName(sIncName, (*iFile)->IsIDLFile(),
			(*iFile)->IsStdIncludeFile(), *iFile);
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEHeaderFile::%s(file: %s) finished\n", __func__,
		pFEFile->GetFileName().c_str());
}

/** \brief prepares the header file for the back-end
 *  \param pFELibrary the corresponding front-end library
 *  \param nFileType the type of the file
 *  \return true if creation was successful
 */
void CBEHeaderFile::CreateBackEnd(CFELibrary * pFELibrary, FILE_TYPE nFileType)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEHeaderFile::%s(library: %s) called\n", __func__,
		pFELibrary->GetName().c_str());

	m_nFileType = nFileType;
	CBENameFactory *pNF = CBENameFactory::Instance();
	m_sFilename = pNF->GetFileName(pFELibrary, m_nFileType);
	m_sIncludeName = pNF->GetIncludeFileName(pFELibrary, m_nFileType);
}

/** \brief prepares the back-end file for usage as per interface file
 *  \param pFEInterface the respective interface to prepare for
 *  \param nFileType the type of the file
 *  \return true if code generation was successful
 */
void CBEHeaderFile::CreateBackEnd(CFEInterface * pFEInterface, FILE_TYPE nFileType)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEHeaderFile::%s(interface: %s) called\n", __func__,
		pFEInterface->GetName().c_str());

	m_nFileType = nFileType;
	CBENameFactory *pNF = CBENameFactory::Instance();
	m_sFilename = pNF->GetFileName(pFEInterface, m_nFileType);
	m_sIncludeName = pNF->GetIncludeFileName(pFEInterface, m_nFileType);
}

/** \brief prepares the back-end file for usage as per operation file
 *  \param pFEOperation the respective front-end operation to prepare for
 *  \param nFileType the type of the file
 *  \return true if back-end was created correctly
 */
void CBEHeaderFile::CreateBackEnd(CFEOperation * pFEOperation, FILE_TYPE nFileType)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEHeaderFile::%s(operation: %s) called\n", __func__,
		pFEOperation->GetName().c_str());

	m_nFileType = nFileType;
	CBENameFactory *pNF = CBENameFactory::Instance();
	m_sFilename = pNF->GetFileName(pFEOperation, m_nFileType);
	m_sIncludeName= pNF->GetIncludeFileName(pFEOperation, m_nFileType);
}

/** \brief writes the content of the header file
 *
 * The content of the header file includes the functions, constants and type
 * definitions.
 *
 * Before we can actually write anything we have to create the file.
 *
 * The content of a header file is always braced by a symbol, so a multiple
 * include of this file will not result in multiple constant, type or function
 * declarations.
 *
 * In C we start with the constants, then the type definitions and after that
 * we write the functions using the base class' Write operation.  For the
 * server's side we print the typedefs before the includes. This way we can
 * use the message buffer type of the derived server-loop for the functions of
 * the base interface.
 */
void CBEHeaderFile::Write()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEHeaderFile::%s called\n", __func__);
	string sFilename;
	CCompiler::GetBackEndOption(string("output-dir"), sFilename);
	sFilename += GetFileName();
	if (is_open())
	{
		std::cerr << "ERROR: Header file " << sFilename << " is already opened.\n";
		return;
	}
	open(sFilename.c_str());
	if (!good())
	{
		std::cerr << "ERROR: Failed to open header file " << sFilename << ".\n";
		return;
	}
	m_sFilename = sFilename;
	m_nIndent = m_nLastIndent = 0;
	// sort our members/elements depending on source line number
	// into extra vector
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEHeaderFile::%s create ordered elements\n", __func__);
	CreateOrderedElementList();

	// write intro
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEHeaderFile::%s write intro\n", __func__);
	WriteIntro();
	// write include define
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sDefine = pNF->GetHeaderDefine(GetFileName());
	if (!sDefine.empty())
	{
		*this << "#if !defined(" << sDefine << ")\n";
		*this << "#define " << sDefine << "\n";
	}
	*this << "\n";

	// default includes always come first, because they define standard headers
	// needed by other includes
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEHeaderFile::%s write default includes\n", __func__);
	WriteDefaultIncludes();

	// write target file
	vector<CObject*>::iterator iter = m_vOrderedElements.begin();
	int nLastType = 0, nCurrType = 0;
	for (; iter != m_vOrderedElements.end(); iter++)
	{
		if (dynamic_cast<CIncludeStatement*>(*iter))
			nCurrType = 1;
		else if (dynamic_cast<CBEClass*>(*iter))
			nCurrType = 2;
		else if (dynamic_cast<CBENameSpace*>(*iter))
			nCurrType = 3;
		else if (dynamic_cast<CBEConstant*>(*iter))
			nCurrType = 4;
		else if (dynamic_cast<CBETypedef*>(*iter))
			nCurrType = 5;
		else if (dynamic_cast<CBEType*>(*iter))
			nCurrType = 6;
		else if (dynamic_cast<CBEFunction*>(*iter))
		{
			/* only write functions if this is client header or component
			 * header */
			if (IsOfFileType(FILETYPE_CLIENTHEADER) ||
				IsOfFileType(FILETYPE_COMPONENTHEADER))
				nCurrType = 7;
		}
		else
			nCurrType = 0;
		if (nCurrType != nLastType)
		{
			// brace functions with extern C
			if (nLastType == 6)
			{
				*this << "#ifdef __cplusplus\n" <<
					"}\n" <<
					"#endif\n\n";
			}
			*this << "\n";
			nLastType = nCurrType;
			// brace functions with extern C
			if (nCurrType == 6)
			{
				*this << "#ifdef __cplusplus\n" <<
					"extern \"C\" {\n" <<
					"#endif\n\n";
			}
		}
		// add pre-processor directive to denote source line
		if (CCompiler::IsOptionSet(PROGRAM_GENERATE_LINE_DIRECTIVE))
		{
			*this << "# " << (*iter)->m_sourceLoc.getBeginLine() << " \"" <<
				(*iter)->m_sourceLoc.getFilename() << "\"\n";
		}
		switch (nCurrType)
		{
		case 1:
			WriteInclude((CIncludeStatement*)(*iter));
			break;
		case 2:
			WriteClass((CBEClass*)(*iter));
			break;
		case 3:
			WriteNameSpace((CBENameSpace*)(*iter));
			break;
		case 4:
			WriteConstant((CBEConstant*)(*iter));
			break;
		case 5:
			WriteTypedef((CBETypedef*)(*iter));
			break;
		case 6:
			WriteTaggedType((CBEType*)(*iter));
			break;
		case 7:
			WriteFunction((CBEFunction*)(*iter));
			break;
		default:
			break;
		}
	}
	// if last element was function, close braces
	if (nLastType == 6)
	{
		*this << "#ifdef __cplusplus\n" <<
			"}\n" <<
			"#endif\n\n";
	}

	// write helper functions, if any
	/* only write functions if this is client header or component header */
	if (IsOfFileType(FILETYPE_CLIENTHEADER) ||
		IsOfFileType(FILETYPE_COMPONENTHEADER))
	{
		WriteHelperFunctions();
	}

	// write include define closing statement
	if (!sDefine.empty())
	{
		*this << "#endif /* " << sDefine << " */\n";
	}
	*this << "\n";

	// close file
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEHeaderFile::%s close file\n", __func__);
	close();
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEHeaderFile::%s done.\n", __func__);
}

/** \brief writes includes, which have to appear before any type definition
 */
void CBEHeaderFile::WriteDefaultIncludes()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEHeaderFile::%s called\n", __func__);

	string ver(VERSION);
	string::size_type l = ver.find('.');
	string::size_type r = ver.rfind('.');
	assert(l != string::npos && r != string::npos);
	*this <<
		"#ifndef DICE_MAJOR_VERSION\n" <<
		"#define DICE_MAJOR_VERSION " << ver.substr(0,l) << "\n" <<
		"#define DICE_MINOR_VERSION " << ver.substr(l+1,r-l-1) << "\n" <<
		"#define DICE_SUBMINOR_VERSION " << ver.substr(r+1) << "\n" <<
		"#endif\n\n";

	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CTrace *pTrace = pCF->GetNewTrace();
	if (pTrace)
		pTrace->DefaultIncludes(*this);
	delete pTrace;

	*this <<
		"/* needed for CORBA types */\n" <<
		"#include \"dice/dice.h\"\n" <<
		"\n";

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEHeaderFile::%s returns\n", __func__);
}

/** \brief creates a list of ordered elements
 *
 * This method iterates each member vector and inserts their
 * elements into the ordered element list using bubble sort.
 * Sort criteria is the source line number.
 */
void CBEHeaderFile::CreateOrderedElementList()
{
	// first call base class
	CBEFile::CreateOrderedElementList();

	// add own vectors
	// typedef
	vector<CBETypedef*>::iterator iterT;
	for (iterT = m_Typedefs.begin();
		iterT != m_Typedefs.end();
		iterT++)
	{
		InsertOrderedElement(*iterT);
	}
	// tagged types
	vector<CBEType*>::iterator iterTa;
	for (iterTa = m_TaggedTypes.begin();
		iterTa != m_TaggedTypes.end();
		iterTa++)
	{
		InsertOrderedElement(*iterTa);
	}
	// consts
	vector<CBEConstant*>::iterator iterC;
	for (iterC = m_Constants.begin();
		iterC != m_Constants.end();
		iterC++)
	{
		InsertOrderedElement(*iterC);
	}
}

/** \brief writes a class
 *  \param pClass the class to write
 */
void CBEHeaderFile::WriteClass(CBEClass *pClass)
{
	assert(pClass);
	pClass->Write(*this);
}

/** \brief writes the namespace
 *  \param pNameSpace the namespace to write
 */
void CBEHeaderFile::WriteNameSpace(CBENameSpace *pNameSpace)
{
	assert(pNameSpace);
	pNameSpace->Write(*this);
}

/** \brief writes the function
 *  \param pFunction the function to write
 */
void CBEHeaderFile::WriteFunction(CBEFunction *pFunction)
{
	assert(pFunction);
	if (pFunction->DoWriteFunction(this))
		pFunction->Write(*this);
}

/** \brief  writes a constant
 *  \param pConstant the constant to write
 */
void CBEHeaderFile::WriteConstant(CBEConstant *pConstant)
{
	assert(pConstant);
	pConstant->Write(*this);
}

/** \brief write a typedef
 *  \param pTypedef the typedef to write
 */
void CBEHeaderFile::WriteTypedef(CBETypedef *pTypedef)
{
	assert(pTypedef);
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called for %s.\n", __func__,
		pTypedef->m_Declarators.First()->GetName().c_str());
	pTypedef->WriteDeclaration(*this);
}

/** \brief writes a tagged type
 *  \param pType the type to write
 */
void CBEHeaderFile::WriteTaggedType(CBEType *pType)
{
	assert(pType);
	// get tag
	string sTag;
	if (dynamic_cast<CBEStructType*>(pType))
		sTag = ((CBEStructType*)pType)->GetTag();
	if (dynamic_cast<CBEUnionType*>(pType))
		sTag = ((CBEUnionType*)pType)->GetTag();
	sTag = CBENameFactory::Instance()->GetTypeDefine(sTag);
	*this <<
		"#ifndef " << sTag << "\n" <<
		"#define " << sTag << "\n";
	pType->Write(*this);
	*this << ";\n" <<
		"#endif /* !" << sTag << " */\n" <<
		"\n";
}

