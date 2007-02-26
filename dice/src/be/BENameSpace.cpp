/**
 *	\file	dice/src/be/BENameSpace.cpp
 *	\brief	contains the implementation of the class CBENameSpace
 *
 *	\date	Tue Jun 25 2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

#include "be/BENameSpace.h"
#include "be/BEContext.h"
#include "be/BEClass.h"
#include "be/BENameSpace.h"
#include "be/BEConstant.h"
#include "be/BEType.h"
#include "be/BETypedef.h"
#include "be/BEAttribute.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEEnumType.h"
#include "be/BERoot.h"

#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FEConstructedType.h"

IMPLEMENT_DYNAMIC(CBENameSpace);

CBENameSpace::CBENameSpace()
: m_vClasses(RUNTIME_CLASS(CBEClass)),
  m_vNestedNamespaces(RUNTIME_CLASS(CBENameSpace)),
  m_vConstants(RUNTIME_CLASS(CBEConstant)),
  m_vTypedefs(RUNTIME_CLASS(CBETypedef)),
  m_vTypeDeclarations(RUNTIME_CLASS(CBEType)),
  m_vAttributes(RUNTIME_CLASS(CBEAttribute))
{
    IMPLEMENT_DYNAMIC_BASE(CBENameSpace, CBEObject);
}

/** destructs the class class */
CBENameSpace::~CBENameSpace()
{
    m_vClasses.DeleteAll();
    m_vNestedNamespaces.DeleteAll();
    m_vConstants.DeleteAll();
    m_vTypedefs.DeleteAll();
    m_vTypeDeclarations.DeleteAll();
    m_vAttributes.DeleteAll();
}


/** \brief adds an Class to the internal collextion
 *  \param pClass the Class to add
 */
void CBENameSpace::AddClass(CBEClass *pClass)
{
    m_vClasses.Add(pClass);
    pClass->SetParent(this);
}

/** \brief removes an Class from the collection
 *  \param pClass the Class to remove
 */
void CBENameSpace::RemoveClass(CBEClass *pClass)
{
    m_vClasses.Remove(pClass);
}

/** \brief retrieves a pointer to the first Class
 *  \return a pointer to the first Class
 */
VectorElement* CBENameSpace::GetFirstClass()
{
    return m_vClasses.GetFirst();
}

/** \brief retrieves the next Class
 *  \param pIter the pointer to the next Class
 *  \return a reference to the Class or 0 if end of collection
 */
