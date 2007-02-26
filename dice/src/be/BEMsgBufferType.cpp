/**
 *	\file	dice/src/be/BEMsgBufferType.cpp
 *	\brief	contains the implementation of the class CBEMsgBufferType
 *
 *	\date	02/13/2002
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

IMPLEMENT_DYNAMIC(CBEMsgBufferType);

CBEMsgBufferType::CBEMsgBufferType()
{
    m_nFixedCount[0] = m_nFixedCount[1] = 0;
    m_nVariableCount[0] = m_nVariableCount[1] = 0;
    m_nStringCount[0] = m_nStringCount[1] = 0;
    m_pAliasType = 0;
	m_bCountAllVarsAsMax = false;
    IMPLEMENT_DYNAMIC_BASE(CBEMsgBufferType, CBETypedef);
}

CBEMsgBufferType::CBEMsgBufferType(CBEMsgBufferType & src)
: CBETypedef(src)
{
    m_nFixedCount[0] = src.m_nFixedCount[0];
    m_nFixedCount[1] = src.m_nFixedCount[1];
    m_nVariableCount[0] = src.m_nVariableCount[0];
    m_nVariableCount[1] = src.m_nVariableCount[1];
    m_nStringCount[0] = src.m_nStringCount[0];
    m_nStringCount[1] = src.m_nStringCount[1];
    m_pAliasType = src.m_pAliasType;
	m_bCountAllVarsAsMax = src.m_bCountAllVarsAsMax;
    IMPLEMENT_DYNAMIC_BASE(CBEMsgBufferType, CBETypedef);
}

/**	\brief destructor of this instance */
CBEMsgBufferType::~CBEMsgBufferType()
{
}

