/** \file   dice/src/be/l4/v4/L4V4BEMsgBuffer.cpp
 *  \brief  contains the implementation of the class CL4V4BEMsgBuffer
 *
 *  \date   06/15/2004
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004 by
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
#include "be/l4/v4/L4V4BEMsgBuffer.h"
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

CL4V4BEMsgBuffer::CL4V4BEMsgBuffer()
: CL4BEClientMsgBuffer()
{
}

CL4V4BEMsgBuffer::CL4V4BEMsgBuffer(CL4V4BEMsgBuffer &src)
: CL4BEClientMsgBuffer(src)
{
}

/** destroys objects of this class */
CL4V4BEMsgBuffer::~CL4V4BEMsgBuffer()
{
}


/** \brief creates a copy of this instance
 *  \return a reference to the copy
 */
CObject* CL4V4BEMsgBuffer::Clone()
{
    return new CL4V4BEMsgBuffer(*this);
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
void CL4V4BEMsgBuffer::InitCounts(CBEClass* pClass,  CBEContext* pContext)
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
void CL4V4BEMsgBuffer::InitCounts(CBEFunction* pFunction,  CBEContext* pContext)
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
 *
 */
CFETypeSpec* CL4V4BEMsgBuffer::GetMsgBufferType(CFEInterface* pFEInterface,  CFEDeclarator* &pFEDeclarator,  CBEContext* pContext)
{
    // add "l4_mword_t buffer[64]" to type
    string sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
    CFETypeSpec *pType = new CFEUserDefinedType(sMWord);

    CFEExpression *pValue = new CFEPrimaryExpression(EXPR_INT, (long int) 64);
    CFEDeclarator *pNewDecl = new CFEArrayDeclarator(pFEDeclarator->GetName(), pValue);
    pNewDecl->SetParent(pFEDeclarator->GetParent());
    delete pFEDeclarator;
    pFEDeclarator = pNewDecl;

    return pType;
}

/**    \brief creates a message buffer type for a function
 *    \param pFEOperation the operation to create the message buffer for
 *    \param pFEDeclarator the declarator of the message buffer
 *    \param pContext the context of the create process
 *    \return the message buffer's type
 */
CFETypeSpec* CL4V4BEMsgBuffer::GetMsgBufferType(CFEOperation* pFEOperation,  CFEDeclarator* &pFEDeclarator,  CBEContext* pContext)
{
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

    CFEExpression *pValue = new CFEPrimaryExpression(EXPR_INT, (long int) nWords);

    CFEDeclarator *pNewDecl = new CFEArrayDeclarator(pFEDeclarator->GetName(), pValue);
    pNewDecl->SetParent(pFEDeclarator->GetParent());
    delete pFEDeclarator;
    pFEDeclarator = pNewDecl;

    return pType;
}

