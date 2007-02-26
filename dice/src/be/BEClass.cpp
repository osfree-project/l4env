/**
 *	\file	dice/src/be/BEClass.cpp
 *	\brief	contains the implementation of the class CBEClass
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

#include "be/BEClass.h"

#include "be/BEFunction.h"
#include "be/BEOperationFunction.h"
#include "be/BEConstant.h"
#include "be/BETypedef.h"
#include "be/BEEnumType.h"
#include "be/BEMsgBufferType.h"
#include "be/BEAttribute.h"
#include "be/BEContext.h"
#include "be/BERoot.h"
#include "be/BEComponent.h"
#include "be/BETestsuite.h"
#include "be/BETestMainFunction.h"
#include "be/BEOperationFunction.h"
#include "be/BEInterfaceFunction.h"
#include "be/BECallFunction.h"
#include "be/BEUnmarshalFunction.h"
#include "be/BEReplyWaitFunction.h"
#include "be/BEReplyRcvFunction.h"
#include "be/BEComponentFunction.h"
#include "be/BETestFunction.h"
#include "be/BESndFunction.h"
#include "be/BEWaitFunction.h"
#include "be/BERcvFunction.h"
#include "be/BEWaitAnyFunction.h"
#include "be/BERcvAnyFunction.h"
#include "be/BEReplyAnyWaitAnyFunction.h"
#include "be/BESrvLoopFunction.h"
#include "be/BETestServerFunction.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEClient.h"
#include "be/BEOpcodeType.h"
#include "be/BEExpression.h"
#include "be/BENameSpace.h"
#include "be/BEDeclarator.h"

#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FEOperation.h"
#include "fe/FEUnaryExpression.h"
#include "fe/FEIntAttribute.h"
#include "fe/FEConstructedType.h"

// CFunctionGroup IMPLEMENTATION
IMPLEMENT_DYNAMIC(CFunctionGroup);

CFunctionGroup::CFunctionGroup(CFEOperation *pFEOperation)
: m_vFunctions(RUNTIME_CLASS(CBEFunction))
{
    IMPLEMENT_DYNAMIC_BASE(CFunctionGroup, CObject);
    m_pFEOperation = pFEOperation;
}

/** \brief destroys a function group object
 *
 * This doesn't really do anything: The String object cleans up itself, and
 * the Vector contains references to objects we do not want to delete. What we have
 * to remove are the vector-elements.
 */
CFunctionGroup::~CFunctionGroup()
{
    while (m_vFunctions.GetFirst()) m_vFunctions.RemoveFirst();
}

/** \brief retrieves the name of the group
 *  \return the name of the group
 */
String CFunctionGroup::GetName()
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
    m_vFunctions.Add(pFunction);
}

/** \brief returns a pointer to the first function in the group
 *  \return a pointer to the first function in the group
 */
VectorElement *CFunctionGroup::GetFirstFunction()
{
    return m_vFunctions.GetFirst();
}

/** \brief returns a reference to the next function in the group
 *  \param pIter the iterator pointing to the next function
 *  \return a reference to the next function
 */
