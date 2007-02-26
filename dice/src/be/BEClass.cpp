/**
 *    \file    dice/src/be/BEClass.cpp
 * \brief   contains the implementation of the class CBEClass
 *
 *    \date    Tue Jun 25 2002
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

#include "BEClass.h"

#include "BEFunction.h"
#include "BEOperationFunction.h"
#include "BEConstant.h"
#include "BETypedef.h"
#include "BEEnumType.h"
#include "BEMsgBufferType.h"
#include "BEAttribute.h"
#include "BEContext.h"
#include "BERoot.h"
#include "BEComponent.h"
#include "BETestsuite.h"
#include "BETestMainFunction.h"
#include "BEOperationFunction.h"
#include "BEInterfaceFunction.h"
#include "BECallFunction.h"
#include "BEUnmarshalFunction.h"
#include "BEMarshalFunction.h"
#include "BEReplyFunction.h"
#include "BEComponentFunction.h"
#include "BETestFunction.h"
#include "BESndFunction.h"
#include "BEWaitFunction.h"
#include "BEWaitAnyFunction.h"
#include "BESrvLoopFunction.h"
#include "BEDispatchFunction.h"
#include "BETestServerFunction.h"
#include "BEHeaderFile.h"
#include "BEImplementationFile.h"
#include "BEClient.h"
#include "BEOpcodeType.h"
#include "BEExpression.h"
#include "BENameSpace.h"
#include "BEDeclarator.h"
#include "BEStructType.h"
#include "BEUnionType.h"
#include "BEUserDefinedType.h"

#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FEOperation.h"
#include "fe/FEUnaryExpression.h"
#include "fe/FEIntAttribute.h"
#include "fe/FEConstructedType.h"
#include "fe/FEUserDefinedType.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEDeclarator.h"
#include "fe/FEAttributeDeclarator.h"
#include "fe/FESimpleType.h"
#include "fe/FEAttribute.h"
#include "fe/FEFile.h"

#include "Compiler.h"

#include <string>
using namespace std;


// CFunctionGroup IMPLEMENTATION

CFunctionGroup::CFunctionGroup(CFEOperation *pFEOperation)
{
    m_pFEOperation = pFEOperation;
}

/** \brief destroys a function group object
 *
 * This doesn't really do anything: The string object cleans up itself, and
 * the vector contains references to objects we do not want to delete. What we have
 * to remove are the vector-elements.
 */
CFunctionGroup::~CFunctionGroup()
{
    m_vFunctions.clear();
}

/** \brief retrieves the name of the group
 *  \return the name of the group
 */
string CFunctionGroup::GetName()
{
    return m_pFEOperation->GetName();
}

/** \brief access the front-end operation member
 *  \return a reference to the front-end operation member
 */
CFEOperation *CFunctionGroup::GetOperation()
{
    return m_pFEOperation;
}

/** \brief adds another function to this group
 *  \param pFunction the function to add
 */
void CFunctionGroup::AddFunction(CBEFunction *pFunction)
{
    if (pFunction)
        m_vFunctions.push_back(pFunction);
}

/** \brief returns a pointer to the first function in the group
 *  \return a pointer to the first function in the group
 */
vector<CBEFunction*>::iterator CFunctionGroup::GetFirstFunction()
{
    return m_vFunctions.begin();
}

/** \brief returns a reference to the next function in the group
 *  \param iter the iterator pointing to the next function
 *  \return a reference to the next function
 */
CBEFunction *CFunctionGroup::GetNextFunction(vector<CBEFunction*>::iterator &iter)
{
    if (iter == m_vFunctions.end())
        return 0;
    return *iter++;
}


// CBEClass IMPLEMENTATION

CBEClass::CBEClass()
{
    m_pMsgBuffer = 0;
}

/** \brief destructor of target class */
CBEClass::~CBEClass()
{
    while (!m_vFunctions.empty())
    {
        delete m_vFunctions.back();
        m_vFunctions.pop_back();
    }
    while (!m_vConstants.empty())
    {
        delete m_vConstants.back();
        m_vConstants.pop_back();
    }
    while (!m_vTypedefs.empty())
    {
        delete m_vTypedefs.back();
        m_vTypedefs.pop_back();
    }
    while (!m_vTypeDeclarations.empty())
    {
        delete m_vTypeDeclarations.back();
        m_vTypeDeclarations.pop_back();
    }
    while (!m_vAttributes.empty())
    {
        delete m_vAttributes.back();
        m_vAttributes.pop_back();
    }
    while (!m_vFunctionGroups.empty())
    {
        delete m_vFunctionGroups.back();
        m_vFunctionGroups.pop_back();
    }
    if (m_pMsgBuffer)
        delete m_pMsgBuffer;
    m_vBaseNames.clear();
}

/** \brief returns the name of the class
 *  \return the name of the class
 */
string CBEClass::GetName()
{
    return m_sName;
}

/** \brief adds a new function to the functions vector
 *  \param pFunction the function to add
 */
void CBEClass::AddFunction(CBEFunction * pFunction)
{
    if (!pFunction)
        return;
    m_vFunctions.push_back(pFunction);
    pFunction->SetParent(this);
}

/** \brief removes a function from the functions vector
 *  \param pFunction the function to remove
 */
void CBEClass::RemoveFunction(CBEFunction * pFunction)
{
    vector<CBEFunction*>::iterator iter;
    for (iter = m_vFunctions.begin(); iter != m_vFunctions.end(); iter++)
    {
        if (*iter == pFunction)
        {
            m_vFunctions.erase(iter);
            return;
        }
    }
}

/** \brief retrieves a pointer to the first function
 *  \return a pointer to the first function
 */
vector<CBEFunction*>::iterator CBEClass::GetFirstFunction()
{
    return m_vFunctions.begin();
}

/** \brief retrieves reference to the next function
 *  \param iter the pointer to the next function
 *  \return a reference to the next function
 */
CBEFunction *CBEClass::GetNextFunction(vector<CBEFunction*>::iterator &iter)
{
    if (iter == m_vFunctions.end())
        return 0;
    return *iter++;
}

/** \brief returns the number of functions in this class
 *  \return the number of functions in this class
 */
int CBEClass::GetFunctionCount()
{
    return m_vFunctions.size();
}

/** \brief returns the number of functions in this class which are written
 *  \return the number of functions in this class which are written
 */
int CBEClass::GetFunctionWriteCount(CBEFile *pFile, CBEContext *pContext)
{
    int nCount = 0;
    vector<CBEFunction*>::iterator iter = GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextFunction(iter)) != 0)
    {
        if (dynamic_cast<CBEHeaderFile*>(pFile) &&
            pFunction->DoWriteFunction((CBEHeaderFile*)pFile, pContext))
            nCount++;
        if (dynamic_cast<CBEImplementationFile*>(pFile) &&
            pFunction->DoWriteFunction((CBEImplementationFile*)pFile, pContext))
            nCount++;
    }

    return nCount;
}
/** \brief adds an attribute
 *  \param pAttribute the attribute to add
 */
void CBEClass::AddAttribute(CBEAttribute *pAttribute)
{
    if (!pAttribute)
        return;
    m_vAttributes.push_back(pAttribute);
    pAttribute->SetParent(this);
}

/** \brief removes an attribute
 *  \param pAttribute the attribute to remove
 */
void CBEClass::RemoveAttribute(CBEAttribute *pAttribute)
{
    vector<CBEAttribute*>::iterator iter;
    for (iter = m_vAttributes.begin(); iter != m_vAttributes.end(); iter++)
    {
        if (*iter == pAttribute)
        {
            m_vAttributes.erase(iter);
            return;
        }
    }
}

/** \brief retrieves a pointer to the first attribute
 *  \return a pointer to the first attribute
 */
vector<CBEAttribute*>::iterator CBEClass::GetFirstAttribute()
{
    return m_vAttributes.begin();
}

/** \brief retrieves a reference to the next attribute
 *  \param iter the pointer to the next attribute
 *  \return a reference to the next attribute
 */
CBEAttribute* CBEClass::GetNextAttribute(vector<CBEAttribute*>::iterator &iter)
{
    if (iter == m_vAttributes.end())
        return 0;
    return *iter++;
}

/** \brief adds a new base Class
 *  \param pClass the Class to add
 *
 * Do not set parent, because we are not parent of this Class.
 */
void CBEClass::AddBaseClass(CBEClass *pClass)
{
    if (!pClass)
        return;
    m_vBaseClasses.push_back(pClass);
    // if we add a base class, we add us to that class' derived classes
    pClass->AddDerivedClass(this);
}

/** \brief removes a base Class
 *  \param pClass the Class to remove
 */
void CBEClass::RemoveBaseClass(CBEClass *pClass)
{
    vector<CBEClass*>::iterator iter;
    for (iter = m_vBaseClasses.begin(); iter != m_vBaseClasses.end(); iter++)
    {
        if (*iter == pClass)
        {
            m_vBaseClasses.erase(iter);
            break;
        }
    }
    // we also have to remove us from the derived classes
    pClass->RemoveDerivedClass(this);
}

/** \brief retrieves a pointer to the first base Class
 *  \return a pointer to the first base Class
 *
 * If m_vBaseClasses is empty and m_nBaseNameSize is bigger than 0,
 * we have to add the references to the base classes first.
 */
vector<CBEClass*>::iterator CBEClass::GetFirstBaseClass()
{
    if (m_vBaseClasses.empty() && !m_vBaseNames.empty())
    {
        CBERoot *pRoot = GetSpecificParent<CBERoot>();
        assert(pRoot);
        vector<string>::iterator iter;
        for (iter = m_vBaseNames.begin(); iter != m_vBaseNames.end(); iter++)
        {
            // locate Class
            // if we cannot find class it is not there, because this should be called
            // way after all classes are created
            CBEClass *pBaseClass = pRoot->FindClass(*iter);
            if (!pBaseClass)
            {
                CCompiler::Warning("CBEClass::GetFirstBaseClass failed because base class \"%s\" cannot be found\n",
                    (*iter).c_str());
                return m_vBaseClasses.end();
            }
            AddBaseClass(pBaseClass);
        }
    }
    return m_vBaseClasses.begin();
}

/** \brief retrieves a pointer to the next base Class
 *  \param iter a pointer to the next base Class
 *  \return a reference to the next base Class
 */
CBEClass* CBEClass::GetNextBaseClass(vector<CBEClass*>::iterator &iter)
{
    if (iter == m_vBaseClasses.end())
        return 0;
    return *iter++;
}

/** \brief adds a new derived Class
 *  \param pClass the Class to add
 *
 * Do not set parent, because we are not parent of this Class.
 */
void CBEClass::AddDerivedClass(CBEClass *pClass)
{
    if (!pClass)
        return;
    m_vDerivedClasses.push_back(pClass);
}

/** \brief removes a derived Class
 *  \param pClass the Class to remove
 */
void CBEClass::RemoveDerivedClass(CBEClass *pClass)
{
    vector<CBEClass*>::iterator iter;
    for (iter = m_vDerivedClasses.begin(); iter != m_vDerivedClasses.end(); iter++)
    {
        if (*iter == pClass)
        {
            m_vDerivedClasses.erase(iter);
            return;
        }
    }
}

/** \brief retrieves a pointer to the first derived Class
 *  \return a pointer to the first derived Class
 */