CBEClass* CBENameSpace::GetNextClass(VectorElement* &pIter)
{
    if (!pIter)
        return 0;
    CBEClass *pRet = (CBEClass*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextClass(pIter);
    return pRet;
}

/** \brief searches for a specific Class
 *  \param sClassName the name of the Class
 *  \return a reference to the Class or 0 if not found
 */
CBEClass* CBENameSpace::FindClass(String sClassName)
{
    // first search own Classes
    VectorElement *pIter = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        if (pClass->GetName() == sClassName)
            return pClass;
    }
    // then check nested libs
    pIter = GetFirstNameSpace();
    CBENameSpace *pNestedNameSpace;
    while ((pNestedNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        if ((pClass = pNestedNameSpace->FindClass(sClassName)) != 0)
            return pClass;
    }
    return 0 ;
}

/** \brief adds a nested NameSpace to the internal collection
 *  \param pNameSpace the NameSpace to add
 */
void CBENameSpace::AddNameSpace(CBENameSpace* pNameSpace)
{
    m_vNestedNamespaces.Add(pNameSpace);
    pNameSpace->SetParent(this);
}

/** \brief removes a nested NameSpace from the internal collection
 *  \param pNameSpace the NameSpace to remove
 */
void CBENameSpace::RemoveNameSpace(CBENameSpace *pNameSpace)
{
    m_vNestedNamespaces.Remove(pNameSpace);
}

/** \brief retrieves a pointer to the first NameSpace
 *  \return a pointer to the first NameSpace
 */
VectorElement* CBENameSpace::GetFirstNameSpace()
{
    return m_vNestedNamespaces.GetFirst();
}

/** \brief retrieves a reference to the next nested NameSpace
 *  \param pIter the pointer to the next NameSpace
 *  \return a reference to the next NameSpace or 0 if no more libraries
 */
CBENameSpace* CBENameSpace::GetNextNameSpace(VectorElement* &pIter)
{
    if (!pIter)
        return 0;
    CBENameSpace *pRet = (CBENameSpace*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextNameSpace(pIter);
    return pRet;
}

/** \brief tries to find a NameSpace
 *  \param sNameSpaceName the name of the NameSpace
 *  \return a reference to the NameSpace
 */
CBENameSpace* CBENameSpace::FindNameSpace(String sNameSpaceName)
{
    // then check nested libs
    VectorElement *pIter = GetFirstNameSpace();
    CBENameSpace *pNestedNameSpace, *pFoundNameSpace;
    while ((pNestedNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        if (pNestedNameSpace->GetName() == sNameSpaceName)
            return pNestedNameSpace;
        if ((pFoundNameSpace = pNestedNameSpace->FindNameSpace(sNameSpaceName)) != 0)
            return pFoundNameSpace;
    }
    return 0;
}

/** \brief retrieves the name of the NameSpace
 *  \return the name of the lib
 */
String CBENameSpace::GetName()
{
    return m_sName;
}

/** \brief creates the back-end NameSpace
 *  \param pFELibrary the front-end library to use as reference
 *  \param pContext the context of the creation
 *  \return true if successful
 *
 * This function should be callable multiple types, because the library can be
 * defined mulitple times including different interfaces and such.
 */
bool CBENameSpace::CreateBackEnd(CFELibrary *pFELibrary, CBEContext *pContext)
{
    if (!pFELibrary)
    {
        VERBOSE("CBENameSpace::CreateBackEnd failed because library is 0\n");
        return false;
    }
    // set target file name
    SetTargetFileName(pFELibrary, pContext);
    // set own name
    m_sName = pFELibrary->GetName();
    // search for attributes
    VectorElement *pIter = pFELibrary->GetFirstAttribute();
    CFEAttribute *pFEAttribute;
    while ((pFEAttribute = pFELibrary->GetNextAttribute(pIter)) != 0)
    {
        if (!CreateBackEnd(pFEAttribute, pContext))
            return false;
    }
    // search for constants
    pIter = pFELibrary->GetFirstConstant();
    CFEConstDeclarator *pFEConstant;
    while ((pFEConstant = pFELibrary->GetNextConstant(pIter)) != 0)
    {
        if (!CreateBackEnd(pFEConstant, pContext))
            return false;
    }
    // search for types
    pIter = pFELibrary->GetFirstTypedef();
    CFETypedDeclarator *pFETypedef;
    while ((pFETypedef = pFELibrary->GetNextTypedef(pIter)) != 0)
    {
        if (!CreateBackEnd(pFETypedef, pContext))
            return false;
    }
    // search for type declarations
    pIter = pFELibrary->GetFirstTaggedDecl();
    CFEConstructedType *pFETaggedType;
    while ((pFETaggedType = pFELibrary->GetNextTaggedDecl(pIter)) != 0)
    {
        if (!CreateBackEnd(pFETaggedType, pContext))
            return false;
    }
    // search for interfaces
    // there might be some classes, which have base-classes defined in the libs,
    // which come afterwards.
    pIter = pFELibrary->GetFirstInterface();
    CFEInterface *pFEInterface;
    while ((pFEInterface = pFELibrary->GetNextInterface(pIter)) != 0)
    {
        if (!CreateBackEnd(pFEInterface, pContext))
            return false;
    }
    // search for libraries
    pIter = pFELibrary->GetFirstLibrary();
    CFELibrary *pNestedLib;
    while ((pNestedLib = pFELibrary->GetNextLibrary(pIter)) != 0)
    {
        CBENameSpace *pNameSpace = FindNameSpace(pNestedLib->GetName());
        if (!pNameSpace)
        {
            pNameSpace = pContext->GetClassFactory()->GetNewNameSpace();
            AddNameSpace(pNameSpace);
            if (!pNameSpace->CreateBackEnd(pNestedLib, pContext))
            {
                RemoveNameSpace(pNameSpace);
                VERBOSE("CBENameSpace::CreateBE failed because NameSpace %s could not be created\n",
                        (const char*)(pNestedLib->GetName()));
                return false;
            }
        }
        else
        {
            // call create function again to add the interface of the redefined nested lib
            if (!pNameSpace->CreateBackEnd(pNestedLib, pContext))
            {
                VERBOSE("CBENameSpace::CreateBE failed because NameSpace %s could not be re-created in scope of %s\n",
                        (const char*)(pNestedLib->GetName()), (const char*)m_sName);
                return false;
            }
        }
    }

    return true;
}

/** \brief internal function to create a backend of this NameSpace
 *  \param pFEInterface the respective front-end interface
 *  \param pContext the context of the creation
 *  \return true if successful
 */
bool CBENameSpace::CreateBackEnd(CFEInterface *pFEInterface, CBEContext *pContext)
{
    CBEClass *pClass = NULL;
    // check if class already exists
    CBERoot *pRoot = GetRoot();
    ASSERT(pRoot);
    pClass = pRoot->FindClass(pFEInterface->GetName());
    if (!pClass)
    {
        pClass = pContext->GetClassFactory()->GetNewClass();
        AddClass(pClass);
        // recreate class to add additional members
        if (!pClass->CreateBackEnd(pFEInterface, pContext))
        {
            RemoveClass(pClass);
            delete pClass;
            VERBOSE("CBENameSpace::CreateBE failed because class %s could not be created\n",
                (const char*)(pFEInterface->GetName()));
            return false;
        }
    }
    // otherwise: it was the base class of some class and has been created by it directly
    return true;
}

/** \brief internal function to create constants
 *  \param pFEConstant the respective front-end constant
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CBENameSpace::CreateBackEnd(CFEConstDeclarator *pFEConstant, CBEContext *pContext)
{
    CBEConstant *pConstant = pContext->GetClassFactory()->GetNewConstant();
    AddConstant(pConstant);
    if (!pConstant->CreateBackEnd(pFEConstant, pContext))
    {
        RemoveConstant(pConstant);
        delete pConstant;
        VERBOSE("CBENameSpace::CreateBE failed because constant %s could not be created\n",
                (const char*)pFEConstant->GetName());
        return false;
    }
    return true;
}

/** \brief internal function to create typedef
 *  \param pFETypedef the respective front-end typedefinition
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CBENameSpace::CreateBackEnd(CFETypedDeclarator *pFETypedef, CBEContext *pContext)
{
    CBETypedef *pTypedef = pContext->GetClassFactory()->GetNewTypedef();
    AddTypedef(pTypedef);
    pTypedef->SetParent(this);
    if (!pTypedef->CreateBackEnd(pFETypedef, pContext))
    {
        RemoveTypedef(pTypedef);
        delete pTypedef;
        VERBOSE("CBENameSpace::CreateBE failed because typedef could not be created\n");
        return false;
    }
    return true;
}

/** \brief internale function to add an attribute
 *  \param pFEAttribute the respective front.end attribute
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CBENameSpace::CreateBackEnd(CFEAttribute *pFEAttribute, CBEContext *pContext)
{
    CBEAttribute *pAttribute = pContext->GetClassFactory()->GetNewAttribute();
    AddAttribute(pAttribute);
    if (!pAttribute->CreateBackEnd(pFEAttribute, pContext))
    {
        RemoveAttribute(pAttribute);
        delete pAttribute;
        VERBOSE("CBENameSpace::CreateBackEnd failed because attribute could not be created\n");
        return false;
    }
    return true;
}

/** \brief adds a constant
 *  \param pConstant the const to add
 */
void CBENameSpace::AddConstant(CBEConstant *pConstant)
{
    m_vConstants.Add(pConstant);
    pConstant->SetParent(this);
}

/** \brief removes a constant
 *  \param pConstant the const to remove
 */
void CBENameSpace::RemoveConstant(CBEConstant *pConstant)
{
    m_vConstants.Remove(pConstant);
}

/** \brief retrieves a pointer to the first constant
 *  \return a pointer to the first constant
 */
VectorElement* CBENameSpace::GetFirstConstant()
{
    return m_vConstants.GetFirst();
}

/** \brief retrieves a reference to the next constant
 *  \param pIter the pointer to the next const
 *  \return a reference to the next const
 */
CBEConstant* CBENameSpace::GetNextConstant(VectorElement *&pIter)
{
    if (!pIter)
        return 0;
    CBEConstant *pRet = (CBEConstant*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextConstant(pIter);
    return pRet;
}

/** \brief adds a typedef
 *  \param pTypedef the typedef to add
 */
void CBENameSpace::AddTypedef(CBETypedef *pTypedef)
{
    m_vTypedefs.Add(pTypedef);
    pTypedef->SetParent(this);
}

/** \brief removes a typedef
 *  \param pTypedef the typedef to remove
 */
void CBENameSpace::RemoveTypedef(CBETypedef *pTypedef)
{
    m_vTypedefs.Remove(pTypedef);
}

/** \brief retrieves a pointer to the first typedef
 *  \return a pointer to the first typedef
 */
VectorElement* CBENameSpace::GetFirstTypedef()
{
    return m_vTypedefs.GetFirst();
}

/** \brief retrieves a reference to the next typedef
 *  \param pIter the pointer to the next typedef
 *  \return a reference to the typedef
 */
CBETypedef* CBENameSpace::GetNextTypedef(VectorElement *&pIter)
{
    if (!pIter)
        return 0;
    CBETypedef *pRet = (CBETypedef*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextTypedef(pIter);
    return pRet;
}

/** \brief adds an attribute
 *  \param pAttribute the attribute to add
 */
void CBENameSpace::AddAttribute(CBEAttribute *pAttribute)
{
    m_vAttributes.Add(pAttribute);
    pAttribute->SetParent(this);
}

/** \brief remove an attribute
 *  \param pAttribute the attribute to remove
 */
void CBENameSpace::RemoveAttribute(CBEAttribute *pAttribute)
{
    m_vAttributes.Remove(pAttribute);
}

/** \brief retrieves the pointer to the first attribute
 *  \return a pointer to the first attribute
 */
VectorElement* CBENameSpace::GetFirstAttribute()
{
    return m_vAttributes.GetFirst();
}

/** \brief retrieve a reference to the next attribute
 *  \param pIter the pointer to the next attribute
 *  \return a reference to the attribute
 */
CBEAttribute* CBENameSpace::GetNextAttribute(VectorElement *&pIter)
{
    if (!pIter)
        return 0;
    CBEAttribute *pRet = (CBEAttribute*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextAttribute(pIter);
    return pRet;
}

/** \brief adds this NameSpace to the target file
 *  \param pHeader the header file to add to
 *  \param pContext the context of the operation
 *  \return true if successful
 *
 * This implementation adds its types, constants, interfaces and
 * nested libs.
 */
bool CBENameSpace::AddToFile(CBEHeaderFile *pHeader, CBEContext *pContext)
{
    // add this namespace to the file
    if (IsTargetFile(pHeader))
		pHeader->AddNameSpace(this);
    return true;
}

/** \brief adds this namespace to the file
 *  \param pImpl the implementation file to add this namespace to
 *  \param pContext the context of the creation
 *  \return true if successful
 */
bool CBENameSpace::AddToFile(CBEImplementationFile *pImpl, CBEContext *pContext)
{
	// if compiler options for interface or function target file are set
	// iterate over interfaces and add them
	if (pContext->IsOptionSet(PROGRAM_FILE_FUNCTION) ||
		pContext->IsOptionSet(PROGRAM_FILE_INTERFACE))
	{
		VectorElement *pIter = GetFirstClass();
		CBEClass *pClass;
		while ((pClass = GetNextClass(pIter)) != 0)
		{
			pClass->AddToFile(pImpl, pContext);
		}
	}
    // add this namespace to the file
	if (IsTargetFile(pImpl))
		pImpl->AddNameSpace(this);
    return true;
}

/** \brief adds the opcodes for this namespace to the file
 *  \param pFile the file to add the opcodes to
 *  \param pContext the context of the whole create process
 *  \return true if successful
 *
 * This implements the opcodes of the included classes and nested namespaces.
 */
bool CBENameSpace::AddOpcodesToFile(CBEHeaderFile *pFile, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        if (!pClass->AddOpcodesToFile(pFile, pContext))
            return false;
    }

    pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        if (!pNameSpace->AddOpcodesToFile(pFile, pContext))
            return false;
    }

    return true;
}

/** \brief tries to optimize the namespace
 *  \param nLevel the optimization level
 *  \param pContext the context of the optimization
 *  \return 0 on success or error code on failure
 */
int CBENameSpace::Optimize(int nLevel, CBEContext *pContext)
{
    // iterate over classes
    VectorElement *pIter = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        int nRet;
        if ((nRet = pClass->Optimize(nLevel, pContext)) != 0)
            return nRet;
    }

    pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        int nRet;
        if ((nRet = pNameSpace->Optimize(nLevel, pContext)) != 0)
            return nRet;
    }

    return 0;
}

/** \brief writes the name-space to the header file
 *  \param pFile the header file to write to
 *  \param pContext the context of the operation
 *
 * For the C implementation the name-space simply calls the embedded
 * constants, types, classes, and namespaces.
 */
void CBENameSpace::Write(CBEHeaderFile *pFile, CBEContext *pContext)
{
    WriteConstants(pFile, pContext);
    WriteTypedefs(pFile, pContext);
    WriteTaggedTypes(pFile, pContext);
    WriteClasses(pFile, pContext);
    WriteNameSpaces(pFile, pContext);
}

/** \brief writes the name-space to the header file
 *  \param pFile the header file to write to
 *  \param pContext the context of the operation
 *
 * For the C implementation the name-space simply calls the embedded
 * classes and namespaces.
 */
void CBENameSpace::Write(CBEImplementationFile *pFile, CBEContext *pContext)
{
    WriteClasses(pFile, pContext);
    WriteNameSpaces(pFile, pContext);
}

/** \brief writes the constants to the header file
 *  \param pFile the header file to write to
 *  \param pContext the context of the write operation
 */
void CBENameSpace::WriteConstants(CBEHeaderFile *pFile, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstConstant();
    CBEConstant *pConstant;
    while ((pConstant = GetNextConstant(pIter)) != 0)
    {
        pConstant->Write(pFile, pContext);
    }
}

/** \brief writes the typedefs to the header file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBENameSpace::WriteTypedefs(CBEHeaderFile *pFile, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstTypedef();
    CBETypedef *pTypedef;
    while ((pTypedef = GetNextTypedef(pIter)) != 0)
    {
        pTypedef->WriteDeclaration(pFile, pContext);
    }
}

/** \brief writes the classes to the header file
 *  \param pFile the file to write to
 *  \param pContext the context the context of the write operation
 */
void CBENameSpace::WriteClasses(CBEHeaderFile *pFile, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        pClass->Write(pFile, pContext);
    }
}

/** \brief writes the classes to the implementation file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBENameSpace::WriteClasses(CBEImplementationFile *pFile, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        pClass->Write(pFile, pContext);
    }
}

/** \brief writes the nested name-spaces to the header file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBENameSpace::WriteNameSpaces(CBEHeaderFile *pFile, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        pNameSpace->Write(pFile, pContext);
    }
}

/** \brief write the nested name-spaces to the implementation file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBENameSpace::WriteNameSpaces(CBEImplementationFile *pFile, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        pNameSpace->Write(pFile, pContext);
    }
}

/** \brief tries to find a function
 *  \param sFunctionName the name of the funtion to find
 *  \return a reference to the found function (or 0 if not found)
 *
 * Because the namespace does not have functions itself, it searches its classes and
 * netsed namespaces.
 */
CBEFunction* CBENameSpace::FindFunction(String sFunctionName)
{
    CBEFunction *pFunction;

    VectorElement *pIter = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        if ((pFunction = pClass->FindFunction(sFunctionName)) != 0)
            return pFunction;
    }

    pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        if ((pFunction = pNameSpace->FindFunction(sFunctionName)) != 0)
            return  pFunction;
    }

    return 0;
}

