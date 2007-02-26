/**
 *    \file    dice/src/be/l4/L4BEMsgBufferType.cpp
 *    \brief   contains the implementation of the class CL4BEMsgBufferType
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

#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BESizes.h"
#include "be/BEContext.h"
#include "be/BETypedDeclarator.h"
#include "be/BEFunction.h"
#include "be/BESrvLoopFunction.h"
#include "be/BEType.h"
#include "be/BEUserDefinedType.h"
#include "be/BEDeclarator.h"
#include "be/BEAttribute.h"
#include "be/BEHeaderFile.h"

#include "fe/FETaggedStructType.h"
#include "fe/FESimpleType.h"
#include "fe/FEUserDefinedType.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FEExpression.h"
#include "fe/FEPrimaryExpression.h"
#include "fe/FEArrayDeclarator.h"


CL4BEMsgBufferType::CL4BEMsgBufferType()
: CBEMsgBufferType()
{
    m_bCountAllVarsAsMax = false;
    m_vMaxima[0].clear();
    m_vMaxima[1].clear();
}

CL4BEMsgBufferType::CL4BEMsgBufferType(CL4BEMsgBufferType & src)
: CBEMsgBufferType(src)
{
    m_bCountAllVarsAsMax = src.m_bCountAllVarsAsMax;
    m_vMaxima[0] = src.m_vMaxima[0];
    m_vMaxima[1] = src.m_vMaxima[1];
}

/**    \brief destructor of this instance */
CL4BEMsgBufferType::~CL4BEMsgBufferType()
{

}

/**    \brief creates the message buffer type
 *    \param pFEInterface the respective front-end interface
 *    \param pFEDeclarator the type's name
 *    \param pContext the context of the code creation
 *    \return the new message buffer type
 *
 * This function creates, depending of the members of the interface a new message buffer for
 * that interface. This message buffer is then used as parameter to wait-any and unmarshal functions
 * of that interface.
 *
 * The L4 message buffer consists of:
 * - l4_fpage_t (a possible flexpage descriptor)
 * - l4_msgdope_t (the size of the total message structure)
 * - l4_msgdope_t (the size of the actually send message)
 * - l4_umword_t[] (an message buffer, containing direct values or flexpages)
 * - l4_strdope_t[] (string dopes, describing the strings to send)
 *
 */
CFETypeSpec *CL4BEMsgBufferType::GetMsgBufferType(CFEInterface *pFEInterface,
     CFEDeclarator* &pFEDeclarator,
     CBEContext *pContext)
{
    VERBOSE("CL4BEMsgBufferType::GetMsgBufferType(interface %s)\n", pFEInterface->GetName().c_str());
    vector<CFETypedDeclarator*> *pMembers = new vector<CFETypedDeclarator*>();

    // add "l4_fpage_t fpage" to type
    string sTypeName = pContext->GetNameFactory()->GetTypeName(TYPE_RCV_FLEXPAGE,
        true, pContext);
    CFETypeSpec *pType = new CFEUserDefinedType(sTypeName);
    string sFpageBuf = pContext->GetNameFactory()->GetMessageBufferMember(TYPE_RCV_FLEXPAGE, pContext);
    CFEDeclarator *pDecl = new CFEDeclarator(DECL_IDENTIFIER, sFpageBuf);
    vector<CFEDeclarator*> *pDeclarators = new vector<CFEDeclarator*>();
    pDeclarators->push_back(pDecl);
    pDecl = 0;
    CFETypedDeclarator *pMember = new CFETypedDeclarator(TYPEDECL_FIELD, pType, pDeclarators);
    pType = 0;
    delete pDeclarators;
    pMembers->push_back(pMember);
    pMember = 0;

    // add "l4_msgdope_t size" to type
    sTypeName = pContext->GetNameFactory()->GetTypeName(TYPE_MSGDOPE_SIZE,
        true, pContext);
    pType = new CFEUserDefinedType(sTypeName);
    string sSize = pContext->GetNameFactory()->GetMessageBufferMember(TYPE_MSGDOPE_SIZE, pContext);
    pDecl = new CFEDeclarator(DECL_IDENTIFIER, sSize);
    pDeclarators = new vector<CFEDeclarator*>();
    pDeclarators->push_back(pDecl);
    pDecl = 0;
    pMember = new CFETypedDeclarator(TYPEDECL_FIELD, pType, pDeclarators);
    pType = 0;
    delete pDeclarators;
    pMembers->push_back(pMember);
    pMember = 0;

    // add "l4_msgdope_t send" to type
    sTypeName = pContext->GetNameFactory()->GetTypeName(TYPE_MSGDOPE_SEND,
        true, pContext);
    pType = new CFEUserDefinedType(sTypeName);
    string sSend = pContext->GetNameFactory()->GetMessageBufferMember(TYPE_MSGDOPE_SEND, pContext);
    pDecl = new CFEDeclarator(DECL_IDENTIFIER, sSend);
    pDeclarators = new vector<CFEDeclarator*>();
    pDeclarators->push_back(pDecl);
    pDecl = 0;
    pMember = new CFETypedDeclarator(TYPEDECL_FIELD, pType, pDeclarators);
    pType = 0;
    delete pDeclarators;
    pMembers->push_back(pMember);
    pMember = 0;

    // we need the size for the IN and OUT parameters.
    int nSizeIn = GetCount(TYPE_FIXED, DIRECTION_IN);
    int nSizeOut = GetCount(TYPE_FIXED, DIRECTION_OUT);
    int nFlexpageIn = GetCount(TYPE_FLEXPAGE, DIRECTION_IN);
    int nFlexpageOut = GetCount(TYPE_FLEXPAGE, DIRECTION_OUT);
    assert(pContext->GetSizes());
    int nFlexpageSize = pContext->GetSizes()->GetSizeOfType(TYPE_FLEXPAGE);
    if (nFlexpageIn > 0)
        nSizeIn += (nFlexpageIn+1)*nFlexpageSize;
    if (nFlexpageOut > 0)
        nSizeOut += (nFlexpageOut+1)*nFlexpageSize;
    int nSize = MAX(nSizeIn, nSizeOut);
    // get minimum size
    assert(dynamic_cast<CL4BESizes*>(pContext->GetSizes()));
    CL4BESizes *pSizes = (CL4BESizes*)pContext->GetSizes();
    int nMin = pSizes->GetMaxShortIPCSize(DIRECTION_IN);
    if (nSize < nMin)
        nSize = nMin;
    if ((GetCount(TYPE_VARSIZED, DIRECTION_IN) + 
	    GetCount(TYPE_VARSIZED, DIRECTION_OUT)) > 0)
    {
        nSize = 0;
        pFEDeclarator->SetStars(1);
    }
    // dword-align size
    nSize = (nSize+3) & ~3;
    CFEExpression *pValue = new CFEPrimaryExpression(EXPR_INT, (long int) nSize);

    // add "char buffer[]" to type
    string sCharBuf = pContext->GetNameFactory()->GetMessageBufferMember(TYPE_CHAR, pContext);
    pType = new CFESimpleType(TYPE_CHAR);
    pDecl = new CFEArrayDeclarator(sCharBuf, pValue);
    pValue = 0;
    pDeclarators = new vector<CFEDeclarator*>();
    pDeclarators->push_back( pDecl);
    pDecl = 0;
    pMember = new CFETypedDeclarator(TYPEDECL_FIELD, pType, pDeclarators);
    pType = 0;
    delete pDeclarators;
    pMembers->push_back(pMember);
    pMember = 0;

    // get number of strings
    int nStringIn = GetCount(TYPE_STRING, DIRECTION_IN);
    int nStringOut = GetCount(TYPE_STRING, DIRECTION_OUT);
    int nStrings = MAX(nStringIn, nStringOut);
    if (nStrings > 0)
    {
        string sStrBuf = pContext->GetNameFactory()->GetMessageBufferMember(TYPE_REFSTRING, pContext);
        pValue = new CFEPrimaryExpression(EXPR_INT, (long int) nStrings);
        pDecl = new CFEArrayDeclarator(sStrBuf, pValue);
        pValue = 0;
        pDeclarators = new vector<CFEDeclarator*>();
        pDeclarators->push_back(pDecl);
        pDecl = 0;

        sTypeName = pContext->GetNameFactory()->GetTypeName(TYPE_REFSTRING,
            true, pContext);
        pType = new CFEUserDefinedType(sTypeName);
        pMember = new CFETypedDeclarator(TYPEDECL_FIELD, pType, pDeclarators);
        pType = 0;
        delete pDeclarators;
        pMembers->push_back(pMember);
        pMember = 0;
    }

    // add tag to be able to declare the message buffer before defining it
    string sName = pContext->GetNameFactory()->GetMessageBufferTypeName(pFEInterface, pContext);
    sName = "_" + sName;
    CFETaggedStructType *pReturn = new CFETaggedStructType(sName, pMembers);
    delete pMembers;
    pReturn->SetParent(pFEInterface);
    pReturn->SetSourceLine(pFEInterface->GetSourceLine());
    return pReturn;
}

