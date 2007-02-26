/**
 *    \file    dice/src/fe/FELibrary.cpp
 *    \brief   contains the implementation of the class CFELibrary
 *
 *    \date    01/31/2001
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "fe/FELibrary.h"
#include "fe/FEIdentifier.h"
#include "fe/FEInterface.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FEConstructedType.h"
#include "fe/FETaggedStructType.h"
#include "fe/FETaggedUnionType.h"
#include "fe/FETaggedEnumType.h"
#include "fe/FEAttribute.h"
#include "File.h"

#include <typeinfo>
using namespace std;

CFELibrary::CFELibrary(string sName, vector<CFEAttribute*> * pAttributes,
    vector<CFEFileComponent*> * pElements)
{
    m_sLibName = sName;
    m_pSameLibraryNext = 0;
    m_pSameLibraryPrev = 0;
    if (pAttributes)
        m_vAttributes.swap(*pAttributes);
    vector<CFEAttribute*>::iterator iter;
    for (iter = m_vAttributes.begin(); iter != m_vAttributes.end(); iter++)
    {
        (*iter)->SetParent(this);
    }
    if (pElements)
    {
        // set parent
        vector<CFEFileComponent*>::iterator iterF;
        for (iterF = pElements->begin(); iterF != pElements->end(); iterF++)
        {
            (*iterF)->SetParent(this);
        }
        for (iterF = pElements->begin(); iterF != pElements->end(); iterF++)
        {
            if (!(*iterF))
                continue;
            if (dynamic_cast<CFEConstDeclarator*>(*iterF))
                AddConstant((CFEConstDeclarator*) *iterF);
            else if (dynamic_cast<CFETypedDeclarator*>(*iterF))
                AddTypedef((CFETypedDeclarator*) *iterF);
            else if (dynamic_cast<CFEInterface*>(*iterF))
                AddInterface((CFEInterface*) *iterF);
            else if (dynamic_cast<CFELibrary*>(*iterF))
                AddLibrary((CFELibrary*) *iterF);
            else if (dynamic_cast<CFEConstructedType*>(*iterF))
                AddTaggedDecl((CFEConstructedType*) *iterF);
            else
            {
                TRACE("Unknown Library element: %s\n", typeid(*(*iterF)).name());
                assert(false);
            }
        }
    }
}

CFELibrary::CFELibrary(CFELibrary & src)
: CFEFileComponent(src)
{
    m_sLibName = src.m_sLibName;
    m_pSameLibraryNext = 0;
    m_pSameLibraryPrev = 0;

    COPY_VECTOR(CFEAttribute, m_vAttributes, iterA);
    COPY_VECTOR(CFEConstDeclarator, m_vConstants, iterC);
    COPY_VECTOR(CFETypedDeclarator, m_vTypedefs, iterT);
    COPY_VECTOR(CFEInterface, m_vInterfaces, iterI);
    COPY_VECTOR(CFELibrary, m_vLibraries, iterL);
    COPY_VECTOR(CFEConstructedType, m_vTaggedDeclarators, iterCT);

    src.AddSameLibrary(this);
}

/** cleans up the library object */
CFELibrary::~CFELibrary()
{
    DEL_VECTOR(m_vAttributes);
    DEL_VECTOR(m_vConstants);
    DEL_VECTOR(m_vTypedefs);
    DEL_VECTOR(m_vInterfaces);
    DEL_VECTOR(m_vLibraries);
    DEL_VECTOR(m_vTaggedDeclarators);
}

/**
 *    \brief retrieves a pointer to the first interface
 *    \return an iterator which points to the first interface
 */
vector<CFEInterface*>::iterator CFELibrary::GetFirstInterface()
{
    return m_vInterfaces.begin();
}

/**
 *    \brief retrieves the next interface
 *    \param iter the iterator pointing to the next interface
 *    \return a reference to the next interface
 */
CFEInterface *CFELibrary::GetNextInterface(vector<CFEInterface*>::iterator &iter)
{
    if (iter == m_vInterfaces.end())
        return 0;
    return *iter++;
}

/**
 *    \brief retrives the first a pointer to the attribute
 *    \return an iterator which points to the first attribute
 */
vector<CFEAttribute*>::iterator CFELibrary::GetFirstAttribute()
{
    return m_vAttributes.begin();
}

/**
 *    \brief retrieves a reference to the next attribute
 *    \param iter the iterator which points to the next attribute
 *    \return a reference to the next attribute
 */
CFEAttribute *CFELibrary::GetNextAttribute(vector<CFEAttribute*>::iterator &iter)
{
    if (iter == m_vAttributes.end())
        return 0;
    return *iter++;
}

