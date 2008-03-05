/**
 *  \file    dice/src/fe/FELibrary.cpp
 *  \brief   contains the implementation of the class CFELibrary
 *
 *  \date    01/31/2001
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

#include "FELibrary.h"
#include "FEIdentifier.h"
#include "FEInterface.h"
#include "FETypedDeclarator.h"
#include "FEConstDeclarator.h"
#include "FEConstructedType.h"
#include "FEStructType.h"
#include "FEEnumType.h"
#include "FEUnionType.h"
#include "FEAttribute.h"
#include "Compiler.h"
#include "Visitor.h"
#include <iostream>
#include <stdexcept>

CFELibrary::CFELibrary(std::string sName, vector<CFEAttribute*> * pAttributes, CFEBase* pParent)
: CFEFileComponent(static_cast<CObject*>(pParent)),
	m_Attributes(pAttributes, this),
	m_Constants(0, this),
	m_Typedefs(0, this),
	m_Interfaces(0, this),
	m_Libraries(0, this),
	m_TaggedDeclarators(0, this)
{
	m_sLibName = sName;
	m_pSameLibraryNext = 0;
	m_pSameLibraryPrev = 0;
}

CFELibrary::CFELibrary(CFELibrary* src)
: CFEFileComponent(src),
	m_Attributes(src->m_Attributes),
	m_Constants(src->m_Constants),
	m_Typedefs(src->m_Typedefs),
	m_Interfaces(src->m_Interfaces),
	m_Libraries(src->m_Libraries),
	m_TaggedDeclarators(src->m_TaggedDeclarators)
{
	m_sLibName = src->m_sLibName;
	m_pSameLibraryNext = 0;
	m_pSameLibraryPrev = 0;
	m_Attributes.Adopt(this);
	m_Constants.Adopt(this);
	m_Typedefs.Adopt(this);
	m_Interfaces.Adopt(this);
	m_Libraries.Adopt(this);
	m_TaggedDeclarators.Adopt(this);

	src->AddSameLibrary(this);
}

/** cleans up the library object */
CFELibrary::~CFELibrary()
{ }

/** \brief create a copy of this object
 *  \return a reference to the clone
 */
CFELibrary* CFELibrary::Clone()
{
	return new CFELibrary(this);
}

/** \brief add the components of the library
 *  \param pComponents the components to add
 */
void
CFELibrary::AddComponents(vector<CFEFileComponent*> *pComponents)
{
	if (pComponents)
	{
		// set parent
		for_each(pComponents->begin(), pComponents->end(),
			std::bind2nd(std::mem_fun(&CFEFileComponent::SetParent), this));
		vector<CFEFileComponent*>::iterator iterF;
		for (iterF = pComponents->begin(); iterF != pComponents->end(); iterF++)
		{
			if (!(*iterF))
				continue;
			if (dynamic_cast<CFEConstDeclarator*>(*iterF))
				m_Constants.Add(static_cast<CFEConstDeclarator*>(*iterF));
			else if (dynamic_cast<CFETypedDeclarator*>(*iterF))
				m_Typedefs.Add(static_cast<CFETypedDeclarator*>(*iterF));
			else if (dynamic_cast<CFEInterface*>(*iterF))
				m_Interfaces.Add(static_cast<CFEInterface*>(*iterF));
			else if (dynamic_cast<CFELibrary*>(*iterF))
				m_Libraries.Add(static_cast<CFELibrary*>(*iterF));
			else if (dynamic_cast<CFEConstructedType*>(*iterF))
				m_TaggedDeclarators.Add(static_cast<CFEConstructedType*>(*iterF));
			else
				throw new std::invalid_argument("Unknown lib element");
		}
	}
}

/**
 *  \brief returns the library's name
 *  \return the library's name
 */
string CFELibrary::GetName()
{
	return m_sLibName;
}

/** \brief returns true if this library is the one we are looking for
 *  \param sName the name to match
 *  \return true if matches
 */
bool CFELibrary::Match(std::string sName)
{
	return GetName() == sName;
}

/** \brief accept iterations of the visitors
 *  \param v reference to the visitor
 */