vector<CBEClass*>::iterator CBEClass::GetFirstDerivedClass()
{
    return m_vDerivedClasses.begin();
}

/** \brief retrieves a pointer to the next derived Class
 *  \param iter a pointer to the next derived Class
 *  \return a reference to the next derived Class
 */
CBEClass* CBEClass::GetNextDerivedClass(vector<CBEClass*>::iterator &iter)
{
    if (iter == m_vDerivedClasses.end())
        return 0;
    return *iter++;
}

/** \brief adds another constant
 *  \param pConstant the const to add
 */
void CBEClass::AddConstant(CBEConstant *pConstant)
{
    if (!pConstant)
        return;
    m_vConstants.push_back(pConstant);
    pConstant->SetParent(this);
}

/** \brief removes a constant
 *  \param pConstant the const to remove
 */
void CBEClass::RemoveConstant(CBEConstant *pConstant)
{
    vector<CBEConstant*>::iterator iter;
    for (iter = m_vConstants.begin(); iter != m_vConstants.end(); iter++)
    {
        if (*iter == pConstant)
        {
            m_vConstants.erase(iter);
            return;
        }
    }
}

/** \brief get a pointer to the first constant
 *  \return a pointer to the first constant
 */
vector<CBEConstant*>::iterator CBEClass::GetFirstConstant()
{
    return m_vConstants.begin();
}

/** \brief gets a reference to the next constant
 *  \param iter the pointer to the next constant
 *  \return a reference to the next constant
 */
CBEConstant* CBEClass::GetNextConstant(vector<CBEConstant*>::iterator &iter)
{
    if (iter == m_vConstants.end())
        return 0;
    return *iter++;
}

/** \brief searches for a constant
 *  \param sConstantName the name of the constant to look for
 *  \return a reference to the found constant
 */
CBEConstant* CBEClass::FindConstant(string sConstantName)
{
    if (sConstantName.empty())
        return 0;
    // simply scan the function for a match
    vector<CBEConstant*>::iterator iter = GetFirstConstant();
    CBEConstant *pConstant;
    while ((pConstant = GetNextConstant(iter)) != 0)
    {
        if (pConstant->GetName() == sConstantName)
            return pConstant;
    }
    return 0;
}

/** \brief adds a new typedef
 *  \param pTypedef the typedef to add
 */
void CBEClass::AddTypedef(CBETypedef *pTypedef)
{
    if (!pTypedef)
        return;
    m_vTypedefs.push_back(pTypedef);
    pTypedef->SetParent(this);
}

/** \brief removes a typedef
 *  \param pTypedef the typedef to remove
 */
void CBEClass::RemoveTypedef(CBETypedef *pTypedef)
{
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = m_vTypedefs.begin(); iter != m_vTypedefs.end(); iter++)
    {
        if (*iter == pTypedef)
        {
            m_vTypedefs.erase(iter);
            return;
        }
    }
}

/** \brief gets a pointer to the first typedef
 *  \return a pointer to the first typedef
 */
vector<CBETypedDeclarator*>::iterator CBEClass::GetFirstTypedef()
{
    return m_vTypedefs.begin();
}

/** \brief gets a reference to the next typedef
 *  \param iter the pointer to the next typedef
 *  \return a reference to the next typedef
 */
CBETypedDeclarator* CBEClass::GetNextTypedef(vector<CBETypedDeclarator*>::iterator &iter)
{
    if (iter == m_vTypedefs.end())
        return 0;
    return *iter++;
}

/** \brief creates the members of this class
 *  \param pFEInterface the front-end interface to use as source
 *  \param pContext the context of the code generation
 *  \return true if successful
 */
bool CBEClass::CreateBackEnd(CFEInterface * pFEInterface, CBEContext * pContext)
{
    // call CBEObject's CreateBackEnd method
    if (!CBEObject::CreateBackEnd(pFEInterface))
        return false;

    // set target file name
    SetTargetFileName(pFEInterface, pContext);
    // set own name
    m_sName = pFEInterface->GetName();
    // add attributes
    vector<CFEAttribute*>::iterator iterA = pFEInterface->GetFirstAttribute();
    CFEAttribute *pFEAttribute;
    while ((pFEAttribute = pFEInterface->GetNextAttribute(iterA)) != 0)
    {
        if (!CreateBackEnd(pFEAttribute, pContext))
            return false;
    }
    // we can resolve this if we only add the base names now, but when the
    // base classes are first used, add the actual references.
    // add references to base Classes
    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    vector<CFEInterface*>::iterator iterBI = pFEInterface->GetFirstBaseInterface();
    CFEInterface *pFEBaseInterface;
    while ((pFEBaseInterface = pFEInterface->GetNextBaseInterface(iterBI)) != 0)
    {
        // add the name of the base interface
        AddBaseName(pFEBaseInterface->GetName());
    }
    // add constants
    vector<CFEConstDeclarator*>::iterator iterC = 
	pFEInterface->GetFirstConstant();
    CFEConstDeclarator *pFEConstant;
    while ((pFEConstant = pFEInterface->GetNextConstant(iterC)) != 0)
    {
        if (!CreateBackEnd(pFEConstant, pContext))
            return false;
    }
    // add typedefs
    vector<CFETypedDeclarator*>::iterator iterTD = 
	pFEInterface->GetFirstTypeDef();
    CFETypedDeclarator *pFETypedef;
    while ((pFETypedef = pFEInterface->GetNextTypeDef(iterTD)) != 0)
    {
        if (!CreateBackEnd(pFETypedef, pContext))
            return false;
    }
    // add tagged decls
    vector<CFEConstructedType*>::iterator iterT = 
	pFEInterface->GetFirstTaggedDecl();
    CFEConstructedType *pFETaggedType;
    while ((pFETaggedType = pFEInterface->GetNextTaggedDecl(iterT)) != 0)
    {
        if (!CreateBackEnd(pFETaggedType, pContext))
            return false;
    }
    // add types for Class (only for C)
    if (pContext->IsBackEndSet(PROGRAM_BE_C))
	if (!CreateAliasForClass(pFEInterface, pContext))
	    return false;
    // first create operation functions
    vector<CFEOperation*>::iterator iterO = pFEInterface->GetFirstOperation();
    CFEOperation *pFEOperation;
    while ((pFEOperation = pFEInterface->GetNextOperation(iterO)) != 0)
    {
        if (!CreateFunctionsNoClassDependency(pFEOperation, pContext))
            return false;
    }
    // create class' message buffer
    m_pMsgBuffer = pContext->GetClassFactory()->GetNewMessageBufferType(true);
    m_pMsgBuffer->SetParent(this);
    if (!m_pMsgBuffer->CreateBackEnd(pFEInterface, pContext))
    {
        VERBOSE("%s failed because message buffer type could not be created\n",
            __PRETTY_FUNCTION__);
        delete m_pMsgBuffer;
        return false;
    }
    // create interface functions
    iterO = pFEInterface->GetFirstOperation();
    while ((pFEOperation = pFEInterface->GetNextOperation(iterO)) != 0)
    {
        if (!CreateFunctionsClassDependency(pFEOperation, pContext))
            return false;
    }
    // add attribute interface members
    vector<CFEAttributeDeclarator*>::iterator iterAD = pFEInterface->GetFirstAttributeDeclarator();
    CFEAttributeDeclarator *pFEAttrDecl;
    while ((pFEAttrDecl = pFEInterface->GetNextAttributeDeclarator(iterAD)) != 0)
    {
        if (!CreateBackEnd(pFEAttrDecl, pContext))
            return false;
    }
    // add functions for interface
    return AddInterfaceFunctions(pFEInterface, pContext);
}

/** \brief adds the functions for an interface
 *  \param pFEInterface the interface to add the functions for
 *  \param pContext the context of this operation
 *  \return true if successful
 */
bool 
CBEClass::AddInterfaceFunctions(CFEInterface* pFEInterface, 
    CBEContext* pContext)
{
    CBEClassFactory *pCF = pContext->GetClassFactory();
    CBEInterfaceFunction *pFunction = pCF->GetNewWaitAnyFunction();
    AddFunction(pFunction);
    pFunction->SetComponentSide(true);
    if (!pFunction->CreateBackEnd(pFEInterface, pContext))
    {
        RemoveFunction(pFunction);
        VERBOSE("%s failed because wait-any function could not be created\n",
            __PRETTY_FUNCTION__);
        delete pFunction;
        return false;
    }
    pFunction = pCF->GetNewRcvAnyFunction();
    AddFunction(pFunction);
    pFunction->SetComponentSide(true);
    if (!pFunction->CreateBackEnd(pFEInterface, pContext))
    {
        RemoveFunction(pFunction);
        VERBOSE("%s failed because receive-any function could not be created\n",
            __PRETTY_FUNCTION__);
        delete pFunction;
        return false;
    }
    pFunction = pCF->GetNewReplyAnyWaitAnyFunction();
    AddFunction(pFunction);
    pFunction->SetComponentSide(true);
    if (!pFunction->CreateBackEnd(pFEInterface, pContext))
    {
        RemoveFunction(pFunction);
        VERBOSE("%s faile dbecause reply-any-wait-any function could not be created\n",
            __PRETTY_FUNCTION__);
        delete pFunction;
        return false;
    }
    if (!(pContext->IsOptionSet(PROGRAM_NO_DISPATCHER) &&
          pContext->IsOptionSet(PROGRAM_NO_SERVER_LOOP)))
    {
        pFunction = pCF->GetNewDispatchFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(true);
        if (!pFunction->CreateBackEnd(pFEInterface, pContext))
        {
            RemoveFunction(pFunction);
            VERBOSE("%s failed because dispatch function could not be created\n",
                __PRETTY_FUNCTION__);
            delete pFunction;
            return false;
        }
    }
    if (!pContext->IsOptionSet(PROGRAM_NO_SERVER_LOOP))
    {
        pFunction = pCF->GetNewSrvLoopFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(true);
        if (!pFunction->CreateBackEnd(pFEInterface, pContext))
        {
            RemoveFunction(pFunction);
            VERBOSE("%s failed because server-loop function could not be created\n",
                __PRETTY_FUNCTION__);
            delete pFunction;
            return false;
        }
    }

    if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
    {
        pFunction = pCF->GetNewTestServerFunction();
        AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEInterface, pContext))
        {
            RemoveFunction(pFunction);
            VERBOSE("%s failed because test-server function could not be created\n",
                __PRETTY_FUNCTION__);
            delete pFunction;
            return false;
        }
	// set line number
	pFunction->SetSourceLine(pFEInterface->GetSourceLineEnd());
    }

    // sort the parameters of the functions
    vector<CBEFunction*>::iterator iter = GetFirstFunction();
    CBEFunction *pF;
    while ((pF = GetNextFunction(iter)) != 0)
    {
        if (!pF->SortParameters(0, pContext))
        {
            VERBOSE("%s failed, because the parameters of function %s could not be sorted\n",
                __PRETTY_FUNCTION__, pF->GetName().c_str());
            return false;
        }
    }

    return true;
}

/** \brief creates an alias type for the class
 *  \param pFEInterface the interface to use as reference
 *  \param pContext the context of the creation
 *  \return true if successful
 *
 * In C we have an alias of CORBA_Object type tothe name if the interface.
 * In C++ this is not needed, because the class is derived from CORBA_Object
 */
