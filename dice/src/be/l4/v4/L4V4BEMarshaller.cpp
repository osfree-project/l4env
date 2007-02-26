/**
 *    \file    dice/src/be/l4/v4/L4V4BEMarshaller.cpp
 *    \brief    contains the implementation of the class CL4V4BEMarshaller
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

#include "L4V4BEMarshaller.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEFunction.h"
#include "be/BEMsgBufferType.h"
#include "File.h"
#include "TypeSpec-Type.h"

CL4V4BEMarshaller::CL4V4BEMarshaller()
 : CL4BEMarshaller()
{
}

/** deletes the instance of this class */
CL4V4BEMarshaller::~CL4V4BEMarshaller()
{
}

/** \brief writes the assignment of a declarator to the msgbuffer or the other \
 *           way around
 *    \param pType the type of the current parameter
 *    \param nStartOffset the offset where to start in the message buffer
 *    \param bUseConstOffset true if nStartOffset should be used
 *    \param nAlignment the alignment of the parameter (if we do alignment)
 *    \param pContext the context of the whole writing
 *
 * For V4 we can use the convenience interface to marshal parameters
 */
void
CL4V4BEMarshaller::WriteAssignment(CBEType *pType,
    int nStartOffset,
    bool& bUseConstOffset,
    int nAlignment,
    CBEContext *pContext)
{
    // get current decl
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.back();
    // get name of msg_t
    string sMsgBuffer =
        pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    // true if parameter is pointer
    bool bUsePointer = m_pParameter->IsString();
    // check if msgbuffer is pointer
    bool bVarMsgBuf = false;
    if (m_pFunction && m_pFunction->GetMessageBuffer())
    {
        CBEMsgBufferType *pMsgBuf = m_pFunction->GetMessageBuffer();
        bVarMsgBuf = pMsgBuf->IsVariableSized();
        if (!bVarMsgBuf && pMsgBuf->GetAlias())
            bVarMsgBuf = pMsgBuf->GetAlias()->GetStars() > 0;
    }
    // marshal
    string sOffsetVar = pContext->GetNameFactory()->GetOffsetVariable(pContext);
    if (m_bMarshal)
    {
        *m_pFile << "\tL4_MsgAppendWord (" << ((bVarMsgBuf) ? "" : "&") <<
            sMsgBuffer << ", ";
        // if declarators original type is different from the type, we
        // cast the message buffer to, then we have to cast the declarator
        // to that type
        bool bUsePointer = m_pParameter->IsString();
        if (m_pType && pType && (m_pType->GetFEType() != pType->GetFEType()))
        {
            if ((m_pType->IsSimpleType() && pType->IsSimpleType()) ||
                (pCurrent->pDeclarator->GetStars() == 0))
                pType->WriteCast(m_pFile, false, pContext);
            else
            {
                // if parameter or transmit_as type are not simple,
                // we use the indirection of pointers to cast the
                // declarator
                *m_pFile << "*";
                pType->WriteCast(m_pFile, true, pContext);
                bUsePointer = true;
            }
        }
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack,
            bUsePointer, false, pContext);
        *m_pFile << ");\n";
    }
    else
    {
        // if the declarators original type is different from the type, we
        // cast the message buffer to, then we have to cast the message buffer
        // to that type instead
        if (!bUseConstOffset && nAlignment)
            *m_pFile << "\t" << sOffsetVar << " += " << nAlignment << ";\n";
        if (m_pType && !m_pType->IsVoid() &&
            pType && (m_pType->GetFEType() != pType->GetFEType()) )
        {
            *m_pFile << "\t";
            CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack,
                m_pParameter->IsString(), false, pContext);
            *m_pFile << " = ";

            if ((m_pType->IsSimpleType() && pType->IsSimpleType()) ||
                (pCurrent->pDeclarator->GetStars() == 0))
                pType->WriteCast(m_pFile, false, pContext);
            else
            {
                // if parameter or transmit_as type are not simple,
                // we use the indirection of pointers to cast the
                // declarator
                m_pFile->Print("*");
                pType->WriteCast(m_pFile, true, pContext);
                bUsePointer = true;
            }
            *m_pFile << " L4_MsgWord (" << ((bVarMsgBuf) ? "" : "&") <<
                sMsgBuffer << ", " << sOffsetVar << "/4 );\n";
        }
        else
        {
            *m_pFile << "\t";
            CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack,
                m_pParameter->IsString(), false, pContext);
            *m_pFile << " = L4_MsgWord (" <<  ((bVarMsgBuf) ? "" : "&") <<
                sMsgBuffer << ", " << (nStartOffset + nAlignment) / 4 << " );\n";
        }
    }
}

/** \brief marshals a constant integer value into the message buffer
 *  \param nBytes the number of bytes this value should use
 *  \param nValue the value itself
 *  \param nStartOffset the offset into the message buffer
 *  \param bUseConstOffset true if nStartOffset can be used
 *  \param bIncOffsetVariable true if the offset variable has to be incremented by this function
 *  \param pContext the context of the marshalling
 *  \return the number of bytes marshalled
 */
int
CL4V4BEMarshaller::MarshalValue(int nBytes,
    int nValue,
    int nStartOffset,
    bool & bUseConstOffset,
    bool bIncOffsetVariable,
    CBEContext * pContext)
{
    // first create the respective BE type
    CBEType *pType = pContext->GetClassFactory()->GetNewType(TYPE_INTEGER);
    pType->SetParent(m_pParameter);
    if (!pType->CreateBackEnd(false, nBytes, TYPE_INTEGER, pContext))
    {
        delete pType;
        return 0;
    }
    // get name of msg_t
    string sMsgBuffer =
        pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    // check if msgbuffer is pointer
    bool bVarMsgBuf = false;
    if (m_pFunction && m_pFunction->GetMessageBuffer())
    {
        CBEMsgBufferType *pMsgBuf = m_pFunction->GetMessageBuffer();
        bVarMsgBuf = pMsgBuf->IsVariableSized();
        if (!bVarMsgBuf && pMsgBuf->GetAlias())
            bVarMsgBuf = pMsgBuf->GetAlias()->GetStars() > 0;
    }
    // now check for marshalling/unmarshalling
    if (m_bMarshal)
    {
        *m_pFile << "\tL4_MsgPutWord ( " << ((bVarMsgBuf) ? "" : "&") <<
            sMsgBuffer << ", ";
        if (bUseConstOffset)
             *m_pFile << nStartOffset;
        else
        {
            string sOffsetVar =
                pContext->GetNameFactory()->GetOffsetVariable(pContext);
            // L4_MsgPutWord ( msgbuf, offset, value )
            *m_pFile << sOffsetVar;

        }
        *m_pFile << ", " << nValue << " );\n";
    }
    // we don't do anything when unmarshalling, we skip this fixed value
    return nBytes;
}