CBEFunction *CFunctionGroup::GetNextFunction(VectorElement *& pIter)
{
    if (!pIter)
        return 0;
    CBEFunction *pRet = (CBEFunction*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextFunction(pIter);
    return pRet;
}


// CBEClass IMPLEMENTATION
IMPLEMENT_DYNAMIC(CBEClass);

CBEClass::CBEClass()
: m_vFunctions(RUNTIME_CLASS(CBEFunction)),
  m_vConstants(RUNTIME_CLASS(CBEConstant)),
  m_vTypedefs(RUNTIME_CLASS(CBETypedDeclarator)),
  m_vTypeDeclarations(RUNTIME_CLASS(CBEType)),
  m_vAttributes(RUNTIME_CLASS(CBEAttribute)),
  m_vBaseClasses(RUNTIME_CLASS(CBEClass)),
  m_vFunctionGroups(RUNTIME_CLASS(CFunctionGroup))
{
    m_pMsgBuffer = 0;
    m_sBaseNames = 0;
    m_nBaseNameSize = 0;
    IMPLEMENT_DYNAMIC_BASE(CBEClass, CBEObject);
}

/**	\brief destructor of target class */
CBEClass::~CBEClass()
{
    m_vFunctions.DeleteAll();
    m_vConstants.DeleteAll();
    m_vTypedefs.DeleteAll();
    m_vTypeDeclarations.DeleteAll();
    m_vAttributes.DeleteAll();
    m_vFunctionGroups.DeleteAll();
    if (m_pMsgBuffer)
        delete m_pMsgBuffer;
    for (int i=0; i<m_nBaseNameSize; i++)
        delete m_sBaseNames[i];
    free(m_sBaseNames);
}

/** \brief returns the name of the class
 *  \return the name of the class
 */
String CBEClass::GetName()
{
    return m_sName;
}

/**	\brief adds a new function to the functions vector
 *	\param pFunction the function to add
 */
void CBEClass::AddFunction(CBEFunction * pFunction)
{
    m_vFunctions.Add(pFunction);
    pFunction->SetParent(this);
}

/**	\brief removes a function from the functions vector
 *	\param pFunction the function to remove
 */
void CBEClass::RemoveFunction(CBEFunction * pFunction)
{
    m_vFunctions.Remove(pFunction);
}

/**	\brief retrieves a pointer to the first function
 *	\return a pointer to the first function
 */
VectorElement *CBEClass::GetFirstFunction()
{
    return m_vFunctions.GetFirst();
}

/**	\brief retrieves reference to the next function
 *	\param pIter the pointer to the next function
 *	\return a reference to the next function
 */
CBEFunction *CBEClass::GetNextFunction(VectorElement * &pIter)
{
    if (!pIter)
        return 0;
    CBEFunction *pRet = (CBEFunction *) pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextFunction(pIter);
    return pRet;
}

/** \brief adds an attribute
 *  \param pAttribute the attribute to add
 */
void CBEClass::AddAttribute(CBEAttribute *pAttribute)
{
    m_vAttributes.Add(pAttribute);
    pAttribute->SetParent(this);
}

/** \brief removes an attribute
 *  \param pAttribute the attribute to remove
 */
void CBEClass::RemoveAttribute(CBEAttribute *pAttribute)
{
    m_vAttributes.Remove(pAttribute);
}

/** \brief retrieves a pointer to the first attribute
 *  \return a pointer to the first attribute
 */
VectorElement* CBEClass::GetFirstAttribute()
{
    return m_vAttributes.GetFirst();
}

/** \brief retrieves a reference to the next attribute
 *  \param pIter the pointer to the next attribute
 *  \return a reference to the next attribute
 */
CBEAttribute* CBEClass::GetNextAttribute(VectorElement *&pIter)
{
    if (!pIter)
        return 0;
    CBEAttribute *pRet = (CBEAttribute*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextAttribute(pIter);
    return pRet;
}

/** \brief adds a new base Class
 *  \param pClass the Class to add
 *
 * Do not set parent, because we are not parent of this Class.
 */
void CBEClass::AddBaseClass(CBEClass *pClass)
{
    m_vBaseClasses.Add(pClass);
}

/** \brief removes a base Class
 *  \param pClass the Class to remove
 */
void CBEClass::RemoveBaseClass(CBEClass *pClass)
{
    m_vBaseClasses.Remove(pClass);
}

/** \brief retrieves a pointer to the first base Class
 *  \return a pointer to the first base Class
 *
 * If m_vBaseClasses is empty and m_nBaseNameSize is bigger than 0,
 * we have to add the references to the base classes first.
 */
VectorElement* CBEClass::GetFirstBaseClass()
{
    if ((m_vBaseClasses.GetSize() == 0) && (m_nBaseNameSize > 0))
    {
        CBERoot *pRoot = GetRoot();
        ASSERT(pRoot);
        for (int i=0; i<m_nBaseNameSize; i++)
        {
            // locate Class
            // if we cannot find class it is not there, because this should be called
            // way after all classes are created
            CBEClass *pBaseClass = pRoot->FindClass(*(m_sBaseNames[i]));
            if (!pBaseClass)
            {
                CCompiler::Warning("CBEClass::GetFirstBaseClass failed because base class \"%s\" cannot be found\n",
                    (const char*)(*(m_sBaseNames[i])));
                return NULL;
            }
            AddBaseClass(pBaseClass);
        }
    }
    return m_vBaseClasses.GetFirst();
}

/** \brief retrieves a pointer to the next base Class
 *  \param pIter a pointer to the next base Class
 *  \return a reference to the next base Class
 */
CBEClass* CBEClass::GetNextBaseClass(VectorElement*&pIter)
{
    if (!pIter)
        return 0;
    CBEClass *pRet = (CBEClass*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextBaseClass(pIter);
    return pRet;
}

/** \brief adds another constant
 *  \param pConstant the const to add
 */
void CBEClass::AddConstant(CBEConstant *pConstant)
{
    m_vConstants.Add(pConstant);
    pConstant->SetParent(this);
}

/** \brief removes a constant
 *  \param pConstant the const to remove
 */
void CBEClass::RemoveConstant(CBEConstant *pConstant)
{
    m_vConstants.Remove(pConstant);
}

/** \brief get a pointer to the first constant
 *  \return a pointer to the first constant
 */
VectorElement* CBEClass::GetFirstConstant()
{
    return m_vConstants.GetFirst();
}

/** \brief gets a reference to the next constant
 *  \param pIter the pointer to the next constant
 *  \return a reference to the next constant
 */
CBEConstant* CBEClass::GetNextConstant(VectorElement*&pIter)
{
    if (!pIter)
        return 0;
    CBEConstant *pRet = (CBEConstant*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextConstant(pIter);
    return pRet;
}

/** \brief adds a new typedef
 *  \param pTypedef the typedef to add
 */
void CBEClass::AddTypedef(CBETypedef *pTypedef)
{
    m_vTypedefs.Add(pTypedef);
    pTypedef->SetParent(this);
}

/** \brief removes a typedef
 *  \param pTypedef the typedef to remove
 */
void CBEClass::RemoveTypedef(CBETypedef *pTypedef)
{
    m_vTypedefs.Remove(pTypedef);
}

/** \brief gets a pointer to the first typedef
 *  \return a pointer to the first typedef
 */
VectorElement* CBEClass::GetFirstTypedef()
{
    return m_vTypedefs.GetFirst();
}

/** \brief gets a reference to the next typedef
 *  \param pIter the pointer to the next typedef
 *  \return a reference to the next typedef
 */
CBETypedDeclarator* CBEClass::GetNextTypedef(VectorElement *&pIter)
{
    if (!pIter)
        return 0;
    CBETypedef *pRet = (CBETypedef*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextTypedef(pIter);
    return pRet;
}

/**	\brief creates the members of this class
 *	\param pFEInterface the front-end interface to use as source
 *	\param pContext the context of the code generation
 *  \return true if successful
 */
bool CBEClass::CreateBackEnd(CFEInterface * pFEInterface, CBEContext * pContext)
{
	// set target file name
	SetTargetFileName(pFEInterface, pContext);
	// set own name
    m_sName = pFEInterface->GetName();
    // add attributes
    VectorElement *pIter = pFEInterface->GetFirstAttribute();
    CFEAttribute *pFEAttribute;
    while ((pFEAttribute = pFEInterface->GetNextAttribute(pIter)) != 0)
    {
        if (!CreateBackEnd(pFEAttribute, pContext))
            return false;
    }
    // we can resolve this if we only add the base names now, but when the
    // base classes are first used, add the actual references.
    // add references to base Classes
    CBERoot *pRoot = GetRoot();
    ASSERT(pRoot);
    pIter = pFEInterface->GetFirstBaseInterface();
    CFEInterface *pFEBaseInterface;
    while ((pFEBaseInterface = pFEInterface->GetNextBaseInterface(pIter)) != 0)
    {
        // add the name of the base interface
        AddBaseName(pFEBaseInterface->GetName());
    }
    // add constants
    pIter = pFEInterface->GetFirstConstant();
    CFEConstDeclarator *pFEConstant;
    while ((pFEConstant = pFEInterface->GetNextConstant(pIter)) != 0)
    {
        if (!CreateBackEnd(pFEConstant, pContext))
            return false;
    }
    // add typedefs
    pIter = pFEInterface->GetFirstTypeDef();
    CFETypedDeclarator *pFETypedef;
    while ((pFETypedef = pFEInterface->GetNextTypeDef(pIter)) != 0)
    {
        if (!CreateBackEnd(pFETypedef, pContext))
            return false;
    }
    // add tagged decls
    pIter = pFEInterface->GetFirstTaggedDecl();
    CFEConstructedType *pFETaggedType;
    while ((pFETaggedType = pFEInterface->GetNextTaggedDecl(pIter)) != 0)
    {
        if (!CreateBackEnd(pFETaggedType, pContext))
            return false;
    }
    // add types for Class
    // need msg buffer types
    m_pMsgBuffer = pContext->GetClassFactory()->GetNewMessageBufferType();
    m_pMsgBuffer->SetParent(this);
    if (!m_pMsgBuffer->CreateBackEnd(pFEInterface, pContext))
    {
        VERBOSE("CBEClass::CreateBackEnd failed because message buffer type could not be created\n");
        delete m_pMsgBuffer;
        return false;
    }
    // add functions
    pIter = pFEInterface->GetFirstOperation();
    CFEOperation *pFEOperation;
    while ((pFEOperation = pFEInterface->GetNextOperation(pIter)) != 0)
    {
        if (!CreateBackEnd(pFEOperation, pContext))
            return false;
    }
    // init message buffer sizes, since it needs initialized function for it
    // we simply call the create function again.
    m_pMsgBuffer->ZeroCounts();
    if (!m_pMsgBuffer->CreateBackEnd(pFEInterface, pContext))
    {
        VERBOSE("CBEClass::CreateBackEnd failed because message buffer type could not be created\n");
        delete m_pMsgBuffer;
        return false;
    }
    // add functions for interface
    CBEInterfaceFunction *pFunction = pContext->GetClassFactory()->GetNewWaitAnyFunction();
    AddFunction(pFunction);
    pFunction->SetComponentSide(true);
    if (!pFunction->CreateBackEnd(pFEInterface, pContext))
    {
        RemoveFunction(pFunction);
        VERBOSE("CBEClass::CreateBackEnd failed because wait-any function could not be created\n");
        delete pFunction;
        return false;
    }
    pFunction = pContext->GetClassFactory()->GetNewRcvAnyFunction();
    AddFunction(pFunction);
    pFunction->SetComponentSide(true);
    if (!pFunction->CreateBackEnd(pFEInterface, pContext))
    {
        RemoveFunction(pFunction);
        VERBOSE("CBEClass::CreateBackEnd failed because receive-any function could not be created\n");
        delete pFunction;
        return false;
    }
    pFunction = pContext->GetClassFactory()->GetNewReplyAnyWaitAnyFunction();
    AddFunction(pFunction);
    pFunction->SetComponentSide(true);
    if (!pFunction->CreateBackEnd(pFEInterface, pContext))
    {
        RemoveFunction(pFunction);
        VERBOSE("CBEClass::CreateBackEnd faile dbecause reply-any-wait-any function could not be created\n");
        delete pFunction;
        return false;
    }
    pFunction = pContext->GetClassFactory()->GetNewSrvLoopFunction();
    AddFunction(pFunction);
    pFunction->SetComponentSide(true);
    if (!pFunction->CreateBackEnd(pFEInterface, pContext))
    {
        RemoveFunction(pFunction);
        VERBOSE("CBEClass::CreateBackEnd failed because server-loop function could not be created\n");
        delete pFunction;
        return false;
    }

    if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
    {
        pFunction = pContext->GetClassFactory()->GetNewTestServerFunction();
        AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEInterface, pContext))
        {
            RemoveFunction(pFunction);
            VERBOSE("CBEClass::CreateBackEnd failed because test-server function could not be created\n");
            delete pFunction;
            return false;
        }
    }
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
        VERBOSE("CBEClass::CreateBackEnd failed because const %s could not be created\n",
                (const char*)pFEConstant->GetName());
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
    if (!pTypedef->CreateBackEnd(pFETypedef, pContext))
    {
        RemoveTypedef(pTypedef);
        VERBOSE("CBEClass::CreateBackEnd failed because typedef could not be created\n");
        delete pTypedef;
        return false;
    }
    return true;
}

/** \brief internal function to create the back-end functions
 *  \param pFEOperation the respective front-end function
 *  \param pContext the context of the create process
 *  \return true if successful
 *
 * A function has to be generated depending on its attributes. If it is a call function,
 * we have to generate different back-end function than for a message passing function.
 *
 * We depend on the fact, that either the [in] or the [out] attribute are specified.
 * Never both may appear.
 */
bool CBEClass::CreateBackEnd(CFEOperation *pFEOperation, CBEContext *pContext)
{
    CFunctionGroup *pGroup = new CFunctionGroup(pFEOperation);
    AddFunctionGroup(pGroup);

    if (!(pFEOperation->FindAttribute(ATTR_IN)) &&
        !(pFEOperation->FindAttribute(ATTR_OUT)))
    {
        // the call case:
        // we need the functions call, unmarshal, reply-and-wait, skeleton, reply-and-recv
        // for client side: call
        CBEOperationFunction *pFunction = pContext->GetClassFactory()->GetNewCallFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(false);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("CBEClass::CreateBackEnd failed, because call function could not be created for %s\n",
                    (const char*)pFEOperation->GetName());
            return false;
        }
        // for server side: unmarshal, reply-and-wait, reply-and-recv, skeleton
        pFunction = pContext->GetClassFactory()->GetNewUnmarshalFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(true);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("CBEClass::CreateBackEnd failed, because unmarshal function could not be created for %s\n",
                    (const char*)pFEOperation->GetName());
            return false;
        }

        pFunction = pContext->GetClassFactory()->GetNewReplyWaitFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(true);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("CBEClass::CreateBackEnd failed, because wait function could not be created for %s\n",
                    (const char*)pFEOperation->GetName());
            return false;
        }

        pFunction = pContext->GetClassFactory()->GetNewReplyRcvFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(true);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("CBEClass::CreateBackEnd failed, because reply-recv function could not be created for %s\n",
                    (const char*)pFEOperation->GetName());
            return false;
        }

        pFunction = pContext->GetClassFactory()->GetNewComponentFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(true);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("CBEClass::CreateBackEnd failed, because component function could not be created for %s\n",
                    (const char*)pFEOperation->GetName());
            return false;
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
                VERBOSE("CBEClass::CreateBackEnd failed because test function could not be created for %s\n",
                        (const char*)pFEOperation->GetName());
                return false;
            }
        }
    }
    else
    {
        // the MP case
        // we need the functions send, recv, wait, unmarshal
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
            VERBOSE("CBEClass::CreateBackEnd failed, because send function could not be created for %s\n",
                    (const char*)pFEOperation->GetName());
            return false;
        }

        // receiver: wait, recv, unmarshal
        pFunction = pContext->GetClassFactory()->GetNewWaitFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(!bComponent);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("CBEClass::CreateBackEnd failed, because wait function could not be created for %s\n",
                    (const char*)pFEOperation->GetName());
            return false;
        }

        pFunction = pContext->GetClassFactory()->GetNewRcvFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(!bComponent);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("CBEClass::CreateBackEnd failed because receive function could not be created for %s\n",
                    (const char*)pFEOperation->GetName());
            return false;
        }

        pFunction = pContext->GetClassFactory()->GetNewUnmarshalFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(!bComponent);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("CBEClass::CreateBackEnd failed because unmarshal function could not be created for %s\n",
                    (const char*)pFEOperation->GetName());
            return false;
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
                VERBOSE("CBEClass::CreateBackEnd failed because test function could not be created for %s\n",
                        (const char*)pFEOperation->GetName());
                return false;
            }
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
        VERBOSE("CBEClass::CreateBackEnd failed because attribute could not be created\n");
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
    // add this class to the file
    if (IsTargetFile(pHeader))
		pHeader->AddClass(this);
    return true;
}

