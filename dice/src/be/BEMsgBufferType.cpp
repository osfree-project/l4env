/**
 *    \file    dice/src/be/BEMsgBufferType.cpp
 *    \brief   contains the implementation of the class CBEMsgBufferType
 *
 *    \date    02/13/2002
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

#include "be/BEMsgBufferType.h"
#include "be/BEType.h"
#include "be/BEContext.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BERoot.h"
#include "be/BEHeaderFile.h"
#include "be/BEFunction.h"
#include "be/BEAttribute.h"

#include "fe/FETypedDeclarator.h"
#include "fe/FEPrimaryExpression.h"
#include "fe/FESimpleType.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FEArrayDeclarator.h"

CBEMsgBufferType::CBEMsgBufferType()
{
    m_pAliasType = 0;
    m_bCountAllVarsAsMax = false;
    m_pFunction = 0;
    m_vCounts[0].clear();
    m_vCounts[1].clear();
}

CBEMsgBufferType::CBEMsgBufferType(CBEMsgBufferType & src)
: CBETypedef(src)
{
    m_vCounts[0] = src.m_vCounts[0];
    m_vCounts[1] = src.m_vCounts[1];
    m_pAliasType = src.m_pAliasType;
    m_pFunction = src.m_pFunction;
    m_bCountAllVarsAsMax = src.m_bCountAllVarsAsMax;
}

/** \brief destructor of this instance */
CBEMsgBufferType::~CBEMsgBufferType()
{
    m_vCounts[0].clear();
    m_vCounts[1].clear();
}

/** \brief creates the message buffer type
 *  \param pFEInterface the respective front-end interface
 *  \param pContext the context of the code creation
 *  \return true if successful
 *
 * This function creates, depending of the members of the interface a new message buffer for
 * that interface. This message buffer is then used as parameter to wait-any and unmarshal functions
 * of that interface.
 *
 * To simplify the whole operation we create a CFETypedDeclarator object which is handed to the base class' CreateBE
 * function.
 */
bool CBEMsgBufferType::CreateBackEnd(CFEInterface * pFEInterface, CBEContext * pContext)
{
    VERBOSE("CBEMsgBufferType::CreateBackEnd(interface %s)\n", (pFEInterface)?pFEInterface->GetName().c_str():"nil");
    // init counts first, because GetMsgBufferType may access them
    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    CBEClass *pClass = pRoot->FindClass(pFEInterface->GetName());
    assert(pClass);
    InitCounts(pClass, pContext);

    // create declarator
    string sName = pContext->GetNameFactory()->GetMessageBufferTypeName(pContext); // interface name is added be typedef
    CFEDeclarator *pMemDecl = new CFEDeclarator(DECL_IDENTIFIER, sName);
    // create type
    CFETypeSpec *pMemType = GetMsgBufferType(pFEInterface, pMemDecl, pContext);
    // create vector here, because pMemDecl might be manipulated inside GetMsgBufferType
    vector<CFEDeclarator*> *pMemDecls = new vector<CFEDeclarator*>();
    pMemDecls->push_back(pMemDecl);
    // create typedef
    CFETypedDeclarator *pTypedef = new CFETypedDeclarator(TYPEDECL_TYPEDEF, pMemType, pMemDecls);
    pMemDecl->SetParent(pTypedef);
    pMemType->SetParent(pTypedef);
    pTypedef->SetParent(pFEInterface);
    delete pMemDecls;
    // call base class' create function
    bool bRet = CBETypedef::CreateBackEnd(pTypedef, pContext);
    // clean up
    delete pTypedef;
    // return status
    VERBOSE("CBEMsgBufferType::CreateBackEnd(interface) return %s\n", bRet?"true":"false");
    return bRet;
}

