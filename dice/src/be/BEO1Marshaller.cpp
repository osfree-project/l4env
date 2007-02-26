/**
 *	\file	BEO1Marshaller.cpp
 *	\brief	contains the implementation of the class CBEAttribute
 *
 *	\date	05/16/2002
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

#include "be/BEO1Marshaller.h"
#include "be/BEDeclarator.h"
#include "be/BEExpression.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEContext.h"
#include "be/BEFunction.h"
#include "be/BEAttribute.h"

#include "fe/FEAttribute.h"
#include "fe/FETypeSpec.h"

IMPLEMENT_DYNAMIC(CBEO1Marshaller);

CBEO1Marshaller::CBEO1Marshaller()
{
    IMPLEMENT_DYNAMIC_BASE(CBEO1Marshaller, CBEMarshaller);
}

/** destructs the marshaller object */
CBEO1Marshaller::~CBEO1Marshaller()
{
}

/** \brief marshals const sized arrays
 *  \param pType the base type of the array
 *  \param nStartOffset the starting offset in the message buffer
 *  \param bUseConstOffset true if the nStartOffset can be used to determine a constant offset
 *  \param bLastParameter true if this is the last parameter to be marshalled
 *  \param pContext the context of the marshalling
 *  \return the size of the marshalled data in bytes
 *
 * We optimize this marshalling by using memcpy
 */
int CBEO1Marshaller::MarshalConstArray(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext)
{
    int nSize = 0;
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    CBEDeclarator *pDecl = pCurrent->pDeclarator;
    // for each dimension
    VectorElement *pIter = pDecl->GetFirstArrayBound();
    CBEExpression *pBound;
    while ((pBound = pDecl->GetNextArrayBound(pIter)) != 0)
    {
        pCurrent->nIndex = -1;
        m_pFile->PrintIndent("memcpy(");
        if (m_bMarshal)
        {
            // now marshal array
            WriteBuffer(pType, nStartOffset, bUseConstOffset, false, pContext);
            m_pFile->Print(", ");
            m_vDeclaratorStack.Write(m_pFile, false, false, pContext);
        }
        else
        {
            // now unmarshal array
            m_vDeclaratorStack.Write(m_pFile, false, false, pContext);
            m_pFile->Print(", ");
            WriteBuffer(pType, nStartOffset, bUseConstOffset, false, pContext);
        }
        m_pFile->Print(", %d", pBound->GetIntValue());
        if (pType->GetSize() > 1)
        {
            m_pFile->Print("*sizeof");
            pType->WriteCast(m_pFile, false, pContext);
        }
        m_pFile->Print(");\n");
        nSize += pBound->GetIntValue()*pType->GetSize();
    }
    // return size
    return nSize;
}

/** \brief marshals a variable sized array
 *  \param pType the base type of the array
 *  \param nStartOffset the starting point in the message buffer
 *  \param bUseConstOffset true if nStartOffset can be used to index the message buffer
 *  \param bLastParameter true if this is the last parameter to be marshalled
 *  \param pContext the context of the mashalling
 *
 * If unmarshalling, we need to allocate memory for the data.
 */