bool CBEClass::CreateAliasForClass(CFEInterface *pFEInterface, CBEContext *pContext)
{
    assert(pFEInterface);
    // create the BE type
    CBETypedef *pTypedef = pContext->GetClassFactory()->GetNewTypedef();
    AddTypedef(pTypedef);
    // create CORBA_Object type
    CBEUserDefinedType *pType = (CBEUserDefinedType*)pContext->GetClassFactory()->GetNewType(TYPE_USER_DEFINED);
    pType->SetParent(pTypedef);
    if (!pType->CreateBackEnd(string("CORBA_Object"), pContext))
    {
        delete pType;
        VERBOSE("%s failed, because alias type could not be created.\n",
            __PRETTY_FUNCTION__);
        return false;
    }
    // finally create typedef
    if (!pTypedef->CreateBackEnd(pType, pFEInterface->GetName(), pFEInterface, pContext))
    {
        RemoveTypedef(pTypedef);
        VERBOSE("%s failed because typedef could not be created\n",
            __PRETTY_FUNCTION__);
        delete pTypedef;
        delete pType;
        return false;
    }

    // set source line and file
    pTypedef->SetSourceLine(pFEInterface->GetSourceLine()-1);
    pTypedef->SetSourceFileName(pFEInterface->GetSpecificParent<CFEFile>()->GetFileName());

    return true;
}

/** \brief internal function to create a constant
 *  \param pFEConstant the respective front-end constant
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CBEClass::CreateBackEnd(CFEConstDeclarator *pFEConstant, CBEContext *pContext)
{
    CBEConstant *pConstant = pContext->GetClassFactory()->GetNewConstant();
    AddConstant(pConstant);
    pConstant->SetParent(this);
    if (!pConstant->CreateBackEnd(pFEConstant, pContext))
    {
        RemoveConstant(pConstant);
        VERBOSE("%s failed because const %s could not be created\n",
                __PRETTY_FUNCTION__, pFEConstant->GetName().c_str());
        delete pConstant;
        return false;
    }
    return true;
}

/** \brief internal function to create a typedefinition
 *  \param pFETypedef the respective front-end type definition
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CBEClass::CreateBackEnd(CFETypedDeclarator *pFETypedef, CBEContext *pContext)
{
    CBETypedef *pTypedef = pContext->GetClassFactory()->GetNewTypedef();
    AddTypedef(pTypedef);
    pTypedef->SetParent(this);
    if (!pTypedef->CreateBackEnd(pFETypedef, pContext))
    {
        RemoveTypedef(pTypedef);
        VERBOSE("%s failed because typedef could not be created\n",
            __PRETTY_FUNCTION__);
        delete pTypedef;
        return false;
    }
    return true;
}

/** \brief internal function to create a functions for attribute declarator
 *  \param pFEAttrDecl the respective front-end attribute declarator definition
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CBEClass::CreateBackEnd(CFEAttributeDeclarator *pFEAttrDecl, CBEContext *pContext)
{
    if (!pFEAttrDecl)
    {
        VERBOSE("%s failed because attribute declarator is NULL\n",
            __PRETTY_FUNCTION__);
        return false;
    }
    // add function "<param_type_spec> [out]__get_<simple_declarator>();" for each decl
    // if !READONLY:
    //   add function "void [in]__set_<simple_declarator>(<param_type_spec> 'first letter of <decl>');" for each decl
    vector<CFEDeclarator*>::iterator iterD = pFEAttrDecl->GetFirstDeclarator();
    CFEDeclarator *pFEDecl;
    while ((pFEDecl = pFEAttrDecl->GetNextDeclarator(iterD)) != 0)
    {
        // get function
        string sName = string("_get_");
        sName += pFEDecl->GetName();
        CFETypeSpec *pFEType = (CFETypeSpec*)pFEAttrDecl->GetType()->Clone();
        CFEOperation *pFEOperation = new CFEOperation(pFEType, sName, NULL);
        pFEType->SetParent(pFEOperation);
        // get parent interface
        CFEInterface *pFEInterface = pFEAttrDecl->GetSpecificParent<CFEInterface>();
        assert(pFEInterface);
        pFEInterface->AddOperation(pFEOperation);
        if (!CreateFunctionsNoClassDependency(pFEOperation, pContext) ||
            !CreateFunctionsClassDependency(pFEOperation, pContext))
        {
            delete pFEOperation;
            return false;
        }

        // set function
        if (!pFEAttrDecl->FindAttribute(ATTR_READONLY))
        {
            sName = string("_set_");
            sName += pFEDecl->GetName();
            CFEAttribute *pFEAttr = new CFEAttribute(ATTR_IN);
            vector<CFEAttribute*> *pFEAttributes = new vector<CFEAttribute*>();
            pFEAttributes->push_back(pFEAttr);
            pFEType = new CFESimpleType(TYPE_VOID);
            CFETypeSpec *pFEParamType = (CFETypeSpec*)pFEAttrDecl->GetType()->Clone();
            CFEDeclarator *pFEParamDecl = new CFEDeclarator(DECL_IDENTIFIER, pFEDecl->GetName().substr(0,1));
            vector<CFEDeclarator*> *pFEParameters = new vector<CFEDeclarator*>();
            pFEParameters->push_back(pFEParamDecl);
            CFETypedDeclarator *pFEParam = new CFETypedDeclarator(TYPEDECL_PARAM,
                pFEParamType, pFEParameters, pFEAttributes);
            pFEParamType->SetParent(pFEParam);
            pFEParamDecl->SetParent(pFEParam);
            delete pFEAttributes;
            delete pFEParameters;
            // create function
            vector<CFETypedDeclarator*> *pParams = new vector<CFETypedDeclarator*>();
            pParams->push_back(pFEParam);
            pFEOperation = new CFEOperation(pFEType, sName, pParams);
            delete pParams;
            pFEType->SetParent(pFEOperation);
            pFEAttr->SetParent(pFEOperation);
            pFEParam->SetParent(pFEOperation);
            pFEInterface->AddOperation(pFEOperation);
            if (!CreateFunctionsNoClassDependency(pFEOperation, pContext) ||
                !CreateFunctionsClassDependency(pFEOperation, pContext))
            {
                delete pFEOperation;
                return false;
            }
        }
    }
    return true;
}

/** \brief internal function to create the back-end functions
 *  \param pFEOperation the respective front-end function
 *  \param pContext the context of the create process
 *  \return true if successful
 *
 * A function has to be generated depending on its attributes. If it is a call
 * function, we have to generate different back-end function than for a
 * message passing function.
 *
 * We depend on the fact, that either the [in] or the [out] attribute are
 * specified.  Never both may appear.
 *
 * In this method we create functions independant of the class' message buffer
 */