/** \brief creates the message buffer type for a specific function
 *  \param pFEOperation the repsective front-end function
 *  \param pFEDeclarator the name of the message buffer (for manipulation)
 *  \param pContext the context of the create process
 *  \return the type of the message buffer
 */
CFETypeSpec * CL4BEMsgBufferType::GetMsgBufferType(CFEOperation * pFEOperation,
    CFEDeclarator* &pFEDeclarator,
    CBEContext * pContext)
{
    VERBOSE("CL4BEMsgBufferType::GetMsgBufferType(operation %s)\n", pFEOperation->GetName().c_str());
    vector<CFETypedDeclarator*> *pMembers = new vector<CFETypedDeclarator*>();

    // add "l4_fpage_t fpage" to type
    string sTypeName = pContext->GetNameFactory()->GetTypeName(TYPE_RCV_FLEXPAGE,
        true, pContext);
    CFETypeSpec *pType = new CFEUserDefinedType(sTypeName);
    string sFpageBuf = pContext->GetNameFactory()->GetMessageBufferMember(TYPE_RCV_FLEXPAGE, pContext);
    CFEDeclarator *pDecl = new CFEDeclarator(DECL_IDENTIFIER, sFpageBuf);
    vector<CFEDeclarator*> *pDeclarators = new vector<CFEDeclarator*>();
    pDeclarators->push_back(pDecl);
    pDecl = 0;
    CFETypedDeclarator *pMember = new CFETypedDeclarator(TYPEDECL_FIELD, pType, pDeclarators);
    pType = 0;
    delete pDeclarators;
    pMembers->push_back(pMember);
    pMember = 0;

    // add "l4_msgdope_t size" to type
    sTypeName = pContext->GetNameFactory()->GetTypeName(TYPE_MSGDOPE_SIZE,
        true, pContext);
    pType = new CFEUserDefinedType(sTypeName);
    string sSize = pContext->GetNameFactory()->GetMessageBufferMember(TYPE_MSGDOPE_SIZE, pContext);
    pDecl = new CFEDeclarator(DECL_IDENTIFIER, sSize);
    pDeclarators = new vector<CFEDeclarator*>();
    pDeclarators->push_back(pDecl);
    pDecl = 0;
    pMember = new CFETypedDeclarator(TYPEDECL_FIELD, pType, pDeclarators);
    pType = 0;
    delete pDeclarators;
    pMembers->push_back(pMember);
    pMember = 0;

    // add "l4_msgdope_t send" to type
    sTypeName = pContext->GetNameFactory()->GetTypeName(TYPE_MSGDOPE_SEND,
        true, pContext);
    pType = new CFEUserDefinedType(sTypeName);
    string sSend = pContext->GetNameFactory()->GetMessageBufferMember(TYPE_MSGDOPE_SEND, pContext);
    pDecl = new CFEDeclarator(DECL_IDENTIFIER, sSend);
    pDeclarators = new vector<CFEDeclarator*>();
    pDeclarators->push_back(pDecl);
    pDecl = 0;
    pMember = new CFETypedDeclarator(TYPEDECL_FIELD, pType, pDeclarators);
    pType = 0;
    delete pDeclarators;
    pMembers->push_back(pMember);
    pMember = 0;

    // we need the size for the IN and OUT parameters.
    #warning size instead of count
    int nBytesIn = GetCount(TYPE_FIXED, DIRECTION_IN);
    int nBytesOut = GetCount(TYPE_FIXED, DIRECTION_OUT);

    // we add the number of flexpages
    int nFlexpageSize = pContext->GetSizes()->GetSizeOfType(TYPE_FLEXPAGE);
    int nFlexpagesIn = GetCount(TYPE_FLEXPAGE, DIRECTION_IN);
    int nFlexpagesOut = GetCount(TYPE_FLEXPAGE, DIRECTION_OUT);
    if (nFlexpagesIn > 0)
        nBytesIn += (nFlexpagesIn+1)*nFlexpageSize;
    if (nFlexpagesOut > 0)
        nBytesOut += (nFlexpagesOut+1)*nFlexpageSize;

    int nSize = MAX(nBytesIn, nBytesOut);
    // get minimum size
    CL4BESizes *pSizes = (CL4BESizes*)pContext->GetSizes();
    int nMin = pSizes->GetMaxShortIPCSize(DIRECTION_IN);
    if (nSize < nMin)
        nSize = nMin;
    if ((GetCount(TYPE_VARSIZED, DIRECTION_IN) +
         GetCount(TYPE_VARSIZED, DIRECTION_OUT)) > 0)
    {
        nSize = 0;
        pFEDeclarator->SetStars(1);
    }
    // dword-align size
    nSize = (nSize+3) & ~3;
    CFEExpression *pValue = new CFEPrimaryExpression(EXPR_INT, (long int) nSize);

    // add "char buffer[]" to type
    string sCharBuf =
        pContext->GetNameFactory()->GetMessageBufferMember(TYPE_CHAR, pContext);
    pType = new CFESimpleType(TYPE_CHAR);
    pDecl = new CFEArrayDeclarator(sCharBuf, pValue);
    pValue = 0;
    pDeclarators = new vector<CFEDeclarator*>();
    pDeclarators->push_back(pDecl);
    pDecl = 0;
    pMember = new CFETypedDeclarator(TYPEDECL_FIELD, pType, pDeclarators);
    pType = 0;
    delete pDeclarators;
    pMembers->push_back(pMember);
    pMember = 0;

    // get number of strings
    int nStrings = MAX(GetCount(TYPE_STRING, DIRECTION_IN),
                       GetCount(TYPE_STRING, DIRECTION_OUT));
    if ((nStrings > 0) && !IsVariableSized())
    {
        string sStrBuf =
            pContext->GetNameFactory()->GetMessageBufferMember(TYPE_REFSTRING,
                pContext);
        pValue = new CFEPrimaryExpression(EXPR_INT, (long int) nStrings);
        sTypeName = pContext->GetNameFactory()->GetTypeName(TYPE_REFSTRING,
            true, pContext);
        pType = new CFEUserDefinedType(sTypeName);
        pDecl = new CFEArrayDeclarator(sStrBuf, pValue);
        pValue = 0;
        pDeclarators = new vector<CFEDeclarator*>();
        pDeclarators->push_back(pDecl);
        pDecl = 0;
        pMember = new CFETypedDeclarator(TYPEDECL_FIELD, pType, pDeclarators);
        pType = 0;
        delete pDeclarators;
        pMembers->push_back(pMember);
        pMember = 0;
    }

    // add tag to be able to declare the message buffer before defining it
    string sName =
        pContext->GetNameFactory()->GetMessageBufferTypeName(string(), pContext);
    sName = "_" + sName;

    CFEStructType *pReturn = new CFETaggedStructType(sName, pMembers);
    delete pMembers;
    pReturn->SetParent(pFEOperation);
    pReturn->SetSourceLine(pFEOperation->GetSourceLine());
    return pReturn;
}