/** \brief tries to find a type definition
 *  \param sTypeName the name of the searched typedef
 *  \return a reference to the found type definition
 */
CBETypedef* CBENameSpace::FindTypedef(String sTypeName)
{
    VectorElement *pIter = GetFirstTypedef();
    CBETypedef *pTypedef;
    while ((pTypedef = GetNextTypedef(pIter)) != 0)
    {
        if (pTypedef->FindDeclarator(sTypeName) != 0)
            return pTypedef;
    }

    pIter = GetFirstClass();
    CBEClass *pClass;
    CBETypedDeclarator *pTypedDecl;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        if ((pTypedDecl = pClass->FindTypedef(sTypeName)) != 0)
            if (pTypedDecl->IsKindOf(RUNTIME_CLASS(CBETypedef)))
                return (CBETypedef*)pTypedDecl;
    }

    pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        if ((pTypedef = pNameSpace->FindTypedef(sTypeName)) != 0)
            return pTypedef;
    }

    return 0;
}

/** \brief test if the given file is a target file for the namespace
 *  \param pFile the file to test
 *  \return true if name-space should be added to file
 *
 * A file is target file of a name-space if at least one of its classes or
 * nested name-spaces belongs to this class.
 */
bool CBENameSpace::IsTargetFile(CBEHeaderFile * pFile)
{
	VectorElement *pIter = GetFirstClass();
	CBEClass *pClass;
	while ((pClass = GetNextClass(pIter)) != 0)
	{
		if (pClass->IsTargetFile(pFile))
			return true;
	}
	pIter = GetFirstNameSpace();
	CBENameSpace *pNameSpace;
	while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
	{
		if (pNameSpace->IsTargetFile(pFile))
			return true;
	}
	return false;
}

