/**
 *  \file    dice/src/be/BETypedef.cpp
 *  \brief   contains the implementation of the class CBETypedef
 *
 *  \date    01/18/2002
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

#include "BETypedef.h"
#include "BEContext.h"
#include "BEHeaderFile.h"
#include "BEImplementationFile.h"
#include "BEDeclarator.h"
#include "BEStructType.h"
#include "BEUserDefinedType.h"
#include "BENameFactory.h"
#include "BEClassFactory.h"
#include "Compiler.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEDeclarator.h"
#include "fe/FEFile.h"
#include <cassert>

CBETypedef::CBETypedef()
{ }

CBETypedef::CBETypedef(CBETypedef * src)
	: CBETypedDeclarator(src)
{ }

/** \brief destructor of this instance */
CBETypedef::~CBETypedef()
{ }

/** \brief create a copy of this object
 *  \return reference to clone
 */
CBETypedef* CBETypedef::Clone()
{
	return new CBETypedef(this);
}

/** \brief creates the typedef class
 *  \param pFETypedef the corresponding type definition
 *  \return true if successful
 *
 * This implementation extracts the define symbol.
 */
void
CBETypedef::CreateBackEnd(CFETypedDeclarator * pFETypedef)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBETypedef::%s called\n",
		__func__);

	// set target file name
	SetTargetFileName(pFETypedef);
	// if declarator changes, we have to reset the alias as well

	CBETypedDeclarator::CreateBackEnd(pFETypedef);
	// we are making a globalized name out of the declarator here by
	// extracting the name, getting a typename for it from the name factory
	// and recreating the declarator
	CBENameFactory *pNF = CBENameFactory::Instance();
	CBEDeclarator *pDecl = m_Declarators.First();
	assert(pDecl);
	string sAlias = pNF->GetTypeName(pFETypedef, pDecl->GetName());
	pDecl->CreateBackEnd(sAlias, pDecl->GetStars());
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBETypedef::%s created typedef <type>(%p) %s (%d stars) at %p\n",
		__func__, GetType(), sAlias.c_str(), pDecl->GetStars(), this);

	if (dynamic_cast<CBEUserDefinedType*>(m_pType))
	{
		assert(dynamic_cast<CBEUserDefinedType*>(m_pType)->GetName() !=
			pDecl->GetName());
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBETypedef::%s returns true\n", __func__);
}

/** \brief creates the typed declarator using a given back-end type and a name
 *  \param pType the type of the typed declarator
 *  \param sName the name of the declarator
 *  \param pFERefObject a reference object at the same position of the newly \
 *         created type
 *  \return true if successful
 */
void
CBETypedef::CreateBackEnd(CBEType * pType,
	string sName,
	CFEBase *pFERefObject)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBETypedef::%s(%p, %s) called\n", __func__, pType, sName.c_str());
	SetTargetFileName(pFERefObject);
	// if declarator changes, we have to reset the alias as well

	if (dynamic_cast<CBEUserDefinedType*>(pType))
	{
		assert(dynamic_cast<CBEUserDefinedType*>(pType)->GetName() != sName);
	}
	CBETypedDeclarator::CreateBackEnd(pType, sName);
	// a typedef can have only one name
	CBENameFactory *pNF = CBENameFactory::Instance();
	CBEDeclarator *pDecl = m_Declarators.First();
	string sAlias = pNF->GetTypeName(pFERefObject, pDecl->GetName());
	// recreate decl
	pDecl->CreateBackEnd(sAlias, pDecl->GetStars());
	// set source file information
	CBEObject::CreateBackEnd(pFERefObject);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBETypedef::%s(type, name) returns\n",	__func__);
}

/** \brief adds a typedefinition to the header file
 *  \param pHeader the header file to be added to
 *  \return true if successful
 *
 * A type definition is usually always added to a header file, if it is the
 * respective target file (for the IDL file).
 */
void CBETypedef::AddToHeader(CBEHeaderFile* pHeader)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBETypedef::%s(header: %s) for typedef %s called\n", __func__,
		pHeader->GetFileName().c_str(),
		m_Declarators.First()->GetName().c_str());
	if (IsTargetFile(pHeader))
		pHeader->m_Typedefs.Add(this);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBETypedef::%s(header) return true\n", __func__);
}

/** \brief writes the content of a typed declarator to the target file
 *  \param pFile the file to write to
 *
 * A typed declarator, such as a parameter, contain a type, name(s) and
 * optional attributes.
 */
void
CBETypedef::WriteDeclaration(CBEFile& pFile)
{
	CBEDeclarator *pDecl = m_Declarators.First();
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBETypedef::%s called for %s\n", __func__,
		pDecl->GetName().c_str());
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sDefine = pNF->GetTypeDefine(pDecl->GetName());
	// we need to write the define statement if the typedef does not belong to
	// a function.
	bool bNeedDefine = !sDefine.empty() && !GetSpecificParent<CBEFunction>();
	if (bNeedDefine)
	{
		pFile << "#if !defined(" << sDefine << ")\n";
		pFile << "#define " << sDefine << "\n";
	}

	int nSize = GetSize();
	pFile << "\t/* size = " << nSize << " bytes == " << ((nSize+3) >> 2) <<
		" dwords */\n";

	pFile << "\ttypedef ";
	CBETypedDeclarator::WriteDeclaration(pFile);
	pFile << ";\n";

	if (bNeedDefine)
	{
		pFile << "#endif /* " << sDefine << " */\n\n";
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBETypedef::%s returned\n", __func__);
}