/** \brief counts the L4 specific members of the function
 *  \param pFunction the function to count
 *  \param pContext the context of the init operation
 *
 * The sizes we need are the size for the byte-buffer, which contains
 * fixed sized parameters and varaiable sized parameters, count for the
 * indirect string elements, which are variable sized parameters with
 * the ATTR_REF attribute, and the count of flexpages.
 *
 * The byte buffer has to be sized according to these rules:
 * # if only fixed sized parameters, count their size in bytes
 * # if variable sized parameters are send and _NOT_ received,
 *   use a dynamically allocated buffer on the stack
 * # if variable sized parameters are received and _NOT_ send,
 *   determine their maximum size
 * # if variable sized parameter are send _AND_ received, determine
 *   which side is larger: if received maximum is larger, then
 *   use fixed size; if send maximum is larger, then use dynamically
 *   allocated, but at least the receivers maximum
 * # indirect string are simply counted
 * # so are flexpages
 *
 * Because the maximum size of variable sized parameters has such a
 * great impact on the fixed sized parameters, we check them first.
 * To determine which direction is the send direction by using the
 * function's GetSendDirection() and GetReceiveDirection() methods.
 * Still, we store the values using the DIRECTION_IN and DIRECTION_OUT
 * constants. The caller has to be aware of this. In plain this means:
 * if the marshaller wants to determine how big the send buffer has to be
 * it first calls the functions GetSendDirection() and then using that
 * value the message buffers GetFixedSize(nDirection).
 */
void 
CL4BEMsgBufferType::InitCounts(CBEFunction * pFunction, 
    CBEContext *pContext)
{
    assert(pFunction);
    // get directions
    int nSendDir = pFunction->GetSendDirection();
    int nRecvDir = pFunction->GetReceiveDirection();
    // count flexpages
    unsigned int nFlexpagesSend = 
	pFunction->GetParameterCount(TYPE_FLEXPAGE, nSendDir);
    unsigned int nFlexpagesRecv = 
	pFunction->GetParameterCount(TYPE_FLEXPAGE, nRecvDir);
    int nFlexpageSize = pContext->GetSizes()->GetSizeOfType(TYPE_FLEXPAGE);
    vector<struct TypeCount>::iterator iterSend, iterRecv;
    iterSend = GetCountIter(TYPE_FLEXPAGE, nSendDir);
    iterRecv = GetCountIter(TYPE_FLEXPAGE, nRecvDir);
    (*iterSend).nCount = MAX(nFlexpagesSend, (*iterSend).nCount);
    (*iterRecv).nCount = MAX(nFlexpagesRecv, (*iterRecv).nCount);
    // since the flexpages are counted as fixed sized parameters as well,
    // we have to substract them from the fixed size now.
    // get fixed sized parameters
    unsigned int nTempSend = pFunction->GetFixedSize(nSendDir, pContext);
    unsigned int nTempRecv = pFunction->GetFixedSize(nRecvDir, pContext);
    if (nFlexpagesSend > 0)
        nTempSend -= nFlexpageSize*nFlexpagesSend; // NO terminating zero flexpage!
    if (nFlexpagesRecv > 0)
        nTempRecv -= nFlexpageSize*nFlexpagesRecv; // NO terminating zero flexpage!
    iterSend = GetCountIter(TYPE_FIXED, nSendDir);
    iterRecv = GetCountIter(TYPE_FIXED, nRecvDir);
    (*iterSend).nCount = MAX(nTempSend, (*iterSend).nCount);
    (*iterRecv).nCount = MAX(nTempRecv, (*iterRecv).nCount);
    // now get var sized parameter counts
    nTempSend = pFunction->GetVariableSizedParameterCount(nSendDir);
    nTempSend += pFunction->GetStringParameterCount(nSendDir, 0, ATTR_REF); // strings without ref
    nTempRecv = pFunction->GetVariableSizedParameterCount(nRecvDir);
    nTempRecv += pFunction->GetStringParameterCount(nRecvDir, 0, ATTR_REF); // strings without ref
    // if there are variable sized parameters for send and NOT for receive
    // -> add var of send to var sized count
    if ((nTempSend > 0) && (nTempRecv == 0))
    {
        if (m_bCountAllVarsAsMax)
        {
            unsigned int nMaxSend = pFunction->GetMaxSize(nSendDir, pContext);// + pFunction->GetFixedSize(nSendDir, pContext);
            (*iterSend).nCount = MAX(nMaxSend, (*iterSend).nCount);
        }
        else
        {
	    vector<struct TypeCount>::iterator iter =
		GetCountIter(TYPE_VARSIZED, nSendDir);
            (*iter).nCount = MAX(nTempSend, (*iter).nCount);
        }
    }
    // if there are variable sized parameters for recv and NOT for send
    // -> get max of receive (dont't forget "normal" fixed) and max with fixed
    // pFunction->GetMaxSize includes the normal fixed size already
    if ((nTempSend == 0) && (nTempRecv > 0))
    {
        unsigned int nMaxRecv = pFunction->GetMaxSize(nRecvDir, pContext);// + pFunction->GetFixedSize(nRecvDir, pContext);
        (*iterRecv).nCount = MAX(nMaxRecv, (*iterRecv).nCount);
    }
    // if there are variable sized parameters for send AND recveive
    // -> add recv max to fixed AND if send max bigger, set var sized of send
    if ((nTempSend > 0) && (nTempRecv > 0))
    {
        unsigned int nMaxSend = pFunction->GetMaxSize(nSendDir, pContext);
        unsigned int nMaxRecv = pFunction->GetMaxSize(nRecvDir, pContext);
        (*iterRecv).nCount = MAX(nMaxRecv, (*iterRecv).nCount);
        if (nMaxSend > nMaxRecv)
        {
            if (m_bCountAllVarsAsMax)
                (*iterSend).nCount = MAX(nMaxSend, (*iterSend).nCount);
            else
            {
                vector<struct TypeCount>::iterator iter =
                    GetCountIter(TYPE_VARSIZED, nSendDir);
                (*iter).nCount = MAX(nTempSend, (*iter).nCount);
            }
        }
        else
            // count send max as fixed, so we do not forget this value
            (*iterSend).nCount = MAX(nMaxSend, (*iterSend).nCount);
    }

    // count indirect strings (we count all parameters with the ref attribute)
    nTempSend = pFunction->GetParameterCount(ATTR_REF, 0, nSendDir);
    nTempRecv = pFunction->GetParameterCount(ATTR_REF, 0, nRecvDir);
    iterSend = GetCountIter(TYPE_STRING, nSendDir);
    iterRecv = GetCountIter(TYPE_STRING, nRecvDir);
    (*iterSend).nCount = MAX(nTempSend, (*iterSend).nCount);
    (*iterRecv).nCount = MAX(nTempRecv, (*iterRecv).nCount);
    // resize the maxima vector, if necessary
    if (m_vMaxima[nSendDir-1].size() < MAX(nTempSend, nTempRecv) )
    {
        m_vMaxima[nSendDir-1].resize(MAX(nTempSend, nTempRecv), 0);
        m_vMaxima[nRecvDir-1].resize(MAX(nTempSend, nTempRecv), 0);
    }
    // get maxima for function
    int nCurrentString = 0;
    vector<CBETypedDeclarator*>::iterator iter = pFunction->GetFirstSortedParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = pFunction->GetNextSortedParameter(iter)) != 0)
    {
        if (!(pParameter->FindAttribute(ATTR_REF)))
            continue;
        CBEAttribute *pAttr = pParameter->FindAttribute(ATTR_MAX_IS);
        if (!pAttr)
            continue;
        nTempSend = pAttr->GetIntValue();
        if (pParameter->IsDirection(nSendDir))
            m_vMaxima[nSendDir-1][nCurrentString] = MAX(nTempSend,
                (unsigned int)m_vMaxima[nSendDir-1][nCurrentString]);
        if (pParameter->IsDirection(nRecvDir))
            m_vMaxima[nRecvDir-1][nCurrentString] = MAX(nTempSend,
                (unsigned int)m_vMaxima[nRecvDir-1][nCurrentString]);
        nCurrentString++;
    }

    // set reference to function
    m_pFunction = pFunction;
}

/** \brief writes the initialization code for the message buffer
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This initialization does mostly initialize a variable size message buffer.
 * To initialize it we first have to calculate the size using the size_is, length_is or max_is attributes
 * of the variable sized parameters. The size is the maximum of the IN and the OUT buffer size
 * (don't forget the opcode size) plus the base size for size, send and fpage dopes.
 *
 * Then we allocate the message buffer on the stack using 'alloca()' (The whole buffer has to
 * be allocated on the stack, not just the byte array!)
 *
 * First we count all fixed sized values, which we obtain using GetSize() and then we dword align
 * this value and store it in the offset variable.
 *
 * The we add all variable sized parameters' sizes.
 *
 * Regard that OUT variable sized parameters need max storage amount in msg buffer!!!
 */
