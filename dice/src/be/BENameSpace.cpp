/**
 *  \file    dice/src/be/BENameSpace.cpp
 *  \brief   contains the implementation of the class CBENameSpace
 *
 *  \date    Tue Jun 25 2002
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

#include "BENameSpace.h"
#include "BEContext.h"
#include "BEClass.h"
#include "BENameSpace.h"
#include "BEConstant.h"
#include "BEType.h"
#include "BETypedef.h"
#include "BEAttribute.h"
#include "BEHeaderFile.h"
#include "BEImplementationFile.h"
#include "BERoot.h"
#include "BEStructType.h"
#include "BEUnionType.h"
#include "BEContext.h"
#include "Compiler.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FEConstructedType.h"
#include "fe/FEIdentifier.h"
#include <cassert>

CBENameSpace::CBENameSpace()
: m_Constants(0, this),
  m_Typedefs(0, this),
  m_Attributes(0, this),
  m_Classes(0, this),
  m_NestedNamespaces(0, this),
  m_TypeDeclarations(0, this)
{
}

/** \brief searches for a specific Class
 *  \param sClassName the name of the Class
 *  \return a reference to the Class or 0 if not found
 */
CBEClass* CBENameSpace::FindClass(string sClassName)
{
    // first search own Classes
    CBEClass *pClass = m_Classes.Find(sClassName);
    if (pClass)
	return pClass;
    // then check nested libs
    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_NestedNamespaces.begin();
	 iterN != m_NestedNamespaces.end();
	 iterN++)
    {
        if ((pClass = (*iterN)->FindClass(sClassName)) != 0)
            return pClass;
    }
    return 0 ;
}

/** \brief tries to find a NameSpace
 *  \param sNameSpaceName the name of the NameSpace
 *  \return a reference to the NameSpace
 */
CBENameSpace* CBENameSpace::FindNameSpace(string sNameSpaceName)
{
    // then check nested libs
    vector<CBENameSpace*>::iterator iter;
    for (iter = m_NestedNamespaces.begin();
	 iter != m_NestedNamespaces.end();
	 iter++)
    {
	CBENameSpace *pFoundNameSpace;
        if ((*iter)->GetName() == sNameSpaceName)
            return *iter;
        if ((pFoundNameSpace = (*iter)->FindNameSpace(sNameSpaceName)) != 0)
            return pFoundNameSpace;
    }
    return 0;
}

/** \brief creates the back-end NameSpace
 *  \param pFELibrary the front-end library to use as reference
 *  \return true if successful
 *
 * This function should be callable multiple types, because the library can be
 * defined mulitple times including different interfaces and such.
 */