/** \brief adds this class (or its members) to an implementation file
 *  \param pImpl the implementation file to add this class to
 *  \param pContext the context of this creation
 *  \return true if successful
 */
bool CBEClass::AddToFile(CBEImplementationFile *pImpl, CBEContext *pContext)
{
	// check compiler option
	if (pContext->IsOptionSet(PROGRAM_FILE_FUNCTION))
	{
		VectorElement *pIter = GetFirstFunction();
		CBEFunction *pFunction;
		while ((pFunction = GetNextFunction(pIter)) != 0)
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
    VectorElement *pIter = GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextFunction(pIter)) != 0)
    {
        nCurr = pFunction->GetParameterCount(nFEType, nDirection);
         if ((nCount > 0) && (nCurr != nCount) && (nCurr > 0))
            bSameCount = false;
        if (nCurr > nCount) nCount = nCurr;
    }

    return nCount;
}

/**	\brief counts the number of string parameter needed for this interface
 *	\param nDirection the direction to count
 *  \param nMustAttrs the attributes which have to be set for the parameters
 *  \param nMustNotAttrs the attributes which must not be set for the parameters
 *	\return the number of strings needed
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

    VectorElement *pIterG = GetFirstFunctionGroup();
    CFunctionGroup *pFuncGroup;
    while ((pFuncGroup = GetNextFunctionGroup(pIterG)) != 0)
    {
        VectorElement *pIterF = pFuncGroup->GetFirstFunction();
        CBEOperationFunction *pFunc;
        while ((pFunc = (CBEOperationFunction*)(pFuncGroup->GetNextFunction(pIterF))) != 0)
        {
            nCurr = pFunc->GetStringParameterCount(nDirection, nMustAttrs, nMustNotAttrs);
            nCount = (nCount > nCurr) ? nCount : nCurr;
        }
    }

    return nCount;
}

/**	\brief calculates the size of the interface function
 *	\param nDirection the direction to count
 *  \param pContext the context of the calculation (needed to test for message buffer)
 *	\return the number of bytes needed to transmit any of the functions
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

    VectorElement *pIter = GetFirstFunction();
    CBEOperationFunction *pFunc;
    while ((pFunc = (CBEOperationFunction*)GetNextFunction(pIter)) != 0)
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
CBEFunction* CBEClass::FindFunction(String sFunctionName)
{
    if (sFunctionName.IsEmpty())
        return 0;
    // simply scan the function for a match
    VectorElement *pIter = GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextFunction(pIter)) != 0)
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
    // first create classes in reverse order, so we can build correct parent relationships
    CBEConstant *pOpcode = pContext->GetClassFactory()->GetNewConstant();
    CBEOpcodeType *pType = pContext->GetClassFactory()->GetNewOpcodeType();
    pType->SetParent(pOpcode);
    CBEExpression *pBrace = pContext->GetClassFactory()->GetNewExpression();
    pBrace->SetParent(pOpcode);
    CBEExpression *pInterfaceCode = pContext->GetClassFactory()->GetNewExpression();
    pInterfaceCode->SetParent(pBrace);
    CBEExpression *pValue = pContext->GetClassFactory()->GetNewExpression();
    pValue->SetParent(pInterfaceCode);
    CBEExpression *pBits = pContext->GetClassFactory()->GetNewExpression();
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
    String sShift = pContext->GetNameFactory()->GetInterfaceNumberShiftConstant();
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
    if (!pInterfaceCode->CreateBackEndBinary(pValue, EXPR_LSHIFT, pBits, pContext))
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
    String sName = pContext->GetNameFactory()->GetOpcodeConst(this, pContext);
    // add const to file
    pFile->AddConstant(pOpcode);
    if (!pOpcode->CreateBackEnd(pType, sName, pBrace, pContext))
    {
        pFile->RemoveConstant(pOpcode);
        delete pOpcode;
        delete pValue;
        delete pType;
        return false;
    }

    // iterate over functions
    VectorElement *pIter = GetFirstFunctionGroup();
    CFunctionGroup *pFunctionGroup;
    while ((pFunctionGroup = GetNextFunctionGroup(pIter)) != 0)
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
    // first create classes, so we can build parent relationship correctly
    CBEConstant *pOpcode = pContext->GetClassFactory()->GetNewConstant();
    CBEOpcodeType *pType = pContext->GetClassFactory()->GetNewOpcodeType();
    pType->SetParent(pOpcode);
    CBEExpression *pValue = pContext->GetClassFactory()->GetNewExpression();
    pValue->SetParent(pOpcode);
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
    String sBase = pContext->GetNameFactory()->GetOpcodeConst(this, pContext);
    if (!pBase->CreateBackEnd(sBase, pContext))
    {
        delete pOpcode;
        delete pType;
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
        delete pValue;
        delete pNumber;
        delete pBase;
        delete pBrace;
        delete pFuncCode;
        delete pBitMask;
        return false;
    }
    // create bitmask
    String sBitMask =  pContext->GetNameFactory()->GetFunctionBitMaskConstant();
    if (!pBitMask->CreateBackEnd(sBitMask, pContext))
    {
        delete pOpcode;
        delete pType;
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
        delete pValue;
        delete pNumber;
        delete pBase;
        delete pBrace;
        delete pFuncCode;
        delete pBitMask;
        return false;
    }
    // create opcode name
    String sName = pContext->GetNameFactory()->GetOpcodeConst(pFEOperation, pContext);
    // create constant
    ((CBEHeaderFile *) pFile)->AddConstant(pOpcode);
    if (!pOpcode->CreateBackEnd(pType, sName, pValue, pContext))
    {
        ((CBEHeaderFile *) pFile)->RemoveConstant(pOpcode);
        delete pOpcode;
        delete pType;
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

    VectorElement *pIter = GetFirstBaseClass();
    CBEClass *pBaseClass;
    int nNumber = 1;
    while ((pBaseClass = GetNextBaseClass(pIter)) != 0)
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
        if (pParent->IsKindOf(RUNTIME_CLASS(CBENameSpace)))
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
    VectorElement *pIter = GetFirstAttribute();
    CBEAttribute *pAttr;
    while ((pAttr = GetNextAttribute(pIter)) != 0)
    {
        if (pAttr->GetType() == nAttrType)
            return pAttr;
    }
    return 0;
}

/** \brief tries to optimize this class according to the optimization level
 *  \param nLevel the optimization level
 *  \param pContext the context of the optimization
 *  \return success or failure code (zero is no error)
 *
 * Currently we only iterate over the functions of this class and try to optimize them.
 */