bool CBEClass::CreateFunctionsNoClassDependency(CFEOperation *pFEOperation, CBEContext *pContext)
{
    CFunctionGroup *pGroup = new CFunctionGroup(pFEOperation);
    AddFunctionGroup(pGroup);

    vector<CBEFunction*> vFunctionsToSort;

    if (!(pFEOperation->FindAttribute(ATTR_IN)) &&
        !(pFEOperation->FindAttribute(ATTR_OUT)))
    {
	// the call case: we need the functions call, unmarshal,
	// reply-and-wait, skeleton, reply-and-recv; for client side: call
        CBEOperationFunction *pFunction = pContext->GetClassFactory()->GetNewCallFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(false);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("%s  failed, because call function could not be created for %s\n",
                    __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
            return false;
        }
        vFunctionsToSort.push_back(pFunction);

        // for server side: reply-and-wait, reply-and-recv, skeleton
        pFunction = pContext->GetClassFactory()->GetNewComponentFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(true);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("%s failed, because component function could not be created for %s\n",
                    __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
            return false;
        }
        vFunctionsToSort.push_back(pFunction);

        if (pFEOperation->FindAttribute(ATTR_ALLOW_REPLY_ONLY))
        {
            pFunction = pContext->GetClassFactory()->GetNewReplyFunction();
            AddFunction(pFunction);
            pFunction->SetComponentSide(true);
            pGroup->AddFunction(pFunction);
            if (!pFunction->CreateBackEnd(pFEOperation, pContext))
            {
                RemoveFunction(pFunction);
                delete pFunction;
                VERBOSE("%s failed, because reply function could not be created for %s\n",
                    __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
                return false;
            }
            vFunctionsToSort.push_back(pFunction);
        }

        if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
        {
            pFunction = pContext->GetClassFactory()->GetNewTestFunction();
            AddFunction(pFunction);
            pFunction->SetComponentSide(false);
            pGroup->AddFunction(pFunction);
            if (!pFunction->CreateBackEnd(pFEOperation, pContext))
            {
                RemoveFunction(pFunction);
                delete pFunction;
                VERBOSE("%s failed because test function could not be created for %s\n",
                        __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
                return false;
            }
            vFunctionsToSort.push_back(pFunction);
        }
    }
    else
    {
        // the MP case
        // we need the functions send, recv, wait
        bool bComponent = (pFEOperation->FindAttribute(ATTR_OUT));
        // sender: send
        CBEOperationFunction *pFunction = pContext->GetClassFactory()->GetNewSndFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(bComponent);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("%s failed, because send function could not be created for %s\n",
                    __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
            return false;
        }
        vFunctionsToSort.push_back(pFunction);

        // receiver: wait, recv, unmarshal
        pFunction = pContext->GetClassFactory()->GetNewWaitFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(!bComponent);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("%s failed, because wait function could not be created for %s\n",
                    __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
            return false;
        }
        vFunctionsToSort.push_back(pFunction);

        pFunction = pContext->GetClassFactory()->GetNewRcvFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(!bComponent);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("%s failed because receive function could not be created for %s\n",
                    __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
            return false;
        }
        vFunctionsToSort.push_back(pFunction);

        // if we send oneway to the server we need a component function
        if (pFEOperation->FindAttribute(ATTR_IN))
        {
            pFunction = pContext->GetClassFactory()->GetNewComponentFunction();
            AddFunction(pFunction);
            pFunction->SetComponentSide(true);
            pGroup->AddFunction(pFunction);
            if (!pFunction->CreateBackEnd(pFEOperation, pContext))
            {
                RemoveFunction(pFunction);
                delete pFunction;
                VERBOSE("%s failed, because component function could not be created for %s\n",
                        __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
                return false;
            }
            vFunctionsToSort.push_back(pFunction);
        }

        if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
        {
            pFunction = pContext->GetClassFactory()->GetNewTestFunction();
            AddFunction(pFunction);
            pFunction->SetComponentSide(false);
            pGroup->AddFunction(pFunction);
            if (!pFunction->CreateBackEnd(pFEOperation, pContext))
            {
                RemoveFunction(pFunction);
                delete pFunction;
                VERBOSE("%s failed because test function could not be created for %s\n",
                        __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
                return false;
            }
            vFunctionsToSort.push_back(pFunction);
        }
    }

    // sort the parameters of the functions
    vector<CBEFunction*>::iterator iter = vFunctionsToSort.begin();
    for (; iter != vFunctionsToSort.end(); iter++)
    {
        if (!(*iter)->SortParameters(0, pContext))
        {
            VERBOSE("%s failed, because the parameters of function %s could not be sorted\n",
                __PRETTY_FUNCTION__, (*iter)->GetName().c_str());
            return false;
        }
    }

    return true;
}

/** \brief internal function to create the back-end functions
 *  \param pFEOperation the respective front-end function
 *  \param pContext the context of the create process
 *  \return true if successful
 *
 * A function has to be generated depending on its attributes. If it is a call
 * function, we have to generate different back-end function than for a
 * message passing function.
 *
 * We depend on the fact, that either the [in] or the [out] attribute are
 * specified.  Never both may appear.
 *
 * Here we create functions, which depend on the class' message buffer.
 */
bool
CBEClass::CreateFunctionsClassDependency(CFEOperation *pFEOperation, 
    CBEContext *pContext)
{
    // get function group of pFEOperation (should have been create above)
    vector<CFunctionGroup*>::iterator iterFG = GetFirstFunctionGroup();
    CFunctionGroup *pGroup;
    while ((pGroup = GetNextFunctionGroup(iterFG)) != NULL)
    {
        if (pGroup->GetOperation() == pFEOperation)
            break;
    }
    assert(pGroup);

    vector<CBEFunction*> vFunctionsToSort;

    if (!(pFEOperation->FindAttribute(ATTR_IN)) &&
        !(pFEOperation->FindAttribute(ATTR_OUT)))
    {
        // the call case:
        // we need the functions unmarshal, marshal
        CBEOperationFunction *pFunction = pContext->GetClassFactory()->GetNewUnmarshalFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(true);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("%s failed, because unmarshal function could not be created for %s\n",
                    __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
            return false;
        }
        vFunctionsToSort.push_back(pFunction);

        pFunction = pContext->GetClassFactory()->GetNewMarshalFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(true);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("%s failed, because marshal function coudl not be created for %s\n",
                __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
            return false;
        }
        vFunctionsToSort.push_back(pFunction);
    }
    else
    {
        // the MP case
        // we need the functions send, recv, wait, unmarshal
        bool bComponent = (pFEOperation->FindAttribute(ATTR_OUT));

        CBEOperationFunction *pFunction = pContext->GetClassFactory()->GetNewUnmarshalFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(!bComponent);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("%s failed because unmarshal function could not be created for %s\n",
                    __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
            return false;
        }
        vFunctionsToSort.push_back(pFunction);
    }

    // sort the parameters of the functions
    vector<CBEFunction*>::iterator iter = vFunctionsToSort.begin();
    for (; iter != vFunctionsToSort.end(); iter++)
    {
        if (!(*iter)->SortParameters(0, pContext))
        {
            VERBOSE("%s failed, because the parameters of function %s could not be sorted\n",
                __PRETTY_FUNCTION__, (*iter)->GetName().c_str());
            return false;
        }
    }

    return true;
}

/** \brief interna function to create an attribute
 *  \param pFEAttribute the respective front-end attribute
 *  \param pContext the context of the creation
 *  \return true if successful
 */
bool CBEClass::CreateBackEnd(CFEAttribute *pFEAttribute, CBEContext *pContext)
{
    CBEAttribute *pAttribute = pContext->GetClassFactory()->GetNewAttribute();
    AddAttribute(pAttribute);
    if (!pAttribute->CreateBackEnd(pFEAttribute, pContext))
    {
        RemoveAttribute(pAttribute);
        VERBOSE("%s failed because attribute could not be created\n",
            __PRETTY_FUNCTION__);
        delete pAttribute;
        return false;
    }
    return true;
}

/** \brief adds the Class to the header file
 *  \param pHeader file the header file to add to
 *  \param pContext the context of the operation
 *  \return true if successful
 *
 * An Class adds its included types, constants and functions.
 */
bool CBEClass::AddToFile(CBEHeaderFile *pHeader, CBEContext *pContext)
{
    VERBOSE("CBEClass::AddToFile(header: %s) for class %s called\n",
        pHeader->GetFileName().c_str(), GetName().c_str());
    // add this class to the file
    if (IsTargetFile(pHeader))
        pHeader->AddClass(this);
    return true;
}

/** \brief adds this class (or its members) to an implementation file
 *  \param pImpl the implementation file to add this class to
 *  \param pContext the context of this creation
 *  \return true if successful
 *
 * if the options PROGRAM_FILE_FUNCTION is set, we have to add each function
 * seperately for the client implementation file. Otherwise we add the
 * whole class.
 */
bool CBEClass::AddToFile(CBEImplementationFile *pImpl, CBEContext *pContext)
{
    VERBOSE("CBEClass::AddToFile(impl: %s) for class %s called\n",
        pImpl->GetFileName().c_str(), GetName().c_str());
    // check compiler option
    if (pContext->IsOptionSet(PROGRAM_FILE_FUNCTION) &&
        dynamic_cast<CBEClient*>(pImpl->GetTarget()))
    {
        vector<CBEFunction*>::iterator iter = GetFirstFunction();
        CBEFunction *pFunction;
        while ((pFunction = GetNextFunction(iter)) != 0)
        {
            pFunction->AddToFile(pImpl, pContext);
        }
    }
    // add this class to the file
    if (IsTargetFile(pImpl))
        pImpl->AddClass(this);
    return true;
}

/** \brief counts the parameters with a specific type
 *  \param nFEType the front-end type to test for
 *  \param bSameCount true if the count is the same for all functions (false if functions have different count)
 *  \param nDirection the direction to count
 *  \return the number of parameters of this type
 */
int CBEClass::GetParameterCount(int nFEType, bool& bSameCount, int nDirection)
{
    if (nDirection == 0)
    {
        // count both and return max
        int nCountIn = GetParameterCount(nFEType, bSameCount, DIRECTION_IN);
        int nCountOut = GetParameterCount(nFEType, bSameCount, DIRECTION_OUT);
        return (nCountIn > nCountOut) ? nCountIn : nCountOut;
    }

    int nCount = 0, nCurr;
    vector<CBEFunction*>::iterator iter = GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextFunction(iter)) != 0)
    {
        nCurr = pFunction->GetParameterCount(nFEType, nDirection);
         if ((nCount > 0) && (nCurr != nCount) && (nCurr > 0))
            bSameCount = false;
        if (nCurr > nCount) nCount = nCurr;
    }

    return nCount;
}

/** \brief counts the number of string parameter needed for this interface
 *  \param nDirection the direction to count
 *  \param nMustAttrs the attributes which have to be set for the parameters
 *  \param nMustNotAttrs the attributes which must not be set for the parameters
 *  \return the number of strings needed
 */
int CBEClass::GetStringParameterCount(int nDirection, int nMustAttrs, int nMustNotAttrs)
{
    if (nDirection == 0)
    {
        int nStringsIn = GetStringParameterCount(DIRECTION_IN, nMustAttrs, nMustNotAttrs);
        int nStringsOut = GetStringParameterCount(DIRECTION_OUT, nMustAttrs, nMustNotAttrs);
        return ((nStringsIn > nStringsOut) ? nStringsIn : nStringsOut);
    }

    int nCount = 0, nCurr;

    vector<CFunctionGroup*>::iterator iterG = GetFirstFunctionGroup();
    CFunctionGroup *pFuncGroup;
    while ((pFuncGroup = GetNextFunctionGroup(iterG)) != 0)
    {
        vector<CBEFunction*>::iterator iterF = pFuncGroup->GetFirstFunction();
        CBEOperationFunction *pFunc;
        while ((pFunc = dynamic_cast<CBEOperationFunction*>(pFuncGroup->GetNextFunction(iterF))) != 0)
        {
            nCurr = pFunc->GetStringParameterCount(nDirection, nMustAttrs, nMustNotAttrs);
            nCount = (nCount > nCurr) ? nCount : nCurr;
        }
    }

    return nCount;
}

/** \brief calculates the size of the interface function
 *  \param nDirection the direction to count
 *  \param pContext the context of the calculation (needed to test for message buffer)
 *  \return the number of bytes needed to transmit any of the functions
 */
int CBEClass::GetSize(int nDirection, CBEContext *pContext)
{
    if (nDirection == 0)
    {
        int nSizeIn = GetSize(DIRECTION_IN, pContext);
        int nSizeOut = GetSize(DIRECTION_OUT, pContext);
        return ((nSizeIn > nSizeOut) ? nSizeIn : nSizeOut);
    }

    int nSize = 0, nCurr;

    vector<CBEFunction*>::iterator iter = GetFirstFunction();
    CBEOperationFunction *pFunc;
    while ((pFunc = dynamic_cast<CBEOperationFunction*>(GetNextFunction(iter))) != 0)
    {
        nCurr = pFunc->GetSize(nDirection, pContext);
        nSize = (nSize > nCurr) ? nSize : nCurr;
    }

    return nSize;
}

/** \brief tries to find a function
 *  \param sFunctionName the name of the function to search for
 *  \return a reference to the searched class or 0
 */
CBEFunction* CBEClass::FindFunction(string sFunctionName)
{
    if (sFunctionName.empty())
        return 0;
    // simply scan the function for a match
    vector<CBEFunction*>::iterator iter = GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextFunction(iter)) != 0)
    {
        if (pFunction->GetName() == sFunctionName)
            return pFunction;
    }
    return 0;
}

/** \brief adds the opcodes of this class' functions to the header file
 *  \param pFile the file to write to
 *  \param pContext the context of this operation
 *  \return true if successfule
 *
 * This implementation adds the base opcode for this class and all opcodes for its functions.
 */
bool CBEClass::AddOpcodesToFile(CBEHeaderFile *pFile, CBEContext *pContext)
{
    VERBOSE("CBEClass::AddOpcodesToFile(header: %s) called\n",
        pFile->GetFileName().c_str());
    // check if the file is really our target file
    if (!IsTargetFile(pFile))
        return true;

    // first create classes in reverse order, so we can build correct parent
    // relationships
    CBEClassFactory *pCF = pContext->GetClassFactory();
    CBEConstant *pOpcode = pCF->GetNewConstant();
    CBEOpcodeType *pType = pCF->GetNewOpcodeType();
    pType->SetParent(pOpcode);
    CBEExpression *pBrace = pCF->GetNewExpression();
    pBrace->SetParent(pOpcode);
    CBEExpression *pInterfaceCode = pCF->GetNewExpression();
    pInterfaceCode->SetParent(pBrace);
    CBEExpression *pValue = pCF->GetNewExpression();
    pValue->SetParent(pInterfaceCode);
    CBEExpression *pBits = pCF->GetNewExpression();
    pBits->SetParent(pInterfaceCode);

    // now call Create functions, which require correct parent relationshsips

    // create opcode type
    if (!pType->CreateBackEnd(pContext))
    {
        delete pOpcode;
        delete pValue;
        delete pType;
        delete pBrace;
        delete pInterfaceCode;
        delete pBits;
        return false;
    }
    // get opcode number
    int nInterfaceNumber = GetClassNumber();
    // create value
    if (!pValue->CreateBackEnd(nInterfaceNumber, pContext))
    {
        delete pOpcode;
        delete pValue;
        delete pType;
        delete pBrace;
        delete pInterfaceCode;
        delete pBits;
        return false;
    }
    // create shift bits
    CBENameFactory *pNF = pContext->GetNameFactory();
    string sShift = pNF->GetInterfaceNumberShiftConstant();
    if (!pBits->CreateBackEnd(sShift, pContext))
    {
        delete pOpcode;
        delete pValue;
        delete pType;
        delete pBrace;
        delete pInterfaceCode;
        delete pBits;
        return false;
    }
    // create value << bits
    if (!pInterfaceCode->CreateBackEndBinary(pValue, EXPR_LSHIFT, pBits, 
	    pContext))
    {
        delete pOpcode;
        delete pValue;
        delete pType;
        delete pBrace;
        delete pInterfaceCode;
        delete pBits;
        return false;
    }
    // brace it
    if (!pBrace->CreateBackEndPrimary(EXPR_PAREN, pInterfaceCode, pContext))
    {
        delete pOpcode;
        delete pValue;
        delete pType;
        delete pBrace;
        delete pInterfaceCode;
        delete pBits;
        return false;
    }
    // create constant
    // create opcode name
    string sName = pContext->GetNameFactory()->GetOpcodeConst(this, pContext);
    // add const to file
    pFile->AddConstant(pOpcode);
    if (!pOpcode->CreateBackEnd(pType, sName, pBrace, true/* always define*/,
	    pContext))
    {
        pFile->RemoveConstant(pOpcode);
        delete pOpcode;
        delete pValue;
        delete pType;
        return false;
    }

    // iterate over functions
    vector<CFunctionGroup*>::iterator iter = GetFirstFunctionGroup();
    CFunctionGroup *pFunctionGroup;
    while ((pFunctionGroup = GetNextFunctionGroup(iter)) != 0)
    {
        if (!AddOpcodesToFile(pFunctionGroup->GetOperation(), pFile, pContext))
            return false;
    }

    return true;
}