void CL4BEMsgBufferType::WriteInitialization(CBEFile * pFile, CBEContext * pContext)
{
    string sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
    string sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);

    if (IsVariableSized())
    {
        // use offset var for size calculation
        pFile->PrintIndent("%s = ", sOffset.c_str());
        // get size of payload
        WriteSizeOfPayload(pFile, pContext);
        // add basic size
        int nStartOfMsgBuffer = pContext->GetSizes()->GetSizeOfEnvType(string("l4_fpage_t"))
                              + pContext->GetSizes()->GetSizeOfEnvType(string("l4_msgdope_t"))*2;
        pFile->Print("+%d;\n", nStartOfMsgBuffer);
        // dword align size
        pFile->PrintIndent("%s = (%s+3) & ~3;\n", sOffset.c_str(), sOffset.c_str());
        // allocate message data structure
        pFile->PrintIndent("%s = ", sMsgBuffer.c_str());
        m_pType->WriteCast(pFile, true, pContext);
        pFile->Print("_dice_alloca(%s);\n", sOffset.c_str());
    }
    // if we receive flexpages, we have to initialize the fpage member
    // this is done in the receive/wait functions explicetly. Therefore we
    // can remove it here
}

/**    \brief initializes members of the message buffer
 *    \param pFile the file to write to
 *    \param nType the type of the member to initialize
 *    \param nDirection the direction of the type to initialize
 *    \param pContext the context of the writing
 *
 * This implementation does nothing
 */
void
CL4BEMsgBufferType::WriteInitialization(CBEFile *pFile,
    unsigned int nType,
    int nDirection,
    CBEContext *pContext)
{
    switch (nType)
    {
    case TYPE_REFSTRING:
        WriteReceiveIndirectStringInitialization(pFile, pContext);
        break;
    case TYPE_RCV_FLEXPAGE:
    case TYPE_FLEXPAGE:
        WriteReceiveFlexpageInitialization(pFile, nDirection, pContext);
        break;
    case TYPE_MSGDOPE_SIZE:
        WriteSizeDopeInit(pFile, pContext);
        break;
    case TYPE_MSGDOPE_SEND:
        if (nDirection == 0)
            WriteSendDopeInit(pFile, pContext);
        else
            WriteSendDopeInit(pFile, nDirection, pContext);
        break;
    default:
        break;
    }
}

/** \brief writes the string needed to obtain the size of the byte buffer
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BEMsgBufferType::WriteSizeOfBytes(CBEFile *pFile, CBEContext *pContext)
{
    int nBufferSize = MAX(GetCount(TYPE_FIXED, DIRECTION_IN),
                          GetCount(TYPE_FIXED, DIRECTION_OUT));
    // flexpages?
    int nFlexpages = MAX(GetCount(TYPE_FLEXPAGE, DIRECTION_IN),
                         GetCount(TYPE_FLEXPAGE, DIRECTION_OUT));
    if (nFlexpages > 0)
        nBufferSize += (nFlexpages+1)*pContext->GetSizes()->GetSizeOfType(TYPE_FLEXPAGE);
    // add space for opcode / exception
    /* if we have a reference function, test if both attributes
     * (noopcode and noexceptions) are set. If so, do not add
     * opcode size.
     */
    bool bHaveBothAttrs = false;
    if (m_pFunction &&
        (m_pFunction->FindAttribute(ATTR_NOOPCODE) &&
         m_pFunction->FindAttribute(ATTR_NOEXCEPTIONS)))
        bHaveBothAttrs = true;
    if (!bHaveBothAttrs)
    {
        int nOpcodeSize = pContext->GetSizes()->GetOpcodeSize();
        int nExceptionSize = pContext->GetSizes()->GetExceptionSize();
        nBufferSize += (nOpcodeSize > nExceptionSize) ? nOpcodeSize : nExceptionSize;
    }
    // make dword aligned
    nBufferSize = (nBufferSize+3) & ~3;
    // declare constant size (at least opcode size)
    pFile->Print("%d", nBufferSize);
    // iterate over variable sized parameters
    WriteInitializationVarSizedParameters(pFile, pContext);
}

/** \brief writes the string needed to obtain the size of the indirect strings in bytes
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BEMsgBufferType::WriteSizeOfRefStrings(CBEFile *pFile, CBEContext *pContext)
{
    // get number of indirect strings
    int nStrings = MAX(GetCount(TYPE_STRING, DIRECTION_IN),
                       GetCount(TYPE_STRING, DIRECTION_OUT));
    if (nStrings > 0)
    {
        // add size of strings
        pFile->Print("+sizeof(l4_strdope_t)");
        if (nStrings > 1)
            pFile->Print("*%d", nStrings);
    }
}

/** \brief write the string needed to obtain the size of the payload in bytes
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BEMsgBufferType::WriteSizeOfPayload(CBEFile *pFile, CBEContext *pContext)
{
    // get number of indirect strings
    int nStrings = MAX(GetCount(TYPE_STRING, DIRECTION_IN),
        GetCount(TYPE_STRING, DIRECTION_OUT));
    // if variable sized, we have to align the size of the dwords to the
    // size of l4_strdope_t
    if (nStrings > 0)
        pFile->Print("((");
    WriteSizeOfBytes(pFile, pContext);
    if (nStrings > 0)
    {
        // align bytes
        pFile->Print("+0xf) & ~0xf)");
    }
    // add strings
    WriteSizeOfRefStrings(pFile, pContext);
}

/** \brief initializes the flexpage member of the message buffer
 *  \param pFile the file to write to
 *    \param nDirection the direction which is receive
 *  \param pContext the context of the write operation
 *
 * If flexpage can be received by this function, we have to inialize the receive window.
 * We will not have a TYPE_RCV_FLEXPAGE parameter, because the receive window is hidden in
 * the CORBA environment. Ths we have to find flexpages and check whether we might receive some.
 * This is easy here, because call can only be at client side, so flexpages with out count.
 *
 * Before using it, test if it is NULL. If it is we have to init the fpage descriptor with a default
 * value
 */
void CL4BEMsgBufferType::WriteReceiveFlexpageInitialization(CBEFile *pFile, int nDirection, CBEContext *pContext)
{
    if (GetCount(TYPE_FLEXPAGE, nDirection) == 0)
        return;
    string sEnv = pContext->GetNameFactory()->GetCorbaEnvironmentVariable(pContext);
    // test on NULL
    pFile->PrintIndent("if (%s)\n", sEnv.c_str());
    // use env
    pFile->IncIndent();
    pFile->PrintIndent("");
    WriteMemberAccess(pFile, TYPE_RCV_FLEXPAGE, nDirection, pContext);
    pFile->Print(" = %s->rcv_fpage;\n", sEnv.c_str());
    pFile->DecIndent();
    // else use default
    pFile->PrintIndent("else\n");
    pFile->IncIndent();
    pFile->PrintIndent("");
    WriteMemberAccess(pFile, TYPE_RCV_FLEXPAGE, nDirection, pContext);
    pFile->Print(" = l4_fpage(0, L4_WHOLE_ADDRESS_SPACE, L4_FPAGE_RW, L4_FPAGE_GRANT);\n");
    pFile->DecIndent();
}

/** \brief initialize indirect strings if they can be received
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * We have to iterate over the parameters and initialize for each the receive string.
 *
 * This function is either called by a "normal" function - the call-function or
 * the receive-any-function or the server loop function.
 *
 * \todo make receive string allocation fixed size
 * \todo if variable sized msg-buffer, where do we start to set strings (need receive sizes all params, which are not refstr)
 * \todo calculate the maximum all possible string for each indirect string and use this to allocate memory
 */