int CBEClass::Optimize(int nLevel, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextFunction(pIter)) != 0)
    {
        int nRet;
        if ((nRet = pFunction->Optimize(nLevel, pContext)) != 0)
            return nRet;
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
    WriteConstants(pFile, pContext);
    WriteTypedefs(pFile, pContext);
    WriteTaggedTypes(pFile, pContext);
    WriteFunctions(pFile, pContext);
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
    WriteFunctions(pFile, pContext);
}

/** \brief writes the constants to the header file
 *  \param pFile the header file to write them to
 *  \param pContext the context of the write operation
 */
void CBEClass::WriteConstants(CBEHeaderFile *pFile, CBEContext *pContext)
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
 *
 * Always write message buffer types at server side and at client's side
 * only if needed. The type is needed if any of the client's functions
 * has the message buffer type as a parameter type.
 */
void CBEClass::WriteTypedefs(CBEHeaderFile *pFile, CBEContext *pContext)
{
    bool bWroteTypedDecls = false;
    // write message buffer type seperately
    if (m_pMsgBuffer)
    {
        // test for client and if type is needed
        CBETarget *pTarget = pFile->GetTarget();
        if (!pTarget->IsKindOf(RUNTIME_CLASS(CBEClient)) ||
            pTarget->HasFunctionWithUserType(m_pMsgBuffer->GetAlias()->GetName(), pContext))
        {
            m_pMsgBuffer->WriteDefinition(pFile, true, pContext);
            pFile->Print("\n");
            bWroteTypedDecls = true;
        }
    }
    // write other typedefs
    VectorElement *pIter = GetFirstTypedef();
    CBETypedDeclarator *pTypedef;
    while ((pTypedef = GetNextTypedef(pIter)) != 0)
    {
        if (pTypedef->IsKindOf(RUNTIME_CLASS(CBETypedef)))
            ((CBETypedef*)pTypedef)->WriteDeclaration(pFile, pContext);
        else
        {
            pTypedef->WriteDeclaration(pFile, pContext);
            pFile->Print(";\n");
            bWroteTypedDecls = true;
        }
    }
    if (bWroteTypedDecls)
        pFile->Print("\n");
}

