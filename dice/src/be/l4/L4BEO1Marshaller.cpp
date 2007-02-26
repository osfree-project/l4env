/**
 *	\file	dice/src/be/l4/L4BEO1Marshaller.cpp
 *	\brief	contains the implementation of the class CL4BEO1Marshaller
 *
 *	\date	05/17/2002
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

#include "be/l4/L4BEO1Marshaller.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEFunction.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "be/BEMsgBufferType.h"

#include "fe/FETypeSpec.h"
#include "fe/FEAttribute.h"

IMPLEMENT_DYNAMIC(CL4BEO1Marshaller);

CL4BEO1Marshaller::CL4BEO1Marshaller()
{
    IMPLEMENT_DYNAMIC_BASE(CL4BEO1Marshaller, CBEO1Marshaller);
    m_nTotalFlexpages = 0;
    m_nCurrentFlexpages = m_nTotalFlexpages-1;
    m_nCurrentString = 0;
}

/** destructs the L4 marshaller object */
CL4BEO1Marshaller::~CL4BEO1Marshaller()
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
int CL4BEO1Marshaller::MarshalDeclarator(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bIncOffsetVariable, bool bLastParameter, CBEContext * pContext)
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
    return CBEO1Marshaller::MarshalDeclarator(pType, nStartOffset, bUseConstOffset, bIncOffsetVariable, bLastParameter, pContext);
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
int CL4BEO1Marshaller::MarshalFlexpage(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext)
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
    if (!pMember->CreateBackEnd(String("snd_base"), 0, pContext))
    {
        delete pMember;
        delete pMemberType;
        return 0;
    }
    m_vDeclaratorStack.Push(pMember);
    nSize += MarshalDeclarator(pMemberType, nStartOffset, bUseConstOffset, false, bLastParameter, pContext);
    m_vDeclaratorStack.Pop();
    delete pMember;
    // create and marshal second
    pMember = pContext->GetClassFactory()->GetNewDeclarator();
    pMember->SetParent(m_pParameter);
    if (!pMember->CreateBackEnd(String("fpage.fpage"), 0, pContext))
    {
        delete pMember;
        delete pMemberType;
        return 0;
    }
    m_vDeclaratorStack.Push(pMember);
    nSize += MarshalDeclarator(pMemberType, nStartOffset+nSize, bUseConstOffset, false, bLastParameter, pContext);
    m_vDeclaratorStack.Pop();
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
int CL4BEO1Marshaller::Marshal(CBEFile * pFile, CBETypedDeclarator * pParameter, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext)
{
    int nSize = CBEO1Marshaller::Marshal(pFile, pParameter, nStartOffset, bUseConstOffset, bLastParameter, pContext);
    // check if flexpage linit is reached
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
 *  \param nStartOffset the starting offset in the message buffer
 *  \param bUseConstOffset true if nStartOffset can be used to index message buffer
 *  \param pContext the context of the marshalling
 *  \return the size in bytes of the marshalled data
 *
 * This implementation has to count the total number of flexpages first and init the
 * current counter.
 */
int CL4BEO1Marshaller::Marshal(CBEFile * pFile, CBEFunction * pFunction, int nFEType, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext)
{
    m_nTotalFlexpages = 0;
    VectorElement *pIter = pFunction->GetFirstSortedParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = pFunction->GetNextSortedParameter(pIter)) != 0)
    {
        // do not test for TYPE_RCV_FLEXPAGE, because this is type
        // of message buffer member
        if (pParameter->GetType()->IsOfType(TYPE_FLEXPAGE))
            m_nTotalFlexpages++;
    }
    if (m_nTotalFlexpages > 0)
        m_nCurrentFlexpages = 0;
    // call base class
    return CBEO1Marshaller::Marshal(pFile, pFunction, nFEType, nStartOffset, bUseConstOffset, pContext);
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
int CL4BEO1Marshaller::MarshalIndirectString(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext)
{
    bool bUsePointer = !pType->IsPointerType();
    String sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
    CL4BEMsgBufferType *pMsgBuffer = (CL4BEMsgBufferType*)(m_pFunction->GetMessageBuffer());
    // if the message buffer is variable sized, we cannot
    // marshal into l4_strdope_t members, but have to marshal directly into the byte buffer
    bool bVarSized = pMsgBuffer->IsVariableSized();
    // if first ref-string in var-sized buffer: l4_strdope_t-align the offset
    if (bVarSized && (m_nCurrentString == 0) && !bUseConstOffset)
    {
        String sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        String sTmpOffset = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        // save offset variable in temp offset variable (because it contains the actual marshalled size of dwords)
        m_pFile->PrintIndent("%s = %s; // save offset \n", (const char*)sTmpOffset, (const char*)sOffset);
        // align offset to l4_strdope_t size
        m_pFile->PrintIndent("%s = ", (const char*)sOffset);
        pMsgBuffer->WriteMemberAccess(m_pFile, TYPE_MSGDOPE_SIZE, pContext);
        m_pFile->Print(".md.dwords*4;");
        m_pFile->Print(" // set offset to l4_strdope_t-aligned size of dword buffer\n");
    }
    // do "normal" refstring marshalling
    if (m_bMarshal)
    {
        // write snd_str
        m_pFile->PrintIndent("");
        if (bVarSized)
        {
            m_pFile->Print("(");
            WriteBuffer(String("l4_strdope_t"), nStartOffset, bUseConstOffset, true, pContext);
            m_pFile->Print(")");
        }
        else
        {
            pMsgBuffer->WriteMemberAccess(m_pFile, TYPE_REFSTRING, pContext);
            m_pFile->Print("[%d]", m_nCurrentString);
        }
        m_pFile->Print(".snd_str = (%s)(", (const char*)sMWord);
        m_vDeclaratorStack.Write(m_pFile, bUsePointer, false, pContext);
        m_pFile->Print(");\n");
        // now marshal size
        m_pFile->PrintIndent("");
        if (bVarSized)
        {
            m_pFile->Print("(");
            WriteBuffer(String("l4_strdope_t"), nStartOffset, bUseConstOffset, true, pContext);
            m_pFile->Print(")");
        }
        else
        {
            pMsgBuffer->WriteMemberAccess(m_pFile, TYPE_REFSTRING, pContext);
            m_pFile->Print("[%d]", m_nCurrentString);
        }
        m_pFile->Print(".snd_size = ");
        // first test size attributes
        if ((m_pParameter->FindAttribute(ATTR_SIZE_IS)) ||
            (m_pParameter->FindAttribute(ATTR_LENGTH_IS)))
            m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
        // then test string attribute
        else if (m_pParameter->FindAttribute(ATTR_STRING))
        {
            m_pFile->Print("strlen(");
            m_vDeclaratorStack.Write(m_pFile, bUsePointer, false, pContext);
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
        m_vDeclaratorStack.Write(m_pFile, bUsePointer, false, pContext);
        m_pFile->Print(" = ");
        pType->WriteCast(m_pFile, true, pContext);
        m_pFile->Print("");
        if (bVarSized)
        {
            m_pFile->Print("(");
            WriteBuffer(String("l4_strdope_t"), nStartOffset, bUseConstOffset, true, pContext);
            m_pFile->Print(")");
        }
        else
        {
            pMsgBuffer->WriteMemberAccess(m_pFile, TYPE_REFSTRING, pContext);
            m_pFile->Print("[%d]", m_nCurrentString);
        }
        m_pFile->Print(".rcv_str;\n");
        // write rcv size
        if (!(m_pParameter->FindAttribute(ATTR_STRING)))
        {
            m_pFile->PrintIndent("");
            m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
            m_pFile->Print(" = ");
            if (bVarSized)
            {
                m_pFile->Print("(");
                WriteBuffer(String("l4_strdope_t"), nStartOffset, bUseConstOffset, true, pContext);
                m_pFile->Print(")");
            }
            else
            {
                pMsgBuffer->WriteMemberAccess(m_pFile, TYPE_REFSTRING, pContext);
                m_pFile->Print("[%d]", m_nCurrentString);
            }
            m_pFile->Print(".rcv_size;\n", m_nCurrentString);
        }
        // go to next string
        m_nCurrentString++;
    }
    // if last string restore offset, because we need it for send dope calculation
    if (bVarSized && (m_nCurrentString == pMsgBuffer->GetRefStringCount()) && !bUseConstOffset)
    {
        String sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        String sTmpOffset = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        m_pFile->PrintIndent("%s = %s;\n", (const char*)sOffset, (const char*)sTmpOffset);
    }
    if (bVarSized)
        return pContext->GetSizes()->GetSizeOfEnvType(String("l4_strdope_t"));
    return 0;
}

/** \brief marshals a string
 *  \param pType the type of the string (usually CHAR_ASTERISK)
 *  \param nStartOffset the place where to start the string in the msg-buffer
 *  \param bUseConstOffset true if nStartOffset can be used
 *  \param bLastParameter true if this is the last parameter to be marshalled
 *  \param pContext the context of the marshalling
 *  \return the number of marshalled bytes if any
 */
int CL4BEO1Marshaller::MarshalString(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext)
{
    // check for indirect strings
    // indirect string if attributes [ref] exist
    // marshal/unmarshal
    return CBEO1Marshaller::MarshalString(pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
}