void CL4BEMsgBufferType::WriteReceiveIndirectStringInitialization(CBEFile *pFile, CBEContext *pContext)
{
    CBEFunction *pFunction = dynamic_cast<CBEFunction*>(GetParent());
    int nDirection = 0;
    if (pFunction)
        nDirection = pFunction->GetReceiveDirection();
    // get an array of values, which contain the maxima
    int nStrings = GetCount(TYPE_STRING, nDirection);
    int nMaxStrSize = pContext->GetSizes()->GetMaxSizeOfType(TYPE_CHAR);
    // get the offset variable (just in case we need it)
    string sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
    // if we have an variable sized string, we need to offset into the bytes member of the message
    // buffer. For this we need an offset. This function is called after the size dope of the
    // message has been set, which is the number of dwords used for variable sized parameters.
    // we get the offset for the cast by multiplying this number with the size of an mword
    if (IsVariableSized())
    {
        char sNumber[5];
        snprintf(sNumber, 5, "*%d",
            pContext->GetSizes()->GetSizeOfType(TYPE_MWORD));
        sOffset += sNumber;
    }
    else
        sOffset = "";
    // iterate over the number of indirect strings, and init them
    vector<CBETypedDeclarator*>::iterator iter;
    CBETypedDeclarator *pParameter = 0;
    if (pFunction)
        iter = pFunction->GetFirstSortedParameter();
    string sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
    for (int i=0; i < nStrings; i++)
    {
        // get next indirect string parameter (with ATTR_REF)
        if (pFunction)
        {
            do {
                pParameter = pFunction->GetNextSortedParameter(iter);
            } while ((pParameter && !pParameter->FindAttribute(ATTR_REF)) ||
                     (pParameter && !pParameter->IsDirection(nDirection)));
        }
        int nStrSize = 0;
        if (!m_vMaxima[nDirection-1].empty())
            nStrSize = m_vMaxima[nDirection-1][i];
        // only if there is none defining a maximum for this string
        // we use the string maximum
        if (nStrSize == 0)
            nStrSize = nMaxStrSize;
        // get message buffer variables
        // allocate maximum memory
        // check if we use the user-provided function
        CBEClass *pClass = 0;
        if (pFunction)
            pClass = pFunction->GetClass();
        if (pContext->IsOptionSet(PROGRAM_INIT_RCVSTRING) ||
            (pClass &&
            (pClass->FindAttribute(ATTR_INIT_RCVSTRING) ||
             pClass->FindAttribute(ATTR_INIT_RCVSTRING_CLIENT) ||
             pClass->FindAttribute(ATTR_INIT_RCVSTRING_SERVER))))
        {
            // call is <function-name>(<string-number>, <pointer to string>, <pointer to size>)
            string sFuncName;
            if (pClass &&
                (pClass->FindAttribute(ATTR_INIT_RCVSTRING) ||
                 pClass->FindAttribute(ATTR_INIT_RCVSTRING_CLIENT) ||
                 pClass->FindAttribute(ATTR_INIT_RCVSTRING_SERVER)))
            {
                CBEAttribute *pAttr = 0;
                if ((pAttr = pClass->FindAttribute(ATTR_INIT_RCVSTRING)) != NULL)
                    sFuncName = pAttr->GetString();
                if (((pAttr = pClass->FindAttribute(ATTR_INIT_RCVSTRING_CLIENT)) != NULL) &&
                    pFile->IsOfFileType(FILETYPE_CLIENT))
                    sFuncName = pAttr->GetString();
                if (((pAttr = pClass->FindAttribute(ATTR_INIT_RCVSTRING_SERVER)) != NULL) &&
                    pFile->IsOfFileType(FILETYPE_COMPONENT))
                    sFuncName = pAttr->GetString();
            }
            if (sFuncName.empty())
                sFuncName = pContext->GetNameFactory()->GetString(STR_INIT_RCVSTRING_FUNC, pContext);
            else
                sFuncName = pContext->GetNameFactory()->GetString(STR_INIT_RCVSTRING_FUNC, pContext, (void*)&sFuncName);
            // get env variable
            string sEnv = pContext->GetNameFactory()->GetCorbaEnvironmentVariable(pContext);
            // call the init function for the indorect string
            pFile->PrintIndent("%s( %d, &(", sFuncName.c_str(), i);
            WriteMemberAccess(pFile, TYPE_REFSTRING, DIRECTION_OUT, pContext, sOffset);
            pFile->Print("[%d].rcv_str), &(", i);
            WriteMemberAccess(pFile, TYPE_REFSTRING, DIRECTION_OUT, pContext, sOffset);
            pFile->Print("[%d].rcv_size), ", i);
            // only if parent is server-loop, we have to test if the environment is a pointer
            if (dynamic_cast<CBESrvLoopFunction*>(pFunction))
            {
                if (!((CBESrvLoopFunction*)pFunction)->DoUseParameterAsEnv(pContext))
                    pFile->Print("&");
            }
            pFile->Print("%s);\n", sEnv.c_str());
        }
        else
        {
            if (pParameter && pParameter->FindAttribute(ATTR_PREALLOC))
            {
                pFile->PrintIndent("");
                WriteMemberAccess(pFile, TYPE_REFSTRING, DIRECTION_OUT, pContext, sOffset);
                *pFile << "[" << i << "].rcv_str = (" << sMWord << ")(";
                vector<CBEDeclarator*>::iterator iterD = pParameter->GetFirstDeclarator();
                CBEDeclarator *pD = *iterD;
                int nStart = 1, nStars;
                CBEType *pType = pParameter->GetType();
                if (pType->IsPointerType())
                    nStart--;
                for (nStars=nStart; nStars<pD->GetStars(); nStars++)
                    *pFile << "*";
                *pFile << pD->GetName() << ");\n";
                // set receive size
                pFile->PrintIndent("");
                WriteMemberAccess(pFile, TYPE_REFSTRING, DIRECTION_OUT, pContext, sOffset);
                *pFile << "[" << i << "].rcv_size = ";
                if ((pParameter->FindAttribute(ATTR_SIZE_IS)) ||
                    (pParameter->FindAttribute(ATTR_LENGTH_IS)))
                {
                    if (pType->GetSize() > 1)
                        pFile->Print("(");
                    pParameter->WriteGetSize(pFile, NULL, pContext);
                    if (pType->GetSize() > 1)
                    {
                        pFile->Print(")");
                        pFile->Print("*sizeof");
                        pType->WriteCast(pFile, false, pContext);
                    }
                }
                else
                    pParameter->WriteGetSize(pFile, NULL, pContext);
                *pFile << ";\n";
            }
            else
            {
                pFile->PrintIndent("");
                WriteMemberAccess(pFile, TYPE_REFSTRING, DIRECTION_OUT, pContext, sOffset);
                pFile->Print("[%d].rcv_str = (%s)", i, sMWord.c_str());
                pContext->WriteMalloc(pFile, pFunction);
                pFile->Print("(%d);\n", nStrSize);
                // set receive size
                pFile->PrintIndent("");
                WriteMemberAccess(pFile, TYPE_REFSTRING, DIRECTION_OUT, pContext, sOffset);
                pFile->Print("[%d].rcv_size = %d;\n", i, nStrSize);
            }
        }
    }
}

/** \brief sets the receive indirect strings to zero
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This function is used instead of the init function to zero the memory
 * of the receive buffer.
 *
 * We assume that rcv_str points to valid memory, and rcv_size is the correct
 * size of that memory.
 */
void CL4BEMsgBufferType::WriteReceiveIndirectStringSetZero(CBEFile *pFile, CBEContext *pContext)
{
    CBEFunction *pFunction = dynamic_cast<CBEFunction*>(GetParent());
    int nDirection = 0;
    if (pFunction)
        nDirection = pFunction->GetReceiveDirection();
    // iterate over the number of indirect strings, and init them
    int nStrings = GetCount(TYPE_STRING, nDirection);
    for (int i=0; i < nStrings; i++)
    {
        // get message buffer variables
        pFile->PrintIndent("memset((void*)((");
        WriteMemberAccess(pFile, TYPE_REFSTRING, DIRECTION_OUT, pContext);
        pFile->Print("[%d]).rcv_str), 0, (", i);
        WriteMemberAccess(pFile, TYPE_REFSTRING, DIRECTION_OUT, pContext);
        pFile->Print("[%d]).rcv_size);\n", i);
    }
}

