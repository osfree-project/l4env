/**
 *	\file	dice/src/fe/FELibrary.cpp
 *	\brief	contains the implementation of the class CFELibrary
 *
 *	\date	01/31/2001
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

#include "fe/FELibrary.h"
#include "fe/FEIdentifier.h"
#include "fe/FEInterface.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FEConstructedType.h"
#include "fe/FETaggedStructType.h"
#include "fe/FETaggedUnionType.h"
#include "fe/FETaggedEnumType.h"
#include "File.h"

IMPLEMENT_DYNAMIC(CFELibrary) 

CFELibrary::CFELibrary(String sName, Vector * pAttributes, Vector * pElements)
: m_vAttributes(RUNTIME_CLASS(CFEAttribute)),
  m_vSameLibrary(RUNTIME_CLASS(CFELibrary)),
  m_vConstants(RUNTIME_CLASS(CFEConstDeclarator)),
  m_vTypedefs(RUNTIME_CLASS(CFETypedDeclarator)),
  m_vInterfaces(RUNTIME_CLASS(CFEInterface)),
  m_vLibraries(RUNTIME_CLASS(CFELibrary)),
  m_vTaggedDeclarators(RUNTIME_CLASS(CFEConstructedType))
{
    IMPLEMENT_DYNAMIC_BASE(CFELibrary, CFEFileComponent);

    m_sLibName = sName;
    if (pAttributes)
		m_vAttributes.Add(pAttributes);
	if (pElements)
	{
		VectorElement *pIter;
		for (pIter = pElements->GetFirst(); pIter; pIter = pIter->GetNext())
		{
			if (pIter->GetElement())
			{
				if (pIter->GetElement()->IsKindOf(RUNTIME_CLASS(CFEConstDeclarator)))
					AddConstant((CFEConstDeclarator*) pIter->GetElement());
				else if (pIter->GetElement()->IsKindOf(RUNTIME_CLASS(CFETypedDeclarator)))
					AddTypedef((CFETypedDeclarator*) pIter->GetElement());
				else if (pIter->GetElement()->IsKindOf(RUNTIME_CLASS(CFEInterface)))
					AddInterface((CFEInterface*) pIter->GetElement());
				else if (pIter->GetElement()->IsKindOf(RUNTIME_CLASS(CFELibrary)))
					AddLibrary((CFELibrary*) pIter->GetElement());
				else if (pIter->GetElement()->IsKindOf(RUNTIME_CLASS(CFEConstructedType)))
					AddTaggedDecl((CFEConstructedType*) pIter->GetElement());
				else
				{
					TRACE("Unknown Library element: %s\n", pIter->GetElement()->GetClassName());
					assert(false);
				}
			}
		}
	}
}

CFELibrary::CFELibrary(CFELibrary & src)
: CFEFileComponent(src),
  m_vAttributes(RUNTIME_CLASS(CFEAttribute)),
  m_vSameLibrary(RUNTIME_CLASS(CFELibrary)),
  m_vConstants(RUNTIME_CLASS(CFEConstDeclarator)),
  m_vTypedefs(RUNTIME_CLASS(CFETypedDeclarator)),
  m_vInterfaces(RUNTIME_CLASS(CFEInterface)),
  m_vLibraries(RUNTIME_CLASS(CFELibrary)),
  m_vTaggedDeclarators(RUNTIME_CLASS(CFEConstructedType))
{
    IMPLEMENT_DYNAMIC_BASE(CFELibrary, CFEFileComponent);

    m_sLibName = src.m_sLibName;
    m_vAttributes.Add(&src.m_vAttributes);
    m_vSameLibrary.Add(&src.m_vSameLibrary);
    m_vConstants.Add(&src.m_vConstants);
    m_vTypedefs.Add(&src.m_vTypedefs);
    m_vInterfaces.Add(&src.m_vInterfaces);
    m_vLibraries.Add(&src.m_vLibraries);
    m_vTaggedDeclarators.Add(&src.m_vTaggedDeclarators);
}

/** cleans up the library object */
CFELibrary::~CFELibrary()
{
}

/**
 *	\brief retrieves a pointer to the first interface
 *	\return an iterator which points to the first interface
 */
VectorElement *CFELibrary::GetFirstInterface()
{
	return m_vInterfaces.GetFirst();
}

/**
 *	\brief retrieves the next interface
 *	\param iter the iterator pointing to the next interface
 *	\return a reference to the next interface
 */
CFEInterface *CFELibrary::GetNextInterface(VectorElement * &iter)
{
    if (!iter)
        return 0;
    CFEInterface *pRet = (CFEInterface*)iter->GetElement();
    iter = iter->GetNext();
    if (!pRet)
		return GetNextInterface(iter);
	return pRet;
}