void CBENameSpace::CreateBackEnd(CFELibrary *pFELibrary)
{
    assert(pFELibrary);

    string exc = string(__func__);
    // set target file name
    SetTargetFileName(pFELibrary);
    // call base class to set source file information
    CBEObject::CreateBackEnd(pFELibrary);
    // set own name
    m_sName = pFELibrary->GetName();

    // search for attributes
    vector<CFEAttribute*>::iterator iterA;
    for (iterA = pFELibrary->m_Attributes.begin();
	 iterA != pFELibrary->m_Attributes.end();
	 iterA++)
    {
        CreateBackEnd(*iterA);
    }
    // search for constants
    vector<CFEConstDeclarator*>::iterator iterC;
    for (iterC = pFELibrary->m_Constants.begin();
	 iterC != pFELibrary->m_Constants.end();
	 iterC++)
    {
        CreateBackEnd(*iterC);
    }
    // search for types
    vector<CFETypedDeclarator*>::iterator iterT;
    for (iterT = pFELibrary->m_Typedefs.begin();
	 iterT != pFELibrary->m_Typedefs.end();
	 iterT++)
    {
        CreateBackEnd(*iterT);
    }
    // search for type declarations
    vector<CFEConstructedType*>::iterator iterCT;
    for (iterCT = pFELibrary->m_TaggedDeclarators.begin();
	 iterCT != pFELibrary->m_TaggedDeclarators.end();
	 iterCT++)
    {
	CreateBackEnd(*iterCT);
    }
    // search for libraries, which contains base interfaces of our own
    // interfaces we therefore iterate over the interfaces and try to find
    // their base interfaces in any nested library we have to remember those
    // libraries, so we won't initialize them twice
    vector<CFELibrary*> vLibraries;
    vector<CFEInterface*>::iterator iterI;
    for (iterI = pFELibrary->m_Interfaces.begin();
	 iterI != pFELibrary->m_Interfaces.end();
	 iterI++)
    {
	CFELibrary *pNestedLib;
        // get base interface names
        vector<CFEIdentifier*>::iterator iterBIN;
	for (iterBIN = (*iterI)->m_BaseInterfaceNames.begin();
	     iterBIN != (*iterI)->m_BaseInterfaceNames.end();
	     iterBIN++)
        {
            // search for interface
            CFEInterface *pFEBaseInterface = 
		pFELibrary->FindInterface((*iterBIN)->GetName());
            if (pFEBaseInterface)
            {
                pNestedLib = pFEBaseInterface->GetSpecificParent<CFELibrary>();
                if (pNestedLib && (pNestedLib != pFELibrary))
                {
                    // init nested library
                    CBENameSpace *pNameSpace = 
			FindNameSpace(pNestedLib->GetName());
                    if (!pNameSpace)
                    {
                        pNameSpace =
			    CCompiler::GetClassFactory()->GetNewNameSpace();
                        m_NestedNamespaces.Add(pNameSpace);
			pNameSpace->CreateBackEnd(pNestedLib);
                    }
                    else
                    {
			// call create function again to add the interface of
			// the redefined nested lib
			pNameSpace->CreateBackEnd(pNestedLib);
                    }
                    vLibraries.push_back(pNestedLib);
                }
            }
        }
    }
    // search for interfaces
    // there might be some classes, which have base-classes defined in the
    // libs, which come afterwards.
    for (iterI = pFELibrary->m_Interfaces.begin();
	 iterI != pFELibrary->m_Interfaces.end();
	 iterI++)
    {
        CreateBackEnd(*iterI);
    }
    // search for libraries
    vector<CFELibrary*>::iterator iterL;
    for (iterL = pFELibrary->m_Libraries.begin();
	 iterL != pFELibrary->m_Libraries.end();
	 iterL++)
    {
        // if we initialized this lib already, skip it
	bool bSkip = false;
	vector<CFELibrary*>::iterator iter;
	for (iter = vLibraries.begin();
	    iter != vLibraries.end() && !bSkip;
	    iter++)
	{
	    assert(*iter);
	    if (*iter == *iterL)
		bSkip = true;
	}
	if (bSkip)
	    continue;

	CBENameSpace *pNameSpace = FindNameSpace((*iterL)->GetName());
	if (!pNameSpace)
        {
            pNameSpace = CCompiler::GetClassFactory()->GetNewNameSpace();
            m_NestedNamespaces.Add(pNameSpace);
	    pNameSpace->CreateBackEnd(*iterL);
        }
        else
        {
	    // call create function again to add the interface of the
	    // redefined nested lib
    	    pNameSpace->CreateBackEnd(*iterL);
        }
    }
}

/** \brief internal function to create a backend of this NameSpace
 *  \param pFEInterface the respective front-end interface
 *  \return true if successful
 */
void
CBENameSpace::CreateBackEnd(CFEInterface *pFEInterface)
{
    CBEClass *pClass = 0;
    // check if class already exists
    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    pClass = pRoot->FindClass(pFEInterface->GetName());
    if (!pClass)
    {
        pClass = CCompiler::GetClassFactory()->GetNewClass();
        m_Classes.Add(pClass);
        // recreate class to add additional members
	pClass->CreateBackEnd(pFEInterface);
    }
    // otherwise: it was the base class of some class and has been created by
    // it directly
}

/** \brief internal function to create constants
 *  \param pFEConstant the respective front-end constant
 *  \return true if successful
 */