/** \brief test if the given file is a target file for the namespace
 *  \param pFile the file to test
 *  \return true if name-space should be added to file
 *
 * A file is target file of a name-space if at least one of its classes or
 * nested name-spaces belongs to this class.
 */
bool CBENameSpace::IsTargetFile(CBEImplementationFile * pFile)
{
	VectorElement *pIter = GetFirstClass();
	CBEClass *pClass;
	while ((pClass = GetNextClass(pIter)) != 0)
	{
		if (pClass->IsTargetFile(pFile))
			return true;
	}
	pIter = GetFirstNameSpace();
	CBENameSpace *pNameSpace;
	while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
	{
		if (pNameSpace->IsTargetFile(pFile))
			return true;
	}
	return false;
}

/** \brief tries to find a type declaration of a tagged type
 *  \param nType the type (struct/enum/union) of the searched type
 *  \param sTag the tag of the type
 *  \return a reference to the found type or 0
 */
CBEType* CBENameSpace::FindTaggedType(int nType, String sTag)
{
    // search own types
    VectorElement *pIter = GetFirstTaggedType();
    CBEType *pTypeDecl;
    while ((pTypeDecl = GetNextTaggedType(pIter)) != 0)
    {
        int nFEType = pTypeDecl->GetFEType();
        if (nType != nFEType)
            continue;
        if (nFEType == TYPE_TAGGED_STRUCT)
        {
            if ((CBEStructType*)(pTypeDecl)->HasTag(sTag))
                return pTypeDecl;
        }
        if (nFEType == TYPE_TAGGED_UNION)
        {
            if ((CBEUnionType*)(pTypeDecl)->HasTag(sTag))
                return pTypeDecl;
        }
        if (nFEType == TYPE_TAGGED_ENUM)
        {
            if ((CBEEnumType*)(pTypeDecl)->HasTag(sTag))
                return pTypeDecl;
        }
    }
    // search classes
    pIter = GetFirstClass();
    CBEClass *pClass;
    CBEType *pType;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        if ((pType = pClass->FindTaggedType(nType, sTag)) != 0)
            return pType;
    }
    // search nested namespaces
    pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        if ((pType = pNameSpace->FindTaggedType(nType, sTag)) != 0)
            return pType;
    }
    return 0;
}