/** \brief write the functions to the header files
 *  \param pFile the header file to write to
 *  \param pContext the context of the write operation
 *
 * We only have to write the functions  for the respective communication side.
 *
 * We can determine this by checking the type of the function and the target of the file and
 * the directional attribute of the function.
 */
void CBEClass::WriteFunctions(CBEHeaderFile *pFile, CBEContext *pContext)
{
    ASSERT(pFile->GetTarget());
    VectorElement *pIter = GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextFunction(pIter)) != 0)
    {
        if (pFunction->DoWriteFunction(pFile, pContext))
        {
            pFunction->Write(pFile, pContext);
            pFile->Print("\n");
        }
    }
}

/** \brief writes the functions to the implementation file
 *  \param pFile the target implementation file
 *  \param pContext the context of the write operation
 *
 * Because the class has been added to the target file as whole, we have to
 * check the functions seperately. Since CBEFunction::IsTargetFile only checks a group
 * of files, according to compiler options (It checks if BE functions for
 * specific FE function belong to files -client.[ch] or -server.[ch]), we
 * cannot use this. Instead we use the function DoWriteFunction.
 */
void CBEClass::WriteFunctions(CBEImplementationFile *pFile, CBEContext *pContext)
{
    ASSERT(pFile->GetTarget());
    VectorElement *pIter = GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextFunction(pIter)) != 0)
    {
        if (pFunction->DoWriteFunction(pFile, pContext))
        {
            pFunction->Write(pFile, pContext);
            pFile->Print("\n");
        }
    }
}