/**	\brief creates the message buffer type
 *	\param pFEInterface the respective front-end interface
 *	\param pContext the context of the code creation
 *	\return true if successful
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
    // init counts first, because GetMsgBufferType may access them
    CBERoot *pRoot = GetRoot();
    assert(pRoot);
    CBEClass *pClass = pRoot->FindClass(pFEInterface->GetName());
    assert(pClass);
    InitCounts(pClass, pContext);

    // create declarator
    String sName = pContext->GetNameFactory()->GetMessageBufferTypeName(pContext); // interface name is added be typedef::CB
    CFEDeclarator *pMemDecl = new CFEDeclarator(DECL_IDENTIFIER, sName);
    // create type
    CFETypeSpec *pMemType = GetMsgBufferType(pFEInterface, pMemDecl, pContext);
    // create vector here, because pMemDecl might be manipulated inside GetMsgBufferType
    Vector *pMemDecls = new Vector(RUNTIME_CLASS(CFEDeclarator), 1, pMemDecl);
    // create typedef
    CFETypedDeclarator *pTypedef = new CFETypedDeclarator(TYPEDECL_TYPEDEF, pMemType, pMemDecls);
    pMemDecl->SetParent(pTypedef);
    pMemType->SetParent(pTypedef);
    pTypedef->SetParent(pFEInterface);
    // call base class' create function
    bool bRet = CBETypedef::CreateBackEnd(pTypedef, pContext);
    // clean up
    delete pTypedef;
    // return status
    return bRet;
}

/** \brief creates the message buffer type for a function
 *  \param pFEOperation the respective front-end operation
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CBEMsgBufferType::CreateBackEnd(CFEOperation *pFEOperation, CBEContext * pContext)
{
    // init counts first, because GetMsgBufferType may access them
    assert(GetParent()->IsKindOf(RUNTIME_CLASS(CBEFunction)));
    CBEFunction *pFunction = (CBEFunction*)GetParent();
    InitCounts(pFunction, pContext);

    // create declarator
    String sName = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    CFEDeclarator *pMemDecl = new CFEDeclarator(DECL_IDENTIFIER, sName);
    // create type
    CFETypeSpec *pMemType = GetMsgBufferType(pFEOperation, pMemDecl, pContext);
    // create vector here, because pMemDecl might be manipulated inside GetMsgBufferType
    Vector *pMemDecls = new Vector(RUNTIME_CLASS(CFEDeclarator), 1, pMemDecl);
    // create typedef
    CFETypedDeclarator *pTypedef = new CFETypedDeclarator(TYPEDECL_TYPEDEF, pMemType, pMemDecls);
    pMemDecl->SetParent(pTypedef);
    pMemType->SetParent(pTypedef);
    pTypedef->SetParent(pFEOperation);
    // call base class' create function
    bool bRet = CBETypedDeclarator::CreateBackEnd(pTypedef, pContext);
    // clean up
    delete pTypedef;
    // return status
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
    String sName = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    // set alias type
    m_pAliasType = pMsgBuffer;
    // only add an additional reference if type is not a ptr type
    int nRef = (pMsgBuffer->GetType()->IsPointerType()) ? 0 : 1;
    // call base class' create function
    return CBETypedDeclarator::CreateBackEnd(pMsgBuffer->GetAlias()->GetName(), sName, nRef, pContext);
}

/**	\brief creates the type of the message buffer
 *	\param pFEInterface the corresponding front-end interface
 *	\param pFEDeclarator the name of the type
 *	\param pContext the context of the code creation
 *	\return the new type of the message buffer
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
    if ((GetStringCount() > 0) || (GetVariableCount() > 0))
        pFEDeclarator->SetStars(1);
    else
    {
        long nSize = GetFixedCount();
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
    if ((GetStringCount() > 0) || (GetVariableCount() > 0))
        pFEDeclarator->SetStars(1);
    else
    {
        long nSize = GetFixedCount();
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
    String sName = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    pFile->Print("%s", (const char*)sName);
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
    if (bTypedef && pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)))
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
    String sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
    String sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);

    if (IsVariableSized())
    {
        int nBufferSize = MAX(m_nFixedCount[DIRECTION_IN-1], m_nFixedCount[DIRECTION_OUT-1]);
        // declare constant size
        pFile->PrintIndent("%s = %d", (const char *) sOffset, nBufferSize);
        WriteInitializationVarSizedParameters(pFile, pContext);
        pFile->Print(";\n");
        // allocate message data structure
        pFile->PrintIndent("%s = ", (const char *) sMsgBuffer);
        m_pType->WriteCast(pFile, true, pContext);
        pFile->Print("_dice_alloca(%s);\n", (const char *) sOffset);
    }
}

/** \brief initializes the internal counts with the values of the interface
 *  \param pClass the class to count
 *  \param pContext the context of the init operation
 *
 * Since the message buffer of an interface has to accept messages from base-classes
 * we have to count their values as well.
 *
 * Then we count each function of the class. Since we build maxima of the count values
 * it is o.k. to count all the BE functions for one FE operation (call, send, recv, ...).
 * It's a bit more work, but guarantees more accuracy.
 */
void CBEMsgBufferType::InitCounts(CBEClass *pClass, CBEContext *pContext)
{
    assert(pClass);
    m_bCountAllVarsAsMax = true;
    // base classes
    VectorElement *pIter = pClass->GetFirstBaseClass();
    CBEClass *pBaseClass;
    while ((pBaseClass = pClass->GetNextBaseClass(pIter)) != 0)
    {
        InitCounts(pBaseClass, pContext);
        m_bCountAllVarsAsMax = true; // is set to false, when leaving this function
    }
    // functions
    pIter = pClass->GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = pClass->GetNextFunction(pIter)) != 0)
    {
        InitCounts(pFunction, pContext);
    }

    m_bCountAllVarsAsMax = false;
}

/** \brief initializes the internal counts with the values of the operation
 *  \param pFunction the function to count
 *  \param pContext the context of the initial counting
 *
 * This method uses the BE function's methods to set the counts. To allow the use of this
 * method for classes with multiple functions as well, we use temporary variables and determine
 * maximum values.
 */