/** \brief adds a tagged type declaration
 *  \param pType the type to add
 */
void CBENameSpace::AddTaggedType(CBEType *pType)
{
    m_vTypeDeclarations.Add(pType);
}

/** \brief removes the tagged type declaration
 *  \param pType the type to remove
 */
void CBENameSpace::RemoveTaggedType(CBEType *pType)
{
    m_vTypeDeclarations.Remove(pType);
}

/** \brief tries to find the first type declaration
 *  \return a pointer to the first tagged type declaration
 */
VectorElement* CBENameSpace::GetFirstTaggedType()
{
    return m_vTypeDeclarations.GetFirst();
}

/** \brief retrieves a reference to the next tagged type declaration
 *  \param pIter the pointer to the next tagged type declaration
 *  \return a reference to the next tagged type or 0 if no more
 */
CBEType* CBENameSpace::GetNextTaggedType(VectorElement* &pIter)
{
    if (!pIter)
        return 0;
    CBEType *pRet = (CBEType*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextTaggedType(pIter);
    return pRet;
}

/** \brief tries to create the back-end presentation of a type declaration
 *  \param pFEType the respective front-end type
 *  \param pContext the context of the creat process
 *  \return true if successful
 */
bool CBENameSpace::CreateBackEnd(CFEConstructedType *pFEType, CBEContext *pContext)
{
    CBEType *pType = pContext->GetClassFactory()->GetNewType(pFEType->GetType());
    AddTaggedType(pType);
    pType->SetParent(this);
    if (!pType->CreateBackEnd(pFEType, pContext))
    {
        RemoveTaggedType(pType);
        delete pType;
        VERBOSE("CBENameSpace::CreateBackEnd failed because tagged type could not be created\n");
        return false;
    }
    return true;
}

/** \brief writes the tagged type declarations
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBENameSpace::WriteTaggedTypes(CBEHeaderFile *pFile, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstTaggedType();
    CBEType *pType;
    while ((pType = GetNextTaggedType(pIter)) != 0)
    {
        pType->Write(pFile, pContext);
        pFile->Print(";\n\n");
    }
}

/** \brief search for a function with a specific type
 *  \param sTypeName the name of the type
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if a parameter of that type is found
 *
 * search own classe, namespaces and function for a function, which has
 * a parameter of that type
 */
bool CBENameSpace::HasFunctionWithUserType(String sTypeName, CBEFile *pFile, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        if (pNameSpace->HasFunctionWithUserType(sTypeName, pFile, pContext))
            return true;
    }
    pIter = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        if (pClass->HasFunctionWithUserType(sTypeName, pFile, pContext))
            return true;
    }
    return false;
}