/**
 *    \brief returns the library's name
 *    \return the library's name
 */
string CFELibrary::GetName()
{
    return m_sLibName;
}

/**
 *    \brief tries to find a user defined type
 *    \param sName the name of the type
 *    \return a reference to the type if successfule, 0 otherwise
 */
CFETypedDeclarator *CFELibrary::FindUserDefinedType(string sName)
{
    // search own types
    vector<CFETypedDeclarator*>::iterator iterT = GetFirstTypedef();
    CFETypedDeclarator *pTypedef;
    while ((pTypedef = GetNextTypedef(iterT)) != 0)
    {
        if (pTypedef->FindDeclarator(sName))
            return pTypedef;
    }
    // search interfaces
    vector<CFEInterface*>::iterator iterI = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(iterI)) != 0)
    {
        if ((pTypedef = pInterface->FindUserDefinedType(sName)))
            return pTypedef;
    }
    // search nested libraries
    vector<CFELibrary*>::iterator iterL = GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = GetNextLibrary(iterL)) != 0)
    {
        if ((pTypedef = pLibrary->FindUserDefinedType(sName)))
            return pTypedef;
    }
    // nothing found
    return 0;
}

/** creates a copy of this object
 *    \return a copy of this object
 */
CObject *CFELibrary::Clone()
{
    return new CFELibrary(*this);
}

/** retrives a pointer to the first element of the library
 *    \return a pointer to the first element of the library
 */
vector<CFEConstDeclarator*>::iterator CFELibrary::GetFirstConstant()
{
    return m_vConstants.begin();
}

/**
 *    \brief retrieves a reference to the next constant
 *    \param iter the iterator which points to the next constant
 *    \return a reference to the next constant
 */
CFEConstDeclarator *CFELibrary::GetNextConstant(vector<CFEConstDeclarator*>::iterator &iter)
{
    if (iter == m_vConstants.end())
        return 0;
    return *iter++;
}

/** retrives a pointer to the first element of the library
 *    \return a pointer to the first element of the library
 */
vector<CFETypedDeclarator*>::iterator CFELibrary::GetFirstTypedef()
{
    return m_vTypedefs.begin();
}

/**
 *    \brief retrieves a reference to the next typedef
 *    \param iter the iterator which points to the next typedef
 *    \return a reference to the next typedef
 */
CFETypedDeclarator *CFELibrary::GetNextTypedef(vector<CFETypedDeclarator*>::iterator &iter)
{
    if (iter == m_vTypedefs.end())
        return 0;
    return *iter++;
}

/** retrives a pointer to the first element of the library
 *    \return a pointer to the first element of the library
 */
vector<CFELibrary*>::iterator CFELibrary::GetFirstLibrary()
{
    return m_vLibraries.begin();
}

/**
 *    \brief retrieves a reference to the next nested library
 *    \param iter the iterator which points to the next nested library
 *    \return a reference to the next nested library
 */
CFELibrary *CFELibrary::GetNextLibrary(vector<CFELibrary*>::iterator &iter)
{
    if (iter == m_vLibraries.end())
        return 0;
    return *iter++;
}

/** \brief checks the library
 *  \return true if everything is fine, false if not
 *
 * A library is ok if all its members are ok.
 */
bool CFELibrary::CheckConsistency()
{
    // check typedefs
    vector<CFETypedDeclarator*>::iterator iterT = GetFirstTypedef();
    CFETypedDeclarator *pTypedef;
    while ((pTypedef = GetNextTypedef(iterT)) != 0)
    {
        if (!(pTypedef->CheckConsistency()))
            return false;
    }
    // check constants
    vector<CFEConstDeclarator*>::iterator iterC = GetFirstConstant();
    CFEConstDeclarator *pConst;
    while ((pConst = GetNextConstant(iterC)) != 0)
    {
        if (!(pConst->CheckConsistency()))
            return false;
    }
    // check interfaces
    vector<CFEInterface*>::iterator iterI = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(iterI)) != 0)
    {
        if (!(pInterface->CheckConsistency()))
            return false;
    }
    // check nested libraries
    vector<CFELibrary*>::iterator iterL = GetFirstLibrary();
    CFELibrary *pLib;
    while ((pLib = GetNextLibrary(iterL)) != 0)
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
CFEConstDeclarator *CFELibrary::FindConstant(string sName)
{
    if (sName.empty())
        return 0;
    // first search own constants
    vector<CFEConstDeclarator*>::iterator iterC = GetFirstConstant();
    CFEConstDeclarator *pConst;
    while ((pConst = GetNextConstant(iterC)) != 0)
    {
        if (pConst->GetName() == sName)
            return pConst;
    }
    // next search interfaces
    vector<CFEInterface*>::iterator iterI = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(iterI)) != 0)
    {
        if ((pConst = pInterface->FindConstant(sName)))
            return pConst;
    }
    // next search nested libraries
    vector<CFELibrary*>::iterator iterL = GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = GetNextLibrary(iterL)) != 0)
    {
        if ((pConst = pLibrary->FindConstant(sName)))
            return pConst;
    }
    // nothing found
    return 0;
}