/** \brief fills the message buffer with zeros
 *  \param pFile the file to write to
 *    \param nType the type of the member to initialize with zero
 *    \param nDirection the direction of the message
 *  \param pContext the context of the write operation
 */
void
CL4BEMsgBufferType::WriteSetZero(CBEFile *pFile,
    unsigned int nType,
    int nDirection,
    CBEContext *pContext)
{
    switch (nType)
    {
    case TYPE_FLEXPAGE:
        WriteReceiveIndirectStringSetZero(pFile, pContext);
        break;
    default:
        break;
    }
}

/** \brief init the internal count variables
 *  \param pClass the class to count for
 *  \param pContext the context of the initial counting
 *
 * We have to check the results of the counting after the functions have been
 * counted. The resons is this: this method is called for a server's message buffer,
 * which is NOT dynamically allocated, but has always to be the maximum size to fit
 * all possible messages. Therefore, we cannot use variable sized parameters.
 *
 * We circumwent this by setting an internal status flag, which tells the function
 * counting to count ALL variable sized parameters with their max values.
 */
void CL4BEMsgBufferType::InitCounts(CBEClass * pClass, CBEContext *pContext)
{
    m_bCountAllVarsAsMax = true;
    CBEMsgBufferType::InitCounts(pClass, pContext);
    m_bCountAllVarsAsMax = false;
}

/** \brief wraps the access to indirect parts
 *  \param pFile the file to write to
 *  \param nIndex the index of the indirect part to access
 *  \param pContext the context of the write operation
 *
 * Since the message buffer can be of a base type,the string member might
 * point to a location which is not the location we want. Therefore we have
 * to use the size dope of the message buffer to find the correct location
 * of the string.
 *
 * It looks like this:
 * msgbuf->_buffer[msgbuf->size.md.dwords*4+%d*sizeof(l4strdope_t)]
 */
/*void CL4BEMsgBufferType::WriteIndirectPartAccess(CBEFile* pFile, int nIndex, CBEContext* pContext)
{
    pFile->Print("(*(l4_strdope_t*)(&");
    WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("[");
    WriteMemberAccess(pFile, TYPE_MSGDOPE_SIZE, DIRECTION_IN, pContext);
    pFile->Print(".md.dwords*4");
    if (nIndex > 0)
        pFile->Print(" + %d*sizeof(l4_strdope_t)", nIndex);
    pFile->Print("]))");
}*/

/** \brief writes the access to the members of this type
 *  \param pFile the file to write to
 *  \param nMemberType the type of the member to access
 *  \param nDirection determines the direction of the message
 *  \param pContext the context of the write operation
 *  \param sOffset a possible offset variable if the member access has to be done using a cast of another type
 *
 * We need to implement a special case for TYPE_REFSTRING. They are not members
 * of the message buffer structure if it is variable size. Therefore we have to
 * cast the TYPE_INTEGER member to the l4_strdope_t type and use this.
 */
void
CL4BEMsgBufferType::WriteMemberAccess(CBEFile * pFile,
    int nMemberType,
    int nDirection,
    CBEContext * pContext,
    string sOffset)
{
    if ((nMemberType == TYPE_REFSTRING) && IsVariableSized())
    {
        // create l4_strdope_t
        CBEUserDefinedType *pType = pContext->GetClassFactory()->GetNewUserDefinedType();
        pType->SetParent(this);
        if (!pType->CreateBackEnd(string("l4_strdope_t"), pContext))
        {
            pFile->Print(")");
            return;
        }
        // ((l4_strdope_t*)(&(_buffer->_bytes[offset])))
        pFile->Print("(");
        // cast of l4_strdope_t type
        pType->WriteCast(pFile, true, pContext);
        // get pointer to offset in message buffer
        pFile->Print("(&");
        WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
        pFile->Print("[%s]))", sOffset.c_str());
        // done
        delete pType;
        return;
    }
    // print variable name
    string sName = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    pFile->Print("%s", sName.c_str());
    CBEDeclarator *pDecl = GetAlias();
    if ((pDecl->GetStars() > 0) || IsVariableSized())
        pFile->Print("->");
    else
        pFile->Print(".");
    sName = pContext->GetNameFactory()->GetMessageBufferMember(nMemberType, pContext);
    pFile->Print("%s", sName.c_str());
}

/** \brief writes the access to the members of this type
 *  \param pFile the file to write to
 *  \param pParameter the parameter to access
 *  \param nDirection determines the direction of the message
 *  \param pContext the context of the write operation
 *  \param sOffset a possible offset variable if the member access has to be done using a cast of another type
 *
 * Using this function, a specific member is accessed.
 */
void CL4BEMsgBufferType::WriteMemberAccess(CBEFile * pFile,
    CBETypedDeclarator *pParameter,
    int nDirection,
    CBEContext * pContext,
    string sOffset)
{
    assert(pParameter);
    assert(pParameter->GetType());
    int nMemberType = pParameter->GetType()->GetFEType();
    if ((nMemberType == TYPE_REFSTRING) && IsVariableSized())
    {
        // create l4_strdope_t
        CBEUserDefinedType *pType = pContext->GetClassFactory()->GetNewUserDefinedType();
        pType->SetParent(this);
        if (!pType->CreateBackEnd(string("l4_strdope_t"), pContext))
        {
            pFile->Print(")");
            return;
        }
        // ((l4_strdope_t*)(&(_buffer->_bytes[offset])))
        pFile->Print("(");
        // cast of l4_strdope_t type
        pType->WriteCast(pFile, true, pContext);
        // get pointer to offset in message buffer
        pFile->Print("(&");
        WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
        pFile->Print("[%s]))", sOffset.c_str());
        // done
        delete pType;
        return;
    }
    // print variable name
    string sName = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    pFile->Print("%s", sName.c_str());
    CBEDeclarator *pDecl = GetAlias();
    if ((pDecl->GetStars() > 0) || IsVariableSized())
        pFile->Print("->");
    else
        pFile->Print(".");
    sName = pContext->GetNameFactory()->GetMessageBufferMember(nMemberType, pContext);
    pFile->Print("%s", sName.c_str());
}

/** \brief creates a new instance of this class */
CObject * CL4BEMsgBufferType::Clone()
{
    return new CL4BEMsgBufferType(*this);
}

/** \brief writes the init string for the size dope
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BEMsgBufferType::WriteSizeDopeInit(CBEFile *pFile, CBEContext *pContext)
{
    // get minimum size
    CL4BESizes *pSizes = (CL4BESizes*)pContext->GetSizes();
    int nMin = pSizes->GetMaxShortIPCSize(DIRECTION_IN) / pSizes->GetSizeOfType(TYPE_MWORD);
    // calculate dwords
    int nDwordsIn = GetCount(TYPE_FIXED, DIRECTION_IN);
    int nDwordsOut = GetCount(TYPE_FIXED, DIRECTION_OUT);
    // calculate flexpages
    int nFlexpageIn = GetCount(TYPE_FLEXPAGE, DIRECTION_IN);
    int nFlexpageOut = GetCount(TYPE_FLEXPAGE, DIRECTION_OUT);
    int nFlexpageSize = pContext->GetSizes()->GetSizeOfType(TYPE_FLEXPAGE);
    if (nFlexpageIn > 0)
        nDwordsIn += (nFlexpageIn+1)*nFlexpageSize;
    if (nFlexpageOut > 0)
        nDwordsOut += (nFlexpageOut+1)*nFlexpageSize;
    // make dwords
    nDwordsIn = (nDwordsIn+3) >> 2;
    nDwordsOut = (nDwordsOut+3) >> 2;
    // check minimum value
    if (nDwordsIn < nMin)
        nDwordsIn = nMin;
    int nDwords = MAX(nDwordsIn, nDwordsOut);

    // calculate strings
    int nStrings = MAX(GetCount(TYPE_STRING, DIRECTION_IN),
                       GetCount(TYPE_STRING, DIRECTION_OUT));

    string sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
    if (IsVariableSized())
    {
        // declare constant size
        pFile->PrintIndent("%s = ", sOffset.c_str());
        WriteSizeOfBytes(pFile, pContext);
        pFile->Print(";\n");
        // make dwords
        // if we have indirect strings, we have to set the size
        // of the dwords to all the dwords before the strings, because
        // the access to the strings start at the offset defined by the
        // dwords
        if (nStrings > 0)
        {
            int nStartOfMsgBuffer = pContext->GetSizes()->GetSizeOfEnvType(string("l4_fpage_t"))
                                  + pContext->GetSizes()->GetSizeOfEnvType(string("l4_msgdope_t"))*2;
            pFile->PrintIndent("%s = (((%s+0x%x) & ~0xf) - 0x%x) >> 2;\n", sOffset.c_str(), sOffset.c_str(), nStartOfMsgBuffer+0xf, nStartOfMsgBuffer);
        }
        else
            pFile->PrintIndent("%s = (%s+3) >> 2;\n", sOffset.c_str(), sOffset.c_str());
    }

    // size dope
    pFile->PrintIndent("");
    WriteMemberAccess(pFile, TYPE_MSGDOPE_SIZE, DIRECTION_IN, pContext);
    pFile->Print(" = L4_IPC_DOPE(");
    if (IsVariableSized())
        pFile->Print("%s", sOffset.c_str());
    else
        pFile->Print("%d", nDwords);
    pFile->Print(", %d);\n", nStrings);
}

/** \brief writes the init string for the send dope
 *  \param pFile the file to write to
 *  \param nSendDirection the direction for IN
 *  \param pContext the context of the write operation
 */