void
CBENameSpace::CreateBackEnd(CFEConstDeclarator *pFEConstant)
{
    CBEConstant *pConstant = CCompiler::GetClassFactory()->GetNewConstant();
    m_Constants.Add(pConstant);
    pConstant->CreateBackEnd(pFEConstant);
}

/** \brief internal function to create typedef
 *  \param pFETypedef the respective front-end typedefinition
 *  \return true if successful
 */
void
CBENameSpace::CreateBackEnd(CFETypedDeclarator *pFETypedef)
{
    CBETypedef *pTypedef = CCompiler::GetClassFactory()->GetNewTypedef();
    m_Typedefs.Add(pTypedef);
    pTypedef->SetParent(this);
    pTypedef->CreateBackEnd(pFETypedef);
}

/** \brief internale function to add an attribute
 *  \param pFEAttribute the respective front.end attribute
 *  \return true if successful
 */
void
CBENameSpace::CreateBackEnd(CFEAttribute *pFEAttribute)
{
    CBEAttribute *pAttribute = CCompiler::GetClassFactory()->GetNewAttribute();
    m_Attributes.Add(pAttribute);
    pAttribute->CreateBackEnd(pFEAttribute);
}

/** \brief searches for a constants
 *  \param sConstantName the name of the constants
 *  \return a reference to the constant if found, zero otherwise
 */
CBEConstant* CBENameSpace::FindConstant(string sConstantName)
{
    if (sConstantName.empty())
        return 0;
    // simply scan the namespace for a match
    CBEConstant *pConstant = m_Constants.Find(sConstantName);
    if (pConstant)
	return pConstant;
    // check classes
    vector<CBEClass*>::iterator iterCl;
    for (iterCl = m_Classes.begin();
	 iterCl != m_Classes.end();
	 iterCl++)
    {
        if ((pConstant = (*iterCl)->m_Constants.Find(sConstantName)) != 0)
            return pConstant;
    }
    // check namespaces
    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_NestedNamespaces.begin();
	 iterN != m_NestedNamespaces.end();
	 iterN++)
    {
        if ((pConstant = (*iterN)->FindConstant(sConstantName)) != 0)
            return pConstant;
    }
    // nothing found
    return 0;
}

/** \brief adds this NameSpace to the target file
 *  \param pHeader the header file to add to
 *  \return true if successful
 *
 * This implementation adds its types, constants, interfaces and
 * nested libs.
 */
void CBENameSpace::AddToHeader(CBEHeaderFile* pHeader)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBENameSpace::%s(header: %s) for namespace %s called\n", __func__,
        pHeader->GetFileName().c_str(), GetName().c_str());
    // add this namespace to the file
    if (IsTargetFile(pHeader))
        pHeader->m_NameSpaces.Add(this);
}

/** \brief adds this namespace to the file
 *  \param pImpl the implementation file to add this namespace to
 *  \return true if successful
 */
void CBENameSpace::AddToImpl(CBEImplementationFile* pImpl)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBENameSpace::%s(impl: %s) for namespace %s called\n", __func__,
        pImpl->GetFileName().c_str(), GetName().c_str());
    // if compiler options for interface or function target file are set
    // iterate over interfaces and add them
    if (CCompiler::IsFileOptionSet(PROGRAM_FILE_FUNCTION) ||
        CCompiler::IsFileOptionSet(PROGRAM_FILE_INTERFACE))
    {
	for_each(m_Classes.begin(), m_Classes.end(),
	    std::bind2nd(std::mem_fun(&CBEClass::AddToImpl), pImpl));
    }
    // add this namespace to the file
    // (needed for types, etc.)
    if (IsTargetFile(pImpl))
        pImpl->m_NameSpaces.Add(this);
}

/** \brief adds the opcodes for this namespace to the file
 *  \param pFile the file to add the opcodes to
 *  \return true if successful
 *
 * This implements the opcodes of the included classes and nested namespaces.
 */