/** \brief adds a new function group
 *  \param pGroup the group to add
 */
void CBEClass::AddFunctionGroup(CFunctionGroup *pGroup)
{
    m_vFunctionGroups.Add(pGroup);
}

/** \brief removes a function group
 *  \param pGroup the group to remove
 */
void CBEClass::RemoveFunctionGroup(CFunctionGroup *pGroup)
{
    m_vFunctionGroups.Remove(pGroup);
}

/** \brief retrieves a pointer to the first function group
 *  \return a pointer to the first function group
 */
VectorElement* CBEClass::GetFirstFunctionGroup()
{
    return m_vFunctionGroups.GetFirst();
}

/** \brief retrieves a reference to the next function group
 *  \param pIter the pointer to the next function group
 *  \return a reference to the next function group
 */
CFunctionGroup* CBEClass::GetNextFunctionGroup(VectorElement *& pIter)
{
    if (!pIter)
        return 0;
    CFunctionGroup *pRet = (CFunctionGroup*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextFunctionGroup(pIter);
    return pRet;
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
    CFEInterface *pFEInterface = pFEOperation->GetParentInterface();
    int nInterfaceNumber = GetInterfaceNumber(pFEInterface);
    // search for base interfaces with same interface number and collect them
    Vector vSameInterfaces(RUNTIME_CLASS(CFEInterface));
    FindInterfaceWithNumber(pFEInterface, nInterfaceNumber, &vSameInterfaces);
    // now search for predefined numbers
    Vector vFunctionIDs(RUNTIME_CLASS(CPredefinedFunctionID));
    vSameInterfaces.Add(pFEInterface); // add this interface too
    FindPredefinedNumbers(&vSameInterfaces, &vFunctionIDs, pContext);
    vSameInterfaces.Remove(pFEInterface);
    // check for Uuid attribute and return its value
    if (HasUuid(pFEOperation))
        return GetUuid(pFEOperation);
    // now there is no Uuid, thus we have to calculate the function id
    // get the maximum function ID of the interfaces with the same number
    int nFunctionID = 0;
    VectorElement *pIterI = vSameInterfaces.GetFirst();
    while (pIterI)
    {
        CFEInterface *pCurrI = (CFEInterface*)pIterI->GetElement();
        pIterI = pIterI->GetNext();
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
        nFunctionID += GetMaxOpcodeNumber(pCurrI, &vFunctionIDs);
    }
    // now iterate over functions, incrementing a counter, check if new value is predefined and
    // if the checked function is this function, then return the counter value
    VectorElement *pIter = pFEInterface->GetFirstOperation();
    CFEOperation *pCurrOp;
    while ((pCurrOp = pFEInterface->GetNextOperation(pIter)) != 0)
    {
        nFunctionID++;
        while (IsPredefinedID(&vFunctionIDs, nFunctionID)) nFunctionID++;
        if (pCurrOp == pFEOperation)
            return nFunctionID;
    }
    // something went wrong -> if we are here, the operations parent interface does not
    // have this operation as a child
    ASSERT(false);
    return 0;
}
/** \brief checks if a function ID is predefined
 *  \param pFunctionIDs the predefined IDs
 *  \param nNumber the number to test
 *  \return true if number is predefined
 */
bool CBEClass::IsPredefinedID(Vector *pFunctionIDs, int nNumber)
{
    VectorElement *pIter = pFunctionIDs->GetFirst();
    while (pIter)
    {
        CPredefinedFunctionID *pElement = (CPredefinedFunctionID*)pIter->GetElement();
        pIter = pIter->GetNext();
        if (!pElement)
            continue;
        if (pElement->m_nNumber == nNumber)
            return true;
    }
    return false;
}

/** \brief calculates the maximum function ID
 *  \param pFEInterface the interface to test
 *  \param pFunctionIDs the function IDs to skip
 *  \return the maximum function ID for this interface
 *
 * Theoretically the max opcode count is the number of its operations. This can be
 * calculated calling CFEInterface::GetOperationCount(). This estimate can be wrong
 * if the interface contains functions with uuid's, which are bigger than the
 * operation count.
 */
int CBEClass::GetMaxOpcodeNumber(CFEInterface *pFEInterface, Vector *pFunctionIDs)
{
    int nMax = pFEInterface->GetOperationCount(false);
    // now check operations
    VectorElement *pIter = pFEInterface->GetFirstOperation();
    CFEOperation *pFEOperation;
    while ((pFEOperation = pFEInterface->GetNextOperation(pIter)) != 0)
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
        if (pUuidAttr->IsKindOf(RUNTIME_CLASS(CFEIntAttribute)))
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
        if (pUuidAttr->IsKindOf(RUNTIME_CLASS(CFEIntAttribute)))
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
    ASSERT(pFEInterface);

    CFEAttribute *pUuidAttr = pFEInterface->FindAttribute(ATTR_UUID);
    if (pUuidAttr)
    {
        if (pUuidAttr->IsKindOf(RUNTIME_CLASS(CFEIntAttribute)))
        {
            return ((CFEIntAttribute*)pUuidAttr)->GetIntValue();
        }
    }

    VectorElement *pIter = pFEInterface->GetFirstBaseInterface();
    CFEInterface *pBaseInterface;
    int nNumber = 1;
    while ((pBaseInterface = pFEInterface->GetNextBaseInterface(pIter)) != 0)
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
int CBEClass::FindInterfaceWithNumber(CFEInterface *pFEInterface, int nNumber, Vector *pCollection)
{
    int nCount = 0;
    // search base interfaces
    VectorElement *pIter = pFEInterface->GetFirstBaseInterface();
    CFEInterface *pFEBaseInterface;
    while ((pFEBaseInterface = pFEInterface->GetNextBaseInterface(pIter)) != 0)
    {
        if (GetInterfaceNumber(pFEBaseInterface) == nNumber)
        {
            pCollection->AddUnique(pFEBaseInterface);
            nCount++;
        }
        nCount += FindInterfaceWithNumber(pFEBaseInterface, nNumber, pCollection);
    }
    return nCount;
}

/** \brief find predefined function IDs
 *  \param pCollection the Vector containing the interfaces to test
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
int CBEClass::FindPredefinedNumbers(Vector *pCollection, Vector *pNumbers, CBEContext *pContext)
{
    int nCount = 0;
    // iterate over interfaces with same interface number
    VectorElement *pIter = pCollection->GetFirst();
    CFEInterface *pFEInterface;
    while (pIter)
    {
        pFEInterface = (CFEInterface*)pIter->GetElement();
        pIter = pIter->GetNext();
        if (!pFEInterface)
            continue;
        // iterate over current interface's operations
        VectorElement *pIterO = pFEInterface->GetFirstOperation();
        CFEOperation *pFEOperation;
        while ((pFEOperation = pFEInterface->GetNextOperation(pIterO)) != 0)
        {
            // check if operation has Uuid attribute
            if (HasUuid(pFEOperation))
            {
                int nOpNumber = GetUuid(pFEOperation);
                // check if this number is already defined somewhere
                VectorElement *pIter = pNumbers->GetFirst();
                while (pIter)
                {
                    CPredefinedFunctionID *pElement = (CPredefinedFunctionID*)pIter->GetElement();
                    pIter = pIter->GetNext();
                    if (!pElement)
                        continue;
                    if (nOpNumber == pElement->m_nNumber)
                    {
                        if (pContext->IsWarningSet(PROGRAM_WARNING_IGNORE_DUPLICATE_FID))
                        {
                            CCompiler::GccWarning(pFEOperation, 0, "Function \"%s\" has same Uuid (%d) as function \"%s\"",
                                                  (const char*)pFEOperation->GetName(), nOpNumber, (const char*)pElement->m_sName);
                            break;
                        }
                        else
                        {
                            CCompiler::GccError(pFEOperation, 0, "Function \"%s\" has same Uuid (%d) as function \"%s\"",
                                                (const char*)pFEOperation->GetName(), nOpNumber, (const char*)pElement->m_sName);
                            exit(1);
                        }
                    }
                }
                // check if this number might collide with base interface numbers
                CheckOpcodeCollision(pFEInterface, nOpNumber, pCollection, pFEOperation, pContext);
                // add uuid attribute to list
                CPredefinedFunctionID *pNew = new CPredefinedFunctionID(pFEOperation->GetName(), nOpNumber);
                pNumbers->Add(pNew);
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
int CBEClass::CheckOpcodeCollision(CFEInterface *pFEInterface, int nOpNumber, Vector *pCollection, CFEOperation *pFEOperation, CBEContext *pContext)
{
    VectorElement *pIterI = pFEInterface->GetFirstBaseInterface();
    CFEInterface *pFEBaseInterface;
    int nBaseNumber = 0;
    while ((pFEBaseInterface = pFEInterface->GetNextBaseInterface(pIterI)) != 0)
    {
        // test current base interface only if numbers are identical
        if (GetInterfaceNumber(pFEInterface) != GetInterfaceNumber(pFEBaseInterface))
            continue;
        // first check the base interface's base interfaces
        nBaseNumber += CheckOpcodeCollision(pFEBaseInterface, nOpNumber, pCollection, pFEOperation, pContext);
        // now check interface's range
        nBaseNumber += GetMaxOpcodeNumber(pFEBaseInterface, pCollection);
        if ((nOpNumber > 0) && (nOpNumber <= nBaseNumber))
        {
            if (pContext->IsWarningSet(PROGRAM_WARNING_IGNORE_DUPLICATE_FID))
            {
                CCompiler::GccWarning(pFEOperation, 0, "Function \"%s\" has Uuid (%d) which is used by compiler for base interface \"%s\"",
                                      (const char*)pFEOperation->GetName(), nOpNumber, (const char*)pFEBaseInterface->GetName());
                break;
            }
            else
            {
                CCompiler::GccError(pFEOperation, 0, "Function \"%s\" has Uuid (%d) which is used by compiler for interface \"%s\"",
                                    (const char*)pFEOperation->GetName(), nOpNumber, (const char*)pFEBaseInterface->GetName());
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
    VectorElement *pIter = GetFirstFunctionGroup();
    CFunctionGroup *pFunctionGroup;
    while ((pFunctionGroup = GetNextFunctionGroup(pIter)) != 0)
    {
        // iterate over its functions
        VectorElement *pIterF = pFunctionGroup->GetFirstFunction();
        CBEFunction *pGroupFunction;
        while ((pGroupFunction = pFunctionGroup->GetNextFunction(pIterF)) != 0)
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
CBETypedDeclarator* CBEClass::FindTypedef(String sTypeName)
{
    VectorElement *pIter = GetFirstTypedef();
    CBETypedDeclarator *pTypedef;
    while ((pTypedef = GetNextTypedef(pIter)) != 0)
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
    m_vTypedefs.Add(pTypeDecl);
}

/** \brief removes a type declaration from the class
 *  \param pTypeDecl the type declaration to remove
 */
void CBEClass::RemoveTypeDeclaration(CBETypedDeclarator *pTypeDecl)
{
    m_vTypedefs.Remove(pTypeDecl);
}

/** \brief test if this class belongs to the file
 *  \param pFile the file to test
 *  \return true if the given file is a target file for the class
 *
 * A file is a target file for the class if its teh target class for at least one function.
 */
bool CBEClass::IsTargetFile(CBEImplementationFile * pFile)
{
	VectorElement *pIter = GetFirstFunction();
	CBEFunction *pFunction;
	while ((pFunction = GetNextFunction(pIter)) != 0)
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
	VectorElement *pIter = GetFirstFunction();
	CBEFunction *pFunction;
	while ((pFunction = GetNextFunction(pIter)) != 0)
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
CBEType* CBEClass::FindTaggedType(int nType, String sTag)
{
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
    return 0;
}

/** \brief adds a tagged type declaration
 *  \param pType the type to add
 */
void CBEClass::AddTaggedType(CBEType *pType)
{
    m_vTypeDeclarations.Add(pType);
}

/** \brief removes a tagged type declaration
 *  \param pType the type to remove
 */
void CBEClass::RemoveTaggedType(CBEType *pType)
{
    m_vTypeDeclarations.Remove(pType);
}

/** \brief tries to find a pointer to the first tagged type declaration
 *  \return a pointer to the first tagged type declaration
 */
VectorElement* CBEClass::GetFirstTaggedType()
{
    return m_vTypeDeclarations.GetFirst();
}

/** \brief tries to retrieve a reference to the next type declaration
 *  \param pIter the pointer to the next type declaration
 *  \return a reference to the next type declaration
 */
CBEType* CBEClass::GetNextTaggedType(VectorElement* &pIter)
{
    if (!pIter)
        return 0;
    CBEType *pRet = (CBEType*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextTaggedType(pIter);
    return pRet;
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
        VERBOSE("CBEClass::CreateBackEnd failed because tagged type could not be created\n");
        return false;
    }
    return true;
}

/** \brief writes the tagged type declarations to the target file
 *  \param pFile the file to write to
 *  \param pContext the context of the write process
 */
void CBEClass::WriteTaggedTypes(CBEHeaderFile *pFile, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstTaggedType();
    CBEType *pType;
    while ((pType = GetNextTaggedType(pIter)) != 0)
    {
        pType->Write(pFile, pContext);
        pFile->Print(";\n\n");
    }
}

/** \brief searches for a function with the given type
 *  \param sTypeName the name of the type to look for
 *  \param pFile the file to write to (its used to test if a function shall be written)
 *  \param pContext the context of the write operation
 *  \return true if a parameter of that type is found
 *
 * Search functions for a parameter with that type.
 */
bool CBEClass::HasFunctionWithUserType(String sTypeName, CBEFile *pFile, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextFunction(pIter)) != 0)
    {
        if (pFunction->DoWriteFunction(pFile, pContext))
        {
            if (pFunction->FindParameterType(sTypeName))
                return true;
        }
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
void CBEClass::AddBaseName(String sName)
{
    m_nBaseNameSize++;
    m_sBaseNames = (String**)realloc(m_sBaseNames, m_nBaseNameSize*sizeof(String*));
    m_sBaseNames[m_nBaseNameSize-1] = new String(sName);
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
    VectorElement *pIterG = GetFirstFunctionGroup();
    CFunctionGroup *pFuncGroup;
    while ((pFuncGroup = GetNextFunctionGroup(pIterG)) != 0)
    {
        VectorElement *pIterF = pFuncGroup->GetFirstFunction();
        CBEOperationFunction *pFunc;
        while ((pFunc = (CBEOperationFunction*)(pFuncGroup->GetNextFunction(pIterF))) != 0)
        {
            nCurr = pFunc->GetParameterCount(nMustAttrs, nMustNotAttrs, nDirection);
            nCount = MAX(nCount, nCurr);
        }
    }

    return nCount;
}