void 
CL4BEMsgBufferType::WriteSendDopeInit(CBEFile *pFile,
    int nSendDirection,
    CBEContext *pContext)
{
    // get minimum size
    CL4BESizes *pSizes = (CL4BESizes*)pContext->GetSizes();
    int nMin = pSizes->GetMaxShortIPCSize(DIRECTION_IN) / 
	pSizes->GetSizeOfType(TYPE_MWORD);
    // check if we use offset variable
    assert(m_pFunction);
    bool bHasSizeIsParams =
        (m_pFunction->GetParameterCount(ATTR_SIZE_IS, ATTR_REF, nSendDirection) > 0) ||
        (m_pFunction->GetParameterCount(ATTR_LENGTH_IS, ATTR_REF, nSendDirection) > 0) ||
        (m_pFunction->GetParameterCount(ATTR_STRING, ATTR_REF, nSendDirection) > 0);
    bool bUseOffset = IsVariableSized(nSendDirection) || bHasSizeIsParams;
    // after marshalling set the message dope
    CBENameFactory *pNF = pContext->GetNameFactory();
    if (bUseOffset)
    {
        string sOffset = pNF->GetOffsetVariable(pContext);
        // dword align size
        pFile->PrintIndent("%s = (%s+3) >> 2;\n",
                           sOffset.c_str(),
                           sOffset.c_str());
        // check minimum
        pFile->PrintIndent("%s = (%s<%d)?%d:%s;\n",
                           sOffset.c_str(),
                           sOffset.c_str(), nMin,
                           nMin, sOffset.c_str());
    }
    // send dope
    pFile->PrintIndent("");
    WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_IN, pContext);
    pFile->Print(" = L4_IPC_DOPE(");
    if (bUseOffset)
    {
        string sOffset = pNF->GetOffsetVariable(pContext);
        pFile->Print("%s", sOffset.c_str());
    }
    else
    {
        // calculate bytes
        int nDwordsIn = GetCount(TYPE_FIXED, nSendDirection);
        if (GetCount(TYPE_FLEXPAGE, nSendDirection) > 0)
            nDwordsIn += (GetCount(TYPE_FLEXPAGE, nSendDirection) + 1) *
                pSizes->GetSizeOfType(TYPE_FLEXPAGE);
        // make dwords
        nDwordsIn = (nDwordsIn+3) >> 2;
        // check minimum value
        if (nDwordsIn < nMin)
            nDwordsIn = nMin;
        pFile->Print("%d", nDwordsIn);
    }
    pFile->Print(", %d);\n", GetCount(TYPE_STRING, nSendDirection));
}

/** \brief writes the init string for the send dope if nothing to send
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This inits the send dope to a "zero" send dope, which contains (by default)
 * 2 dwords and no ref-strings
 */
void CL4BEMsgBufferType::WriteSendDopeInit(CBEFile *pFile, CBEContext *pContext)
{
    // send dope
    pFile->PrintIndent("");
    WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_IN, pContext);
    CL4BESizes *pSizes = (CL4BESizes*)pContext->GetSizes();
    int nSize = pSizes->GetMaxShortIPCSize(DIRECTION_IN) / pSizes->GetSizeOfType(TYPE_MWORD);
    pFile->Print(" = L4_IPC_DOPE(%d, 0);\n", nSize);
}

/** \brief tests if this message buffer is suited for a short IPC
 *  \param nDirection the direction to test
 *  \param pContext the context of the test
 *  \param nWords the number of words allowed for a short IPC
 *  \return true if short IPC
 */
bool CL4BEMsgBufferType::IsShortIPC(int nDirection, CBEContext *pContext, int nWords)
{
    if (nDirection == 0)
        return IsShortIPC(DIRECTION_IN, pContext, nWords) &&
               IsShortIPC(DIRECTION_OUT, pContext, nWords);

    if (GetCount(TYPE_VARSIZED, nDirection) > 0)
        return false;
    CL4BESizes *pSizes = (CL4BESizes*)(pContext->GetSizes());
    if (nWords == 0)
        nWords = pSizes->GetMaxShortIPCSize(nDirection);
    else
        nWords *= pSizes->GetSizeOfType(TYPE_MWORD);
    if (GetCount(TYPE_FIXED, nDirection) > (unsigned int)nWords)
        return false;
    if (GetCount(TYPE_STRING, nDirection) > 0)
        return false;
    if (GetCount(TYPE_FLEXPAGE, nDirection) > 0)
        return false;
    return true;
}

/** \brief uses the values of the given message buffer
 *  \param pMsgBuffer the message buffer to use the values from
 *  \param pContext the context of the operation
 */
void CL4BEMsgBufferType::InitCounts(CBEMsgBufferType * pMsgBuffer, CBEContext * pContext)
{
    CBEMsgBufferType::InitCounts(pMsgBuffer, pContext);
    CL4BEMsgBufferType *pL4Buffer = dynamic_cast<CL4BEMsgBufferType*>(pMsgBuffer);
    assert(pL4Buffer);
    m_vMaxima[0] = pL4Buffer->m_vMaxima[0];
    m_vMaxima[1] = pL4Buffer->m_vMaxima[1];
}

/** \brief writes a dumper function for the message buffer
 *  \param pFile the file to write to
 *  \param sResult the result dope string (if empty the send dope is used)
 *  \param pContext the context if the write operation
 */