/** \brief creates the message buffer type for a function
 *  \param pFEOperation the respective front-end operation
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CBEMsgBufferType::CreateBackEnd(CFEOperation *pFEOperation, CBEContext * pContext)
{
    VERBOSE("CBEMsgBufferType::CreateBackEnd(operation %s)\n", (pFEOperation)?pFEOperation->GetName().c_str():"nil");
    // init counts first, because GetMsgBufferType may access them
    assert(GetParent());
    assert(dynamic_cast<CBEFunction*>(GetParent()));
    CBEFunction *pFunction = dynamic_cast<CBEFunction*>(GetParent());
    InitCounts(pFunction, pContext);

    // create declarator
    string sName = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    CFEDeclarator *pMemDecl = new CFEDeclarator(DECL_IDENTIFIER, sName);
    // create type
    CFETypeSpec *pMemType = GetMsgBufferType(pFEOperation, pMemDecl, pContext);
    assert(pMemType);
    // create vector here, because pMemDecl might be manipulated inside GetMsgBufferType
    vector<CFEDeclarator*> *pMemDecls = new vector<CFEDeclarator*>();
    pMemDecls->push_back(pMemDecl);
    // create typedef
    CFETypedDeclarator *pTypedef = new CFETypedDeclarator(TYPEDECL_TYPEDEF, pMemType, pMemDecls);
    pMemDecl->SetParent(pTypedef);
    pMemType->SetParent(pTypedef);
    pTypedef->SetParent(pFEOperation);
    delete pMemDecls;
    // call base class' create function
    bool bRet = CBETypedDeclarator::CreateBackEnd(pTypedef, pContext);
    // clean up
    delete pTypedef;
    // return status
    VERBOSE("CBEMsgBufferType::CreateBackEnd(operation) return %s\n", bRet?"true":"false");
    return bRet;
}

/** \brief creates a new message buffer type
 *  \param pMsgBuffer the reference message buffer, which this one should refer to
 *  \param pContext the context of the create process
 *  \return true if create was successful
 *
 * We create a user defined type, which references the handed pMsgBuffer. Since this is used
 * for parameters of a function, we cannot use the base class directly, which is a 'typedef'
 * but skip it and use the super-super class, which is a 'typed declarator' - a parameter.
 *
 * To create a user defined type, we need a name of the type and the name of the new declarator.
 */
bool CBEMsgBufferType::CreateBackEnd(CBEMsgBufferType *pMsgBuffer, CBEContext *pContext)
{
    // init counts
    InitCounts(pMsgBuffer, pContext);
    // create name
    string sName = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    // set alias type
    m_pAliasType = pMsgBuffer;
    // only add an additional reference if type is not a ptr type
    int nRef = (pMsgBuffer->GetType()->IsPointerType()) ? 0 : 1;
    // call base class' create function
    return CBETypedDeclarator::CreateBackEnd(pMsgBuffer->GetAlias()->GetName(), sName, nRef, pContext);
}

/** \brief creates the type of the message buffer
 *  \param pFEInterface the corresponding front-end interface
 *  \param pFEDeclarator the name of the type
 *  \param pContext the context of the code creation
 *  \return the new type of the message buffer
 *
 * This function receives the name of the type as a parameter to perform possible adaptations.
 *
 * This implementation simply creates a simple type, which is a character array.  To get the
 * size of the character array, we check if we have any strings and variable sized parameters,
 * which make this an unbound array. If we have no such parameter, we can make the array fixed
 * sized:
 */
CFETypeSpec *CBEMsgBufferType::GetMsgBufferType(CFEInterface *pFEInterface, CFEDeclarator *&pFEDeclarator, CBEContext * pContext)
{
    if ((GetCount(TYPE_STRING) > 0) || (GetCount(TYPE_VARSIZED) > 0))
        pFEDeclarator->SetStars(1);
    else
    {
        long nSize = GetCount(TYPE_FIXED);
        CFEPrimaryExpression *pBound = new CFEPrimaryExpression(EXPR_INT, nSize);
        CFEArrayDeclarator *pNewDecl = new CFEArrayDeclarator(pFEDeclarator);
        pNewDecl->AddBounds(NULL, pBound);
        delete pFEDeclarator;
        pFEDeclarator = pNewDecl;
    }

    return new CFESimpleType(TYPE_CHAR);
}

