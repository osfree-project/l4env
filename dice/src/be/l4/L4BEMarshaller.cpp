/**
 *    \file    dice/src/be/l4/L4BEMarshaller.cpp
 *    \brief   contains the implementation of the class CL4BEMarshaller
 *
 *    \date    05/17/2002
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

#include "be/l4/L4BEMarshaller.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEFunction.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "be/BEMsgBufferType.h"

#include "TypeSpec-Type.h"
#include "Attribute-Type.h"

CL4BEMarshaller::CL4BEMarshaller()
{
    m_nTotalFlexpages = 0;
    m_nCurrentFlexpages = m_nTotalFlexpages-1;
    m_nCurrentString = 0;
}

/** destructs the L4 marshaller object */
CL4BEMarshaller::~CL4BEMarshaller()
{
}

/** \brief marshals a single declarator
 *  \param pType the type of the declarator to marshal
 *  \param nStartOffset the start offset into  the message buffer
 *  \param bUseConstOffset true if nStartOffset can be used to index the message buffer
 *  \param bIncOffsetVariable true if the offset variable should be incremented by this function
 *  \param bLastParameter true if this is the last parameter
 *  \param pContext the context of the marshalling
 *  \return the size of the marshalled data in bytes
 *
 * This implementation is responsible for marshalling flexpages and indirect strings.
 */