void CFELibrary::Accept(CVisitor& v)
{
	v.Visit(*this);
}

/**
 *  \brief tries to find a user defined type
 *  \param sName the name of the type
 *  \return a reference to the type if successfule, 0 otherwise
 */
CFETypedDeclarator *CFELibrary::FindUserDefinedType(std::string sName)
{
	// search own types
	CFETypedDeclarator *pTypedef = m_Typedefs.Find(sName);
	if (pTypedef)
		return pTypedef;
	// search interfaces
	vector<CFEInterface*>::iterator iterI;
	for (iterI = m_Interfaces.begin();
		iterI != m_Interfaces.end();
		iterI++)
	{
		if ((pTypedef = (*iterI)->m_Typedefs.Find(sName)))
			return pTypedef;
	}
	// search nested libraries
	vector<CFELibrary*>::iterator iterL;
	for (iterL = m_Libraries.begin();
		iterL != m_Libraries.end();
		iterL++)
	{
		if ((pTypedef = (*iterL)->FindUserDefinedType(sName)))
			return pTypedef;
	}
	// nothing found
	return 0;
}

/** \brief tries to find a constant
 *  \param sName the name of the constant
 *  \return a reference to the constant if found, 0 if not found
 */
CFEConstDeclarator *CFELibrary::FindConstant(std::string sName)
{
	if (sName.empty())
		return 0;
	// first search own constants
	CFEConstDeclarator *pConst = m_Constants.Find(sName);
	if (pConst)
		return pConst;
	// next search interfaces
	vector<CFEInterface*>::iterator iterI;
	for (iterI = m_Interfaces.begin();
		iterI != m_Interfaces.end();
		iterI++)
	{
		if ((pConst = (*iterI)->m_Constants.Find(sName)))
			return pConst;
	}
	// next search nested libraries
	vector<CFELibrary*>::iterator iterL;
	for (iterL = m_Libraries.begin();
		iterL != m_Libraries.end();
		iterL++)
	{
		if ((pConst = (*iterL)->FindConstant(sName)))
			return pConst;
	}
	// nothing found
	return 0;
}

/** \brief searches for a library in the nested libs
 *  \param sName the name of the lib to search for
 *  \return a reference to the library or 0 if not found
 */
CFELibrary *CFELibrary::FindLibrary(std::string sName)
{
	if (sName.empty())
		return 0;

	// if scoped name
	string::size_type nScopePos;
	if ((nScopePos = sName.find("::")) != string::npos)
	{
		string sRest = sName.substr(nScopePos+2);
		string sScope = sName.substr(0, nScopePos);
		if (sScope.empty())
		{
			// has been a "::<name>"
			// should not happend -> always ask root for this
			return 0;
		}
		else
		{
			CFELibrary *pFELibrary = FindLibrary(sScope);
			if (pFELibrary == 0)
				return 0;
			return pFELibrary->FindLibrary(sRest);
		}
	}

	CFELibrary *pLib = m_Libraries.Find(sName);
	if (pLib)
		return pLib;
	// do not search lib in nested libs (breaks scope)

	return 0;
}

#include "fe/FEFile.h"

/** \brief searches for interface
 *  \param sName the name of the interface to search for
 *  \param pStart the start of the search through the same lib
 *  \return a reference to the found interface or 0 if nothing found
 */