/**    \brief searches for a library in the nested libs
 *    \param sName the name of the lib to search for
 *    \return a reference to the library or 0 if not found
 */
CFELibrary *CFELibrary::FindLibrary(string sName)
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

    vector<CFELibrary*>::iterator iterL = GetFirstLibrary();
    CFELibrary *pLib;
    while ((pLib = GetNextLibrary(iterL)) != 0)
    {
        if (pLib->GetName() == sName)
            return pLib;
// do not search lib in nested libs (breaks scope)
//         if ((pLib2 = pLib->FindLibrary(sName)))
//             return pLib2;
    }

    return 0;
}

/**    \brief searches for interface
 *    \param sName the name of the interface to search for
 *    \param pStart the start of the search through the same lib
 *    \return a reference to the found interface or 0 if nothing found
 */
CFEInterface *CFELibrary::FindInterface(string sName, CFELibrary* pStart)
{
    if (sName.empty())
        return 0;

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

    vector<CFEInterface*>::iterator iterI = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(iterI)) != 0)
    {
        if (pInterface->GetName() == sName)
            return pInterface;
    }

    vector<CFELibrary*>::iterator iterL = GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = GetNextLibrary(iterL)) != 0)
    {
        if ((pInterface = pLibrary->FindInterface(sName)))
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
    else
    {
        // no next, go to begin of list (this is end of list)
        CFELibrary *pSameLib = this;
        while (pSameLib->m_pSameLibraryPrev)
            pSameLib = pSameLib->m_pSameLibraryPrev;
        // and call Find function for begin of list
        // since any result of this call is valid, simply return it
        return pSameLib->FindInterface(sName, pStart);
    }

    // should not be reached
    return 0;
}

/** for debugging purposes only */
void CFELibrary::Dump()
{
    printf("Dump: CFELibrary (%s)\n", GetName().c_str());
    printf("Dump: CFELibrary (%s): typedefs\n", GetName().c_str());
    vector<CFETypedDeclarator*>::iterator iterT = GetFirstTypedef();
    CFEBase *pElement;
    while ((pElement = GetNextTypedef(iterT)) != 0)
    {
        pElement->Dump();
    }
    printf("Dump: CFELibrary (%s): constants\n", GetName().c_str());
    vector<CFEConstDeclarator*>::iterator iterC = GetFirstConstant();
    while ((pElement = GetNextConstant(iterC)) != 0)
    {
        pElement->Dump();
    }
    printf("Dump: CFELibrary (%s): nested libraries\n", GetName().c_str());
    vector<CFELibrary*>::iterator iterL = GetFirstLibrary();
    while ((pElement = GetNextLibrary(iterL)) != 0)
    {
        pElement->Dump();
    }
    printf("Dump: CFELibrary (%s): interfaces\n", GetName().c_str());
    vector<CFEInterface*>::iterator iterI = GetFirstInterface();
    while ((pElement = GetNextInterface(iterI)) != 0)
    {
        pElement->Dump();
    }
}

/**    \brief serialize this object
 *    \param pFile the file to write to/read from
 */