/** \brief creates the message buffer type for a specific function
 *  \param pFEOperation the respective front-end operation
 *  \param pFEDeclarator the name of the type (for manipulation of pointers, etc.)
 *  \param pContext the context of the create process
 *  \return the message buffer type
 */
CFETypeSpec *CBEMsgBufferType::GetMsgBufferType(CFEOperation *pFEOperation, CFEDeclarator* &pFEDeclarator, CBEContext * pContext)
{
    if ((GetCount(TYPE_STRING) > 0) || (GetCount(TYPE_VARSIZED) > 0))
        pFEDeclarator->SetStars(1);
    else
    {
        long nSize = GetCount(TYPE_FIXED);
        CFEPrimaryExpression *pBound = new CFEPrimaryExpression(EXPR_INT, nSize);
        CFEArrayDeclarator *pNewDecl = new CFEArrayDeclarator(pFEDeclarator);
        pNewDecl->AddBounds(NULL, pBound);
        delete pFEDeclarator;
        pFEDeclarator = pNewDecl;
    }

    return new CFESimpleType(TYPE_CHAR);
}

/** \brief writes the declaration of this message buffer
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * Since the declaration always involves a veriable we never want
 * to have the type-name in here, which might be possible. Therefore,
 * we "hard-code" the msg-buffer variable name in here. To check if the
 * declarators had any pointers, we get the first one and print as many
 * stars as necessary.
 */
void CBEMsgBufferType::WriteDeclaration(CBEFile *pFile, CBEContext *pContext)
{
    if (m_pType)
        m_pType->Write(pFile, pContext);
    pFile->Print(" ");
    // print variable name
    CBEDeclarator *pDecl = GetAlias();
    for (int i=0; i<pDecl->GetStars(); i++)
        pFile->Print("*");
    string sName = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    pFile->Print("%s", sName.c_str());
}

/** \brief writes the variable definition
 *  \param pFile the file to write to
 *  \param bTypedef true if this should be a 'typedef' construct
 *  \param pContext the context of the write operation
 *
 *  The difference between a declaration and a definition is, that a definition includes
 * a full description of the type (e.g. the full struct construct). If bTypedef is true
 * this is called to write the typedef variant of this class. If it (bTypedef) is false,
 * its the typed declarator variant.
 */
void CBEMsgBufferType::WriteDefinition(CBEFile *pFile, bool bTypedef, CBEContext *pContext)
{
    if (bTypedef && dynamic_cast<CBEHeaderFile*>(pFile))
        CBETypedef::WriteDeclaration((CBEHeaderFile*)pFile, pContext);
    else
    {
        pFile->PrintIndent("");
        CBETypedDeclarator::WriteDeclaration(pFile, pContext);
        pFile->Print(";\n");
    }
}

/** \brief initializes the message buffer
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * If the message buffer is of variable size, it has to be allocated.
 */
void CBEMsgBufferType::WriteInitialization(CBEFile *pFile, CBEContext *pContext)
{
    string sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
    string sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);

    if (IsVariableSized())
    {
        int nBufferSize = GetCount(TYPE_FIXED);
        // declare constant size
        pFile->PrintIndent("%s = %d", sOffset.c_str(), nBufferSize);
        WriteInitializationVarSizedParameters(pFile, pContext);
        pFile->Print(";\n");
        // allocate message data structure
        pFile->PrintIndent("%s = ", sMsgBuffer.c_str());
        m_pType->WriteCast(pFile, true, pContext);
        pFile->Print("_dice_alloca(%s);\n", sOffset.c_str());
    }
}

/** \brief initializes members of the message buffer
 *  \param pFile the file to write to
 *  \param nType the type of the member to initialize
 *  \param nDirection the direction of the type to initialize
 *  \param pContext the context of the writing
 *
 * This implementation does nothing
 */
void
CBEMsgBufferType::WriteInitialization(CBEFile *pFile,
    unsigned int nType,
    int nDirection,
    CBEContext *pContext)
{
}