void CBEMsgBufferType::InitCounts(CBEFunction *pFunction, CBEContext *pContext)
{
    assert(pFunction);
    int nSendDir = pFunction->GetSendDirection();
    int nRecvDir = pFunction->GetReceiveDirection();
    // count parameters
    int nTempSend = pFunction->GetFixedSize(nSendDir, pContext);
    int nTempRecv = pFunction->GetFixedSize(nRecvDir, pContext);
    m_nFixedCount[nSendDir-1] = MAX(nTempSend, m_nFixedCount[nSendDir-1]);
    m_nFixedCount[nRecvDir-1] = MAX(nTempRecv, m_nFixedCount[nRecvDir-1]);
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
            int nMaxSend = pFunction->GetMaxSize(nSendDir, pContext) + pFunction->GetFixedSize(nSendDir, pContext);
            m_nFixedCount[nSendDir-1] = MAX(nMaxSend, m_nFixedCount[nSendDir-1]);
        }
        else
            m_nVariableCount[nSendDir-1] = MAX(nTempSend, m_nVariableCount[nSendDir-1]);
    }
    // if there are variable sized parameters for recv and NOT for send
    // -> get max of receive (dont't forget "normal" fixed) and max with fixed
    if ((nTempSend == 0) && (nTempRecv > 0))
    {
        int nMaxRecv = pFunction->GetMaxSize(nRecvDir, pContext) + pFunction->GetFixedSize(nRecvDir, pContext);
        m_nFixedCount[nRecvDir-1] = MAX(nMaxRecv, m_nFixedCount[nRecvDir-1]);
    }
    // if there are variable sized parameters for send AND recveive
    // -> add recv max to fixed AND if send max bigger, set var sized of send
    if ((nTempSend > 0) && (nTempRecv > 0))
    {
        int nMaxSend = pFunction->GetMaxSize(nSendDir, pContext) + pFunction->GetFixedSize(nSendDir, pContext);
        int nMaxRecv = pFunction->GetMaxSize(nRecvDir, pContext) + pFunction->GetFixedSize(nRecvDir, pContext);
        m_nFixedCount[nRecvDir-1] = MAX(nMaxRecv, m_nFixedCount[nRecvDir-1]);
        if (nMaxSend > nMaxRecv)
        {
            if (m_bCountAllVarsAsMax)
                m_nFixedCount[nSendDir-1] = MAX(nMaxSend, m_nFixedCount[nSendDir-1]);
            else
                m_nVariableCount[nSendDir-1] = MAX(nTempSend, m_nVariableCount[nSendDir-1]);
        }
    }

//    m_nVariableCount[nSendDir-1] = MAX(nTempSendVar, m_nVariableCount[nSendDir-1]);
//    m_nVariableCount[nRecvDir-1] = MAX(nTempRecvVar, m_nVariableCount[nRecvDir-1]);
//
//    m_nStringCount[nSendDir-1] = MAX(nTempSend, m_nStringCount[nSendDir-1]);
//    m_nStringCount[nRecvDir-1] = MAX(nTempRecv, m_nStringCount[nRecvDir-1]);
}

/** \brief initializes own counters with the counters of alias type
 *  \param pMsgBuffer the alias type
 *  \param pContext the context of the init operation
 */
void CBEMsgBufferType::InitCounts(CBEMsgBufferType *pMsgBuffer, CBEContext *pContext)
{
    assert(pMsgBuffer);
    m_nFixedCount[0] = pMsgBuffer->m_nFixedCount[0];
    m_nFixedCount[1] = pMsgBuffer->m_nFixedCount[1];
    m_nStringCount[0] = pMsgBuffer->m_nStringCount[0];
    m_nStringCount[1] = pMsgBuffer->m_nStringCount[1];
    m_nVariableCount[0] = pMsgBuffer->m_nVariableCount[0];
    m_nVariableCount[1] = pMsgBuffer->m_nVariableCount[1];
}