bool CBENameSpace::AddOpcodesToFile(CBEHeaderFile* pFile)
{
    vector<CBEClass*>::iterator iterC;
    for (iterC = m_Classes.begin();
	 iterC != m_Classes.end();
	 iterC++)
    {
        if (!(*iterC)->AddOpcodesToFile(pFile))
            return false;
    }

    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_NestedNamespaces.begin();
	 iterN != m_NestedNamespaces.end();
	 iterN++)
    {
        if (!(*iterN)->AddOpcodesToFile(pFile))
            return false;
    }

    return true;
}

/** \brief writes the name-space to the header file
 *  \param pFile the header file to write to
 *
 * Before writing, we create an ordered list of contained element,
 * depending on their appearance in the source file. Then we iterate
 * over the generated list and write each element.
 */
void CBENameSpace::WriteElements(CBEHeaderFile& pFile)
{
    // create ordered list
    CreateOrderedElementList();

    // for C++ write namespace opening
    if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP) &&
	IsTargetFile(&pFile))
    {
	pFile << "\tnamespace " << GetName() << "\n";
	pFile << "\t{\n";
	++pFile;
    }

    // write target file
    vector<CObject*>::iterator iter = m_vOrderedElements.begin();
    int nLastType = 0, nCurrType = 0;
    for (; iter != m_vOrderedElements.end(); iter++)
    {
        nCurrType = 0;
        if (dynamic_cast<CBEClass*>(*iter))
            nCurrType = 1;
        else if (dynamic_cast<CBENameSpace*>(*iter))
            nCurrType = 2;
        else if (dynamic_cast<CBEConstant*>(*iter))
            nCurrType = 3;
        else if (dynamic_cast<CBETypedef*>(*iter))
            nCurrType = 4;
        else if (dynamic_cast<CBEType*>(*iter))
            nCurrType = 5;
        if (nCurrType != nLastType)
        {
            pFile << "\n";
            nLastType = nCurrType;
        }
        // add pre-processor directive to denote source line
        if (CCompiler::IsOptionSet(PROGRAM_GENERATE_LINE_DIRECTIVE) &&
            (nCurrType >= 1) && (nCurrType <= 5))
        {
            pFile << "# " << (*iter)->GetSourceLine() <<  " \"" <<
                (*iter)->GetSourceFileName() << "\"\n";
        }
        switch (nCurrType)
        {
        case 1:
            WriteClass((CBEClass*)(*iter), pFile);
            break;
        case 2:
            WriteNameSpace((CBENameSpace*)(*iter), pFile);
            break;
        case 3:
            WriteConstant((CBEConstant*)(*iter), pFile);
            break;
        case 4:
            WriteTypedef((CBETypedef*)(*iter), pFile);
            break;
        case 5:
            WriteTaggedType((CBEType*)(*iter), pFile);
            break;
        default:
            break;
        }
    }

    // close namespace for C++
    if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP) &&
	IsTargetFile(&pFile))
    {
	--pFile << "\t}\n";
    }
}

/** \brief writes the name-space to the header file
 *  \param pFile the header file to write to
 */
void CBENameSpace::WriteElements(CBEImplementationFile& pFile)
{
    // create ordered list
    CreateOrderedElementList();

    /** only write classes and interfaces if this is not
     * -fFfunction or -fFinterface (the classes have been added to the file
     * seperately, and are thus written by CBEFile::WriteClasses...
     */
    if (CCompiler::IsFileOptionSet(PROGRAM_FILE_FUNCTION) ||
        CCompiler::IsFileOptionSet(PROGRAM_FILE_INTERFACE))
        return;

    // for C++ write namespace opening
    if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP) &&
	IsTargetFile(&pFile))
    {
	pFile << "\tnamespace " << GetName() << "\n";
	pFile << "\t{\n";
	++pFile;
    }

    // write target file
    vector<CObject*>::iterator iter = m_vOrderedElements.begin();
    int nLastType = 0, nCurrType = 0;
    for (; iter != m_vOrderedElements.end(); iter++)
    {
        nCurrType = 0;
        if (dynamic_cast<CBEClass*>(*iter))
            nCurrType = 1;
        else if (dynamic_cast<CBENameSpace*>(*iter))
            nCurrType = 2;
        if ((nCurrType != nLastType) &&
            (nCurrType > 0))
        {
            pFile << "\n";
            nLastType = nCurrType;
        }
        // add pre-processor directive to denote source line
        if (CCompiler::IsOptionSet(PROGRAM_GENERATE_LINE_DIRECTIVE) &&
            (nCurrType >= 1) && (nCurrType <= 2))
        {
            pFile << "# " << (*iter)->GetSourceLine() <<  " \"" <<
                (*iter)->GetSourceFileName() << "\"\n";
        }
        switch (nCurrType)
        {
        case 1:
            WriteClass((CBEClass*)(*iter), pFile);
            break;
        case 2:
            WriteNameSpace((CBENameSpace*)(*iter), pFile);
            break;
        default:
            break;
        }
    }

    // close namespace for C++
    if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP) &&
	IsTargetFile(&pFile))
    {
	--pFile << "\t}\n";
    }
}