/** \brief adds the opcode for a single function
 *  \param pFEOperation the function to add the opcode for
 *  \param pFile the file to add the opcode to
 *  \param pContext the context of this operation
 */
bool CBEClass::AddOpcodesToFile(CFEOperation *pFEOperation, CBEHeaderFile *pFile, CBEContext *pContext)
{
    VERBOSE("CBEClass::AddOpcodesToFile(operation: %s) called\n",
        pFEOperation->GetName().c_str());
    // first create classes, so we can build parent relationship correctly
    CBEConstant *pOpcode = pContext->GetClassFactory()->GetNewConstant();
    CBEOpcodeType *pType = pContext->GetClassFactory()->GetNewOpcodeType();
    pType->SetParent(pOpcode);
    CBEExpression *pTopBrace = pContext->GetClassFactory()->GetNewExpression();
    pTopBrace->SetParent(pOpcode);
    CBEExpression *pValue = pContext->GetClassFactory()->GetNewExpression();
    pValue->SetParent(pTopBrace);
    CBEExpression *pBrace = pContext->GetClassFactory()->GetNewExpression();
    pBrace->SetParent(pValue);
    CBEExpression *pFuncCode = pContext->GetClassFactory()->GetNewExpression();
    pFuncCode->SetParent(pBrace);
    CBEExpression *pBase = pContext->GetClassFactory()->GetNewExpression();
    pBase->SetParent(pValue);
    CBEExpression *pNumber = pContext->GetClassFactory()->GetNewExpression();
    pNumber->SetParent(pFuncCode);
    CBEExpression *pBitMask = pContext->GetClassFactory()->GetNewExpression();
    pBitMask->SetParent(pFuncCode);

    // now call the create functions, which require an correct parent relationship
    // get base opcode name
    string sBase = pContext->GetNameFactory()->GetOpcodeConst(this, pContext);
    if (!pBase->CreateBackEnd(sBase, pContext))
    {
        delete pOpcode;
        delete pType;
        delete pTopBrace;
        delete pValue;
        delete pNumber;
        delete pBase;
        delete pBrace;
        delete pFuncCode;
        delete pBitMask;
        return false;
    }
    // create number
    int nOperationNb = GetOperationNumber(pFEOperation, pContext);
    if (!pNumber->CreateBackEnd(nOperationNb, pContext))
    {
        delete pOpcode;
        delete pType;
        delete pTopBrace;
        delete pValue;
        delete pNumber;
        delete pBase;
        delete pBrace;
        delete pFuncCode;
        delete pBitMask;
        return false;
    }
    // create bitmask
    string sBitMask =  pContext->GetNameFactory()->GetFunctionBitMaskConstant();
    if (!pBitMask->CreateBackEnd(sBitMask, pContext))
    {
        delete pOpcode;
        delete pType;
        delete pTopBrace;
        delete pValue;
        delete pNumber;
        delete pBase;
        delete pBrace;
        delete pFuncCode;
        delete pBitMask;
        return false;
    }
    // create function code
    if (!pFuncCode->CreateBackEndBinary(pNumber, EXPR_BITAND, pBitMask, pContext))
    {
        delete pOpcode;
        delete pType;
        delete pTopBrace;
        delete pValue;
        delete pNumber;
        delete pBase;
        delete pBrace;
        delete pFuncCode;
        delete pBitMask;
        return false;
    }
    // create braces
    if (!pBrace->CreateBackEndPrimary(EXPR_PAREN, pFuncCode, pContext))
    {
        delete pOpcode;
        delete pType;
        delete pTopBrace;
        delete pValue;
        delete pNumber;
        delete pBase;
        delete pBrace;
        delete pFuncCode;
        delete pBitMask;
        return false;
    }
    // create value
    if (!pValue->CreateBackEndBinary(pBase, EXPR_PLUS, pBrace, pContext))
    {
        delete pOpcode;
        delete pType;
        delete pTopBrace;
        delete pValue;
        delete pNumber;
        delete pBase;
        delete pBrace;
        delete pFuncCode;
        delete pBitMask;
        return false;
    }
    // create top brace
    if (!pTopBrace->CreateBackEndPrimary(EXPR_PAREN, pValue, pContext))
    {
        delete pOpcode;
        delete pType;
        delete pTopBrace;
        delete pValue;
        delete pNumber;
        delete pBase;
        delete pBrace;
        delete pFuncCode;
        delete pBitMask;
        return false;
    }
    // create opcode type
    if (!pType->CreateBackEnd(pContext))
    {
        delete pOpcode;
        delete pType;
        delete pTopBrace;
        delete pValue;
        delete pNumber;
        delete pBase;
        delete pBrace;
        delete pFuncCode;
        delete pBitMask;
        return false;
    }
    // create opcode name
    string sName = pContext->GetNameFactory()->GetOpcodeConst(pFEOperation, pContext);
    // create constant
    ((CBEHeaderFile *) pFile)->AddConstant(pOpcode);
    if (!pOpcode->CreateBackEnd(pType, sName, pTopBrace, true /*always defined*/, pContext))
    {
        ((CBEHeaderFile *) pFile)->RemoveConstant(pOpcode);
        delete pOpcode;
        delete pType;
        delete pTopBrace;
        delete pValue;
        delete pNumber;
        delete pBase;
        delete pBrace;
        delete pFuncCode;
        delete pBitMask;
        return false;
    }
    // return successfully
    return true;

}

/** \brief calculates the number used as base opcode number
 *  \return the interface number
 *
 * AN interface number is first of all it's uuid. If it is not available
 * the base interfaces are counted and this interface's number is the
 * highest number of it's base interfaces + 1.
 */
int CBEClass::GetClassNumber()
{
    CBEAttribute *pUuidAttr = FindAttribute(ATTR_UUID);
    if (pUuidAttr)
    {
        if (pUuidAttr->IsOfType(ATTR_CLASS_INT))
        {
            return pUuidAttr->GetIntValue();
        }
    }

    vector<CBEClass*>::iterator iter = GetFirstBaseClass();
    CBEClass *pBaseClass;
    int nNumber = 1;
    while ((pBaseClass = GetNextBaseClass(iter)) != 0)
    {
        int nBaseNumber = pBaseClass->GetClassNumber();
        if (nBaseNumber >= nNumber)
            nNumber = nBaseNumber+1;
    }
    return nNumber;
}

/** \brief retrieves a referece to the parent namespace
 *  \return a referece to the parent namespace
 */
CBENameSpace* CBEClass::GetNameSpace()
{
    for (CObject *pParent = GetParent(); pParent; pParent = pParent->GetParent())
    {
        if (dynamic_cast<CBENameSpace*>(pParent))
            return (CBENameSpace*)pParent;
    }
    return 0;
}

/** \brief searches for a specific attribte
 *  \param nAttrType the attribute type to search for
 *  \return a reference to the found attribute or 0
 */
CBEAttribute* CBEClass::FindAttribute(int nAttrType)
{
    vector<CBEAttribute*>::iterator iter = GetFirstAttribute();
    CBEAttribute *pAttr;
    while ((pAttr = GetNextAttribute(iter)) != 0)
    {
        if (pAttr->GetType() == nAttrType)
            return pAttr;
    }
    return 0;
}

/** \brief writes the class to the target file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * With the C implementation the class simply calls the Write methods
 * of its constants, typedefs and functions
 */
void CBEClass::Write(CBEHeaderFile *pFile, CBEContext *pContext)
{
    VERBOSE("CBEClass::Write(head, %s) called\n", GetName().c_str());

    // since message buffer is local for this class, the class declaration
    // wraps the message buffer
    // per default we derive from CORBA_Object
    if (pContext->IsBackEndSet(PROGRAM_BE_CPP))
    {
	// CPP TODO: should derive from CORBA_Object, _but_:
	// then we need to have a declaration of CORBA_Object which is not a
	// typedef of a pointer to CORBA_Object_base, which in turn messes up
	// compilation of C++ files including generated header files generated
	// for C backend... We have to find some other way, such as a dice
	// local define for C++.
	*pFile << "\tclass " << GetName() << "\n";
	*pFile << "\t{\n";
    }
    
    // write message buffer type seperately
    if (m_pMsgBuffer)
    {
	if (pContext->IsBackEndSet(PROGRAM_BE_CPP))
	{
	    *pFile << "\tprotected:\n";
	    pFile->IncIndent();
	}
	// message buffer is protected
	
        // test for client and if type is needed
        CBETarget *pTarget = pFile->GetTarget();
        if (!pFile->IsOfFileType(FILETYPE_CLIENT) ||
            pTarget->HasFunctionWithUserType(
		m_pMsgBuffer->GetAlias()->GetName(), pContext))
        {
            m_pMsgBuffer->WriteDefinition(pFile, true, pContext);
            pFile->Print("\n");
        }

	if (pContext->IsBackEndSet(PROGRAM_BE_CPP))
	    pFile->DecIndent();
    }

    // sort our members/elements depending on source line number
    // into extra vector
    CreateOrderedElementList();

    // members are public
    if (pContext->IsBackEndSet(PROGRAM_BE_CPP))
    {
	*pFile << "\tpublic:\n";
	pFile->IncIndent();
    }

    // write target file
    int nFuncCount = GetFunctionWriteCount(pFile, pContext);
    vector<CObject*>::iterator iter = m_vOrderedElements.begin();
    int nLastType = 0, nCurrType = 0;
    for (; iter != m_vOrderedElements.end(); iter++)
    {
        if (dynamic_cast<CBEConstant*>(*iter))
            nCurrType = 1;
        else if (dynamic_cast<CBETypedDeclarator*>(*iter))
            nCurrType = 2;
        else if (dynamic_cast<CBEType*>(*iter))
            nCurrType = 3;
        else if (dynamic_cast<CBEFunction*>(*iter))
            nCurrType = 4;
        else
            nCurrType = 0;
        // newline when changing types
        if (nCurrType != nLastType)
        {
            // brace functions with extern C
            if ((nLastType == 4) && (nFuncCount > 0) &&
		!pContext->IsBackEndSet(PROGRAM_BE_CPP))
            {
                *pFile << "#ifdef __cplusplus\n" <<
                    "}\n" <<
                    "#endif\n\n";
            }
            *pFile << "\n";
            nLastType = nCurrType;
            // brace functions with extern C
            if ((nCurrType == 4) && (nFuncCount > 0) &&
		!pContext->IsBackEndSet(PROGRAM_BE_CPP))
            {
                *pFile << "#ifdef __cplusplus\n" <<
                    "extern \"C\" {\n" <<
                    "#endif\n\n";
            }
        }
        // add pre-processor directive to denote source line
        if (pContext->IsOptionSet(PROGRAM_GENERATE_LINE_DIRECTIVE))
        {
            *pFile << "# " << (*iter)->GetSourceLine() << " \"" <<
                (*iter)->GetSourceFileName() << "\"\n";
        }
        // now really write the element
        switch (nCurrType)
        {
        case 1:
            WriteConstant((CBEConstant*)(*iter), pFile, pContext);
            break;
        case 2:
            WriteTypedef((CBETypedDeclarator*)(*iter), pFile, pContext);
            break;
        case 3:
            WriteTaggedType((CBEType*)(*iter), pFile, pContext);
            break;
        case 4:
            WriteFunction((CBEFunction*)(*iter), pFile, pContext);
            break;
        default:
            break;
        }
    }
    // brace functions with extern C
    if ((nLastType == 4) && (nFuncCount > 0) &&
	!pContext->IsBackEndSet(PROGRAM_BE_CPP))
    {
        *pFile << "#ifdef __cplusplus\n" <<
            "}\n" <<
            "#endif\n\n";
    }

    WriteHelperFunctions(pFile, pContext);

    if (pContext->IsBackEndSet(PROGRAM_BE_CPP))
    {
	pFile->DecIndent();
	*pFile << "\t};\n";
	*pFile << "\n";
    }

    VERBOSE("CBEClass::Write(head, %s) finished\n", GetName().c_str());
}