void CL4BEMsgBufferType::WriteDump(CBEFile *pFile, string sResult, CBEContext *pContext)
{
    string sFunc = pContext->GetTraceMsgBufFunc();
    if (sFunc.empty())
        sFunc = string("printf");
    bool bSend = sResult.empty();
    // printf fpage
    pFile->PrintIndent("%s(\"", sFunc.c_str());
    WriteMemberAccess(pFile, TYPE_RCV_FLEXPAGE, DIRECTION_IN, pContext);
    pFile->Print(".fp = {\\n\\t grant:%%x,\\n\\t write:%%x,\\n\\t size:%%x,\\n\\t zero:%%x,\\n\\t page:%%x }\\n\", ");
    WriteMemberAccess(pFile, TYPE_RCV_FLEXPAGE, DIRECTION_IN, pContext);
    pFile->Print(".fp.grant, ");
    WriteMemberAccess(pFile, TYPE_RCV_FLEXPAGE, DIRECTION_IN, pContext);
    pFile->Print(".fp.write, ");
    WriteMemberAccess(pFile, TYPE_RCV_FLEXPAGE, DIRECTION_IN, pContext);
    pFile->Print(".fp.size, ");
    WriteMemberAccess(pFile, TYPE_RCV_FLEXPAGE, DIRECTION_IN, pContext);
    pFile->Print(".fp.zero, ");
    WriteMemberAccess(pFile, TYPE_RCV_FLEXPAGE, DIRECTION_IN, pContext);
    pFile->Print(".fp.page);\n");
    // print size dope
    pFile->PrintIndent("%s(\"", sFunc.c_str());
    WriteMemberAccess(pFile, TYPE_MSGDOPE_SIZE, DIRECTION_IN, pContext);
    pFile->Print(".md = {\\n\\t msg_deceited:%%x,\\n\\t fpage_received:%%x,\\n\\t msg_redirected:%%x,\\n\\t "
        "src_inside:%%x,\\n\\t snd_error:%%x,\\n\\t error_code:%%x,\\n\\t strings:%%x,\\n\\t dwords:%%x }\\n\", ");
    WriteMemberAccess(pFile, TYPE_MSGDOPE_SIZE, DIRECTION_IN, pContext);
    pFile->Print(".md.msg_deceited, ");
    WriteMemberAccess(pFile, TYPE_MSGDOPE_SIZE, DIRECTION_IN, pContext);
    pFile->Print(".md.fpage_received, ");
    WriteMemberAccess(pFile, TYPE_MSGDOPE_SIZE, DIRECTION_IN, pContext);
    pFile->Print(".md.msg_redirected, ");
    WriteMemberAccess(pFile, TYPE_MSGDOPE_SIZE, DIRECTION_IN, pContext);
    pFile->Print(".md.src_inside, ");
    WriteMemberAccess(pFile, TYPE_MSGDOPE_SIZE, DIRECTION_IN, pContext);
    pFile->Print(".md.snd_error, ");
    WriteMemberAccess(pFile, TYPE_MSGDOPE_SIZE, DIRECTION_IN, pContext);
    pFile->Print(".md.error_code, ");
    WriteMemberAccess(pFile, TYPE_MSGDOPE_SIZE, DIRECTION_IN, pContext);
    pFile->Print(".md.strings, ");
    WriteMemberAccess(pFile, TYPE_MSGDOPE_SIZE, DIRECTION_IN, pContext);
    pFile->Print(".md.dwords);\n");
    // print send dope
    if (bSend)
    {
        pFile->PrintIndent("%s(\"", sFunc.c_str());
        WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_IN, pContext);
        pFile->Print(".md = { msg_deceited:%%x,\\n\\t fpage_received:%%x,\\n\\t msg_redirected:%%x,\\n\\t "
            "src_inside:%%x,\\n\\t snd_error:%%x,\\n\\t error_code:%%x,\\n\\t strings:%%x,\\n\\t dwords:%%x }\\n\", ");
        WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_IN, pContext);
        pFile->Print(".md.msg_deceited, ");
        WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_IN, pContext);
        pFile->Print(".md.fpage_received, ");
        WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_IN, pContext);
        pFile->Print(".md.msg_redirected, ");
        WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_IN, pContext);
        pFile->Print(".md.src_inside, ");
        WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_IN, pContext);
        pFile->Print(".md.snd_error, ");
        WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_IN, pContext);
        pFile->Print(".md.error_code, ");
        WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_IN, pContext);
        pFile->Print(".md.strings, ");
        WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_IN, pContext);
        pFile->Print(".md.dwords);\n");
    }
    else
    {
        pFile->PrintIndent("%s(\"%s.md = { msg_deceited:%%x,\\n\\t fpage_received:%%x,\\n\\t msg_redirected:%%x,\\n\\t "
            "src_inside:%%x,\\n\\t snd_error:%%x,\\n\\t error_code:%%x,\\n\\t strings:%%x,\\n\\t dwords:%%x }\\n\", "
            "%s.md.msg_deceited, %s.md.fpage_received, %s.md.msg_redirected, %s.md.src_inside, "
            "%s.md.snd_error, %s.md.error_code, %s.md.strings, %s.md.dwords);\n", sFunc.c_str() ,
            sResult.c_str(), sResult.c_str(), sResult.c_str(), sResult.c_str(), sResult.c_str(),
            sResult.c_str(), sResult.c_str(), sResult.c_str(), sResult.c_str());
    }
    // print dwords
    pFile->PrintIndent("for (_i=0; _i<");
    if (pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF_DWORDS))
    {
        int nBytes = pContext->GetTraceMsgBufDwords()*4;
        pFile->Print("((%d <= ", nBytes);
        if (bSend)
            WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_IN, pContext);
        else
            pFile->Print("%s", sResult.c_str());
        pFile->Print(".md.dwords*4) ? %d : ", nBytes);
        if (bSend)
            WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_IN, pContext);
        else
            pFile->Print("%s", sResult.c_str());
        pFile->Print(".md.dwords*4)");
    }
    else
    {
        if (bSend)
            WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_IN, pContext);
        else
            pFile->Print("%s", sResult.c_str());
        pFile->Print(".md.dwords*4");
    }
    pFile->Print("; _i++)\n");
    pFile->PrintIndent("{\n");
    pFile->IncIndent();
    pFile->PrintIndent("if (_i%4 == 0)\n");
    pFile->IncIndent();
    pFile->PrintIndent("%s(\"dwords[%%d]:\", _i/4);\n", sFunc.c_str());
    pFile->DecIndent();
    pFile->PrintIndent("%s(\"%%02x \", ", sFunc.c_str());
    WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("[_i]);\n");
    pFile->PrintIndent("if (_i%4 == 3)\n");
    pFile->IncIndent();
    pFile->PrintIndent("%s(\"\\n\");\n", sFunc.c_str());
    pFile->DecIndent();
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
}

/** \brief writes the definition of the message buffer
 *  \param pFile the file to write to
 *  \param bTypedef true if a type definition should be written
 *  \param pContext the context of the write operation
 */
void CL4BEMsgBufferType::WriteDefinition(CBEFile* pFile,  bool bTypedef,  CBEContext* pContext)
{
    if (bTypedef &&
        dynamic_cast<CBEHeaderFile*>(pFile) &&
        !IsVariableSized())
    {
        // get minimum size
        CL4BESizes *pSizes = (CL4BESizes*)pContext->GetSizes();
        int nMin = pSizes->GetMaxShortIPCSize(DIRECTION_IN) / pSizes->GetSizeOfType(TYPE_MWORD);
        // calculate dwords
        int nDwordsIn = GetCount(TYPE_FIXED, DIRECTION_IN);
        int nDwordsOut = GetCount(TYPE_FIXED, DIRECTION_OUT);
        // calculate flexpages
        int nFlexpageIn = GetCount(TYPE_FLEXPAGE, DIRECTION_IN);
        int nFlexpageOut = GetCount(TYPE_FLEXPAGE, DIRECTION_OUT);
        int nFlexpageSize = pContext->GetSizes()->GetSizeOfType(TYPE_FLEXPAGE);
        if (nFlexpageIn > 0)
            nDwordsIn += (nFlexpageIn+1)*nFlexpageSize;
        if (nFlexpageOut > 0)
            nDwordsOut += (nFlexpageOut+1)*nFlexpageSize;
        // make dwords from bytes
        nDwordsIn = (nDwordsIn+3) >> 2;
        nDwordsOut = (nDwordsOut+3) >> 2;
        // check minimum value
        if (nDwordsIn < nMin)
            nDwordsIn = nMin;
        int nDwords = MAX(nDwordsIn, nDwordsOut);

        // calculate strings
        int nStrings = MAX(GetCount(TYPE_STRING, DIRECTION_IN),
            GetCount(TYPE_STRING, DIRECTION_OUT));
        // get const name
        string sName = pContext->GetNameFactory()->GetString(STR_MSGBUF_SIZE_CONST, pContext, this);
        // write size dope constant
        pFile->Print("#ifndef %s\n", sName.c_str());
        pFile->Print("#define %s L4_IPC_DOPE(%d, %d)\n", sName.c_str(), nDwords, nStrings);
        pFile->Print("#endif /* %s */\n\n", sName.c_str());

    }
    CBEMsgBufferType::WriteDefinition(pFile, bTypedef, pContext);
}

/**    \brief checks if a property is fulfilled by this message buffer
 *    \param nProperty the property to check for
 *    \param nDirection the direction to check
 *    \param pContext the context of it all
 *    \return true if property if fulfilled.
 */
bool CL4BEMsgBufferType::CheckProperty(int nProperty,
    int nDirection,
    CBEContext *pContext)
{
    switch (nProperty)
    {
    case MSGBUF_PROP_SHORT_IPC:
        return IsShortIPC(nDirection, pContext, 2);
        break;
    }
    return false;
}