/**
 *	\brief retrives the first a pointer to the attribute
 *	\return an iterator which points to the first attribute
 */
VectorElement *CFELibrary::GetFirstAttribute()
{
	return m_vAttributes.GetFirst();
}

/**
 *	\brief retrieves a reference to the next attribute
 *	\param iter the iterator which points to the next attribute
 *	\return a reference to the next attribute
 */
CFEAttribute *CFELibrary::GetNextAttribute(VectorElement * &iter)
{
    if (!iter)
        return 0;
    CFEAttribute *pRet = (CFEAttribute *) (iter->GetElement());
    iter = iter->GetNext();
    if (!pRet)
        return GetNextAttribute(iter);
    return pRet;
}

/**
 *	\brief returns the library's name
 *	\return the library's name
 */
String CFELibrary::GetName()
{
    return m_sLibName;
}

/**
 *	\brief tries to find a user defined type
 *	\param sName the name of the type
 *	\return a reference to the type if successfule, 0 otherwise
 */
CFETypedDeclarator *CFELibrary::FindUserDefinedType(String sName)
{
    // search own types
    VectorElement *pIter = GetFirstTypedef();
    CFETypedDeclarator *pTypedef;
    while ((pTypedef = GetNextTypedef(pIter)) != 0)
    {
        if (pTypedef->FindDeclarator(sName))
            return pTypedef;
    }
    // search interfaces
    pIter = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(pIter)) != 0)
    {
        if ((pTypedef = pInterface->FindUserDefinedType(sName)))
            return pTypedef;
    }
    // search nested libraries
    pIter = GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = GetNextLibrary(pIter)) != 0)
    {
        if ((pTypedef = pLibrary->FindUserDefinedType(sName)))
            return pTypedef;
    }
    // nothing found
    return 0;
}

/** creates a copy of this object
 *	\return a copy of this object
 */
CObject *CFELibrary::Clone()
{
    return new CFELibrary(*this);
}

/** retrives a pointer to the first element of the library
 *	\return a pointer to the first element of the library
 */
VectorElement *CFELibrary::GetFirstConstant()
{
	return m_vConstants.GetFirst();
}

/**
 *	\brief retrieves a reference to the next constant
 *	\param iter the iterator which points to the next constant
 *	\return a reference to the next constant
 */
CFEConstDeclarator *CFELibrary::GetNextConstant(VectorElement * &iter)
{
     if (!iter)
         return 0;
     CFEConstDeclarator *pRet = (CFEConstDeclarator*) iter->GetElement();
     iter = iter->GetNext();
     if (!pRet)
         return GetNextConstant(iter);
     return pRet;
}

/** retrives a pointer to the first element of the library
 *	\return a pointer to the first element of the library
 */
VectorElement *CFELibrary::GetFirstTypedef()
{
	return m_vTypedefs.GetFirst();
}

/**
 *	\brief retrieves a reference to the next typedef
 *	\param iter the iterator which points to the next typedef
 *	\return a reference to the next typedef
 */
CFETypedDeclarator *CFELibrary::GetNextTypedef(VectorElement * &iter)
{
     if (!iter)
         return 0;
     CFETypedDeclarator *pRet = (CFETypedDeclarator*) iter->GetElement();
     iter = iter->GetNext();
     if (!pRet)
         return GetNextTypedef(iter);
     return pRet;
}

/** retrives a pointer to the first element of the library
 *	\return a pointer to the first element of the library
 */
VectorElement *CFELibrary::GetFirstLibrary()
{
	return m_vLibraries.GetFirst();
}

/**
 *	\brief retrieves a reference to the next nested library
 *	\param iter the iterator which points to the next nested library
 *	\return a reference to the next nested library
 */
CFELibrary *CFELibrary::GetNextLibrary(VectorElement * &iter)
{
    if (!iter)
        return 0;
    CFELibrary *pRet = (CFELibrary*) iter->GetElement();
    iter = iter->GetNext();
    if (!pRet)
        return GetNextLibrary(iter);
	return pRet;
}

/** \brief checks the library
 *  \return true if everything is fine, false if not
 *
 * A library is ok if all its members are ok.
 */