/** \brief write a constant
 *  \param pConstant the constant to write
 *  \param pFile the file to write to
 */
void
CBENameSpace::WriteConstant(CBEConstant *pConstant,
	CBEHeaderFile& pFile)
{
    assert(pConstant);
    pConstant->Write(pFile);
}

/** \brief write a type definition
 *  \param pTypedef the typedef to write
 *  \param pFile the file to write to
 */
void
CBENameSpace::WriteTypedef(CBETypedef *pTypedef,
	CBEHeaderFile& pFile)
{
    assert(pTypedef);
    pTypedef->WriteDeclaration(pFile);
}

/** \brief write a class
 *  \param pClass the class to write
 *  \param pFile the file to write to
 */
void CBENameSpace::WriteClass(CBEClass *pClass, CBEImplementationFile& pFile)
{
    assert(pClass);
    pClass->Write(pFile);
}

/** \brief write a class
 *  \param pClass the class to write
 *  \param pFile the file to write to
 */
void CBENameSpace::WriteClass(CBEClass *pClass, CBEHeaderFile& pFile)
{
    assert(pClass);
    pClass->Write(pFile);
}

/** \brief write a nested namespace
 *  \param pNameSpace the namespace to write
 *  \param pFile the file to write to
 */
void CBENameSpace::WriteNameSpace(CBENameSpace *pNameSpace, 
    CBEImplementationFile& pFile)
{
    assert(pNameSpace);
    pNameSpace->Write(pFile);
}

/** \brief write a nested namespace
 *  \param pNameSpace the namespace to write
 *  \param pFile the file to write to
 */
void CBENameSpace::WriteNameSpace(CBENameSpace *pNameSpace,
    CBEHeaderFile& pFile)
{
    assert(pNameSpace);
    pNameSpace->Write(pFile);
}

/** \brief write a tagged type declaration
 *  \param pType the type to write
 *  \param pFile the file to write to
 *
 * Writing a tagged type highly depends on language.
 */
void 
CBENameSpace::WriteTaggedType(CBEType *pType, 
    CBEHeaderFile& pFile)
{
    assert(pType);
    string sTag;
    if (dynamic_cast<CBEStructType*>(pType))
        sTag = ((CBEStructType*)pType)->GetTag();
    if (dynamic_cast<CBEUnionType*>(pType))
        sTag = ((CBEUnionType*)pType)->GetTag();
    sTag = CCompiler::GetNameFactory()->GetTypeDefine(sTag);
    pFile << "#ifndef " << sTag << "\n";
    pFile << "#define " << sTag << "\n";
    pType->Write(pFile);
    pFile << ";\n";
    pFile << "#endif /* !" << sTag << " */\n\n";
}

/** \brief tries to find a function
 *  \param sFunctionName the name of the function to find
 *  \param nFunctionType the type of the function to find
 *  \return a reference to the found function (or 0 if not found)
 *
 * Because the namespace does not have functions itself, it searches its
 * classes and netsed namespaces.
 */
