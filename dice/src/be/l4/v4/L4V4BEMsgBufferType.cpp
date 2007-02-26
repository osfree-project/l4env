/**
 *    \file    dice/src/be/l4/v4/L4V4BEMsgBufferType.cpp
 *    \brief    contains the implementation of the class CL4V4BEMsgBufferType
 *
 *    \date    01/08/2004
 *    \author    Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
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

#include "be/l4/v4/L4V4BEMsgBufferType.h"
#include "be/BEFunction.h"
#include "be/BEContext.h"
#include "be/BEType.h"
#include "be/BEAttribute.h"
#include "be/l4/v4/L4V4BENameFactory.h"

#include "TypeSpec-Type.h"
#include "Attribute-Type.h"

#include "fe/FEUserDefinedType.h"
#include "fe/FEPrimaryExpression.h"
#include "fe/FEArrayDeclarator.h"
#include "fe/FETypedDeclarator.h"

CL4V4BEMsgBufferType::CL4V4BEMsgBufferType()
 : CL4BEMsgBufferType()
{
}

CL4V4BEMsgBufferType::CL4V4BEMsgBufferType(CL4V4BEMsgBufferType &src)
 : CL4BEMsgBufferType(src)
{
}

/** destroys the instance of this class */
CL4V4BEMsgBufferType::~CL4V4BEMsgBufferType()
{
}

/** \brief creates a copy of this instance
 *  \return a reference to the copy
 */