/** \brief writes the size initialization of variable sized parameters
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void
CBEMsgBufferType::WriteInitializationVarSizedParameters(CBEFile *pFile,
    CBEContext *pContext)
{
    // iterate over variable sized parameters
    CBEFunction *pFunction = dynamic_cast<CBEFunction*>(GetParent());
    if (pFunction)
    {
        int nRecvDir = pFunction->GetReceiveDirection();
        vector<CBETypedDeclarator*>::iterator iter = pFunction->GetFirstSortedParameter();
        CBETypedDeclarator *pParameter;
        while ((pParameter = pFunction->GetNextSortedParameter(iter)) != 0)
        {
            if (!pParameter->IsVariableSized())
                continue;
            // if we receive this parameter and it has an max-attribute,
            // then it's size is already counted in the fixed size of the buffer
            if (pParameter->IsDirection(nRecvDir) &&
                (pParameter->FindAttribute(ATTR_MAX_IS) != 0))
                continue;
            if (pParameter->IsDirection(nRecvDir) &&
                pParameter->IsString() &&
                (pParameter->FindAttribute(ATTR_MAX_IS) == 0))
            {
                *pFile << "+" <<
                    pContext->GetSizes()->GetMaxSizeOfType(TYPE_CHAR_ASTERISK);
            }
            else
            {
                pFile->Print("+(");
		if (pParameter->IsString() && 
		    pContext->IsOptionSet(PROGRAM_ALIGN_TO_TYPE))
		    *pFile << "(";
                pParameter->WriteGetSize(pFile, NULL, pContext);
                CBEType *pType = pParameter->GetType();
                CBEAttribute *pAttr;
                if ((pAttr = pParameter->FindAttribute(ATTR_TRANSMIT_AS)) != 0)
                    pType = pAttr->GetAttrType();
                if ((pType->GetSize() > 1) && !(pParameter->IsString()))
                {
                    pFile->Print("*sizeof");
                    pParameter->GetType()->WriteCast(pFile, false, pContext);
                }
                else if (pParameter->IsString())
                {
                    // add terminating zero
                    pFile->Print("+1");
                    bool bHasSizeAttr = pParameter->HasSizeAttr(ATTR_SIZE_IS) ||
                            pParameter->HasSizeAttr(ATTR_LENGTH_IS) ||
                            pParameter->HasSizeAttr(ATTR_MAX_IS);
                    if (!bHasSizeAttr)
                        pFile->Print("+%d", 
			    pContext->GetSizes()->GetSizeOfType(TYPE_INTEGER));
		    if (pContext->IsOptionSet(PROGRAM_ALIGN_TO_TYPE))
			*pFile << "+3)&~3";
                }
                pFile->Print(")");
            }
        }
    }
}

/** \brief initializes the internal counts with the values of the interface
 *  \param pClass the class to count
 *  \param pContext the context of the init operation
 *
 * Since the message buffer of an interface has to accept messages from
 * base-classes we have to count their values as well.
 *
 * Then we count each function of the class. Since we build maxima of the
 * count values it is o.k. to count all the BE functions for one FE operation
 * (call, send, recv, ...).  It's a bit more work, but guarantees more
 * accuracy.
 */
void CBEMsgBufferType::InitCounts(CBEClass *pClass, CBEContext *pContext)
{
    assert(pClass);
    m_bCountAllVarsAsMax = true;
    // base classes
    vector<CBEClass*>::iterator iter = pClass->GetFirstBaseClass();
    CBEClass *pBaseClass;
    while ((pBaseClass = pClass->GetNextBaseClass(iter)) != 0)
    {
        InitCounts(pBaseClass, pContext);
        m_bCountAllVarsAsMax = true; // is set to false, when leaving this function
    }
    // functions
    vector<CBEFunction*>::iterator iterF = pClass->GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = pClass->GetNextFunction(iterF)) != 0)
    {
        InitCounts(pFunction, pContext);
    }
    // reset member function, since this message buffer belongs to a class
    m_pFunction = 0;

    m_bCountAllVarsAsMax = false;
}