CFEInterface *CFELibrary::FindInterface(std::string sName, CFELibrary* pStart)
{
	if (sName.empty())
		return 0;

	CCompiler::Verbose("CFELibrary::%s(%s, %s) called (in file %s)\n", __func__,
		sName.c_str(), pStart ? pStart->GetName().c_str() : "(null)",
		GetSpecificParent<CFEFile>()->GetFileName().c_str());

	// if scoped name
	string::size_type nScopePos;
	if ((nScopePos = sName.find("::")) != string::npos)
	{
		string sRest = sName.substr(sName.length()-nScopePos-2);
		string sScope = sName.substr(0, nScopePos);
		if (sScope.empty())
		{
			// has been a "::<name>"
			return FindInterface(sRest);
		}
		else
		{
			CFELibrary *pFELibrary = FindLibrary(sScope);
			if (pFELibrary == 0)
				return 0;
			return pFELibrary->FindInterface(sRest);
		}
	}

	CFEInterface *pInterface = m_Interfaces.Find(sName);
	CCompiler::Verbose("CFELibrary::%s: search for own interface found %p\n", __func__,
		pInterface);
	if (pInterface)
		return pInterface;

	vector<CFELibrary*>::iterator iterL;
	for (iterL = m_Libraries.begin();
		iterL != m_Libraries.end();
		iterL++)
	{
		if ((pInterface = (*iterL)->FindInterface(sName)))
			return pInterface;
	}

	// if we have "same libraries", maybe interface is defined there
	// first we check if we are the start. If so, we have to stop.
	// if not, simply check if we have next lib. If so, call its Find function
	// if not, go to begin of list and call its Find function
	//
	// This way we first check the local members of the originally called
	// library. If we did not find anything there, we check the next in line
	// and tell it, that we are the start of the search. When we reach the end
	// of the list we wrap around to the start of the list (originally called
	// lib can be anywhere in the middle). Because Find function always test the
	// next lib, we only have to call Find for our next lib.
	if (!pStart)
		pStart = this;
	else
		if (pStart == this)
			// we wrapped around and did not find anything
			return 0;

	// now check for next member and call it
	if (m_pSameLibraryNext)
		// either we find interface there or not. The result is valid.
		return m_pSameLibraryNext->FindInterface(sName, pStart);
	// no next, go to begin of list (this is end of list)
	CFELibrary *pLib = this;
	while (pLib->m_pSameLibraryPrev)
		pLib = pLib->m_pSameLibraryPrev;
	// and call Find function at begin of list
	// since any result of this call is valid, simply return it
	return pLib->FindInterface(sName, pStart);
}

/** \brief adds a library with the same name to the same lib vector
 *  \param pLibrary the library to add
 *
 * Same libraries have the same name (and name hierarchy) but reside in
 * different files. Because we cannot simply add the members of one lib to
 * another lib (they would belong to another file) we have to do it this way.
 */
void CFELibrary::AddSameLibrary(CFELibrary* pLibrary)
{
	if (!pLibrary)
		return;

	// search for last element in list
	CFELibrary *pLast = this;
	while (pLast->m_pSameLibraryNext) pLast = pLast->m_pSameLibraryNext;
	// add given lib at end of list
	pLast->m_pSameLibraryNext = pLibrary;
	pLibrary->m_pSameLibraryPrev = pLast;
}

/** \brief search for a tagged declarator
 *  \param sName the tag (name) of the tagged decl
 *  \return a reference to the found tagged decl or NULL if none found
 */
CFEConstructedType* CFELibrary::FindTypeWithTag(std::string sName)
{
	// own tagged decls
	CFEConstructedType* pTaggedDecl = m_TaggedDeclarators.Find(sName);
	if (pTaggedDecl)
		return pTaggedDecl;
	// can be the type of one of the typedefs
	vector<CFETypedDeclarator*>::iterator iterT;
	for (iterT = m_Typedefs.begin(); iterT != m_Typedefs.end(); iterT++)
	{
		pTaggedDecl = dynamic_cast<CFEConstructedType*>((*iterT)->GetType());
		if (pTaggedDecl && pTaggedDecl->Match(sName))
			return pTaggedDecl;
	}
	// search interfaces
	vector<CFEInterface*>::iterator iterI;
	for (iterI = m_Interfaces.begin();
		iterI != m_Interfaces.end();
		iterI++)
	{
		if ((pTaggedDecl = (*iterI)->FindTypeWithTag(sName)) != 0)
			return pTaggedDecl;
	}
	// search nested libs
	vector<CFELibrary*>::iterator iterL;
	for (iterL = m_Libraries.begin();
		iterL != m_Libraries.end();
		iterL++)
	{
		if ((pTaggedDecl = (*iterL)->FindTypeWithTag(sName)) != 0)
			return pTaggedDecl;
	}
	// nothing found:
	return 0;
}