CObject* CL4V4BEMsgBufferType::Clone()
{
    return new CL4V4BEMsgBufferType(*this);
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
void CL4V4BEMsgBufferType::InitCounts(CBEClass* pClass,  CBEContext* pContext)
{
    // skip generic L4 implementation
    CBEMsgBufferType::InitCounts(pClass, pContext);
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
 * # variable sized parameters go into string items
 * # indirect string are simply counted (string items)
 * # so are flexpages (map items)
 *
 */
void CL4V4BEMsgBufferType::InitCounts(CBEFunction* pFunction,  CBEContext* pContext)
{
    assert(pFunction);
    // get directions
    int nSendDir = pFunction->GetSendDirection();
    int nRecvDir = pFunction->GetReceiveDirection();
    // some helper vars
    unsigned int nFlexpageCountSend = 0;
    unsigned int nFlexpageCountRecv = 0;
    unsigned int nRefCountSend = 0;
    unsigned int nRefCountRecv = 0;
    unsigned int nFixCountSend = 0;
    unsigned int nFixCountRecv = 0;
    // iterate over parameters
    vector<CBETypedDeclarator*>::iterator iterP = pFunction->GetFirstParameter();
    CBETypedDeclarator *pParam;
    CBEType *pType;
    CBEAttribute *pAttr;
    while ((pParam = pFunction->GetNextParameter(iterP)) != 0)
    {
        pType = pParam->GetType();
        if ((pAttr = pParam->FindAttribute(ATTR_TRANSMIT_AS)) != 0)
            pType = pAttr->GetAttrType();
        // if parameter is IN -> count for IN
        if (pParam->IsDirection(nSendDir))
        {
            // if it is flexpage: count as flexpage
            if (pType->IsOfType(TYPE_FLEXPAGE))
                nFlexpageCountSend++;
            else
            // if varaiable sized or struct: count as ref
            if (pParam->IsVariableSized() ||
                !pType->IsSimpleType())
                nRefCountSend++;
            // otherwise: simple/fixed
            else
                nFixCountSend += pType->GetSize();
        }
        // if patameter is OUT -> count for OUT
        if (pParam->IsDirection(nRecvDir))
        {
            // if it is flexpage: count as flexpage
            if (pType->IsOfType(TYPE_FLEXPAGE))
                nFlexpageCountRecv++;
            else
            // if variable sized of struct: count as ref
            if (pParam->IsVariableSized() ||
                !pType->IsSimpleType())
                nRefCountRecv++;
            // otherwise: simple/fixed
            else
                nFixCountRecv += pType->GetSize();
        }
    }
    // set member vars
    vector<struct CBEMsgBufferType::TypeCount>::iterator iter;
    iter = GetCountIter(TYPE_FLEXPAGE, nSendDir);
    (*iter).nCount = MAX(nFlexpageCountSend, (*iter).nCount);
    iter = GetCountIter(TYPE_FLEXPAGE, nRecvDir);
    (*iter).nCount = MAX(nFlexpageCountRecv, (*iter).nCount);
    iter = GetCountIter(TYPE_FIXED, nSendDir);
    (*iter).nCount = MAX(nFixCountSend, (*iter).nCount);
    iter = GetCountIter(TYPE_FIXED, nRecvDir);
    (*iter).nCount = MAX(nFixCountRecv, (*iter).nCount);
    iter = GetCountIter(TYPE_STRING, nSendDir);
    (*iter).nCount = MAX(nRefCountSend, (*iter).nCount);
    iter = GetCountIter(TYPE_STRING, nRecvDir);
    (*iter).nCount = MAX(nRefCountRecv, (*iter).nCount);
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
 * The L4 V4 message buffer consists of an array of 64 word elements
 * - l4_umword_t[] (an message buffer, containing direct values or flexpages)
 * Since the L4 headers already include a definition of a message type
 * (L4_Msg_t) we simply alias this type. So return a user defined type
 * "L4_Msg_t" to the caller.
 *
 */
CFETypeSpec*
CL4V4BEMsgBufferType::GetMsgBufferType(CFEInterface* pFEInterface,
    CFEDeclarator* &pFEDeclarator,
    CBEContext* pContext)
{
    CFETypeSpec *pType = new CFEUserDefinedType(string("L4_Msg_t"));
    return pType;
}

/**    \brief creates a message buffer type for a function
 *    \param pFEOperation the operation to create the message buffer for
 *    \param pFEDeclarator the declarator of the message buffer
 *    \param pContext the context of the create process
 *    \return the message buffer's type
 *
 * The number of necessary words can be calculated from the number of fixed
 * bytes (rounded to next word boundary) plus the number of indirect items
 * plus the numer of flexpage items.
 */
CFETypeSpec*
CL4V4BEMsgBufferType::GetMsgBufferType(CFEOperation* pFEOperation,
    CFEDeclarator* &pFEDeclarator,
    CBEContext* pContext)
{
#if 0
    int nWordsIn = (GetCount(TYPE_FIXED, DIRECTION_IN) + 3) >> 2;
    int nWordsOut = (GetCount(TYPE_FIXED, DIRECTION_OUT) + 3) >> 2;
    int nWordSize = pContext->GetSizes()->GetSizeOfType(TYPE_MWORD);
    int nSize = pContext->GetSizes()->GetSizeOfEnvType(string("StringItem")) / nWordSize;
    nWordsIn = GetCount(TYPE_STRING, DIRECTION_IN) * nSize;
    nWordsOut = GetCount(TYPE_STRING, DIRECTION_OUT) *nSize;
    nSize = pContext->GetSizes()->GetSizeOfEnvType(string("MapItem")) / nWordSize;
    nWordsIn = GetCount(TYPE_FLEXPAGE, DIRECTION_IN) * nSize;
    nWordsOut = GetCount(TYPE_FLEXPAGE, DIRECTION_OUT) * nSize;
    int nWords = MAX(nWordsIn, nWordsOut);
    // add "l4_mword_t buffer[64]" to type
    string sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
    CFETypeSpec *pType = new CFEUserDefinedType(sMWord);
    string sCharBuf = pContext->GetNameFactory()->GetMessageBufferMember(TYPE_MWORD, pContext);
    CFEExpression *pValue = new CFEPrimaryExpression(EXPR_INT, (long int) nWords);
    CFEDeclarator *pDecl = new CFEArrayDeclarator(sCharBuf, pValue);
    pValue = 0;
    vector<CFEDeclarator*> *pDeclarators = new vector<CFEDeclarator*>();
    pDeclarators->push_back(pDecl);
    pDecl = 0;
    CFETypedDeclarator *pMember = new CFETypedDeclarator(TYPEDECL_FIELD, pType, pDeclarators);
    pType = 0;
    pDeclarators = 0;
    //pMembers->Add(pMember);
    pMember = 0;
#endif

    CFETypeSpec *pType = new CFEUserDefinedType(string("L4_Msg_t"));
    return pType;
}

/** \brief writes the size dope initialization
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * For V4 there is nothing comparable to the V2/X0 size dope, because the message buffer
 * is in the UTCB area and has a fixed size of 64 words. So only the MR0 has to be set,
 * which is comparable to the V2/X0 send dope.
 */
void CL4V4BEMsgBufferType::WriteSizeDopeInit(CBEFile* pFile,  CBEContext* pContext)
{
    /* do nothing! */
}

/** \brief writes the send dope initialization
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This method is called for short IPC send dope initialization. There is no such
 * thing as a denoted short IPC for V4. We simply delegate the call to the
 * explicit initialization.
 */
void CL4V4BEMsgBufferType::WriteSendDopeInit(CBEFile* pFile,  CBEContext* pContext)
{
    CBEFunction *pFunction = dynamic_cast<CBEFunction*>(GetParent());
    int nDirection = 0;
    if (pFunction)
        nDirection = pFunction->GetSendDirection();
    WriteSendDopeInit(pFile, nDirection, pContext);
}

/** \brief writes the send dope initialization
 *  \param pFile the file to write to
 *  \param nSendDirection the direction to send
 *  \param pContext the context of the write operation
 *
 * This method is called for IPC send dope initialization. There is no such
 * thing as a V2/X0 size dope, but instead there are bitfields in the message
 * register 0, which should be used for that:
 * Bits 0-5 for numbers of untyped words
 * Bits 6-11 for number of typed items (refstrings + flexpages)
 * Bits 12-15 are padded with 0 (actually bit 12 denotes a propagated IPC)
 */
void CL4V4BEMsgBufferType::WriteSendDopeInit(CBEFile* pFile,  int nSendDirection,  CBEContext* pContext)
{
    string sMsgTag = pContext->GetNameFactory()->GetString(STR_MSGTAG_VARIABLE, pContext, 0);
    // get number of untyped words
    unsigned long nUntyped = GetCount(TYPE_FIXED, nSendDirection) + 3 >> 2;
    unsigned long nTyped = GetCount(TYPE_STRING, nSendDirection) +
        GetCount(TYPE_FLEXPAGE, nSendDirection);
    // now set mr0 = ((unsigned long)opcode << 16) | (((unsigned long)nTyped << 6) & 0xfb0)  | ((unsigned long)nUntyped & 0x3f)

    if (nUntyped > 0)
        *pFile << "\t" << sMsgTag << ".X.u = " << nUntyped << ";\n";
    if (nTyped > 0)
        *pFile << "\t" << sMsgTag << ".X.t = " << nTyped << ";\n";
}