CBEFunction* CBENameSpace::FindFunction(string sFunctionName,
    FUNCTION_TYPE nFunctionType)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s (sFunctionName: %s) called\n",
	__func__, sFunctionName.c_str());

    CBEFunction *pFunction;
    vector<CBEClass*>::iterator iterC;
    for (iterC = m_Classes.begin();
	 iterC != m_Classes.end();
	 iterC++)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s checking class %s\n", __func__,
            (*iterC)->GetName().c_str());
	if ((pFunction = (*iterC)->FindFunction(sFunctionName,
		    nFunctionType)) != 0)
	    return pFunction;
    }

    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_NestedNamespaces.begin();
	 iterN != m_NestedNamespaces.end();
	 iterN++)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s checking nested namespace %s\n",
	    __func__, (*iterN)->GetName().c_str());
        if ((pFunction = (*iterN)->FindFunction(sFunctionName,
		    nFunctionType)) != 0)
	    return  pFunction;
    }

    return 0;
}

/** \brief tries to find a type definition
 *  \param sTypeName the name of the searched typedef
 *  \return a reference to the found type definition
 */
CBETypedef* CBENameSpace::FindTypedef(string sTypeName)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBENameSpace::%s(%s) called\n", __func__,
	sTypeName.c_str());

    CBETypedef *pTypedef = m_Typedefs.Find(sTypeName);
    if (pTypedef)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	    "CBENameSpace::%s: typedef found in namespace, return %p\n", __func__,
	    pTypedef);
	return pTypedef;
    }

    CBETypedDeclarator *pTypedDecl;
    vector<CBEClass*>::iterator iterC;
    for (iterC = m_Classes.begin();
	 iterC != m_Classes.end();
	 iterC++)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "CBENameSpace::%s: checking class %s\n",
	    __func__, (*iterC)->GetName().c_str());
        if ((pTypedDecl = (*iterC)->FindTypedef(sTypeName)) != 0)
            if (dynamic_cast<CBETypedef*>(pTypedDecl))
                return (CBETypedef*)pTypedDecl;
    }

    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_NestedNamespaces.begin();
	 iterN != m_NestedNamespaces.end();
	 iterN++)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "CBENameSpace::%s: checking namespace %s\n",
	    __func__, (*iterN)->GetName().c_str());
        if ((pTypedef = (*iterN)->FindTypedef(sTypeName)) != 0)
            return pTypedef;
    }

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBENameSpace::%s: no typedef found, return 0\n",
	__func__);
    return 0;
}

/** \brief test if the given file is a target file for the namespace
 *  \param pFile the file to test
 *  \return true if name-space should be added to file
 *
 * A file is target file of a name-space if at least one of its classes or
 * nested name-spaces belongs to this class.
 */
bool CBENameSpace::IsTargetFile(CBEHeaderFile* pFile)
{
    vector<CBEClass*>::iterator iterC;
    for (iterC = m_Classes.begin();
	 iterC != m_Classes.end();
	 iterC++)
    {
        if ((*iterC)->IsTargetFile(pFile))
            return true;
    }

    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_NestedNamespaces.begin();
	 iterN != m_NestedNamespaces.end();
	 iterN++)
    {
        if ((*iterN)->IsTargetFile(pFile))
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
bool CBENameSpace::IsTargetFile(CBEImplementationFile* pFile)
{
    vector<CBEClass*>::iterator iterC;
    for (iterC = m_Classes.begin();
	 iterC != m_Classes.end();
	 iterC++)
    {
        if ((*iterC)->IsTargetFile(pFile))
            return true;
    }

    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_NestedNamespaces.begin();
	 iterN != m_NestedNamespaces.end();
	 iterN++)
    {
        if ((*iterN)->IsTargetFile(pFile))
            return true;
    }
    return false;
}

/** \brief tries to find a type declaration of a tagged type
 *  \param nType the type (struct/enum/union) of the searched type
 *  \param sTag the tag of the type
 *  \return a reference to the found type or 0
 */