int CL4BEMarshaller::MarshalDeclarator(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bIncOffsetVariable, bool bLastParameter, CBEContext * pContext)
{
    if (pType->IsVoid())
        return 0;

    // check flexpages
    // do not test for TYPE_RCV_FLEXPAGE, because this is onyl type of
    // message buffer member
    if (pType->IsOfType(TYPE_FLEXPAGE))
    {
        return MarshalFlexpage(pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
    }
    // indirect strings are have to be checked here, because we
    // do not only want to transfer [ref, string], but all other [ref]
    // parameter as well
    if (m_pParameter->FindAttribute(ATTR_REF))
    {
        return MarshalIndirectString(pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
    }

    // call "normal marshalling"
    return CBEMarshaller::MarshalDeclarator(pType, nStartOffset, bUseConstOffset, bIncOffsetVariable, bLastParameter, pContext);
}

/** \brief marshals a flexpage
 *  \param pType the type of the flexpage
 *  \param nStartOffset the starting offset in the message buffer
 *  \param bUseConstOffset true if nStartOffset can be used to index message buffer
 *  \param bLastParameter true if this is the last parameter to be marshalled
 *  \param pContext the context of the marshalling
 *  \return the number marshalled bytes
 *
 * A flexpage is a constructed type, which consist of the members 'snd_base' and 'fpage'. The latter
 * is a union but has a member 'fpage' of typed dword. So it would be easiest to marshal two dwords,
 * by adding the declarators to the stack and using an own dword type.
 */
int CL4BEMarshaller::MarshalFlexpage(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext)
{
    // create type
    CBEType *pMemberType = pContext->GetClassFactory()->GetNewType(TYPE_INTEGER);
    pMemberType->SetParent(m_pParameter);
    if (!pMemberType->CreateBackEnd(false, 4, TYPE_INTEGER, pContext))
    {
        delete pMemberType;
        return 0;
    }
    // add the members of the flexpage type to the declarator stack and marshal them using their types
    int nSize = 0;
    // create and marshal first member
    CBEDeclarator *pMember = pContext->GetClassFactory()->GetNewDeclarator();
    pMember->SetParent(m_pParameter);
    if (!pMember->CreateBackEnd(string("snd_base"), 0, pContext))
    {
        delete pMember;
        delete pMemberType;
        return 0;
    }
    CDeclaratorStackLocation *pLoc = new CDeclaratorStackLocation(pMember);
    m_vDeclaratorStack.push_back(pLoc);
    nSize += MarshalDeclarator(pMemberType, nStartOffset, bUseConstOffset, false, bLastParameter, pContext);
    m_vDeclaratorStack.pop_back();
    delete pLoc;
    delete pMember;
    // create and marshal second
    pMember = pContext->GetClassFactory()->GetNewDeclarator();
    pMember->SetParent(m_pParameter);
    if (!pMember->CreateBackEnd(string("fpage.fpage"), 0, pContext))
    {
        delete pMember;
        delete pMemberType;
        return 0;
    }
    pLoc = new CDeclaratorStackLocation(pMember);
    m_vDeclaratorStack.push_back(pLoc);
    nSize += MarshalDeclarator(pMemberType, nStartOffset+nSize, bUseConstOffset, false, bLastParameter, pContext);
    m_vDeclaratorStack.pop_back();
    delete pLoc;
    delete pMember;
    delete pMemberType;
    // increase the fleypage count
    m_nCurrentFlexpages++;
    // the size of a marshalled flexpage is always 8 bytes (2 dwords)
    return nSize;
}

/** \brief marshals a complete parameter
 *  \param pFile the file to marshal to
 *  \param pParameter the parameter to marshal
 *  \param nStartOffset the starting offset into the message buffer
 *  \param bUseConstOffset true if nStartOffset can be used to index the message buffer
 *  \param bLastParameter true if this is the last parameter
 *  \param pContext the context of this marshalling
 *
 * We first call the base class implementation and then check whether total number of flexpages has been reached.
 * If it has, then all flexpages are marshalled and we write the flexpage seperator.
 */
int CL4BEMarshaller::Marshal(CBEFile * pFile, CBETypedDeclarator * pParameter, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext)
{
    int nSize = CBEMarshaller::Marshal(pFile, pParameter, nStartOffset, bUseConstOffset, bLastParameter, pContext);
    // check if flexpage limit is reached
    if (m_nCurrentFlexpages == m_nTotalFlexpages)
    {
        // write two zero dwords
        MarshalValue(4, 0, nStartOffset+nSize, bUseConstOffset, false, pContext);
        MarshalValue(4, 0, nStartOffset+nSize+4, bUseConstOffset, false, pContext);
        // set current flexpage count to less or more than total flexpage count
        m_nCurrentFlexpages++;
        // adapt size
        nSize += pContext->GetSizes()->GetSizeOfType(TYPE_FLEXPAGE);
     }
     // return size
     return nSize;
}

/** \brief marshals a whole function
 *  \param pFile the file to marshal to
 *  \param pFunction the function to marshal
 *  \param nFEType the type of the parameters to marshal (negative if not this type, zero if all)
 *  \param nNumber the number of parameters to marshal (0 if all, negative if all after abs(nNumber) parameters)
 *  \param nStartOffset the starting offset in the message buffer
 *  \param bUseConstOffset true if nStartOffset can be used to index message buffer
 *  \param pContext the context of the marshalling
 *  \return the size in bytes of the marshalled data
 *
 * This implementation has to count the total number of flexpages first and init the
 * current counter.
 */
int CL4BEMarshaller::Marshal(CBEFile * pFile, CBEFunction * pFunction, int nFEType, int nNumber, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext)
{
    m_nTotalFlexpages = 0;
    vector<CBETypedDeclarator*>::iterator iter = pFunction->GetFirstSortedParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = pFunction->GetNextSortedParameter(iter)) != 0)
    {
        // do not test for TYPE_RCV_FLEXPAGE, because this is type
        // of message buffer member
        if (pParameter->GetType()->IsOfType(TYPE_FLEXPAGE))
            m_nTotalFlexpages++;
    }
    if (m_nTotalFlexpages > 0)
        m_nCurrentFlexpages = 0;
    // call base class
    return CBEMarshaller::Marshal(pFile, pFunction, nFEType, nNumber, nStartOffset, bUseConstOffset, pContext);
}