void CFELibrary::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        pFile->PrintIndent("<library>\n");
        pFile->IncIndent();
        pFile->PrintIndent("<name>%s</name>\n", m_sLibName.c_str());
        // print the library's attributes
        vector<CFEAttribute*>::iterator iterA = GetFirstAttribute();
        CFEBase *pElement;
        while ((pElement = GetNextAttribute(iterA)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // print the library's libraries
        vector<CFELibrary*>::iterator iterL = GetFirstLibrary();
        while ((pElement = GetNextLibrary(iterL)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // print the library's constants
        vector<CFEConstDeclarator*>::iterator iterC = GetFirstConstant();
        while ((pElement = GetNextConstant(iterC)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // print the library's typedefs
        vector<CFETypedDeclarator*>::iterator iterT = GetFirstTypedef();
        while ((pElement = GetNextTypedef(iterT)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // print the library's interfaces
        vector<CFEInterface*>::iterator iterI = GetFirstInterface();
        while ((pElement = GetNextInterface(iterI)) != 0)
        {
            pElement->Serialize(pFile);
        }
        pFile->DecIndent();
        pFile->PrintIndent("</library>\n");
    }
}

/**    \brief adds a library with the same name to the same lib vector
 *    \param pLibrary the library to add
 *
 * Same libraries have the same name (and name hierarchy) but reside in different
 * files. Because we cannot simply add the members of one lib to another lib (they
 * would belong to another file) we have to do it this way.
 */
void CFELibrary::AddSameLibrary(CFELibrary * pLibrary)
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

/** \brief adds a new interface to the library
 *  \param pFEInterface the new interface to add
 */
void CFELibrary::AddInterface(CFEInterface *pFEInterface)
{
    if (!pFEInterface)
        return;
    m_vInterfaces.push_back(pFEInterface);
    pFEInterface->SetParent(this);
}

/** \brief adds a new type definition to the library
 *  \param pFETypedef the new type defintion
 */
void CFELibrary::AddTypedef(CFETypedDeclarator *pFETypedef)
{
    if (!pFETypedef)
        return;
    m_vTypedefs.push_back(pFETypedef);
    pFETypedef->SetParent(this);
}

/** \brief adds a new constant to the library
 *  \param pFEConstant the constant to add
 */
void CFELibrary::AddConstant(CFEConstDeclarator *pFEConstant)
{
    if (!pFEConstant)
        return;
    m_vConstants.push_back(pFEConstant);
    pFEConstant->SetParent(this);
}

/** \brief adds a tagged constructed type to the library
 *  \param pFETaggedDecl the new tagged constructed type
 */
void CFELibrary::AddTaggedDecl(CFEConstructedType *pFETaggedDecl)
{
    if (!pFETaggedDecl)
        return;
    m_vTaggedDeclarators.push_back(pFETaggedDecl);
    pFETaggedDecl->SetParent(this);
}

/** \brief adds a nested library
 *  \param pFELibrary the new nested library to add
 */
void CFELibrary::AddLibrary(CFELibrary *pFELibrary)
{
    if (!pFELibrary)
        return;
    m_vLibraries.push_back(pFELibrary);
    pFELibrary->SetParent(this);
}

/** \brief retrieve a pointer to the first tagged type declaration
 *  \return a pointer to the first tagged type decl
 */
vector<CFEConstructedType*>::iterator CFELibrary::GetFirstTaggedDecl()
{
    return m_vTaggedDeclarators.begin();
}

/** \brief retrieve a reference to the next tagged decl
 *  \param iter the pointer to the next tagged decl
 *  \return a reference to the next tagged decl
 */
CFEConstructedType* CFELibrary::GetNextTaggedDecl(vector<CFEConstructedType*>::iterator &iter)
{
    if (iter == m_vTaggedDeclarators.end())
        return 0;
    return *iter++;
}

/** \brief search for a tagged declarator
 *  \param sName the tag (name) of the tagged decl
 *  \return a reference to the found tagged decl or NULL if none found
 */
CFEConstructedType* CFELibrary::FindTaggedDecl(string sName)
{
    // own tagged decls
    vector<CFEConstructedType*>::iterator iterCT = GetFirstTaggedDecl();
    CFEConstructedType* pTaggedDecl;
    while ((pTaggedDecl = GetNextTaggedDecl(iterCT)) != 0)
    {
        if (dynamic_cast<CFETaggedStructType*>(pTaggedDecl))
            if (((CFETaggedStructType*)pTaggedDecl)->GetTag() == sName)
                return pTaggedDecl;
        if (dynamic_cast<CFETaggedUnionType*>(pTaggedDecl))
            if (((CFETaggedUnionType*)pTaggedDecl)->GetTag() == sName)
                return pTaggedDecl;
        if (dynamic_cast<CFETaggedEnumType*>(pTaggedDecl))
            if (((CFETaggedEnumType*)pTaggedDecl)->GetTag() == sName)
                return pTaggedDecl;
    }
    // search interfaces
    vector<CFEInterface*>::iterator iterI = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(iterI)) != 0)
    {
        if ((pTaggedDecl = pInterface->FindTaggedDecl(sName)) != 0)
            return pTaggedDecl;
    }
    // search nested libs
    vector<CFELibrary*>::iterator iterL = GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = GetNextLibrary(iterL)) != 0)
    {
        if ((pTaggedDecl = pLibrary->FindTaggedDecl(sName)) != 0)
            return pTaggedDecl;
    }
    // nothing found:
    return 0;
}