bool CFELibrary::CheckConsistency()
{
    // check typedefs
    VectorElement *pIter = GetFirstTypedef();
    CFETypedDeclarator *pTypedef;
    while ((pTypedef = GetNextTypedef(pIter)) != 0)
    {
        if (!(pTypedef->CheckConsistency()))
            return false;
    }
    // check constants
    pIter = GetFirstConstant();
    CFEConstDeclarator *pConst;
    while ((pConst = GetNextConstant(pIter)) != 0)
    {
        if (!(pConst->CheckConsistency()))
            return false;
    }
    // check interfaces
    pIter = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(pIter)) != 0)
    {
        if (!(pInterface->CheckConsistency()))
            return false;
    }
    // check nested libraries
    pIter = GetFirstLibrary();
    CFELibrary *pLib;
    while ((pLib = GetNextLibrary(pIter)) != 0)
    {
        if (!(pLib->CheckConsistency()))
            return false;
    }
    // we ran through, so it seems we're ok.
    return true;
}

/** \brief tries to find a constant
 *  \param sName the name of the constant
 *  \return a reference to the constant if found, 0 if not found
 */
CFEConstDeclarator *CFELibrary::FindConstant(String sName)
{
    if (sName.IsEmpty())
        return 0;
    // first search own constants
    VectorElement *pIter = GetFirstConstant();
    CFEConstDeclarator *pConst;
    while ((pConst = GetNextConstant(pIter)) != 0)
    {
        if (pConst->GetName() == sName)
            return pConst;
    }
    // next search interfaces
    pIter = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(pIter)) != 0)
    {
        if ((pConst = pInterface->FindConstant(sName)))
            return pConst;
    }
    // next search nested libraries
    pIter = GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = GetNextLibrary(pIter)) != 0)
    {
        if ((pConst = pLibrary->FindConstant(sName)))
            return pConst;
    }
    // nothing found
    return 0;
}

/**	\brief searches for a library in the nested libs
 *	\param sName the name of the lib to search for
 *	\return a reference to the library or 0 if not found
 */
CFELibrary *CFELibrary::FindLibrary(String sName)
{
    if (sName.IsEmpty())
        return 0;

    VectorElement *pIter = GetFirstLibrary();
    CFELibrary *pLib, *pLib2;
    while ((pLib = GetNextLibrary(pIter)) != 0)
    {
        if (pLib->GetName() == sName)
            return pLib;
        if ((pLib2 = pLib->FindLibrary(sName)))
            return pLib2;
    }

    return 0;
}

/**	\brief searches for interface
 *	\param sName the name of the interface to search for
 *	\return a reference to the found interface or 0 if nothing found
 */
CFEInterface *CFELibrary::FindInterface(String sName)
{
    if (sName.IsEmpty())
        return 0;

    // if scoped name
    int nScopePos;
    if ((nScopePos = sName.Find("::")) >= 0)
    {
        String sRest = sName.Right(sName.GetLength()-nScopePos-2);
        String sScope = sName.Left(nScopePos);
        if (sScope.IsEmpty())
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

    VectorElement *pIter = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(pIter)) != 0)
    {
        if (pInterface->GetName() == sName)
            return pInterface;
    }

    pIter = GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = GetNextLibrary(pIter)) != 0)
    {
        if ((pInterface = pLibrary->FindInterface(sName)))
            return pInterface;
    }

    return 0;
}

/** for debugging purposes only */
void CFELibrary::Dump()
{
    printf("Dump: CFELibrary (%s)\n", (const char *) GetName());
    printf("Dump: CFELibrary (%s): typedefs\n", (const char *) GetName());
    VectorElement *pIter = GetFirstTypedef();
    CFEBase *pElement;
    while ((pElement = GetNextTypedef(pIter)) != 0)
    {
        pElement->Dump();
    }
    printf("Dump: CFELibrary (%s): constants\n", (const char *) GetName());
    pIter = GetFirstConstant();
    while ((pElement = GetNextConstant(pIter)) != 0)
    {
        pElement->Dump();
    }
    printf("Dump: CFELibrary (%s): nested libraries\n", (const char *) GetName());
    pIter = GetFirstLibrary();
    while ((pElement = GetNextLibrary(pIter)) != 0)
    {
        pElement->Dump();
    }
    printf("Dump: CFELibrary (%s): interfaces\n", (const char *) GetName());
    pIter = GetFirstInterface();
    while ((pElement = GetNextInterface(pIter)) != 0)
    {
        pElement->Dump();
    }
}

/**	\brief serialize this object
 *	\param pFile the file to write to/read from
 */