/** \brief writes the class to the target file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * With the C implementation the class simply calls it's
 * function's Write method.
 */
void CBEClass::Write(CBEImplementationFile *pFile, CBEContext *pContext)
{
    VERBOSE("CBEClass::Write(impl, %s) called\n", GetName().c_str());

    // sort our members/elements depending on source line number
    // into extra vector
    CreateOrderedElementList();

    int nFuncCount = GetFunctionWriteCount(pFile, pContext);
    if ((nFuncCount > 0) &&
	!pContext->IsBackEndSet(PROGRAM_BE_CPP))
    {
        *pFile << "#ifdef __cplusplus\n";
        *pFile << "extern \"C\" {\n";
        *pFile << "#endif\n\n";
    }
    // write target functions in ordered appearance
    vector<CObject*>::iterator iter = m_vOrderedElements.begin();
    for (; iter != m_vOrderedElements.end(); iter++)
    {
        if (dynamic_cast<CBEFunction*>(*iter))
        {
            // add pre-processor directive to denote source line
            if (pContext->IsOptionSet(PROGRAM_GENERATE_LINE_DIRECTIVE))
            {
                *pFile << "# " << (*iter)->GetSourceLine() << " \"" <<
                    (*iter)->GetSourceFileName() << "\"\n";
            }
            WriteFunction((CBEFunction*)(*iter), pFile, pContext);
        }
    }
    // write helper functions if any
    WriteHelperFunctions(pFile, pContext);
    if ((nFuncCount > 0) &&
	!pContext->IsBackEndSet(PROGRAM_BE_CPP))
    {
        *pFile << "#ifdef __cplusplus\n";
        *pFile << "}\n";
        *pFile << "#endif\n\n";
    }

    VERBOSE("CBEClass::Write(impl, %s) finished\n", GetName().c_str());
}

/** \brief writes a constant
 *  \param pConstant the constant to write
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEClass::WriteConstant(CBEConstant *pConstant,
    CBEHeaderFile *pFile,
    CBEContext *pContext)
{
    assert(pConstant);
    pConstant->Write(pFile, pContext);
}

/** \brief write a type definition
 *  \param pTypedef the type definition to write
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEClass::WriteTypedef(CBETypedDeclarator *pTypedef,
    CBEHeaderFile *pFile,
    CBEContext *pContext)
{
    assert(pTypedef);
    if (dynamic_cast<CBETypedef*>(pTypedef))
        ((CBETypedef*)pTypedef)->WriteDeclaration(pFile, pContext);
    else
    {
        pTypedef->WriteDeclaration(pFile, pContext);
        *pFile << ";\n";
    }
}

/** \brief write a function to the header file
 *  \param pFunction the function to write
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEClass::WriteFunction(CBEFunction *pFunction,
    CBEHeaderFile *pFile,
    CBEContext *pContext)
{
    assert(pFunction);
    if (pFunction->DoWriteFunction(pFile, pContext))
    {
        pFunction->Write(pFile, pContext);
        *pFile << "\n";
    }
}

/** \brief write a function to the implementation file
 *  \param pFunction the function to write
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEClass::WriteFunction(CBEFunction *pFunction,
    CBEImplementationFile *pFile,
    CBEContext *pContext)
{
    assert(pFunction);
    if (pFunction->DoWriteFunction(pFile, pContext))
    {
        pFunction->Write(pFile, pContext);
        *pFile << "\n";
    }
}

/** \brief adds a new function group
 *  \param pGroup the group to add
 */
void CBEClass::AddFunctionGroup(CFunctionGroup *pGroup)
{
    if (!pGroup)
        return;
    m_vFunctionGroups.push_back(pGroup);
}

/** \brief removes a function group
 *  \param pGroup the group to remove
 */
void CBEClass::RemoveFunctionGroup(CFunctionGroup *pGroup)
{
    vector<CFunctionGroup*>::iterator iter;
    for (iter = m_vFunctionGroups.begin(); iter != m_vFunctionGroups.end(); iter++)
    {
        if (*iter == pGroup)
        {
            m_vFunctionGroups.erase(iter);
            return;
        }
    }
}

/** \brief retrieves a pointer to the first function group
 *  \return a pointer to the first function group
 */
vector<CFunctionGroup*>::iterator CBEClass::GetFirstFunctionGroup()
{
    return m_vFunctionGroups.begin();
}

/** \brief retrieves a reference to the next function group
 *  \param iter the pointer to the next function group
 *  \return a reference to the next function group
 */
CFunctionGroup* CBEClass::GetNextFunctionGroup(vector<CFunctionGroup*>::iterator &iter)
{
    if (iter == m_vFunctionGroups.end())
        return 0;
    return *iter++;
}

/** \brief calculates the function identifier
 *  \param pFEOperation the front-end operation to get the identifier for
 *  \param pContext the context of the calculation
 *  \return the function identifier
 *
 * The function identifier is unique within an interface. Usually an interface's scope
 * is determined by its definition. But since the server loop using the function identifier
 * only differentiates number, we have to be unique within the same interface number.
 *
 * It is sufficient to check base interfaces for their interface ID, because derived interfaces
 * will use this algorithm as well and thus regard our generated function ID's.
 *
 * For all interfaces with the same ID as the operation's interface, we build a grid of
 * predefined function ID's. In the next step we iterate over the functions and count them,
 * skipping the predefined numbers. When we finally reached the given function, we return
 * the calculated function ID.
 *
 * But first of all, the Uuid attribute of a function determines it's ID. To print warnings
 * about same Uuids anyways, we test for this situation after the FindPredefinedNumbers call.
 */
int CBEClass::GetOperationNumber(CFEOperation *pFEOperation, CBEContext *pContext)
{
    assert(pFEOperation);
    VERBOSE("CBEClass::GetOperationNumber for %s\n", pFEOperation->GetName().c_str());
    CFEInterface *pFEInterface = pFEOperation->GetSpecificParent<CFEInterface>();
    int nInterfaceNumber = GetInterfaceNumber(pFEInterface);
    // search for base interfaces with same interface number and collect them
    vector<CFEInterface*> vSameInterfaces;
    FindInterfaceWithNumber(pFEInterface, nInterfaceNumber, &vSameInterfaces);
    // now search for predefined numbers
    vector<CPredefinedFunctionID> vFunctionIDs;
    vSameInterfaces.push_back(pFEInterface); // add this interface too
    FindPredefinedNumbers(&vSameInterfaces, &vFunctionIDs, pContext);
    // find pFEInterface in vector
    vector<CFEInterface*>::iterator iterI = vSameInterfaces.begin();
    for (; iterI != vSameInterfaces.end(); iterI++)
    {
        if (*iterI == pFEInterface)
        {
            vSameInterfaces.erase(iterI);
            break; // stops for loop
        }
    }
    // check for Uuid attribute and return its value
    if (HasUuid(pFEOperation))
        return GetUuid(pFEOperation);
    // now there is no Uuid, thus we have to calculate the function id
    // get the maximum function ID of the interfaces with the same number
    int nFunctionID = 0;
    iterI = vSameInterfaces.begin();
    while (iterI != vSameInterfaces.end())
    {
        CFEInterface *pCurrI = *iterI++;
        if (!pCurrI)
            continue;
        // we sum the max opcodes, because thy have to be disjunct
        // if we would use max(current, functionID) the function ID would be the same ...
        //
        // interface 1: 1, 2, 3, 4    \_ if max
        // interface 2: 1, 2, 3, 4, 5 /
        //
        // interface 1: 1, 2, 3, 4    \_ if sum
        // interface 2: 5, 6, 7, 8, 9 /
        //
        nFunctionID += GetMaxOpcodeNumber(pCurrI);
    }
    // now iterate over functions, incrementing a counter, check if new value is predefined and
    // if the checked function is this function, then return the counter value
    vector<CFEOperation*>::iterator iterO = pFEInterface->GetFirstOperation();
    CFEOperation *pCurrOp;
    while ((pCurrOp = pFEInterface->GetNextOperation(iterO)) != 0)
    {
        nFunctionID++;
        while (IsPredefinedID(&vFunctionIDs, nFunctionID)) nFunctionID++;
        if (pCurrOp == pFEOperation)
        {
            vFunctionIDs.clear();
            return nFunctionID;
        }
    }
    // something went wrong -> if we are here, the operations parent interface does not
    // have this operation as a child
    assert(false);
    return 0;
}
/** \brief checks if a function ID is predefined
 *  \param pFunctionIDs the predefined IDs
 *  \param nNumber the number to test
 *  \return true if number is predefined
 */
bool CBEClass::IsPredefinedID(vector<CPredefinedFunctionID> *pFunctionIDs, int nNumber)
{
    vector<CPredefinedFunctionID>::iterator iter;
    for (iter = pFunctionIDs->begin(); iter != pFunctionIDs->end(); iter++)
    {
        if ((*iter).m_nNumber == nNumber)
            return true;
    }
    return false;
}

/** \brief calculates the maximum function ID
 *  \param pFEInterface the interface to test
 *  \return the maximum function ID for this interface
 *
 * Theoretically the max opcode count is the number of its operations. This can be
 * calculated calling CFEInterface::GetOperationCount(). This estimate can be wrong
 * if the interface contains functions with uuid's, which are bigger than the
 * operation count.
 */