/** \brief returns the iterator to the counter of a specific type and direction
 *  \param nType the type of the counter
 *  \param nDirection the direction of the counter
 *  \return an iterator to the respective element
 *
 * A new (empty) element with the respective type is created if none has been
 * found.
 */
vector<struct CBEMsgBufferType::TypeCount>::iterator
CBEMsgBufferType::GetCountIter(unsigned int nType, int nDirection)
{
    vector<struct TypeCount>::iterator iter;
    for (iter = m_vCounts[nDirection-1].begin();
         iter != m_vCounts[nDirection-1].end(); iter++)
    {
        if ((*iter).nType == nType)
            break;
    }
    if (iter == m_vCounts[nDirection-1].end())
    {
        struct TypeCount sCount;
        sCount.nType = nType;
        sCount.nCount = 0;
        m_vCounts[nDirection-1].push_back(sCount);
        iter = m_vCounts[nDirection-1].end()-1;
    }

    return iter;
}

/** \brief initializes the internal counts with the values of the operation
 *  \param pFunction the function to count
 *  \param pContext the context of the initial counting
 *
 * This method uses the BE function's methods to set the counts. To allow the
 * use of this method for classes with multiple functions as well, we use
 * temporary variables and determine maximum values.
 */
void CBEMsgBufferType::InitCounts(CBEFunction *pFunction, CBEContext *pContext)
{
    assert(pFunction);
    int nSendDir = pFunction->GetSendDirection();
    int nRecvDir = pFunction->GetReceiveDirection();
    // count parameters

    // fixed
    vector<struct TypeCount>::iterator iterSend, iterRecv;
    iterSend = GetCountIter(TYPE_FIXED, DIRECTION_IN);
    iterRecv = GetCountIter(TYPE_FIXED, DIRECTION_OUT);

    unsigned int nTempSend = pFunction->GetFixedSize(nSendDir, pContext);
    unsigned int nTempRecv = pFunction->GetFixedSize(nRecvDir, pContext);
    (*iterSend).nCount = MAX(nTempSend, (*iterSend).nCount);
    (*iterRecv).nCount = MAX(nTempRecv, (*iterRecv).nCount);

    // get var sized
    nTempSend = pFunction->GetVariableSizedParameterCount(nSendDir);
    nTempRecv = pFunction->GetVariableSizedParameterCount(nRecvDir);
    // add strings, since they are var-sized as well
    nTempSend += pFunction->GetStringParameterCount(nSendDir);
    nTempRecv += pFunction->GetStringParameterCount(nRecvDir);
    // if there are variable sized parameters for send and NOT for receive
    // -> add var of send to var sized count
    if ((nTempSend > 0) && (nTempRecv == 0))
    {
        if (m_bCountAllVarsAsMax)
        {
            unsigned int nMaxSend = pFunction->GetMaxSize(nSendDir, pContext) +
                           pFunction->GetFixedSize(nSendDir, pContext);
            (*iterSend).nCount = MAX(nMaxSend, (*iterSend).nCount);
        }
        else
        {
            vector<struct TypeCount>::iterator iter =
                GetCountIter(TYPE_VARSIZED, DIRECTION_IN);
            (*iter).nCount = MAX(nTempSend, (*iter).nCount);
        }
    }
    // if there are variable sized parameters for recv and NOT for send
    // -> get max of receive (dont't forget "normal" fixed) and max with fixed
    if ((nTempSend == 0) && (nTempRecv > 0))
    {
        unsigned int nMaxRecv = pFunction->GetMaxSize(nRecvDir, pContext) +
                       pFunction->GetFixedSize(nRecvDir, pContext);
        (*iterRecv).nCount = MAX(nMaxRecv, (*iterRecv).nCount);
    }
    // if there are variable sized parameters for send AND recveive
    // -> add recv max to fixed AND if send max bigger, set var sized of send
    if ((nTempSend > 0) && (nTempRecv > 0))
    {
        unsigned int nMaxSend = pFunction->GetMaxSize(nSendDir, pContext) +
            pFunction->GetFixedSize(nSendDir, pContext);
        unsigned int nMaxRecv = pFunction->GetMaxSize(nRecvDir, pContext) +
            pFunction->GetFixedSize(nRecvDir, pContext);
        (*iterRecv).nCount = MAX(nMaxRecv, (*iterRecv).nCount);
        if (nMaxSend > nMaxRecv)
        {
            if (m_bCountAllVarsAsMax)
                (*iterSend).nCount = MAX(nMaxSend, (*iterSend).nCount);
            else
            {
                iterSend = GetCountIter(TYPE_VARSIZED, DIRECTION_OUT);
                (*iterSend).nCount = MAX(nTempSend, (*iterSend).nCount);
            }
        }
    }

    // assign member as reference to function
    m_pFunction = pFunction;
}