void CFELibrary::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        pFile->PrintIndent("<library>\n");
        pFile->IncIndent();
        pFile->PrintIndent("<name>%s</name>\n", (const char *) m_sLibName);
        // print the library's attributes
        VectorElement *pIter = GetFirstAttribute();
        CFEBase *pElement;
        while ((pElement = GetNextAttribute(pIter)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // print the library's libraries
        pIter = GetFirstLibrary();
        while ((pElement = GetNextLibrary(pIter)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // print the library's constants
        pIter = GetFirstConstant();
        while ((pElement = GetNextConstant(pIter)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // print the library's typedefs
        pIter = GetFirstTypedef();
        while ((pElement = GetNextTypedef(pIter)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // print the library's interfaces
        pIter = GetFirstInterface();
        while ((pElement = GetNextInterface(pIter)) != 0)
        {
            pElement->Serialize(pFile);
        }
        pFile->DecIndent();
        pFile->PrintIndent("</library>\n");
    }
}

/**	\brief adds a library with the same name to the same lib vector
 *	\param pLibrary the library to add
 *
 * Same libraries have the same name (and name hierarchy) but reside in different
 * files. Because we cannot simply add the members of one lib to another lib (they
 * would belong to another file) we have to do it this way.
 */
void CFELibrary::AddSameLibrary(CFELibrary * pLibrary)
{
    m_vSameLibrary.Add(pLibrary);
}

/** \brief adds a new interface to the library
 *  \param pFEInterface the new interface to add
 */
void CFELibrary::AddInterface(CFEInterface *pFEInterface)
{
	m_vInterfaces.Add(pFEInterface);
	pFEInterface->SetParent(this);
}

/** \brief adds a new type definition to the library
 *  \param pFETypedef the new type defintion
 */
void CFELibrary::AddTypedef(CFETypedDeclarator *pFETypedef)
{
	m_vTypedefs.Add(pFETypedef);
	pFETypedef->SetParent(this);
}

/** \brief adds a new constant to the library
 *  \param pFEConstant the constant to add
 */
void CFELibrary::AddConstant(CFEConstDeclarator *pFEConstant)
{
	m_vConstants.Add(pFEConstant);
	pFEConstant->SetParent(this);
}

/** \brief adds a tagged constructed type to the library
 *  \param pFETaggedDecl the new tagged constructed type
 */
void CFELibrary::AddTaggedDecl(CFEConstructedType *pFETaggedDecl)
{
	m_vTaggedDeclarators.Add(pFETaggedDecl);
	pFETaggedDecl->SetParent(this);
}

/** \brief adds a nested library
 *  \param pFELibrary the new nested library to add
 */
void CFELibrary::AddLibrary(CFELibrary *pFELibrary)
{
	m_vLibraries.Add(pFELibrary);
	pFELibrary->SetParent(this);
}

/** \brief retrieve a pointer to the first tagged type declaration
 *  \return a pointer to the first tagged type decl
 */
VectorElement* CFELibrary::GetFirstTaggedDecl()
{
    return m_vTaggedDeclarators.GetFirst();
}

/** \brief retrieve a reference to the next tagged decl
 *  \param pIter the pointer to the next tagged decl
 *  \return a reference to the next tagged decl
 */
CFEConstructedType* CFELibrary::GetNextTaggedDecl(VectorElement* &pIter)
{
    if (!pIter)
        return 0;
    CFEConstructedType *pRet = (CFEConstructedType*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextTaggedDecl(pIter);
    return pRet;
}

/** \brief search for a tagged declarator
 *  \param sName the tag (name) of the tagged decl
 *  \return a reference to the found tagged decl or NULL if none found
 */
CFEConstructedType* CFELibrary::FindTaggedDecl(String sName)
{
    // own tagged decls
    VectorElement *pIter = GetFirstTaggedDecl();
    CFEConstructedType* pTaggedDecl;
    while ((pTaggedDecl = GetNextTaggedDecl(pIter)) != 0)
    {
        if (pTaggedDecl->IsKindOf(RUNTIME_CLASS(CFETaggedStructType)))
            if (((CFETaggedStructType*)pTaggedDecl)->GetTag() == sName)
                return pTaggedDecl;
        if (pTaggedDecl->IsKindOf(RUNTIME_CLASS(CFETaggedUnionType)))
            if (((CFETaggedUnionType*)pTaggedDecl)->GetTag() == sName)
                return pTaggedDecl;
        if (pTaggedDecl->IsKindOf(RUNTIME_CLASS(CFETaggedEnumType)))
            if (((CFETaggedEnumType*)pTaggedDecl)->GetTag() == sName)
                return pTaggedDecl;
    }
    // search interfaces
    pIter = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(pIter)) != 0)
    {
        if ((pTaggedDecl = pInterface->FindTaggedDecl(sName)) != 0)
            return pTaggedDecl;
    }
    // search nested libs
    pIter = GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = GetNextLibrary(pIter)) != 0)
    {
        if ((pTaggedDecl = pLibrary->FindTaggedDecl(sName)) != 0)
            return pTaggedDecl;
    }
    // nothing found:
    return 0;
}