int CBEClass::GetMaxOpcodeNumber(CFEInterface *pFEInterface)
{
    int nMax = pFEInterface->GetOperationCount(false);
    // now check operations
    vector<CFEOperation*>::iterator iter = pFEInterface->GetFirstOperation();
    CFEOperation *pFEOperation;
    while ((pFEOperation = pFEInterface->GetNextOperation(iter)) != 0)
    {
        // if this function has uuid, we check if uuid is bigger than count
        if (!HasUuid(pFEOperation))
            continue;
        // now check
        int nUuid = GetUuid(pFEOperation);
        if (nUuid > nMax)
            nMax = nUuid;
    }
    return nMax;
}

/** \brief returns the functions Uuid
 *  \param pFEOperation the operation to get the UUID from
 *  \return the Uuid or -1 if none exists
 */
int CBEClass::GetUuid(CFEOperation *pFEOperation)
{
    CFEAttribute *pUuidAttr = pFEOperation->FindAttribute(ATTR_UUID);
    if (pUuidAttr)
    {
        if (dynamic_cast<CFEIntAttribute*>(pUuidAttr))
        {
            return ((CFEIntAttribute*)pUuidAttr)->GetIntValue();
        }
    }
    return -1;
}

/** \brief checks if this functio has a Uuid
 *  \param pFEOperation the operation to check
 *  \return true if this operation has a Uuid
 */
bool CBEClass::HasUuid(CFEOperation *pFEOperation)
{
    CFEAttribute *pUuidAttr = pFEOperation->FindAttribute(ATTR_UUID);
    if (pUuidAttr)
    {
        if (dynamic_cast<CFEIntAttribute*>(pUuidAttr))
        {
            return true;
        }
    }
    return false;
}

/** \brief calculates the number used as base opcode number
 *  \param pFEInterface the interface to check.
 *  \return the interface number
 *
 * AN interface number is first of all it's uuid. If it is not available
 * the base interfaces are counted and this interface's number is the
 * highest number of it's base interfaces + 1.
 */
int CBEClass::GetInterfaceNumber(CFEInterface *pFEInterface)
{
    assert(pFEInterface);

    CFEAttribute *pUuidAttr = pFEInterface->FindAttribute(ATTR_UUID);
    if (pUuidAttr)
    {
        if (dynamic_cast<CFEIntAttribute*>(pUuidAttr))
        {
            return ((CFEIntAttribute*)pUuidAttr)->GetIntValue();
        }
    }

    vector<CFEInterface*>::iterator iterI = pFEInterface->GetFirstBaseInterface();
    CFEInterface *pBaseInterface;
    int nNumber = 1;
    while ((pBaseInterface = pFEInterface->GetNextBaseInterface(iterI)) != 0)
    {
        int nBaseNumber = GetInterfaceNumber(pBaseInterface);
        if (nBaseNumber >= nNumber)
            nNumber = nBaseNumber+1;
    }
    return nNumber;
}

/** \brief searches all interfaces with the same interface number
 *  \param pFEInterface its base interfaces are searched
 *  \param nNumber the number to compare with
 *  \param pCollection the vector to add the same interfaces to
 *
 * We only search the base interfaces, because if we would search derived interfaces as well, we might
 * produce different client bindings if a base interface is suddenly derived from. Instead, we generate
 * errors in the derived interface, that function id's might overlap.
 */
int
CBEClass::FindInterfaceWithNumber(CFEInterface *pFEInterface,
    int nNumber,
    vector<CFEInterface*> *pCollection)
{
    assert(pCollection);
    int nCount = 0;
    // search base interfaces
    vector<CFEInterface*>::iterator iter = pFEInterface->GetFirstBaseInterface();
    CFEInterface *pFEBaseInterface;
    while ((pFEBaseInterface = pFEInterface->GetNextBaseInterface(iter)) != 0)
    {
        if (GetInterfaceNumber(pFEBaseInterface) == nNumber)
        {
            // check if we already got this interface (use pointer)
            vector<CFEInterface*>::const_iterator iterI = pCollection->begin();
            for (; iterI != pCollection->end(); iterI++)
            {
                if (*iterI == pFEBaseInterface)
                    break; // stops for(iterI) loop
            }
            if (iterI == pCollection->end()) // no match found
                pCollection->push_back(pFEBaseInterface);
            nCount++;
        }
        nCount += FindInterfaceWithNumber(pFEBaseInterface, nNumber, pCollection);
    }
    return nCount;
}

/** \brief find predefined function IDs
 *  \param pCollection the vector containing the interfaces to test
 *  \param pNumbers the array containing the numbers
 *  \param pContext the context of this operation
 *  \return number of predefined function IDs
 *
 * To find predefined function id'swe have to iterate over the interface's function, check
 * if they have a UUID attribute and get it's number. If it has a number we extend the
 * pNumber array and add this number at the correct position (the array is ordered). If the
 * number exists already, we print a warning containing both function's names and the
 * function ID.
 */
int
CBEClass::FindPredefinedNumbers(vector<CFEInterface*> *pCollection,
    vector<CPredefinedFunctionID> *pNumbers,
    CBEContext *pContext)
{
    assert (pCollection);
    assert (pNumbers);
    int nCount = 0;
    // iterate over interfaces with same interface number
    vector<CFEInterface*>::iterator iterI = pCollection->begin();
    CFEInterface *pFEInterface;
    CPredefinedFunctionID id;
    while (iterI != pCollection->end())
    {
        pFEInterface = *iterI++;
        if (!pFEInterface)
            continue;
        // iterate over current interface's operations
        vector<CFEOperation*>::iterator iterO = pFEInterface->GetFirstOperation();
        CFEOperation *pFEOperation;
        while ((pFEOperation = pFEInterface->GetNextOperation(iterO)) != 0)
        {
            // check if operation has Uuid attribute
            if (HasUuid(pFEOperation))
            {
                int nOpNumber = GetUuid(pFEOperation);
                // check if this number is already defined somewhere
                vector<CPredefinedFunctionID>::iterator iter;
                for (iter = pNumbers->begin(); iter != pNumbers->end(); iter++)
                {
                    if (nOpNumber == (*iter).m_nNumber)
                    {
                        if (pContext->IsWarningSet(PROGRAM_WARNING_IGNORE_DUPLICATE_FID))
                        {
                            CCompiler::GccWarning(pFEOperation, 0,
                                "Function \"%s\" has same Uuid (%d) as function \"%s\"",
                                pFEOperation->GetName().c_str(), nOpNumber,
                                (*iter).m_sName.c_str());
                            break;
                        }
                        else
                        {
                            CCompiler::GccError(pFEOperation, 0,
                                "Function \"%s\" has same Uuid (%d) as function \"%s\"",
                                pFEOperation->GetName().c_str(), nOpNumber,
                                (*iter).m_sName.c_str());
                            exit(1);
                        }
                    }
                }
                // check if this number might collide with base interface numbers
                CheckOpcodeCollision(pFEInterface, nOpNumber, pCollection, pFEOperation, pContext);
                // add uuid attribute to list
                id.m_sName = pFEOperation->GetName();
                id.m_nNumber = nOpNumber;
                pNumbers->push_back(id);
                // incremenet count
                nCount++;
            }
        }
    }
    return nCount;
}

/** \brief checks if the opcode could be used by base interfaces
 *  \param pFEInterface the currently investigated interface
 *  \param nOpNumber the opcode to check for
 *  \param pCollection the collection of predefined function ids
 *  \param pFEOperation a reference to the currently tested function
 *  \param pContext contains the current run-time context
 */
int
CBEClass::CheckOpcodeCollision(CFEInterface *pFEInterface,
    int nOpNumber,
    vector<CFEInterface*> *pCollection,
    CFEOperation *pFEOperation,
    CBEContext *pContext)
{
    vector<CFEInterface*>::iterator iterI = pFEInterface->GetFirstBaseInterface();
    CFEInterface *pFEBaseInterface;
    int nBaseNumber = 0;
    while ((pFEBaseInterface = pFEInterface->GetNextBaseInterface(iterI)) != 0)
    {
        // test current base interface only if numbers are identical
        if (GetInterfaceNumber(pFEInterface) != GetInterfaceNumber(pFEBaseInterface))
            continue;
        // first check the base interface's base interfaces
        nBaseNumber += CheckOpcodeCollision(pFEBaseInterface, nOpNumber, pCollection, pFEOperation, pContext);
        // now check interface's range
        nBaseNumber += GetMaxOpcodeNumber(pFEBaseInterface);
        if ((nOpNumber > 0) && (nOpNumber <= nBaseNumber))
        {
            if (pContext->IsWarningSet(PROGRAM_WARNING_IGNORE_DUPLICATE_FID))
            {
                CCompiler::GccWarning(pFEOperation, 0, "Function \"%s\" has Uuid (%d) which is used by compiler for base interface \"%s\"",
                                      pFEOperation->GetName().c_str(), nOpNumber, pFEBaseInterface->GetName().c_str());
                break;
            }
            else
            {
                CCompiler::GccError(pFEOperation, 0, "Function \"%s\" has Uuid (%d) which is used by compiler for interface \"%s\"",
                                    pFEOperation->GetName().c_str(), nOpNumber, pFEBaseInterface->GetName().c_str());
                exit(1);
            }
        }
    }
    // return checked range
    return nBaseNumber;
}

/** \brief tries to find the function group for a specific function
 *  \param pFunction the function to search for
 *  \return a reference to the function group or 0
 */
CFunctionGroup* CBEClass::FindFunctionGroup(CBEFunction *pFunction)
{
    // iterate over function groups
    vector<CFunctionGroup*>::iterator iter = GetFirstFunctionGroup();
    CFunctionGroup *pFunctionGroup;
    while ((pFunctionGroup = GetNextFunctionGroup(iter)) != 0)
    {
        // iterate over its functions
        vector<CBEFunction*>::iterator iterF = pFunctionGroup->GetFirstFunction();
        CBEFunction *pGroupFunction;
        while ((pGroupFunction = pFunctionGroup->GetNextFunction(iterF)) != 0)
        {
            if (pGroupFunction == pFunction)
                return pFunctionGroup;
        }
    }
    return 0;
}

/** \brief tries to find a type definition
 *  \param sTypeName the name of the searched type
 *  \return a reference to the type definition
 *
 * We also have to check the message buffer type (if existent).
 */
CBETypedDeclarator* CBEClass::FindTypedef(string sTypeName)
{
    vector<CBETypedDeclarator*>::iterator iter = GetFirstTypedef();
    CBETypedDeclarator *pTypedef;
    while ((pTypedef = GetNextTypedef(iter)) != 0)
    {
        if (pTypedef->FindDeclarator(sTypeName))
            return pTypedef;
        if (pTypedef->GetType())
            if (pTypedef->GetType()->HasTag(sTypeName))
                return pTypedef;
    }
    if (m_pMsgBuffer)
    {
        if (m_pMsgBuffer->FindDeclarator(sTypeName))
            return m_pMsgBuffer;
        if (m_pMsgBuffer->GetType())
            if (m_pMsgBuffer->GetType()->HasTag(sTypeName))
                return m_pMsgBuffer;
    }
    return 0;
}

/** \brief adds a type declaration to the class
 *  \param pTypeDecl the new type declaration
 */
void CBEClass::AddTypeDeclaration(CBETypedDeclarator *pTypeDecl)
{
    if (!pTypeDecl)
        return;
    m_vTypedefs.push_back(pTypeDecl);
}