/** \brief initializes own counters with the counters of alias type
 *  \param pMsgBuffer the alias type
 *  \param pContext the context of the init operation
 */
void CBEMsgBufferType::InitCounts(CBEMsgBufferType *pMsgBuffer, CBEContext *pContext)
{
    assert(pMsgBuffer);
    m_vCounts[0] = pMsgBuffer->m_vCounts[0];
    m_vCounts[1] = pMsgBuffer->m_vCounts[1];
    // reset reference to function
    m_pFunction = pMsgBuffer->m_pFunction;
}

/** \brief test if this message buffer is variable sized for a particular direction
 *  \param nDirection the direction to test
 *  \return true if it is variable sized
 */
bool CBEMsgBufferType::IsVariableSized(int nDirection)
{
    if (nDirection == 0)
        return  IsVariableSized(DIRECTION_OUT) || IsVariableSized(DIRECTION_IN);
    // look for member of type TYPE_VARSIZED
    vector<struct TypeCount>::iterator iter;
    for (iter = m_vCounts[nDirection-1].begin();
         iter != m_vCounts[nDirection-1].end(); iter++)
    {
        if ((*iter).nType == TYPE_VARSIZED)
            return ((*iter).nCount > 0);
    }
    return false;
}

/** \brief init counters with zeros
 *  \param nDirection the direction to zero out
 */
void CBEMsgBufferType::ZeroCounts(int nDirection)
{
    if (nDirection == 0)
    {
        ZeroCounts(DIRECTION_IN);
        ZeroCounts(DIRECTION_OUT);
        return;
    }
    m_vCounts[nDirection-1].clear();
}

/** \brief returns the count of parameters of a specific type
 *  \param nType the type of the counted parameters
 *  \param nDirection the direction to check
 *  \return the number of parameters (NOT the size)
 */
unsigned int CBEMsgBufferType::GetCount(unsigned int nType, int nDirection)
{
    if (nDirection == 0)
        return MAX(GetCount(nType, DIRECTION_IN), GetCount(nType, DIRECTION_OUT));

    string sName = (m_pFunction)?(m_pFunction->GetName()):string("noname");
    // check all members of the vector with the given type
    vector<struct TypeCount>::iterator iter;
    unsigned int nCount = 0;
    for (iter = m_vCounts[nDirection-1].begin();
         iter != m_vCounts[nDirection-1].end(); iter++)
    {
        if ((*iter).nType == nType)
            nCount += (*iter).nCount;
    }
    return nCount;
}

/** \brief writes the string used to access a member of the message buffer
 *  \param pFile the file to write to
 *  \param nMemberType the type of the member to access
 *  \param nDirection the direction of the message
 *  \param pContext the context of the write operation
 *  \param sOffset the offset if the access to the member has to be done using a cast of another member
 *
 * This is used to access the members correctly.
 * A member access is usually '&lt;msg buffer&gt;(->|.)&lt;member&gt;' if the message buffer is a
 * constructed type. If it is simple then it is only '&lt;msg buffer&gt;'.
 */
void CBEMsgBufferType::WriteMemberAccess(CBEFile *pFile, int nMemberType, int nDirection, CBEContext *pContext, string sOffset)
{
    // print variable name
    // because this implementation uses a simple type, we only write the name.
    *pFile << pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
}