int CBEO1Marshaller::MarshalVariableArray(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext)
{
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    pCurrent->nIndex = -1;
    bool bUsePointer = !pType->IsPointerType();
    // test if we can reference directly into message buffer
    // we do so if:
    // - at the server side
    // - it's variable sized (that's obvious)
    // - has no array dimensions, which lets us conclude that it has only stars to mark
    //   its variable size
    bool bRefMsgBuf = m_pFunction->IsComponentSide() && (pCurrent->pDeclarator->GetArrayDimensionCount() == 0);
    // allocate memory (only if we need it)
    // we need it if:
    // - unmarshalling
    // - no array dimensions, which means we have to allocate the necessary
    //   amount of memory for the array
    //   (if there are array dimensions, they contains values, and the
    //    parameter has them too)
	// - no INIT_WITH_IN attribute
    if (!m_bMarshal && (pCurrent->pDeclarator->GetArrayDimensionCount() == 0) &&
	    !m_pParameter->FindAttribute(ATTR_INIT_WITH_IN))
    {
        if (pContext->IsWarningSet(PROGRAM_WARNING_PREALLOC))
        {
            if (m_pFunction)
                CCompiler::Warning("CORBA_alloc is used to allocate memory for %s in %s.\n",
                                   (const char*)pCurrent->pDeclarator->GetName(), (const char*)m_pFunction->GetName());
            else
                CCompiler::Warning("CORBA_alloc is used to allocate memory for %s.\n", (const char*)pCurrent->pDeclarator->GetName());
        }
        // var = (type*)malloc(size);
        m_pFile->PrintIndent("");
        m_vDeclaratorStack.Write(m_pFile, bUsePointer, false, pContext);
        m_pFile->Print(" = ");
        pType->WriteCast(m_pFile, true, pContext);
		if (((pContext->IsOptionSet(PROGRAM_SERVER_PARAMETER) && m_pFunction->IsComponentSide()) ||
			  !m_pFunction->IsComponentSide()) &&
			  !pContext->IsOptionSet(PROGRAM_FORCE_CORBA_ALLOC))
		{
			CBETypedDeclarator* pEnv = m_pFunction->GetEnvironment();
			CBEDeclarator *pDecl = 0;
			if (pEnv)
			{
				VectorElement* pIter = pEnv->GetFirstDeclarator();
				pDecl = pEnv->GetNextDeclarator(pIter);
			}
			if (pDecl)
			{
				m_pFile->Print("(%s", (const char*)pDecl->GetName());
				if (pDecl->GetStars())
					m_pFile->Print("->malloc)(");
				else
					m_pFile->Print(".malloc)(");
			}
			else
				m_pFile->Print("CORBA_alloc(");
		}
		else
		    m_pFile->Print("CORBA_alloc(");
        if (pType->GetSize() > 1)
            m_pFile->Print("(");
        m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
        if (pType->GetSize() > 1)
        {
            m_pFile->Print(")");
            m_pFile->Print("*sizeof");
            pType->WriteCast(m_pFile, false, pContext);
        }
        m_pFile->Print(");\n");
    }
    // if unmarshalling at server side, reference directly into message buffer
    if (!m_bMarshal && bRefMsgBuf)
    {
        m_pFile->PrintIndent("");
        m_vDeclaratorStack.Write(m_pFile, bUsePointer, false, pContext);
        m_pFile->Print(" = ");
        WriteBuffer(pType, nStartOffset, bUseConstOffset, false, pContext);
        m_pFile->Print(";\n");
    }
    else
    {
        // copy
        m_pFile->PrintIndent("memcpy(");
        if (m_bMarshal)
        {
            // now marshal array
            WriteBuffer(pType, nStartOffset, bUseConstOffset, false, pContext);
            m_pFile->Print(", ");
            m_vDeclaratorStack.Write(m_pFile, bUsePointer, false, pContext);
        }
        else
        {
            m_vDeclaratorStack.Write(m_pFile, bUsePointer, false, pContext);
            m_pFile->Print(", ");
            WriteBuffer(pType, nStartOffset, bUseConstOffset, false, pContext);
        }
        m_pFile->Print(", ");
        if (pType->GetSize() > 1)
            m_pFile->Print("(");
        m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
        if (pType->GetSize() > 1)
        {
            m_pFile->Print(")");
            m_pFile->Print("*sizeof");
            pType->WriteCast(m_pFile, false, pContext);
        }
        m_pFile->Print(");\n");
    }
    // increase offset:
    // if (bUseConstOffset) -> set offset var ourselves using the WriteGetSize()*pType->GetSize() string
    // else -> increase offset var ourselves using the WriteGetSize()*pType->GetSize() string
    String sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
    m_pFile->PrintIndent("%s", (const char*)sOffset);
    if (bUseConstOffset)
        m_pFile->Print(" = ");
    else
        m_pFile->Print(" += ");
    m_pFile->Print("(");
    m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
    m_pFile->Print(")");
    if (pType->GetSize() > 1)
    {
        m_pFile->Print("*sizeof");
        pType->WriteCast(m_pFile, false, pContext);
    }
    if (bUseConstOffset)
    {
        m_pFile->Print(" + %d", nStartOffset);
    }
    m_pFile->Print(";\n");
    // not const offset anymore
    bUseConstOffset = false;
    // variable sized array returns size 0
    return 0;
}