CBEType* CBENameSpace::FindTaggedType(int nType, string sTag)
{
    // search own types
    vector<CBEType*>::iterator iterT;
    for (iterT = m_TypeDeclarations.begin();
	 iterT != m_TypeDeclarations.end();
	 iterT++)
    {
        int nFEType = (*iterT)->GetFEType();
        if (nType != nFEType)
            continue;
        if (nFEType == TYPE_STRUCT ||
	    nFEType == TYPE_UNION ||
	    nFEType == TYPE_ENUM)
        {
            if ((*iterT)->HasTag(sTag))
                return *iterT;
        }
    }
    // search classes
    CBEType *pType;
    vector<CBEClass*>::iterator iterC;
    for (iterC = m_Classes.begin();
	 iterC != m_Classes.end();
	 iterC++)
    {
        if ((pType = (*iterC)->FindTaggedType(nType, sTag)) != 0)
            return pType;
    }
    // search nested namespaces
    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_NestedNamespaces.begin();
	 iterN != m_NestedNamespaces.end();
	 iterN++)
    {
        if ((pType = (*iterN)->FindTaggedType(nType, sTag)) != 0)
            return pType;
    }
    return 0;
}

/** \brief tries to create the back-end presentation of a type declaration
 *  \param pFEType the respective front-end type
 *  \return true if successful
 */
void
CBENameSpace::CreateBackEnd(CFEConstructedType *pFEType)
{
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBEType *pType = pCF->GetNewType(pFEType->GetType());
    m_TypeDeclarations.Add(pType);
    pType->SetParent(this);
    pType->CreateBackEnd(pFEType);
}

/** \brief search for a function with a specific type
 *  \param sTypeName the name of the type
 *  \param pFile the file to write to
 *  \return true if a parameter of that type is found
 *
 * search own classe, namespaces and function for a function, which has
 * a parameter of that type
 */
bool CBENameSpace::HasFunctionWithUserType(string sTypeName, CBEFile* pFile)
{
    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_NestedNamespaces.begin();
	 iterN != m_NestedNamespaces.end();
	 iterN++)
    {
        if ((*iterN)->HasFunctionWithUserType(sTypeName, pFile))
            return true;
    }
    vector<CBEClass*>::iterator iterC;
    for (iterC = m_Classes.begin();
	 iterC != m_Classes.end();
	 iterC++)
    {
        if ((*iterC)->HasFunctionWithUserType(sTypeName, pFile))
            return true;
    }
    return false;
}

/** \brief creates a list of ordered elements
 *
 * This method iterates each member vector and inserts their
 * elements into the ordered element list using bubble sort.
 * Sort criteria is the source line number.
 */
void CBENameSpace::CreateOrderedElementList(void)
{
    // clear vector
    m_vOrderedElements.clear();
    // namespaces
    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_NestedNamespaces.begin();
	 iterN != m_NestedNamespaces.end();
	 iterN++)
    {
        InsertOrderedElement(*iterN);
    }
    // classes
    vector<CBEClass*>::iterator iterCl;
    for (iterCl = m_Classes.begin();
	 iterCl != m_Classes.end();
	 iterCl++)
    {
        InsertOrderedElement(*iterCl);
    }
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
    for (iterTa = m_TypeDeclarations.begin();
	 iterTa != m_TypeDeclarations.end();
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

/** \brief insert one element into the ordered list
 *  \param pObj the new element
 *
 * This is the insert implementation
 */
void CBENameSpace::InsertOrderedElement(CObject *pObj)
{
    // get source line number
    int nLine = pObj->GetSourceLine();
    // search for element with larger number
    vector<CObject*>::iterator iter = m_vOrderedElements.begin();
    for (; iter != m_vOrderedElements.end(); iter++)
    {
        if ((*iter)->GetSourceLine() > nLine)
        {
            // insert before that element
            m_vOrderedElements.insert(iter, pObj);
            return;
        }
    }
    // new object is bigger that all existing
    m_vOrderedElements.push_back(pObj);
}

/** \brief writes the name-space to the header file
 *  \param pFile the header file to write to
 */
void
CBENameSpace::Write(CBEHeaderFile& pFile)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);
    WriteElements(pFile);
}

/** \brief writes the name-space to the header file
 *  \param pFile the implementation file to write to
 */
void
CBENameSpace::Write(CBEImplementationFile& pFile)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);
    WriteElements(pFile);
}