/** \brief writes the string used to access a member of the message buffer
 *  \param pFile the file to write to
 *  \param pParameter the parameter to access
 *  \param nDirection the direction of the message
 *  \param pContext the context of the write operation
 *  \param sOffset the offset if the access to the member has to be done using a cast of another member
 *
 * This is used to access the members correctly.
 * A member access is usually '&lt;msg buffer&gt;(->|.)&lt;member&gt;' if the message buffer is a
 * constructed type. If it is simple then it is only '&lt;msg buffer&gt;'.
 */
void CBEMsgBufferType::WriteMemberAccess(CBEFile *pFile,
    CBETypedDeclarator *pParameter,
    int nDirection,
    CBEContext *pContext,
    string sOffset)
{
    // print variable name
    // because this implementation uses a simple type, we only write the name.
    *pFile << pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
}

/** \brief creates a new instance of this class */
CObject * CBEMsgBufferType::Clone()
{
    return new CBEMsgBufferType(*this);
}

/** \brief test if we need to cast the message buffer
 *  \param nFEType the type to test for
 *  \param pContext the context of the cast operation
 *  \return true if we need to cast the message buffer member
 */
bool CBEMsgBufferType::NeedCast(int nFEType, CBEContext *pContext)
{
    if (nFEType == TYPE_CHAR)
        return false;
    return true;
}

/** \brief writes a dumper function for the message buffer
 *  \param pFile the file to write to
 *  \param sResult the result dope string (if empty the send dope is used)
 *  \param pContext the context if the write operation
 */
void CBEMsgBufferType::WriteDump(CBEFile *pFile, string sResult, CBEContext *pContext)
{
    string sFunc = pContext->GetTraceMsgBufFunc();
    if (sFunc.empty())
        sFunc = string("printf");

    *pFile << "for (_i=0; _i<";
    if (pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF_DWORDS))
        *pFile << pContext->GetTraceMsgBufDwords()*4;
    else
        *pFile << sResult;
    *pFile << "; _i++)\n\t{\n";
    pFile->IncIndent();
    *pFile << "\tif (_i%%4 == 0)\n";
    pFile->IncIndent();
    *pFile << "\t" << sFunc << "(\"dwords[%%d]:\", _i/4);\n";
    pFile->DecIndent();
    *pFile << "\t" << sFunc << "(\"%%02x \", ";
    WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    *pFile << "[_i]);\n";
    *pFile << "\tif (_i%%4 == 3)\n";
    pFile->IncIndent();
    *pFile << "\t" << sFunc << "(\"\\n\");\n";
    pFile->DecIndent();
    pFile->DecIndent();
    *pFile << "\t}\n";

}

/** \brief fills the message buffer with zeros
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEMsgBufferType::WriteSetZero(CBEFile *pFile, CBEContext *pContext)
{
    pFile->PrintIndent("memset(");
    CBEDeclarator *pDecl = GetAlias();
    if ((pDecl->GetStars() == 0) && !IsVariableSized())
        pFile->Print("&");
    string sName = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    pFile->Print("%s, 0, sizeof", sName.c_str());
    m_pType->WriteCast(pFile, false, pContext);
    pFile->Print(");\n");
}

/** \brief fills the message buffer with zeros
 *  \param pFile the file to write to
 *  \param nType the type of the member to initialize with zero
 *  \param nDirection the direction of the message
 *  \param pContext the context of the write operation
 */
void
CBEMsgBufferType::WriteSetZero(CBEFile *pFile,
    unsigned int nType,
    int nDirection,
    CBEContext *pContext)
{
}

/** \brief checks if this message buffer fulfills a specific property
 *  \param nProperty the property to check for
 *  \param nDirection the direction to check for
 *  \param pContext the omnipresent context
 *  \return true if the property is available
 */
bool CBEMsgBufferType::CheckProperty(int nProperty,
    int nDirection,
    CBEContext *pContext)
{
    return false;
}