/** \brief marshals a string
 *  \param pType the type of the string
 *  \param nStartOffset the offset in the message buffer, where to start marshalling
 *  \param bUseConstOffset true if nStartOffset can be used
 *  \param bLastParameter true if this is the last parameter to be marshalled
 *  \param pContext the context of this marshalling operation
 *  \return the number of bytes marshalled.
 *
 * Simple copy the string (strcpy(msgbuf, string)). At the server-side, we use a pointer
 * into the message buffer. To be sure, we don't reference something behind the actual string's
 * size, we first set the terminating zero. This method can only be used if a) unmarshalling and
 * b) at the server side.
 *
 * This way we do not give the user a pointer to some memory he has to free himself, but we
 * "recycle" the memory.
 */
int CBEO1Marshaller::MarshalString(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext)
{
    // marshal size of string
    String sTemp = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
    bool bUsePointer = !(pType->IsPointerType());
    bool bHasSizeAttr = m_pParameter->HasSizeAttr(ATTR_SIZE_IS) ||
            m_pParameter->HasSizeAttr(ATTR_LENGTH_IS);
    // if we have a max attribute we test the length value against this value
    if (m_bMarshal)
    {
        CBEAttribute *pMax = m_pParameter->FindAttribute(ATTR_MAX_IS);
        int nMax = -1;
        // if no max attribute, we have to use global max values
        if (pMax)
        {
            if (pMax->IsOfType(ATTR_CLASS_INT))
                nMax = pMax->GetIntValue();
        }
        else
        {
            nMax = pContext->GetSizes()->GetMaxSizeOfType(pType->GetFEType());
        }
        CBEAttribute *pLen = m_pParameter->FindAttribute(ATTR_SIZE_IS);
        if (!pLen)
            pLen = m_pParameter->FindAttribute(ATTR_LENGTH_IS);
        if (pLen)
        {
            // if (len > max)
            //   len = max;
            // we cannot access the string, because it surely is const
            VectorElement *pI;
            CBEDeclarator *pDMax = NULL, *pDLen = NULL;
            m_pFile->PrintIndent("if (");
            m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
            m_pFile->Print(" > ");
            if (nMax >= 0)
                m_pFile->Print("%d", nMax);
            else
            {
                pI = pMax->GetFirstIsAttribute();
                pDMax = pMax->GetNextIsAttribute(pI);
                m_pFile->Print("%s",  (const char*)pDMax->GetName());
            }
            m_pFile->Print(")\n");
//            m_pFile->PrintIndent("{\n");
            m_pFile->IncIndent();
            
            m_pFile->PrintIndent("");
            m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
            m_pFile->Print(" = ");
            if (nMax >= 0)
                m_pFile->Print("%d", nMax);
            else
            {
                pI = pMax->GetFirstIsAttribute();
                pDMax = pMax->GetNextIsAttribute(pI);
                m_pFile->Print("%s",  (const char*)pDMax->GetName());
            }
            m_pFile->Print(";\n");

//            m_pFile->PrintIndent("");
//            m_vDeclaratorStack.Write(m_pFile, false, false, pContext);
//            m_pFile->Print("[");
//            m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
//            m_pFile->Print("] = 0;\n");
//
            m_pFile->DecIndent();
//            m_pFile->PrintIndent("}\n");
        }
        // cannot do this, because string surely is const
//        else
//        {
//            VectorElement *pI;
//            CBEDeclarator *pD = NULL;
//            // if no length attribute:
//            // if (strlen(var) >= max)
//            //   str[max] = 0;
//            m_pFile->PrintIndent("if (strlen(");
//            m_vDeclaratorStack.Write(m_pFile, false, false, pContext);
//            m_pFile->Print(") >= ");
//            if (nMax >= 0)
//                m_pFile->Print("%d", nMax);
//            else
//            {
//                pI = pMax->GetFirstIsAttribute();
//                pD = pMax->GetNextIsAttribute(pI);
//                m_pFile->Print("%s", (const char*)pD->GetName());
//            }
//            m_pFile->Print(")\n");
//
//            m_pFile->IncIndent();
//            m_pFile->PrintIndent("");
//            m_vDeclaratorStack.Write(m_pFile, bUsePointer, false, pContext);
//            m_pFile->Print("[");
//            if (nMax >= 0)
//                m_pFile->Print("%d", nMax);
//            else
//                m_pFile->Print("%s", (const char*)pD->GetName());
//            m_pFile->Print("] = 0;\n");
//            m_pFile->DecIndent();
//        }
    }
    // if size attribute then attributes declarator is marshalled as well, we don't care about it here
    // if no size attribute and string attribute we marshal strlen size
    // we do this for the max_is attribute, because there might be less than the max_is size
    // transmitted and we want to know how much.
    if (!bHasSizeAttr && (m_pParameter->FindAttribute(ATTR_STRING)))
    {
        String sLong = pContext->GetNameFactory()->GetTypeName(TYPE_INTEGER, false, pContext, 4);
        if (m_bMarshal)
        {
            // marshal strlen
            m_pFile->PrintIndent("");
            WriteBuffer(sLong, nStartOffset, bUseConstOffset, true, pContext);
            m_pFile->Print(" = strlen(");
            m_vDeclaratorStack.Write(m_pFile, bUsePointer, false, pContext);
            m_pFile->Print(")+1;\n");
        }
        else
        {
            m_pFile->PrintIndent("%s = ", (const char*)sTemp);
            WriteBuffer(sLong, nStartOffset, bUseConstOffset, true, pContext);
            m_pFile->Print(";\n");
        }
        nStartOffset += pContext->GetSizes()->GetSizeOfType(TYPE_INTEGER, 4);
        if (!bUseConstOffset)
        {
            String sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
            // we have to increment the offset var, by the size of the size variable's type
            // (which is TYPE_INTEGER, 4)
            m_pFile->PrintIndent("%s += 4;\n", (const char*)sOffset);
        }
    }
    // allocate memory
    if (!m_bMarshal)
    {
        // if server, set terminating zero
        if (m_pFunction->IsComponentSide())
        {
            // only if string and no size or length attribute!
            if (m_pParameter->FindAttribute(ATTR_STRING) &&
                !m_pParameter->FindAttribute(ATTR_SIZE_IS) &&
                !m_pParameter->FindAttribute(ATTR_LENGTH_IS))
            {
                m_pFile->PrintIndent("(");
                WriteBuffer(pType, nStartOffset, bUseConstOffset, false, pContext);
                m_pFile->Print(")[");
                // we exclude max attribute, because we don't know the exact size
                // of the string for sure
                if (bHasSizeAttr)
                    m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
                else
                    m_pFile->Print("%s", (const char*)sTemp);
                m_pFile->Print("-1] = 0;\n"); // use length as index
            }
        }
        else
        {
		    if (!m_pParameter->FindAttribute(ATTR_INIT_WITH_IN))
			{
				if (pContext->IsWarningSet(PROGRAM_WARNING_PREALLOC))
				{
					CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
					if (m_pFunction)
						CCompiler::Warning("CORBA_alloc is used to allocate memory for %s in %s.",
										(const char*)pCurrent->pDeclarator->GetName(), (const char*)m_pFunction->GetName());
					else
						CCompiler::Warning("CORBA_alloc is used to allocate memory for %s.", (const char*)pCurrent->pDeclarator->GetName());
				}
				// var = (type*)malloc(size)
				m_pFile->PrintIndent("");
				m_vDeclaratorStack.Write(m_pFile, bUsePointer, false, pContext);
				m_pFile->Print(" = ");
				pType->WriteCast(m_pFile, true, pContext);
				if (((pContext->IsOptionSet(PROGRAM_SERVER_PARAMETER) && m_pFunction->IsComponentSide()) ||
					!m_pFunction->IsComponentSide()) &&
					!pContext->IsOptionSet(PROGRAM_FORCE_CORBA_ALLOC))
				{
					CBETypedDeclarator* pEnv = m_pFunction->GetEnvironment();
					CBEDeclarator *pDecl = 0;
					if (pEnv)
					{
						VectorElement* pIter = pEnv->GetFirstDeclarator();
						pDecl = pEnv->GetNextDeclarator(pIter);
					}
					if (pDecl)
					{
						m_pFile->Print("(%s", (const char*)pDecl->GetName());
						if (pDecl->GetStars())
							m_pFile->Print("->malloc)(");
						else
							m_pFile->Print(".malloc)(");
					}
					else
						m_pFile->Print("CORBA_alloc(");
				}
				else
					m_pFile->Print("CORBA_alloc(");
				// exclude max_is attribute, because in this case we have the actual size
				if (bHasSizeAttr)
					m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
				else
					m_pFile->Print("%s", (const char*)sTemp);
				m_pFile->Print(");\n");
			}
        }
    }
    // marshal/unmarshal
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    pCurrent->nIndex = -1;
    // if unmarshalling at server side, use pointer into message buffer
    if (!m_bMarshal && m_pFunction->IsComponentSide())
    {
        m_pFile->PrintIndent("");
        m_vDeclaratorStack.Write(m_pFile, bUsePointer, false, pContext);
        m_pFile->Print(" = ");
        WriteBuffer(pType, nStartOffset, bUseConstOffset, false, pContext);
        m_pFile->Print(";\n");
    }
    else
    {
         // if size attributes: use strncpy with size instead of strcpy
         // we exclude max_is attribute, because the actually transmitted size
         // can be smaller than the maximum size, so we have no guarantee to copy the
         // correct size, we use strcpy instead which looks for the end of the string
         if (bHasSizeAttr)
             m_pFile->PrintIndent("memcpy(");
         else
             m_pFile->PrintIndent("strcpy(");
         if (m_bMarshal)
         {
             // now marshal array
             WriteBuffer(pType, nStartOffset, bUseConstOffset, false, pContext);
             m_pFile->Print(", ");
             m_vDeclaratorStack.Write(m_pFile, bUsePointer, false, pContext);
         }
         else
         {
             m_vDeclaratorStack.Write(m_pFile, bUsePointer, false, pContext);
             m_pFile->Print(", ");
             WriteBuffer(pType, nStartOffset, bUseConstOffset, false, pContext);
         }
         if (bHasSizeAttr)
         {
             m_pFile->Print(", ");
             m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
         }
         m_pFile->Print(");\n");
    }
    // if unmarshaling: terminate with 0
    // if we use size attribute, we already copied the terminating 0
    // we do this only if this is a string!
    if (!m_bMarshal && !m_pFunction->IsComponentSide())
    {
        // we do this only if the string attribute is set and no
        // size or length attribute
        if (m_pParameter->FindAttribute(ATTR_STRING) &&
            !m_pParameter->FindAttribute(ATTR_SIZE_IS) &&
            !m_pParameter->FindAttribute(ATTR_LENGTH_IS))
        {
            m_pFile->PrintIndent("");
            m_vDeclaratorStack.Write(m_pFile, bUsePointer, false, pContext);
            m_pFile->Print("[%s-1] = 0;\n", (const char*)sTemp);
        }
    }
    // increase offset:
    // if (bUseConstOffset) -> set offset var ourselves using the strlen function
    // else -> increase offset var ourselves using the strlen function
    String sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
    m_pFile->PrintIndent("%s", (const char*)sOffset);
    if (bUseConstOffset)
        m_pFile->Print(" = ");
    else
        m_pFile->Print(" += ");

    // we exclude max_is attribute again, because we need actual size
    if (bHasSizeAttr)
        m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
    else if (m_bMarshal)
    {
        m_pFile->Print("strlen(");
        m_vDeclaratorStack.Write(m_pFile, bUsePointer, false, pContext);
        m_pFile->Print(")+1");
    }
    else
        m_pFile->Print("%s", (const char*)sTemp);

    if (bUseConstOffset)
        m_pFile->Print(" + %d", nStartOffset);
    m_pFile->Print(";\n");

    // not const offset anymore
    bUseConstOffset = false;
    // variable sized array returns size 0
    return 0;
}

/** \brief marshals a struct
 *  \param pType the type of the struct
 *  \param nStartOffset the offset in the message buffer where to start
 *  \param bUseConstOffset true if nStartOffset can be used
 *  \param bLastParameter true if this is the last parameter to be marshalled
 *  \param pContext the context of the marshalling
 *  \return the size of the marshalled struct
 *
 * This implementation copies the struct using memcpy directly from the message buffer
 * into the receiving struct. Because there still might be variable sized members
 * we have to iterate the members and marshal them seperately.
 */
int CBEO1Marshaller::MarshalStruct(CBEStructType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext)
{
    return CBEMarshaller::MarshalStruct(pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
}