/** \brief marshals an indirect string parameter
 *  \param pType the type of the indirect string
 *  \param nStartOffset the starting offset into the message buffer
 *  \param bUseConstOffset true if nStartOffset can be used to index the message buffer
 *  \param bLastParameter true if this is the last parameter to be marshalled
 *  \param pContext the context of the marshalling
 *  \return the size of the marshalled data in bytes
 *
 * Because the return value is used to increase the offset into the message buffer, and that's not
 * where we marshal to, we return zero.
 *
 * To marshal indirect strings, we use an internal variable to count which string is currently marshalled.
 *
 * To marshal an indirect string is fairly easy. We have to set the FE type of pType to TYPE_REFSTRING, which
 * makes the name factory generate the correct message buffer member.
 *
 * SENDING:
 *
 * (msgbuffer._string[&lt;current-string&gt;]).snd_str = &lt;var&gt;;
 * (msgbuffer._string[&lt;current-string&gt;]).snd_size = strlen(&lt;var&gt;);
 *
 * OR
 *
 * (msgbuffer._string[&lt;current-string&gt;]).snd_str = &lt;var&gt;;
 * (msgbuffer._string[&lt;current-string&gt;]).snd_size = &lt;size-var&gt;; // if size_is/length_is attribute
 *
 * \todo the functions receiving indirect strings have to init the rcv_str and rcv_size value
 *
 * RECEIVING:    (set before invocation)
 *
 * (msgbuffer._string[&lt;current-string&gt;]).rcv_str = &lt;var&gt;; // no allocation!
 * OR
 * (msgbuffer._string[&lt;current-string&gt;]).rcv_str = CORBA_alloc(&lt;size&gt;); // allocation
 * (msgbuffer._string[&lt;current-string&gt;]).rcv_size = &lt;size&gt;;
 *
 * If the message buffer is variable sized, it is declared as:
 * struct { ... char _byes[0]; l4_strdope_t _strings[x]; }
 * after allocation the struct on the stack, the access to _bytes[0] addresses the same
 * location as _strings[0] does. Therefore we have to marshal strings in a variable sized
 * message buffer using _bytes.
 */