/** \brief removes a type declaration from the class
 *  \param pTypeDecl the type declaration to remove
 */
void CBEClass::RemoveTypeDeclaration(CBETypedDeclarator *pTypeDecl)
{
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = m_vTypedefs.begin(); iter != m_vTypedefs.end(); iter++)
    {
        if (*iter == pTypeDecl)
        {
            m_vTypedefs.erase(iter);
            return;
        }
    }
}

/** \brief test if this class belongs to the file
 *  \param pFile the file to test
 *  \return true if the given file is a target file for the class
 *
 * A file is a target file for the class if its the target file for at least
 * one function.
 */
bool CBEClass::IsTargetFile(CBEImplementationFile * pFile)
{
    vector<CBEFunction*>::iterator iter = GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextFunction(iter)) != 0)
    {
        if (pFunction->IsTargetFile(pFile))
            return true;
    }
    return false;
}

/** \brief test if this class belongs to the file
 *  \param pFile the file to test
 *  \return true if the given file is a target file for the class
 *
 * A file is a target file for the class if its teh target class for at least one function.
 */
bool CBEClass::IsTargetFile(CBEHeaderFile * pFile)
{
    vector<CBEFunction*>::iterator iter = GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextFunction(iter)) != 0)
    {
        if (pFunction->IsTargetFile(pFile))
            return true;
    }
    return false;
}

/** \brief searches for a type using its tag
 *  \param nType the type of the searched type
 *  \param sTag the tag to search for
 *  \return a reference to the type
 */
CBEType* CBEClass::FindTaggedType(int nType, string sTag)
{
    vector<CBEType*>::iterator iter = GetFirstTaggedType();
    CBEType *pTypeDecl;
    while ((pTypeDecl = GetNextTaggedType(iter)) != 0)
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
    return 0;
}

/** \brief adds a tagged type declaration
 *  \param pType the type to add
 */
void CBEClass::AddTaggedType(CBEType *pType)
{
    if (!pType)
        return;
    m_vTypeDeclarations.push_back(pType);
}

/** \brief removes a tagged type declaration
 *  \param pType the type to remove
 */
void CBEClass::RemoveTaggedType(CBEType *pType)
{
    vector<CBEType*>::iterator iter;
    for (iter = m_vTypeDeclarations.begin(); iter != m_vTypeDeclarations.end(); iter++)
    {
        if (*iter == pType)
        {
            m_vTypeDeclarations.erase(iter);
            return;
        }
    }
}

/** \brief tries to find a pointer to the first tagged type declaration
 *  \return a pointer to the first tagged type declaration
 */
vector<CBEType*>::iterator CBEClass::GetFirstTaggedType()
{
    return m_vTypeDeclarations.begin();
}

/** \brief tries to retrieve a reference to the next type declaration
 *  \param iter the pointer to the next type declaration
 *  \return a reference to the next type declaration
 */
CBEType* CBEClass::GetNextTaggedType(vector<CBEType*>::iterator &iter)
{
    if (iter == m_vTypeDeclarations.end())
        return 0;
    return *iter++;
}

/** \brief tries to create a new back-end representation of a tagged type declaration
 *  \param pFEType the respective front-end type
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CBEClass::CreateBackEnd(CFEConstructedType *pFEType, CBEContext *pContext)
{
    CBEType *pType = pContext->GetClassFactory()->GetNewType(pFEType->GetType());
    AddTaggedType(pType);
    pType->SetParent(this);
    if (!pType->CreateBackEnd(pFEType, pContext))
    {
        RemoveTaggedType(pType);
        delete pType;
        VERBOSE("%s failed because tagged type could not be created\n",
            __PRETTY_FUNCTION__);
        return false;
    }
    return true;
}

/** \brief writes a tagged type declaration
 *  \param pType the type to write
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEClass::WriteTaggedType(CBEType *pType,
    CBEHeaderFile *pFile,
    CBEContext *pContext)
{
    assert(pType);
    // get tag
    string sTag;
    if (dynamic_cast<CBEStructType*>(pType))
        sTag = ((CBEStructType*)pType)->GetTag();
    if (dynamic_cast<CBEUnionType*>(pType))
        sTag = ((CBEUnionType*)pType)->GetTag();
    sTag = pContext->GetNameFactory()->GetTypeDefine(sTag, pContext);
    pFile->Print("#ifndef %s\n", sTag.c_str());
    pFile->Print("#define %s\n", sTag.c_str());
    pType->Write(pFile, pContext);
    pFile->Print(";\n");
    pFile->Print("#endif /* !%s */\n", sTag.c_str());
    pFile->Print("\n");
}

/** \brief searches for a function with the given type
 *  \param sTypeName the name of the type to look for
 *  \param pFile the file to write to (its used to test if a function shall be written)
 *  \param pContext the context of the write operation
 *  \return true if a parameter of that type is found
 *
 * Search functions for a parameter with that type.
 */
bool CBEClass::HasFunctionWithUserType(string sTypeName, CBEFile *pFile, CBEContext *pContext)
{
    vector<CBEFunction*>::iterator iter = GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextFunction(iter)) != 0)
    {
        if (dynamic_cast<CBEHeaderFile*>(pFile) &&
            pFunction->DoWriteFunction((CBEHeaderFile*)pFile, pContext) &&
            pFunction->FindParameterType(sTypeName))
            return true;
        if (dynamic_cast<CBEImplementationFile*>(pFile) &&
            pFunction->DoWriteFunction((CBEImplementationFile*)pFile, pContext) &&
            pFunction->FindParameterType(sTypeName))
            return true;
    }
    return false;
}

/** \brief retrieves a reference to the class' message buffer type
 *  \return a reference to the message buffer type member
 */
CBEMsgBufferType* CBEClass::GetMessageBuffer()
{
    return m_pMsgBuffer;
}

/** \brief adds a base name to the base name array
 *  \param sName the name to add
 */
void CBEClass::AddBaseName(string sName)
{
    m_vBaseNames.push_back(sName);
}

/** \brief count parameters according to their set and not set attributes
 *  \param nMustAttrs the attribute that must be set to count a parameter
 *  \param nMustNotAttrs the attribute that must not be set to count a parameter
 *  \param nDirection the direction to count
 *  \return the number of parameter with or without the specified attributes
 */
int CBEClass::GetParameterCount(int nMustAttrs, int nMustNotAttrs, int nDirection)
{
    if (nDirection == 0)
    {
        int nCountIn = GetParameterCount(nMustAttrs, nMustNotAttrs, DIRECTION_IN);
        int nCountOut = GetParameterCount(nMustAttrs, nMustNotAttrs, DIRECTION_OUT);
        return MAX(nCountIn, nCountOut);
    }

    int nCount = 0, nCurr;
    vector<CFunctionGroup*>::iterator iterG = GetFirstFunctionGroup();
    CFunctionGroup *pFuncGroup;
    while ((pFuncGroup = GetNextFunctionGroup(iterG)) != 0)
    {
        vector<CBEFunction*>::iterator iterF = pFuncGroup->GetFirstFunction();
        CBEOperationFunction *pFunc;
        while ((pFunc = dynamic_cast<CBEOperationFunction*>(pFuncGroup->GetNextFunction(iterF))) != 0)
        {
            nCurr = pFunc->GetParameterCount(nMustAttrs, nMustNotAttrs, nDirection);
            nCount = MAX(nCount, nCurr);
        }
    }

    return nCount;
}

/** \brief try to find functions with parameters with the given attributes
 *  \param nAttribute1 the first attribute
 *  \param nAttribute2 the second attribute
 *  \param nAttribute3 the third attribute
 *  \return true if such function exists
 */
bool CBEClass::HasParametersWithAttribute(int nAttribute1, int nAttribute2, int nAttribute3)
{
    // check own functions
    vector<CBEFunction*>::iterator iter = GetFirstFunction();
    CBEFunction *pFunction;
    CBETypedDeclarator *pParameter;
    while ((pFunction = GetNextFunction(iter)) != 0)
    {
        if ((pParameter = pFunction->FindParameterAttribute(nAttribute1)) != 0)
        {
            if (nAttribute2 == ATTR_NONE)
                return true;
            if (pParameter->FindAttribute(nAttribute2))
            {
                if (nAttribute3 == ATTR_NONE)
                    return true;
                if (pParameter->FindAttribute(nAttribute3))
                    return true;
            }
        }
    }
    // check base classes
    vector<CBEClass*>::iterator iterC = GetFirstBaseClass();
    CBEClass *pClass;
    while ((pClass = GetNextBaseClass(iterC)) != 0)
    {
        if (pClass->HasParametersWithAttribute(nAttribute1, nAttribute2, nAttribute3))
            return true;
    }
    // nothing found
    return false;
}

/** \brief creates a list of ordered elements
 *
 * This method iterates each member vector and inserts their
 * elements into the ordered element list using bubble sort.
 * Sort criteria is the source line number.
 */
void CBEClass::CreateOrderedElementList(void)
{
    // clear vector
    m_vOrderedElements.erase(m_vOrderedElements.begin(), m_vOrderedElements.end());
    // typedef
    vector<CBEFunction*>::iterator iterF = GetFirstFunction();
    CBEFunction *pF;
    while ((pF = GetNextFunction(iterF)) != NULL)
    {
        InsertOrderedElement(pF);
    }
    // typedef
    vector<CBETypedDeclarator*>::iterator iterT = GetFirstTypedef();
    CBETypedDeclarator *pT;
    while ((pT = GetNextTypedef(iterT)) != NULL)
    {
        InsertOrderedElement(pT);
    }
    // tagged types
    vector<CBEType*>::iterator iterTa = GetFirstTaggedType();
    CBEType* pTa;
    while ((pTa = GetNextTaggedType(iterTa)) != NULL)
    {
        InsertOrderedElement(pTa);
    }
    // consts
    vector<CBEConstant*>::iterator iterC = GetFirstConstant();
    CBEConstant *pC;
    while ((pC = GetNextConstant(iterC)) != NULL)
    {
        InsertOrderedElement(pC);
    }
}

/** \brief insert one element into the ordered list
 *  \param pObj the new element
 *
 * This is the insert implementation
 */
void CBEClass::InsertOrderedElement(CObject *pObj)
{
    // get source line number
    int nLine = pObj->GetSourceLine();
    // search for element with larger number
    vector<CObject*>::iterator iter = m_vOrderedElements.begin();
    for (; iter != m_vOrderedElements.end(); iter++)
    {
        if ((*iter)->GetSourceLine() > nLine)
        {
//             TRACE("Insert element from %s:%d before element from %s:%d\n",
//                 pObj->GetSourceFileName().c_str(), pObj->GetSourceLine(),
//                 (*iter)->GetSourceFileName().c_str(),
//                 (*iter)->GetSourceLine());
            // insert before that element
            m_vOrderedElements.insert(iter, pObj);
            return;
        }
    }
    // new object is bigger that all existing
//     TRACE("Insert element from %s:%d at end\n",
//         pObj->GetSourceFileName().c_str(), pObj->GetSourceLine());
    m_vOrderedElements.push_back(pObj);
}

/** \brief write helper functions, if any
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEClass::WriteHelperFunctions(CBEHeaderFile *pFile,
    CBEContext *pContext)
{
}

/** \brief write helper functions, if any
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEClass::WriteHelperFunctions(CBEImplementationFile *pFile,
    CBEContext *pContext)
{
}