/** \brief test if this message buffer is variable sized for a particular direction
 *  \param nDirection the direction to test
 *  \return true if it is variable sized
 */
bool CBEMsgBufferType::IsVariableSized(int nDirection)
{
    if (nDirection == 0)
        return  IsVariableSized(DIRECTION_OUT) || IsVariableSized(DIRECTION_IN);
    return (m_nVariableCount[nDirection-1] > 0);
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
    m_nFixedCount[nDirection-1] = 0;
    m_nStringCount[nDirection-1] = 0;
    m_nVariableCount[nDirection-1] = 0;
}

/** \brief returns the fixed count
 *  \param nDirection the direction to count
 *  \return the size of fixed size parameters (in bytes)
 */
int CBEMsgBufferType::GetFixedCount(int nDirection)
{
    if (nDirection == 0)
        return MAX(m_nFixedCount[DIRECTION_IN-1], m_nFixedCount[DIRECTION_OUT-1]);
    return m_nFixedCount[nDirection-1];
}

/** \brief returns the number of strings
 *  \param nDirection the direction to count
 *  \return the number of strings (not their size)
 */
int CBEMsgBufferType::GetStringCount(int nDirection)
{
    if (nDirection == 0)
        return MAX(m_nStringCount[DIRECTION_IN-1], m_nStringCount[DIRECTION_OUT-1]);
    return m_nStringCount[nDirection-1];
}

/** \brief returns the number of variable sized parameters
 *  \param nDirection the direction to count
 *  \return the number of variable sized parameters (not their size)
 */
int CBEMsgBufferType::GetVariableCount(int nDirection)
{
    if (nDirection == 0)
        return MAX(m_nVariableCount[DIRECTION_IN-1], m_nVariableCount[DIRECTION_OUT-1]);
    return m_nVariableCount[nDirection-1];
}

/** \brief writes the string used to access a member of the message buffer
 *  \param pFile the file to write to
 *  \param nMemberType the type of the member to access
 *  \param pContext the context of the write operation
 *  \param sOffset the offset if the access to the member has to be done using a cast of another member
 *
 * This is used to access the members correctly.
 * A member access is usually '&lt;msg buffer&gt;(->|.)&lt;member&gt;' if the message buffer is a
 * constructed type. If it is simple then it is only '&lt;msg buffer&gt;'.
 */
void CBEMsgBufferType::WriteMemberAccess(CBEFile *pFile, int nMemberType, CBEContext *pContext, String sOffset)
{
    // print variable name
    // because this implementation uses a simple type, we only write the name.
    String sName = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    pFile->Print("%s", (const char*)sName);
}

/** \brief creates a new instance of this class */
CObject * CBEMsgBufferType::Clone()
{
    return new CBEMsgBufferType(*this);
}

/** \brief writes the size initialization of variable sized parameters
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEMsgBufferType::WriteInitializationVarSizedParameters(CBEFile *pFile, CBEContext *pContext)
{
    // iterate over variable sized parameters
    CBEFunction *pFunction = 0;
    if (GetParent()->IsKindOf(RUNTIME_CLASS(CBEFunction)))
        pFunction = (CBEFunction*)GetParent();
    if (pFunction)
    {
        int nRecvDir = pFunction->GetReceiveDirection();
        VectorElement *pIter = pFunction->GetFirstSortedParameter();
        CBETypedDeclarator *pParameter;
        while ((pParameter = pFunction->GetNextSortedParameter(pIter)) != 0)
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
                pFile->Print("+%d",pContext->GetSizes()->GetMaxSizeOfType(TYPE_CHAR_ASTERISK));
            }
            else
            {
                pFile->Print("+(");
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
                        pFile->Print("+%d", pContext->GetSizes()->GetSizeOfType(TYPE_INTEGER));
                }
                pFile->Print(")");
            }
        }
    }
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