int CL4BEMarshaller::MarshalIndirectString(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext)
{
    bool bUsePointer = !pType->IsPointerType();
    string sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
    CBEMsgBufferType *pMsgBuffer = m_pFunction->GetMessageBuffer();
    assert(pMsgBuffer);

    // if the message buffer is variable sized, we cannot
    // marshal into l4_strdope_t members, but have to marshal directly into the byte buffer
    bool bVarSized = pMsgBuffer->IsVariableSized();

    // a message buffer is also "variable sized" if the function is one of
    // CBEUnmarshalFunction or CBEMarshalFunction:
    // if interface A derives from B, then the message buffer of B can
    // contain more dwords than the one of A. If B's server loop passes its
    // message buffer into one of A's unmarshal or marshal functions, then
    // the _strings member is not where expected: therefore we have to
    // dynamically calculate an offset
    if (m_pFunction && m_pFunction->IsComponentSide())
        bVarSized = true;

    // if first ref-string in var-sized buffer: l4_strdope_t-align the offset
    if (bVarSized)
    {
        string sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        string sTmpOffset = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        if (m_nCurrentString == 0)
        {
            // save offset variable in temp offset variable (because it contains the actual marshalled size of dwords)
            *m_pFile << "\t" << sTmpOffset << " = " << sOffset << "; // save offset\n";
            // align offset to l4_strdope_t size
            *m_pFile << "\t" << sOffset << " = ";
            pMsgBuffer->WriteMemberAccess(m_pFile, TYPE_MSGDOPE_SIZE, DIRECTION_IN, pContext);
            *m_pFile << ".md.dwords*4;" <<
                " // set offset to l4_strdope_t-aligned size of dword buffer\n";
        }
        else
        {
            // set offset to next string
            *m_pFile << "\t" << sOffset << " += " <<
                pContext->GetSizes()->GetSizeOfEnvType(string("l4_strdope_t")) <<
                ";\n";
        }
    }
    // do "normal" refstring marshalling
    if (m_bMarshal)
    {
        bool bUseConst = false;
        // write snd_str
        m_pFile->PrintIndent("");
        if (bVarSized)
        {
            m_pFile->Print("(");
            WriteBuffer(string("l4_strdope_t"), nStartOffset, bUseConst, true, true, pContext);
            m_pFile->Print(")");
        }
        else
        {
            pMsgBuffer->WriteMemberAccess(m_pFile, TYPE_REFSTRING, DIRECTION_IN, pContext);
            m_pFile->Print("[%d]", m_nCurrentString);
        }
        m_pFile->Print(".snd_str = (%s)(", sMWord.c_str());
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack,
            bUsePointer, false, pContext);
        m_pFile->Print(");\n");
        // now marshal size
        m_pFile->PrintIndent("");
        if (bVarSized)
        {
            m_pFile->Print("(");
            WriteBuffer(string("l4_strdope_t"), nStartOffset, bUseConst, true, true, pContext);
            m_pFile->Print(")");
        }
        else
        {
            pMsgBuffer->WriteMemberAccess(m_pFile, TYPE_REFSTRING, DIRECTION_IN, pContext);
            m_pFile->Print("[%d]", m_nCurrentString);
        }
        m_pFile->Print(".snd_size = ");
        // first test size attributes
        if ((m_pParameter->FindAttribute(ATTR_SIZE_IS)) ||
            (m_pParameter->FindAttribute(ATTR_LENGTH_IS)))
        {
            if (pType->GetSize() > 1)
                m_pFile->Print("(");
            m_pParameter->WriteGetSize(m_pFile, &m_vDeclaratorStack, pContext);
            if (pType->GetSize() > 1)
            {
                m_pFile->Print(")");
                m_pFile->Print("*sizeof");
                pType->WriteCast(m_pFile, false, pContext);
            }
        }
        // then test string attribute
        else if (m_pParameter->FindAttribute(ATTR_STRING))
        {
            m_pFile->Print("strlen(");
            CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack,
                bUsePointer, false, pContext);
            m_pFile->Print(")+1");  // zero terminating character
        }
        // if we land here, this might still contain max attribute
        else
            m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
        m_pFile->Print(";\n");
        // go to next string
        m_nCurrentString++;
    }
    else
    {
        // write receive var
        m_pFile->PrintIndent("");
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack,
            bUsePointer, false, pContext);
        m_pFile->Print(" = ");
        // we have to use the type of the original parameter,
        // because this is the variable we fill with the values from
        // the message buffer and it is decalred with the original typed
        m_pParameter->GetType()->WriteCast(m_pFile, true, pContext);
        if (bVarSized)
        {
            bool bUseConst = false;
            m_pFile->Print("(");
            WriteBuffer(string("l4_strdope_t"), nStartOffset, bUseConst,
                true, true, pContext);
            m_pFile->Print(")");
        }
        else
        {
            pMsgBuffer->WriteMemberAccess(m_pFile, TYPE_REFSTRING, DIRECTION_OUT, pContext);
            m_pFile->Print("[%d]", m_nCurrentString);
        }
        m_pFile->Print(".rcv_str;\n");

        // we do not unmarshal the receive size, because:
        // it contains the number of bytes of the array, but we
        // want the size parameter to contain the number of
        // elements, and the rcv_size member of the indirect part
        // contains the size of the receive buffer, not the actual
        // received number of bytes

        // go to next string
        m_nCurrentString++;
    }
    // if last string restore offset, because we need it for send dope calculation
    if (bVarSized && (m_nCurrentString == pMsgBuffer->GetCount(TYPE_STRING)) && !bUseConstOffset)
    {
        string sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        string sTmpOffset = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        m_pFile->PrintIndent("%s = %s;\n", sOffset.c_str(), sTmpOffset.c_str());
    }
    if (bVarSized && !bUseConstOffset)
        return pContext->GetSizes()->GetSizeOfEnvType(string("l4_strdope_t"));
    return 0;
}
