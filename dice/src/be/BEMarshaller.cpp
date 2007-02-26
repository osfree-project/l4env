/**
 *    \file    dice/src/be/BEMarshaller.cpp
 *    \brief   contains the implementation of the class CBEMarshaller
 *
 *    \date    05/08/2002
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

#include "be/BEMarshaller.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEFunction.h"
#include "be/BETypedDeclarator.h"
#include "be/BEType.h"
#include "be/BEDeclarator.h"
#include "be/BEUserDefinedType.h"
#include "be/BETypedef.h"
#include "be/BERoot.h"
#include "be/BEStructType.h"
#include "be/BEUnionType.h"
#include "be/BEAttribute.h"
#include "be/BEUnionCase.h"
#include "be/BEExpression.h"
#include "be/BEMsgBufferType.h"

#include "Attribute-Type.h"
#include "TypeSpec-Type.h"
#include "Compiler.h"

CBEMarshaller::CBEMarshaller()
{
    m_pFile = 0;
    m_pType = 0;
    m_bMarshal = true;
    m_pParameter = 0;
    m_pFunction = 0;
}

/** \brief cleans up the marshaller object */
CBEMarshaller::~CBEMarshaller()
{
}

/** \brief marshals a function
 *  \param pFile the file to marshal to
 *  \param pFunction the function to marshal
 *  \param nStartOffset the starting offset in the message buffer
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param pContext the context of the marshal operation
 *  \return the number of bytes marshalled
 *
 * Marshalling a function usually consists of iterating its sorted(!) parameters and
 * marshalling them seperately.
 */
int CBEMarshaller::Marshal(CBEFile *pFile, CBEFunction *pFunction, int nStartOffset, bool& bUseConstOffset, CBEContext *pContext)
{
    return Marshal(pFile, pFunction, 0/*all types*/, 0/*all parameters*/, nStartOffset, bUseConstOffset, pContext);
}

/** \brief only marshal parameter with specific type
 *  \param pFile the fiel to write to
 *  \param pFunction the function to marshal
 *  \param nFEType the type of the parameters to marshal (0 if all, negative if everything except this type)
 *  \param nNumber the number of parameters of this type to marshal (0 if all, negative if rest after abs(number))
 *  \param nStartOffset the offset where to start in the message buffer
 *  \param bUseConstOffset true if nStartOffset can be used
 *  \param pContext the context of the write
 *  \return the size of the marshalled values
 */
int CBEMarshaller::Marshal(CBEFile * pFile, CBEFunction * pFunction, int nFEType, int nNumber, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext)
{
    int nSize = 0, nCurrent = 0;
    m_pFunction = pFunction;
    CBEType *pType;
    CBEAttribute *pAttr;
    vector<CBETypedDeclarator*>::iterator iter = pFunction->GetFirstSortedParameter();
    CBETypedDeclarator *pParam;
    while ((pParam = pFunction->GetNextSortedParameter(iter)) != 0)
    {
        // if parameter is to be ignored, go to next
        if (pParam->FindAttribute(ATTR_IGNORE))
            continue;

        // reset m_pType
        m_pType = 0;
        pType = pParam->GetType();
        if ((pAttr = pParam->FindAttribute(ATTR_TRANSMIT_AS)) != 0)
        {
            m_pType = pType;
            pType = pAttr->GetAttrType();
        }
        if (nFEType < 0)
        {
            // if type is negative, then marshal everything EXCEPT this type
            if (pType->IsOfType(-nFEType))
                continue;
        }
        else if (nFEType > 0)
        {
            // if type is positive, then marshal ONLY parameters with this type
            if (!pType->IsOfType(nFEType))
                continue;
        }
        if (nNumber < 0)
        {
            // skip first -nNumber parameters
            if (nCurrent++ < -nNumber)
                continue;
        }
        else if (nNumber > 0)
        {
            // skip all after nNumber parameters
            if (nCurrent++ >= nNumber)
                continue;
        }
        // if type is 0, then marshal ALL parameters
        if ((m_bMarshal && (pFunction->DoMarshalParameter(pParam, pContext))) ||
            (!m_bMarshal && (pFunction->DoUnmarshalParameter(pParam, pContext))))
        {
            m_pParameter = pParam;
            nSize += Marshal(pFile, pParam, nStartOffset+nSize, bUseConstOffset,
                pFunction->IsLastSortedParameter(iter), pContext);
            m_pParameter = 0;
        }
    }
    m_pFunction = 0;
    return nSize;
}

/** \brief marshals the given parameter
 *  \param pFile the file to marshal to
 *  \param pParameter the parameter to marshal
 *  \param nStartOffset the starting offset in the message buffer
 *  \param bUseConstOffset true if a constant offset should be used, set it to \
 *         false if not possible
 *  \param bLastParameter true if this is the last parameter
 *  \param pContext the context of the marshalling operation
 *  \return the number of bytes this parameter uses for marshalling
 *
 * This function marshals a specific parameter. It iterates over the declarators
 * of the parameter and calls for each of them the MarshalDeclarator function.
 * Before doing so, the declarator is pushed on the declarator stack. This is
 * done to be able to call the MarshalDeclarator function recursively.
 */
int CBEMarshaller::Marshal(CBEFile *pFile, CBETypedDeclarator *pParameter, int nStartOffset, bool& bUseConstOffset, bool bLastParameter, CBEContext *pContext)
{
    m_pFile = pFile;
    if (!m_pFunction)
        m_pFunction = pParameter->GetSpecificParent<CBEFunction>();

    int nSize = 0;
    // if m_pParameter is already set, then this is a recursive call;
    // if it is not set, than this is a plain call, and we have to set it now
    bool bSetParam = false;
    if (!m_pParameter)
    {
        m_pParameter = pParameter;
        bSetParam = true;
    }

    // get type
    CBEType *pType = pParameter->GetType();
    CBEAttribute *pAttr;
    if ((pAttr = pParameter->FindAttribute(ATTR_TRANSMIT_AS)) != 0)
    {
        m_pType = pType;
        pType = pAttr->GetAttrType();
    }

    vector<CBEDeclarator*>::iterator iterD = pParameter->GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = pParameter->GetNextDeclarator(iterD)) != 0)
    {
        CDeclaratorStackLocation *pLoc = new CDeclaratorStackLocation(pDecl);
        m_vDeclaratorStack.push_back(pLoc);
        nSize += MarshalDeclarator(pType, nStartOffset+nSize, bUseConstOffset,
            true,
            bLastParameter && pParameter->IsLastDeclarator(iterD),
            pContext);
        m_vDeclaratorStack.pop_back();
        delete pLoc;
    }

    if (bSetParam)
        m_pParameter = 0;
    // reset m_pType
    m_pType = 0;
    return nSize;
}

/** \brief this function marshals a single declarator of a parameter
 *  \param pType the type of the current declarator
 *  \param nStartOffset the offset in the message buffer where the decl should be
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param bLastParameter true if this is last parameter to be marshalled
 *  \param bIncOffsetVariable true if this function should increment the offset variable, false if not
 *  \param pContext the context of the marshal operation
 *
 * This function implements the logic behind the marshalling stuff. It checks the type
 * and array dimensions.
 *
 * The logic does first check if this is an array declarator (the current is always the last
 * on the stack). If it is the dimensions are iterated and set on the stack. This is pretty
 * easy, because we iterate of the stack location's index var and simply call ourselves again.
 * To be able to differntiate between a recursive call and the first call, we check if the
 * array dimension is -1. If it is, its the first call.
 *
 * Then we check if this is a user defined type. If it is we lookup the original type and
 * simply replace the pType var with the found type. We will replace this var as long as it
 * is a user defined type.
 *
 * Then we check for struct type. If it is we iterate over the members and call the Marshal
 * function using the member as parameter. After this function returns we exit.
 *
 * If the type is a union we check if it is a C style union. If it is we marshal it just like
 * a usual parameter. The C compiler will marshal a union using the biggest member, which is
 * what we would have to do ourselves anyway - because we cannot specify used member. If it
 * is not a C style union we marshal it using the specific Union marshal function, which marshals
 * the switch variable and depending on its value the respective member, which might save
 * message buffer space. If we do this we have to marshal a variable sized buffer.
 *
 * When we went through all of this and we reached this point, we marshal a simple declarator.
 * The marshalling looks like this:
 * <code>
 * buffer._in.&lt;name&gt; = [&lt;stars&gt;] [&lt;stack locations&gt;] &lt;name&gt; [&lt;index&gt;];
 * </code>
 */
int CBEMarshaller::MarshalDeclarator(CBEType *pType, int nStartOffset, bool& bUseConstOffset, bool bIncOffsetVariable, bool bLastParameter, CBEContext *pContext)
{
    if (pType->IsVoid())
        return 0;

    assert(m_pParameter);
    // get current decl
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.back();
//     TRACE("%sMarshalDeclarator(%d, %d, %s, %s, %s) for %s (m_pType: %d)\n",
//         (m_bMarshal)?"":"Un",
//         pType->GetFEType(), nStartOffset, bUseConstOffset?"true":"false",
//         bIncOffsetVariable?"true":"false", bLastParameter?"true":"false",
//         pCurrent->pDeclarator->GetName().c_str(),
//         m_pType?m_pType->GetFEType():-1);
    if (!pCurrent->HasIndex())
    {
        // is first -> test for array
        if (pCurrent->pDeclarator->IsArray())
            return MarshalArray(pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
        // if there are size_is or length_is attribtes and the
        // decl has stars, then this is a variable sized array
        if ((pCurrent->pDeclarator->GetStars() > 0) &&
            (m_pParameter->FindAttribute(ATTR_SIZE_IS) ||
             m_pParameter->FindAttribute(ATTR_LENGTH_IS)))
            return MarshalArray(pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
        // since the declarator can also be a member of a struct
        // check the direct parent of the declarator
        if (!pCurrent->pDeclarator->GetSpecificParent<CBEFunction>() &&
             pCurrent->pDeclarator->GetSpecificParent<CBEStructType>() &&
            (pCurrent->pDeclarator->GetStars() > 0))
        {
            CBETypedDeclarator *pMember = pCurrent->pDeclarator->GetSpecificParent<CBEStructType>()->FindMember(pCurrent->pDeclarator->GetName());
            if (pMember &&
                (pMember->FindAttribute(ATTR_SIZE_IS) ||
                 pMember->FindAttribute(ATTR_LENGTH_IS)))
            {
                CBETypedDeclarator *pOldParam = m_pParameter;
                m_pParameter = pMember;
                bool bRet = MarshalArray(pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
                m_pParameter = pOldParam;
                return bRet;
            }
        }
        // a string is similar to an array, test for it
        if (m_pParameter->IsString())
            return MarshalString(pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
    }

    // test user defined types (usually they are defined in a C header file and thus
    // not in the IDL namespace)
    vector<CBEExpression*> vBounds;
    while (dynamic_cast<CBEUserDefinedType*>(pType))
    {
        // search for original type and replace it
        CBERoot *pRoot = pType->GetSpecificParent<CBERoot>();
        assert(pRoot);
        CBETypedef *pUserType = pRoot->FindTypedef(((CBEUserDefinedType *) pType)->GetName());
        // if 0: not defined in IDL files -> ???
        if (!pUserType || !pUserType->GetType())
            break;
        // assign aliased type
        pType = pUserType->GetType();
        // check for transmit_as attribute
        CBEAttribute *pAttr;
        if ((pAttr = pUserType->FindAttribute(ATTR_TRANSMIT_AS)) != 0)
        {
            m_pType = pType;
            pType = pAttr->GetAttrType();
        }
        // if alias is array?
        CBEDeclarator *pAlias = pUserType->GetAlias();
        // check if type has alias (it should, since the alias is
        // the name of the user defined type)
        if (pAlias && pAlias->IsArray())
        {
            // add array bounds of alias to temp vector
            vector<CBEExpression*>::iterator iter = pAlias->GetFirstArrayBound();
            CBEExpression *pBound;
            while ((pBound = pAlias->GetNextArrayBound(iter)) != 0)
            {
                vBounds.push_back(pBound);
            }
        }
    }

    // check if user defined types added array bounds
    if (vBounds.size() > 0)
    {
        vector<CBEExpression*>::iterator iterB;
        // add bounds to top declarator
        for (iterB = vBounds.begin(); iterB != vBounds.end(); iterB++)
        {
            pCurrent->pDeclarator->AddArrayBound(*iterB);
        }
        // call MarshalArray
        int nSize = MarshalArray(pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
        // remove those array decls again
        for (iterB = vBounds.begin(); iterB != vBounds.end(); iterB++)
        {
            pCurrent->pDeclarator->RemoveArrayBound(*iterB);
        }
        // reset m_pType
        m_pType = 0;
        // return (work done)
        return nSize;
    }

    // test for struct
    if (dynamic_cast<CBEStructType*>(pType))
        return MarshalStruct((CBEStructType*)pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);

    // test for union
    if (dynamic_cast<CBEUnionType*>(pType))
    {
        // test for C style union
        if (((CBEUnionType*)pType)->IsCStyleUnion())
            return MarshalCUnion((CBEUnionType*)pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
        else
            return MarshalUnion((CBEUnionType*)pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
    }

    // set the size of the type as return size
    int nReturnSize = pType->GetSize();
    if (!m_bMarshal && m_pType && !m_pType->IsVoid() &&
        pType && (m_pType->GetFEType() != pType->GetFEType()))
        nReturnSize = m_pType->GetSize();
    // check alignment
    int nAlignment = 0;
    if (m_pFunction)
        nAlignment = m_pFunction->GetParameterAlignment(nStartOffset,
            nReturnSize, pContext);
    // marshal
    WriteAssignment(pType, nStartOffset, bUseConstOffset, nAlignment, pContext);
    if (!bUseConstOffset && bIncOffsetVariable)
    {
        // add either the temporary offset, which was used for counting the members
        // or we add the size we just marshalled. Either way: we need the size of the
        // (base) type
        string sOffsetVar = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        m_pFile->PrintIndent("%s += sizeof", sOffsetVar.c_str());
        pType->WriteCast(m_pFile, false, pContext);
        m_pFile->Print(";\n");
    }

    return nAlignment + nReturnSize;
}

/** \brief writes the assignment of a declarator to the msgbuffer or the other \
 *           way around
 *  \param pType the type to write
 *  \param nStartOffset the offset where to start in the message buffer
 *  \param bUseConstOffset true if nStartOffset should be used
 *  \param nAlignment the alignment of the parameter (if we do alignment)
 *  \param pContext the context of the whole writing
 */
void
CBEMarshaller::WriteAssignment(CBEType *pType,
    int nStartOffset,
    bool &bUseConstOffset,
    int nAlignment,
    CBEContext *pContext)
{
    // get current decl
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.back();
    // marshal
    string sOffsetVar = pContext->GetNameFactory()->GetOffsetVariable(pContext);
    if (m_bMarshal)
    {
        // align dice_offset if necessary
        if (!bUseConstOffset && nAlignment)
            *m_pFile << "\t" << sOffsetVar << " += " << nAlignment << ";\n";
        m_pFile->PrintIndent("");
        WriteBuffer(pType, nStartOffset + nAlignment, bUseConstOffset, true, true, pContext);
        m_pFile->Print(" = ");
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
                m_pFile->Print("*");
                pType->WriteCast(m_pFile, true, pContext);
                bUsePointer = true;
            }
        }
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack,
            bUsePointer, false, pContext);
        m_pFile->Print(";\n");
    }
    else
    {
        // if the declarators original type is different from the type, we
        // cast the message buffer to, then we have to cast the message buffer
        // to that type instead
        if (m_pType && !m_pType->IsVoid() &&
            pType && (m_pType->GetFEType() != pType->GetFEType()) )
        {
            if (!bUseConstOffset && nAlignment)
                *m_pFile << "\t" << sOffsetVar << " += " << nAlignment << ";\n";
            m_pFile->PrintIndent("");
            CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack,
                m_pParameter->IsString(), false, pContext);
            m_pFile->Print(" = ");
            WriteBuffer(m_pType, nStartOffset + nAlignment, bUseConstOffset,
                        true, true, pContext);
            m_pFile->Print(";\n");
        }
        else
        {
            if (!bUseConstOffset && nAlignment)
                *m_pFile << "\t" << sOffsetVar << " += " << nAlignment << ";\n";
            m_pFile->PrintIndent("");
            CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack,
                m_pParameter->IsString(), false, pContext);
            m_pFile->Print(" = ");
            WriteBuffer(pType, nStartOffset + nAlignment, bUseConstOffset, true,
                        true, pContext);
            m_pFile->Print(";\n");
        }
    }
}

/** \brief marshals an array declarator
 *  \param pType the base type of the array
 *  \param nStartOffset the starting offset in the message buffer
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param bLastParameter treu if this is the last parameter to be marshalled
 *  \param pContext the context of the marshalling operation
 *  \return the size of the array in the message buffer
 *
 * Because fixed sized and variable sized arrays are marshalled differently, we have to find out if the
 * current decl is variable or fixed sized array. If the GetSize function return -1 then it is variable
 * sized. If the parameter has a size_is or length_is attribute, we use that. It might save us some memory in the
 * buffer. Thus we can say a fixed sized array is one with a size > 0 and no size_is/length_is attribute.
 *
 * But, if we marshal a variable sized array, we have to write this code to the file, because we cannot
 * determine the total run size at compile time. We use a temporary variable for the loop.
 */
int CBEMarshaller::MarshalArray(CBEType *pType, int nStartOffset, bool& bUseConstOffset, bool bLastParameter, CBEContext *pContext)
{
    int nSize = 0;
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.back();
    CBEDeclarator *pDecl = pCurrent->pDeclarator;
    // check if there is already somebody iterating over array bounds
    int nLevel = pCurrent->GetUsedIndexCount();
    // skip array bounds already in use
    int i = nLevel;
    vector<CBEExpression*>::iterator iter = pDecl->GetFirstArrayBound();
    while (i-- > 0) pDecl->GetNextArrayBound(iter);
    // if fixed size
    if ((pDecl->GetSize() >= 0) &&
        !(m_pParameter->FindAttribute(ATTR_SIZE_IS)) &&
        !(m_pParameter->FindAttribute(ATTR_LENGTH_IS)))
    {
        nSize = MarshalConstArray(pType, nStartOffset, bUseConstOffset, bLastParameter, iter, nLevel, pContext);
    }
    else
    {
        nSize = MarshalVariableArray(pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
    }
    return nSize;
}

/** \brief marshals a fixed sized array
 *  \param pType the base type of the array
 *  \param nStartOffset the starting offset in the message buffer
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param bLastParameter true if this is last parameter to be marshalled
 *  \param iter the iterator pointing to the next array dimensions
 *  \param nLevel the level reached with the current array boundary
 *  \param pContext the context of the marshalling operation
 *  \return the size of the array in the message buffer
 *
 * We optimize this marshalling by using memcpy
 */
int
CBEMarshaller::MarshalConstArray(CBEType *pType,
    int nStartOffset,
    bool & bUseConstOffset,
    bool bLastParameter,
    vector<CBEExpression*>::iterator iter,
    int nLevel,
    CBEContext *pContext)
{
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.back();
    CBEDeclarator *pDecl = pCurrent->pDeclarator;
    // calculate total array size
    CBEExpression *pBound;
    int nBound = 1;
    while ((pBound = pDecl->GetNextArrayBound(iter)) != 0)
    {
        pCurrent->SetIndex(-1, nLevel++);
        nBound *= pBound->GetIntValue();
    }
    m_pFile->PrintIndent("memcpy(");
    if (m_bMarshal)
    {
        // now marshal array
        WriteBuffer(pType, nStartOffset, bUseConstOffset, false, false, pContext);
        m_pFile->Print(", ");
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, false, false, pContext);
    }
    else
    {
        // now unmarshal array
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, false, false, pContext);
        m_pFile->Print(", ");
        WriteBuffer(pType, nStartOffset, bUseConstOffset, false, false, pContext);
    }
    m_pFile->Print(", ");
    if (nBound > 1)
        m_pFile->Print("%d", nBound);
    if (pType->GetSize() > 1)
    {
        if (nBound > 1)
            m_pFile->Print("*");
        m_pFile->Print("sizeof");
        pType->WriteCast(m_pFile, false, pContext);
    }
    m_pFile->Print(");\n");
    // return size
    return nBound*pType->GetSize();
}

/** \brief marshals variable sized array
 *  \param pType the base type of the array
 *  \param nStartOffset the starting offset in the message buffer
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param bLastParameter true if ths is the last parameter to be marshalled
 *  \param pContext the context of the marshalling operation
 *  \return the size of the array in the message buffer
 *
 * If unmarshalling, this function needs to allocate memory for variable sized arrays.
 */
int CBEMarshaller::MarshalVariableArray(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext)
{
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.back();
    // disable arrays
    for (int i=0; i < pCurrent->GetUsedIndexCount(); i++)
        pCurrent->SetIndex(-1, i);
    // test if we need a pointer of this variable
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
    // - no directly reference into the message buffer
    if (!m_bMarshal && (pCurrent->pDeclarator->GetArrayDimensionCount() == 0) &&
        !m_pParameter->FindAttribute(ATTR_PREALLOC) && !bRefMsgBuf)
    {
        // var = (type*)malloc(size);
        m_pFile->PrintIndent("");
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, bUsePointer, false, pContext);
        m_pFile->Print(" = ");
        pType->WriteCast(m_pFile, true, pContext);
        pContext->WriteMalloc(m_pFile, m_pFunction);
        m_pFile->Print("(");
        if (pType->GetSize() > 1)
            m_pFile->Print("(");
        m_pParameter->WriteGetSize(m_pFile, &m_vDeclaratorStack, pContext);
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
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, bUsePointer, false, pContext);
        m_pFile->Print(" = ");
        WriteBuffer(pType, nStartOffset, bUseConstOffset, false, true, pContext);
        m_pFile->Print(";\n");
    }
    else
    {
        // copy
        m_pFile->PrintIndent("memcpy(");
        if (m_bMarshal)
        {
            // now marshal array
            WriteBuffer(pType, nStartOffset, bUseConstOffset, false, false, pContext);
            m_pFile->Print(", ");
            CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, bUsePointer, false, pContext);
        }
        else
        {
            CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, bUsePointer, false, pContext);
            m_pFile->Print(", ");
            WriteBuffer(pType, nStartOffset, bUseConstOffset, false, false, pContext);
        }
        m_pFile->Print(", ");
        if (pType->GetSize() > 1)
            m_pFile->Print("(");
        m_pParameter->WriteGetSize(m_pFile, &m_vDeclaratorStack, pContext);
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
    string sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
    m_pFile->PrintIndent("%s", sOffset.c_str());
    if (bUseConstOffset)
        m_pFile->Print(" = ");
    else
        m_pFile->Print(" += ");
    m_pFile->Print("(");
    m_pParameter->WriteGetSize(m_pFile, &m_vDeclaratorStack, pContext);
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

/** \brief marshal a struct into the message buffer
 *  \param pType the struct type
 *  \param nStartOffset the offset where the members of the struct will be in the message buffer
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param bLastParameter true if this the last parameter to be marshalled
 *  \param pContext the context of the marshalling operation
 *  \return the size of the struct in the message buffer
 *
 * The struct is marshalled by iterating over every single member. If struct contains bitfield members
 * (directly, not in nested structs), then we marshal the whole struct at once, because we cannot
 * perform bit-wise marshalling.
 */
int CBEMarshaller::MarshalStruct(CBEStructType *pType, int nStartOffset, bool& bUseConstOffset, bool bLastParameter, CBEContext *pContext)
{
    // if this is a tagged struct without members, we have to find the original struct
    if ((pType->GetMemberCount() == 0) && (pType->GetFEType() == TYPE_TAGGED_STRUCT))
    {
        // search for tag
        CBERoot *pRoot = pType->GetSpecificParent<CBERoot>();
        assert(pRoot);
        CBEStructType *pTaggedType = (CBEStructType*)pRoot->FindTaggedType(TYPE_TAGGED_STRUCT, pType->GetTag());
        // if found, marshal this instead
        if ((pTaggedType) && (pTaggedType != pType))
            return MarshalStruct(pTaggedType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
        if (!pTaggedType)
        {
            // cannot be found -> marshal as "normal" declarator
            if (m_bMarshal)
            {
                m_pFile->PrintIndent("");
                WriteBuffer(pType, nStartOffset, bUseConstOffset, true, true, pContext);
                m_pFile->Print(" = ");
                CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack,
                    false, false, pContext);
                m_pFile->Print(";\n");
            }
            else
            {
                m_pFile->PrintIndent("");
                CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack,
                    false, false, pContext);
                m_pFile->Print(" = ");
                WriteBuffer(pType, nStartOffset, bUseConstOffset, true, true, pContext);
                m_pFile->Print(";\n");
            }
            return pType->GetSize();
        }
    }
    // check for bitfields
    vector<CBETypedDeclarator*>::iterator iterS = pType->GetFirstMember();
    CBETypedDeclarator *pMember;
    bool bBitfields = false;
    while (((pMember = pType->GetNextMember(iterS)) != 0) && !bBitfields)
    {
        if (pMember->GetSize() == 0)
            bBitfields = true;
    }
    if (bBitfields)
        return MarshalBitfieldStruct(pType, nStartOffset, bUseConstOffset, pContext);
    // marshal "normal" struct
    int nSize = 0;
    iterS = pType->GetFirstMember();
    CBETypedDeclarator *pOldParameter;
    while ((pMember = pType->GetNextMember(iterS)) != 0)
    {
        // swap current parameter
        pOldParameter = m_pParameter;
        m_pParameter = pMember;
        nSize += Marshal(m_pFile, pMember, nStartOffset+nSize, bUseConstOffset, bLastParameter, pContext);
        m_pParameter = pOldParameter;
    }
    return nSize;
}

/** \brief marshal a union
 *  \param pType the union's type
 *  \param nStartOffset the start of the union in the message buffer
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param bLastParameter true if this is the last parameter to be marshalled
 *  \param pContext the context of the marshalling operation
 *  \return the size of the union in the message buffer
 *
 * A C union should be marshalled using memcpy, because we never know how big
 * it's members are.
 */
int CBEMarshaller::MarshalCUnion(CBEUnionType *pType, int nStartOffset,
    bool& bUseConstOffset, bool bLastParameter, CBEContext *pContext)
{
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.back();
    CBEDeclarator *pDecl = pCurrent->pDeclarator;
    CBEType *pOrigType = pType;
    if (m_pParameter)
        pOrigType = m_pParameter->GetType();
    bool bMakeRef = (pDecl->GetStars() == 0) && !pOrigType->IsPointerType();

    *m_pFile << "\tmemcpy(";
    if (m_bMarshal)
    {
        // now marshal array
        WriteBuffer(pType, nStartOffset, bUseConstOffset, false, false, pContext);
        m_pFile->Print(", ");
        if (bMakeRef)
            *m_pFile << "&(";
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, true, false, pContext);
        if (bMakeRef)
            *m_pFile << ")";
    }
    else
    {
        // now unmarshal array
        if (bMakeRef)
            *m_pFile << "&(";
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, true, false, pContext);
        if (bMakeRef)
            *m_pFile << ")";
        m_pFile->Print(", ");
        WriteBuffer(pType, nStartOffset, bUseConstOffset, false, false, pContext);
    }
    *m_pFile << ", sizeof(";
    CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, false, false, pContext);
    *m_pFile << "));\n";
    // return size
    return pType->GetSize();
}

/** \brief marshal a union
 *  \param pType the union's type
 *  \param nStartOffset the start of the union in the message buffer
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param bLastParameter true if this is the last parameter to be marshalled
 *  \param pContext the context of the marshalling operation
 *  \return the size of the union in the message buffer
 */
int CBEMarshaller::MarshalUnion(CBEUnionType *pType, int nStartOffset, bool& bUseConstOffset, bool bLastParameter, CBEContext *pContext)
{
    int nSize = 0, nCaseSize = 0, nMaxCaseSize = 0;
    // marshal switch variable
    assert(pType->GetSwitchVariable());
    nSize = Marshal(m_pFile, pType->GetSwitchVariable(), nStartOffset, bUseConstOffset, bLastParameter, pContext);

    // write switch statement
    string sSwitchVar = pType->GetSwitchVariableName();
    m_pFile->PrintIndent("switch(");
    CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, false,
        false, pContext);
    m_pFile->Print(".%s)\n", sSwitchVar.c_str());
    m_pFile->PrintIndent("{\n");
    // we have to add the union declarator by hand
    if (pType->GetUnionName())
    {
        CDeclaratorStackLocation *pLoc = new CDeclaratorStackLocation(pType->GetUnionName());
        m_vDeclaratorStack.push_back(pLoc);
        pLoc->SetIndex(-3);
    }
    // write the cases
    vector<CBEUnionCase*>::iterator iterU = pType->GetFirstUnionCase();
    CBEUnionCase *pCase;
    CBETypedDeclarator *pOldParameter;
    // we have to make the usage of the const offset for every branch
    // as if it is the first. Therefor we use the value of bUseConstOffset
    // over and over in each loop. To set it correctly after the loop
    // we need the bChangedConstOffset variable.
    bool bUnionConstOffset, bChangedConstOffset = false;
    while ((pCase = pType->GetNextUnionCase(iterU)) != 0)
    {
        bUnionConstOffset = bUseConstOffset;
        if (pCase->IsDefault())
        {
            m_pFile->PrintIndent("default: \n");
        }
        else
        {
            vector<CBEExpression*>::iterator iterL = pCase->GetFirstLabel();
            CBEExpression *pLabel;
            while ((pLabel = pCase->GetNextLabel(iterL)) != 0)
            {
                m_pFile->PrintIndent("case ");
                pLabel->Write(m_pFile, pContext);
                m_pFile->Print(":\n");
            }
        }
        m_pFile->IncIndent();
        // swap parameter
        pOldParameter = m_pParameter;
        m_pParameter = pCase;
        nCaseSize = Marshal(m_pFile, pCase, nStartOffset+nSize, bUnionConstOffset, bLastParameter, pContext);
        // swap parameter back
        m_pParameter = pOldParameter;
        if (nCaseSize > nMaxCaseSize)
            nMaxCaseSize = nCaseSize;
        // if this is const and one of the others is var and bUseConstOffset is true
        // then we set the offset var now, because it will be used later
        if (pCase->IsFixedSized() &&  (pType->GetSize() < 0) && bUseConstOffset)
        {
            string sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
            m_pFile->PrintIndent("%s = %d;\n", sOffset.c_str(), nCaseSize+nStartOffset+nSize);
        }
        m_pFile->PrintIndent("break;\n");
        m_pFile->DecIndent();
        // if the bUseConstOffset is true and the const offset of the current case is false,
        // then this is a variable sized case, so we have to set the bChangedConstOffset to true
        if (!bUnionConstOffset && bUseConstOffset)
            bChangedConstOffset = true;
    }
    // if the offset changed, the it did change from true to false
    if (bChangedConstOffset)
        bUseConstOffset = false;
    // remove the union name from the stack
    if (pType->GetUnionName())
    {
        CDeclaratorStackLocation *pLoc = m_vDeclaratorStack.back();
        m_vDeclaratorStack.pop_back();
        delete pLoc;
    }
    // close the switch statement
    m_pFile->PrintIndent("}\n");
    // return size
    // we actually return a size value -> this makes Marshal ignore the
    // temp offset * size of type addition.
    return nSize + nMaxCaseSize;
}

/** \brief print the part with the message buffer
 *  \param pType the type to cast the message buffer to
 *  \param nStartOffset which index in the message buffer to use
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param bDereferencePosition true if the message buffer position shall be dereferenced
 *  \param bCast true if the buffer should be casted to the type
 *  \param pContext the context of the write operation
 */
void CBEMarshaller::WriteBuffer(CBEType *pType,
    int nStartOffset,
    bool& bUseConstOffset,
    bool bDereferencePosition,
    bool bCast,
    CBEContext *pContext)
{
    // for the variable sized arrays we have to add the size var to the offset
    // get current
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.back();
    assert(pCurrent);
    // get strings
    int nIdxCnt = pCurrent->GetUsedIndexCount();
    for (int i=0; i<nIdxCnt; i++)
        if (pCurrent->nIndex[i] == -2)
            m_pFile->Print("(");
    // check if we have any var-sized array indices
    if (bDereferencePosition && (pCurrent->nIndex[0] != -2))
        m_pFile->Print("*");
    if (bCast)
        pType->WriteCast(m_pFile, true, pContext);
    // write the buffer
    CBEMsgBufferType *pMsgBuffer = m_pFunction->GetMessageBuffer();
    m_pFile->Print("(&(");
    pMsgBuffer->WriteMemberAccess(m_pFile, pType->GetFEType(),
        (m_bMarshal)?m_pFunction->GetSendDirection():m_pFunction->GetReceiveDirection(),
        pContext);
    // write the offset in the buffer
    if (bUseConstOffset)
        m_pFile->Print("[%d]))", nStartOffset);
    else
    {
        string sOffsetVar = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        m_pFile->Print("[%s]))", sOffsetVar.c_str());
    }
    for (int i=0; i<nIdxCnt; i++)
        if (pCurrent->nIndex[i] == -2)
            m_pFile->Print(")[%s]", pCurrent->sIndex[i].c_str());
}

/** \brief unmarshals a function
 *  \param pFile the file to write to
 *  \param pFunction the function to unmarshal
 *  \param nStartOffset the start in the message buffer where to unmarshal
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param pContext the context of the marshalling
 *  \return the number of bytes unmarshalled
 *
 * The unmarshalling is basically the same as the marshalling, Only the last sequence, when writing the
 * unmarshalling line is different. This can be differentiated when setting a boolean variable.
 */
int CBEMarshaller::Unmarshal(CBEFile * pFile, CBEFunction * pFunction, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
    return Unmarshal(pFile, pFunction, 0/*all types*/, 0/*all parameters*/, nStartOffset, bUseConstOffset, pContext);
}

/** \brief unmarshals a function
 *  \param pFile the file to write to
 *  \param pFunction the function to unmarshal
 *  \param nFEType the type of the parameters to be unmarshalled
 *  \param nNumber the number of parameters to unmarshal
 *  \param nStartOffset the start in the message buffer where to unmarshal
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param pContext the context of the marshalling
 *  \return the number of bytes unmarshalled
 *
 * The unmarshalling is basically the same as the marshalling, Only the last sequence, when writing the
 * unmarshalling line is different. This can be differentiated when setting a boolean variable.
 */
int CBEMarshaller::Unmarshal(CBEFile * pFile, CBEFunction * pFunction, int nFEType, int nNumber, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
    m_bMarshal = false;
    return Marshal(pFile, pFunction, nFEType, nNumber, nStartOffset, bUseConstOffset, pContext);
}

/** \brief unmarshals the given parameter
 *  \param pFile the file to unmarshal to
 *  \param pParameter the parameter to unmarshal
 *  \param nStartOffset the starting offset in the message buffer
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param bLastParameter true if this is the last parameter
 *  \param pContext the context of the unmarshalling operation
 *  \return the number of bytes this parameter uses for unmarshalling
 *
 * The unmarshalling is basically the same as the marshalling, Only the last sequence, when writing the
 * unmarshalling line is different. This can be differentiated when setting a boolean variable.
 */
int CBEMarshaller::Unmarshal(CBEFile * pFile, CBETypedDeclarator * pParameter, int nStartOffset, bool& bUseConstOffset, bool bLastParameter, CBEContext * pContext)
{
    m_bMarshal = false;
    return Marshal(pFile, pParameter, nStartOffset, bUseConstOffset, bLastParameter, pContext);
}

/** \brief marshals a fixed value to the message buffer
 *  \param pFile the file to marshal to
 *    \param pFunction the function using the marshaller
 *  \param nBytes the size to use in the message buffer
 *  \param nValue the value to marshal
 *  \param nStartOffset the offset where to start in the message buffer
 *  \param bUseConstOffset true if nStartOffset should be used
 *  \param pContext the context of this marshalling
 *  \return the size actually marshalled
 */
int CBEMarshaller::MarshalValue(CBEFile *pFile, CBEFunction *pFunction, int nBytes, int nValue, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext)
{
    m_pFile = pFile;
    m_pFunction = pFunction;
    return MarshalValue(nBytes, nValue, nStartOffset, bUseConstOffset, false, pContext);
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
int CBEMarshaller::MarshalValue(int nBytes, int nValue, int nStartOffset, bool & bUseConstOffset, bool bIncOffsetVariable, CBEContext * pContext)
{
    // first create the respective BE type
    CBEType *pType = pContext->GetClassFactory()->GetNewType(TYPE_INTEGER);
    pType->SetParent(m_pParameter);
    if (!pType->CreateBackEnd(false, nBytes, TYPE_INTEGER, pContext))
    {
        delete pType;
        return 0;
    }
    // now check for marshalling/unmarshalling
    if (m_bMarshal)
    {
        CDeclaratorStackLocation *pLoc = new CDeclaratorStackLocation((CBEDeclarator*)0);
        // we need a stack location, so WriteBuffer is working
        m_vDeclaratorStack.push_back(pLoc);
        m_pFile->PrintIndent("");
        WriteBuffer(pType, nStartOffset, bUseConstOffset, true, true, pContext);
        m_pFile->Print(" = %d;\n", nValue);
        m_vDeclaratorStack.pop_back();
        delete pLoc;
    }
    // we don't do anything when unmarshalling, we skip this fixed value
    return nBytes;
}

/** \brief marshals a string parameter (has [string] attribute)
 *  \param pType the type of the declarator (should be char)
 *  \param nStartOffset the starting offset in the message buffer
 *  \param bUseConstOffset true if nStartOffset can be used
 *  \param bLastParameter true if this is the last parameter to be marshalled
 *  \param pContext the context of the marshalling
 *
 * Simply copy the string (strcpy(msgbuf, string)). At the server-side, we use
 * a pointer into the message buffer. To be sure, we don't reference something
 * behind the actual string's size, we first set the terminating zero. This
 * method can only be used if a) unmarshalling and b) at the server side.
 *
 * This way we do not give the user a pointer to some memory he has to free
 * himself, but we "recycle" the memory.
 */
int
CBEMarshaller::MarshalString(CBEType *pType,
    int nStartOffset,
    bool & bUseConstOffset,
    bool bLastParameter,
    CBEContext *pContext)
{
    // marshal size of string
    string sTemp = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
    string sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
    bool bUsePointer = !(pType->IsPointerType());
    bool bHasSizeAttr = m_pParameter->HasSizeAttr(ATTR_SIZE_IS) ||
            m_pParameter->HasSizeAttr(ATTR_LENGTH_IS);
    CBEAttribute *pMax = m_pParameter->FindAttribute(ATTR_MAX_IS);
    int nMax = -1;
    // if no max attribute, we have to use global max values
    if (pMax)
    {
        if (pMax->IsOfType(ATTR_CLASS_INT))
            nMax = pMax->GetIntValue();
    }
    else
        nMax = pContext->GetSizes()->GetMaxSizeOfType(pType->GetFEType());
    // if we have a max attribute we test the length value against this value
    if (m_bMarshal)
    {
        CBEAttribute *pLen = m_pParameter->FindAttribute(ATTR_SIZE_IS);
        if (!pLen)
            pLen = m_pParameter->FindAttribute(ATTR_LENGTH_IS);
        if (pLen)
        {
            // if (len > max)
            //   len = max;
            // we cannot access the string, because it surely is const
            vector<CBEDeclarator*>::iterator iterD;
            CBEDeclarator *pDMax = NULL;
            *m_pFile << "\tif (";
            m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
            *m_pFile << " > ";
            if (nMax >= 0)
                *m_pFile << nMax;
            else
            {
                iterD = pMax->GetFirstIsAttribute();
                pDMax = pMax->GetNextIsAttribute(iterD);
                *m_pFile << pDMax->GetName();
            }
            *m_pFile << ")\n";
            m_pFile->IncIndent();

            *m_pFile << "\t";
            m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
            *m_pFile << " = ";
            if (nMax >= 0)
                *m_pFile << nMax;
            else
            {
                iterD = pMax->GetFirstIsAttribute();
                pDMax = pMax->GetNextIsAttribute(iterD);
                *m_pFile << pDMax->GetName();
            }
            *m_pFile << ";\n";
            m_pFile->DecIndent();
        }
    }
    // if size attribute then attributes declarator is marshalled as well,
    // we don't care about it here
    //  if no size attribute and string attribute we marshal strlen size
    // we do this for the max_is attribute, because there might be less than
    // the max_is size transmitted and we want to know how much.
    if (!bHasSizeAttr && (m_pParameter->FindAttribute(ATTR_STRING)))
    {
        string sLong = pContext->GetNameFactory()->GetTypeName(TYPE_LONG,
            false, pContext, 4);
        if (pContext->IsOptionSet(PROGRAM_ALIGN_TO_TYPE) && !bUseConstOffset)
        {
            // align offset var to size of long
            *m_pFile << "\t" << sOffset << " = (" << sOffset << 
		"+3) & ~3;\n";
        }
        if (m_bMarshal)
        {
            // marshal strlen (check if NULL pointer)
            *m_pFile << "\t";
            WriteBuffer(sLong, nStartOffset, bUseConstOffset, true, true, 
		pContext);
            *m_pFile << " = (";
            CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, 
		bUsePointer, false, pContext);
            *m_pFile << ") ? strlen(";
            CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, 
		bUsePointer, false, pContext);
            * m_pFile << ")+1 : 0;\n";
        }
        else
        {
            *m_pFile << "\t" << sTemp << " = ";
            WriteBuffer(sLong, nStartOffset, bUseConstOffset, true, true, 
		pContext);
            *m_pFile << ";\n";
        }
        nStartOffset += pContext->GetSizes()->GetSizeOfType(TYPE_LONG, 4);
        if (!bUseConstOffset)
        {
            string sOffset =
                pContext->GetNameFactory()->GetOffsetVariable(pContext);
            // we have to increment the offset var, by the size of the size
            // variable's type (which is TYPE_INTEGER, 4)
            *m_pFile << "\t" << sOffset << " += 4;\n";
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
                // if marshalled length was 0, then its a NULL string
                *m_pFile << "\tif (" << sTemp << " > 0)\n";
                m_pFile->IncIndent();
                *m_pFile << "\t(";
                WriteBuffer(pType, nStartOffset, bUseConstOffset, false, true, pContext);
                *m_pFile << ")[";
                // we exclude max attribute, because we don't know the exact
                // size of the string for sure
                if (bHasSizeAttr)
                    m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
                else
                    *m_pFile << sTemp;
                *m_pFile << "-1] = 0;\n"; // use length as index
                m_pFile->DecIndent();
            }
        }
        else
        {
            if (!m_pParameter->FindAttribute(ATTR_PREALLOC))
            {
                // if marshalled length was 0, then its a NULL string
                *m_pFile << "\tif (" << sTemp << " == 0)\n";
                m_pFile->IncIndent();
                *m_pFile << "\t";
                CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, bUsePointer, false, pContext);
                *m_pFile << " = NULL;\n";
                m_pFile->DecIndent();
                *m_pFile << "\telse\n";
                m_pFile->IncIndent();
                // var = (type*)malloc(size)
                *m_pFile << "\t";
                CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, bUsePointer, false, pContext);
                *m_pFile << " = ";
                pType->WriteCast(m_pFile, true, pContext);
                pContext->WriteMalloc(m_pFile, m_pFunction);
                // exclude max_is attribute, because in this case we have the actual size
                *m_pFile << "(";
                if (bHasSizeAttr)
                    m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
                else
                    *m_pFile << sTemp;
                *m_pFile << ");\n";
                m_pFile->DecIndent();
            }
        }
    }
    // marshal/unmarshal
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.back();
    for (int i=0; i<pCurrent->GetUsedIndexCount(); i++)
        pCurrent->SetIndex(-1, i);
    // if unmarshalling at server side, use pointer into message buffer
    if (!m_bMarshal && m_pFunction->IsComponentSide())
    {
        // if marshalled length was 0, then its a NULL string
        *m_pFile << "\tif (" << sTemp << " == 0)\n";
        m_pFile->IncIndent();
        *m_pFile << "\t";
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, bUsePointer, false, pContext);
        *m_pFile << " = NULL;\n";
        m_pFile->DecIndent();
        *m_pFile << "\telse\n";
        m_pFile->IncIndent();

        *m_pFile << "\t";
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, bUsePointer, false, pContext);
        *m_pFile << " = ";
        WriteBuffer(pType, nStartOffset, bUseConstOffset, false, true, pContext);
        *m_pFile << ";\n";
        m_pFile->DecIndent();
    }
    else
    {
        // only if size is larger than 0 then marshal/unmarshal
        *m_pFile << "\tif (";
        if (bHasSizeAttr)
        {
            m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
            *m_pFile << " > 0";
        }
        else if (!m_bMarshal)
            *m_pFile << sTemp << " > 0";
        else
            CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, bUsePointer, false, pContext);
        *m_pFile << ")\n";
        m_pFile->IncIndent();
        // if size attributes: use strncpy with size instead of strcpy
        // we exclude max_is attribute, because the actually transmitted size
        // can be smaller than the maximum size, so we have no guarantee to copy the
        // correct size, we use strcpy instead which looks for the end of the string
        if (bHasSizeAttr || !m_bMarshal)
            m_pFile->PrintIndent("memcpy(");
        else
            m_pFile->PrintIndent("strcpy(");
        if (m_bMarshal)
        {
            // now marshal array
	    //
	    // gcc4 fix: we have to cast to char*, because the buffer is
	    // usually l4_int8_t, which is 'signed char'. That's a mismatch
	    // with gcc4 (See -fsigned-char Option)
	    m_pFile->Print("(char*)");
            WriteBuffer(pType, nStartOffset, bUseConstOffset, false, false, pContext);
            m_pFile->Print(", ");
            CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, bUsePointer, false, pContext);
        }
        else
        {
            CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, bUsePointer, false, pContext);
            m_pFile->Print(", ");
	    // gcc4 fix: we have to cast to char*, because the buffer is
	    // usually l4_int8_t, which is 'signed char'. That's a mismatch
	    // with gcc4 (See -fsigned-char Option)
	    m_pFile->Print("(char*)");
            WriteBuffer(pType, nStartOffset, bUseConstOffset, false, false, pContext);
        }
        if (bHasSizeAttr)
        {
            m_pFile->Print(", ");
            m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
        }
        else if (!m_bMarshal)
        {
            m_pFile->Print(", (%s>%d)?%d:%s", sTemp.c_str(),
                nMax, nMax, sTemp.c_str());
        }
        m_pFile->Print(");\n");

        m_pFile->DecIndent();
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
            *m_pFile << "\tif(" << sTemp << " > 0)\n";
            m_pFile->IncIndent();
            *m_pFile << "\t";
            CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, bUsePointer, false, pContext);
            *m_pFile << "[" << sTemp << "-1] = 0;\n";
            m_pFile->DecIndent();
        }
    }
    // increase offset:
    // if (bUseConstOffset) -> set offset var ourselves using the strlen function
    // else -> increase offset var ourselves using the strlen function
    *m_pFile << "\t" << sOffset << " ";
    if (!bUseConstOffset)
        *m_pFile << "+";
    *m_pFile << "= ";

    // we exclude max_is attribute again, because we need actual size
    if (bHasSizeAttr)
        m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
    else if (m_bMarshal)
    {
        *m_pFile << "((";
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, bUsePointer, false, pContext);
        *m_pFile << ") ? strlen(";
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, bUsePointer, false, pContext);
        *m_pFile << ")+1 : 0)";
    }
    else
        *m_pFile << sTemp;

    if (bUseConstOffset)
        *m_pFile << " + " << nStartOffset;
    *m_pFile << ";\n";

    // not const offset anymore
    bUseConstOffset = false;
    // variable sized array returns size 0
    return 0;
}

/** \brief marshals a struct, which contains bitfield elements
 *  \param pType the struct type
 *  \param nStartOffset the start offset into the message buffer
 *  \param bUseConstOffset true if nStartOffset can be used
 *  \param pContext the context of the marshalling
 *  \return true if successful
 *
 * A bitfield-struct is marshalled as whole. By simple calling WriteBuffer and WriteDeclaratorStack,
 * the message buffer is casted to the struct and the whole struct is written to the message buffer.
 */
int CBEMarshaller::MarshalBitfieldStruct(CBEStructType * pType, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext)
{
    // marshal
    if (m_bMarshal)
    {
        m_pFile->PrintIndent("");
        WriteBuffer(pType, nStartOffset, bUseConstOffset, true, true, pContext);
        m_pFile->Print(" = ");
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, false,
            false, pContext);
        m_pFile->Print(";\n");
    }
    else
    {
        m_pFile->PrintIndent("");
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, false,
            false, pContext);
        m_pFile->Print(" = ");
        WriteBuffer(pType, nStartOffset, bUseConstOffset, true, true, pContext);
        m_pFile->Print(";\n");
    }
    return pType->GetSize();
}

/** \brief print the part with the message buffer
 *  \param sTypeName the name of the type to cast the message buffer to
 *  \param nStartOffset which index in the message buffer to use
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param bDereferencePosition true if the message buffer position shall be dereferenced
 *  \param bCast true if the message buffer should be casted to the type
 *  \param pContext the context of the write operation
 */
void CBEMarshaller::WriteBuffer(string sTypeName,
    int nStartOffset,
    bool & bUseConstOffset,
    bool bDereferencePosition,
    bool bCast,
    CBEContext * pContext)
{
    // for the variable sized arrays we have to add the size var to the offset
    // get current
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.back();
    assert(pCurrent);
    // get strings
    int nIdxCnt = pCurrent->GetUsedIndexCount();
    for (int i=0; i<nIdxCnt; i++)
        if (pCurrent->nIndex[i] == -2)
            m_pFile->Print("(");
    if (bDereferencePosition && (pCurrent->nIndex[0] != -2))
        m_pFile->Print("*");
    if (bCast)
        m_pFile->Print("(%s*)", sTypeName.c_str());
    // write the buffer
    CBEMsgBufferType *pMsgBuffer = m_pFunction->GetMessageBuffer();
    m_pFile->Print("(&(");
    pMsgBuffer->WriteMemberAccess(m_pFile, TYPE_INTEGER,
        (m_bMarshal)?m_pFunction->GetSendDirection():m_pFunction->GetReceiveDirection(),
        pContext);
    // write the offset in the buffer
    if (bUseConstOffset)
        m_pFile->Print("[%d]))", nStartOffset);
    else
    {
        string sOffsetVar = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        m_pFile->Print("[%s]))", sOffsetVar.c_str());
    }
    for (int i=0; i<nIdxCnt; i++)
        if (pCurrent->nIndex[i] == -2)
            m_pFile->Print(")[%s]", pCurrent->sIndex[i].c_str());
}

/**
 * If we call this function, we get the return value true if a parameter
 * has been written to the given position. If the return value is false,
 * no parameter could be written to this position. The calling function
 * should try to write at the next position.
 *
 * To ease this implementation, we first try to find the parameter which
 * needs to be written. Then we call another WriteParameter function and
 * hand this parameter and the size of the position to it. This function
 * then writes this parameter. If it has written the parameter at the position
 * it returns true, if it hasn't it returns false. If the function returned
 * false, return false ourself. If it returned true we try the next parameter.
 * This way we can try to stuff as many parameters into one position.
 * Until one parameter returns false.
 *
 * We can write the parameter pretty easy. We check if it fits in there
 * and write it. It will not fit in there if it is to big. It can be to
 * big if a) it has a constructed type or b) it has a simple type and is
 * simply to big for this position or c) its an array.
 *
 * The parameter migt as well be too small. If it is we simply write it
 * and return true -- successful write. The calling function, should then
 * try to write the next parameter to this position.
 *
 * We handle arrays, by checking until which element prvious calls must
 * have got, so we know where to start. Then we stuff as many elements in
 * there as we can. If there are any elements left, we return false. This
 * should make the caller retry a write at the next position.
 *
 * Constructed types will do something similar. They iterate over their
 * member and test which of the memebers will be the one fitting at the
 * position. Then this function is called recursively with the member.
 * (This requires another method with more parameters...).
 */
bool CBEMarshaller::MarshalToPosition(CBEFile *pFile, CBEFunction *pFunction, int nPosition, int nPosSize, int nDirection, bool bWrite, CBEContext *pContext)
{
    int nCurSize = 0, nParamSize;
    CBETypedDeclarator *pParameter = 0;
    // if we have a return var and this is the receive direction,
    // i.e. OUT for this function, we have to use it
    if ((nDirection == pFunction->GetReceiveDirection()) &&
        ((pParameter = pFunction->GetReturnVariable()) != 0) &&
        !pFunction->GetReturnType()->IsVoid())
    {
        nParamSize = pParameter->GetSize();
        // if we cross the border of the param position we want
        // to write, we stop
        if (!((nPosition-1)*nPosSize < nCurSize+nParamSize))
        {
            // unset parameter
            pParameter = 0;
            // increase current position
            nCurSize += nParamSize;
        }
    }
    // if return var is not an option, we try to find a suitable parameter
    vector<CBETypedDeclarator*>::iterator iter;
    if (!pParameter || pFunction->GetReturnType()->IsVoid())
    {
        iter = pFunction->GetFirstSortedParameter();
        while ((pParameter = pFunction->GetNextSortedParameter(iter)) != 0)
        {
            if (!pParameter->IsDirection(nDirection))
                continue;
            nParamSize = pParameter->GetSize();
            // if parameter size is negative (a pointer)
            // test for size attributes, which would indicate
            // an array. If not, we dereference it
            if (nParamSize < 0)
            {
                if (pParameter->FindAttribute(ATTR_SIZE_IS) ||
                    pParameter->FindAttribute(ATTR_LENGTH_IS) ||
                    pParameter->FindAttribute(ATTR_MAX_IS))
                    continue;
                nParamSize = pParameter->GetType()->GetSize();
            }
            // if we cross the border of the param position we want
            // to write, we stop
            if ((nPosition-1)*nPosSize < nCurSize+nParamSize)
                break;
            nCurSize += nParamSize;
        }
    }
    else
    {
        // set iterator to designated value by just stepping through the
        // list of sorted parameters
        iter = pFunction->GetFirstSortedParameter();
        while (pFunction->GetNextSortedParameter(iter) != 0) ;
    }
    // set function
    if (!m_pFunction)
        m_pFunction = pFunction;

    // now we have to try to put as many parameters in there
    // as we can. If no more parameters fit, the function returns
    // false;
    bool bWrote = false;
    while (pParameter &&
           MarshalToPosition(pFile, pParameter, nPosition, nPosSize, nCurSize,
               bWrite, pContext))
    {
        bWrote = true;
        nParamSize = pParameter->GetSize();
        // if parameter size is negative (a pointer)
        // test for size attributes, which would indicate
        // an array. If not, we dereference it
        if (nParamSize < 0)
        {
            if (pParameter->FindAttribute(ATTR_SIZE_IS) ||
                pParameter->FindAttribute(ATTR_LENGTH_IS) ||
                pParameter->FindAttribute(ATTR_MAX_IS))
                continue;
            nParamSize = pParameter->GetType()->GetSize();
        }
        nCurSize += nParamSize;
        do
        {
            // get the next parameter as long as there are parameters
            // and they have the wrong direction.
            // When we find one, going our way, we stop.
            pParameter = pFunction->GetNextSortedParameter(iter);
        } while (pParameter && !pParameter->IsDirection(nDirection));
    }
    return bWrote;
}

/** \brief writes a single parameter to an assembler position
 *  \param pFile the file to write to
 *  \param pParameter the parameter to marshal to the position
 *  \param nPosition the position to write to
 *  \param nPosSize the size of all positions
 *  \param nCurSize the current position in all positions in bytes
 *  \param bWrite true if the parameter is written to by the assembler statement
 *  \param pContext the context of the write
 */
bool CBEMarshaller::MarshalToPosition(CBEFile *pFile,
    CBETypedDeclarator *pParameter,
    int nPosition,
    int nPosSize,
    int nCurSize,
    bool bWrite,
    CBEContext * pContext)
{
    bool bWrote = false;
    // check out
    bool bOut = (pParameter->FindAttribute(ATTR_OUT) != 0);
    // check constructed type
    CBEType *pType = pParameter->GetType();
    CBEAttribute *pAttr;
    if ((pAttr = pParameter->FindAttribute(ATTR_TRANSMIT_AS)) != 0)
    {
        m_pType = pType;
        pType = pAttr->GetAttrType();
    }
    bool bConstructedType = pType->IsConstructedType();
    // check size attributes
    bool bHasSizeAttr = pParameter->FindAttribute(ATTR_SIZE_IS) ||
                        pParameter->FindAttribute(ATTR_LENGTH_IS) ||
                        pParameter->FindAttribute(ATTR_MAX_IS);
    m_pFile = pFile;
    // if m_pParameter is already set, then this is a recursive call;
    // if it is not set, than this is a plain call, and we have to set it now
    bool bSetParam = false;
    if (!m_pParameter)
    {
        m_pParameter = pParameter;
        bSetParam = true;
    }
    int nTypeSize = pType->GetSize();
    vector<CBEDeclarator*>::iterator iterD = pParameter->GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = pParameter->GetNextDeclarator(iterD)) != 0)
    {
        int nDeclSize = pDecl->GetSize();
        // if referenced OUT, the size is 1
        // if reference struct, the size is 1
        if ((nDeclSize == -1) &&
            (pDecl->GetStars() == 1) &&
             (bOut || bConstructedType))
        {
            nDeclSize = 1;
        }
        // if referenced IN, test for attributes
        // indicating array, otherwise dereference
        if ((nDeclSize == -1) &&
            (pDecl->GetStars() == 1) &&
            !bHasSizeAttr)
            nDeclSize = 1;
        // if variables sized, return false
        if (nDeclSize < 0)
        {
            if (bSetParam)
                m_pParameter = 0;
            return bWrote;
        }
        // if current decl does not fit, we skip
        if ((nPosition-1)*nPosSize >= nCurSize+(nTypeSize*nDeclSize))
            continue;
        CDeclaratorStackLocation *pLoc = new CDeclaratorStackLocation(pDecl);
        m_vDeclaratorStack.push_back(pLoc);
        if (!MarshalDeclaratorToPosition(pType, nCurSize, nPosition, nPosSize, bWrite, pContext))
        {
            m_vDeclaratorStack.pop_back();
            delete pLoc;
            if (bSetParam)
                m_pParameter = 0;
            // reset m_pType
            m_pType = 0;
            return bWrote;
        }
        m_vDeclaratorStack.pop_back();
        delete pLoc;
        bWrote = true;
        // add type size here, because call to MarshalDeclaratorToPosition requires
        // position befor this declarator.
        nCurSize += nTypeSize;
    }
    if (bSetParam)
        m_pParameter = 0;
    // reset m_pType
    m_pType = 0;
    return bWrote;
}

/**
 * This function write the parameter. It does check
 * if it is an array, a constructed type, or a simple parameter,
 * which is simply to big. If it is an array, we use the start-size
 * parameter to find out which index we need to start. If the
 * parameter is constructed, we use the start-size parameter to
 * find out, which of the members is meant and call this function
 * again with the respective member.
 *
 * To make the access correct, we need a declarator-stack.
 *
 * If the parameter is too small for the position, we use the
 * start-size and the parameter's size to calculate a bit-shift
 * to move the parameter inside the position. This bit-shift is
 * the start-size plus the parameter's size minus the position's
 * size. For array elements we can either fit as many elemnts in the
 * position as we can or we write only one element, return an the
 * calling function has to iterate over the elements. The disadvantage
 * of the latter case is that the calling function has to know
 * about array. If we implement the first case we return true if all
 * elements did fit in, and false if there are more elements to
 * add. This makes the calling function think that the parameter did
 * not fit and it tries to write something else at this position.
 * Therefore do we have to implement the first case, where the calling
 * function knows about the array. This way, it will know when the array
 * is really in the position and when not.
 *
 * Constructed and array parameters are detected here and we call this
 * function recursively to write them. We implement a similar behaviour
 * to the CBEMarshaller class, which uses the DeclaratorStack to signal
 * indices, etc. If the given parameter is an array and the declarator
 * stack's top position has an invalid index, we iterate over the bounds
 * and call this function again with the respective index set. If we reach
 * either the end of the array or a recursive call (not the first) returns
 * false, we return true. If the first recursive call returned false,
 * we return false as well.
 *
 * A constructed type iteates over the members to find the correct member
 * for the start position. (The start-size will be negative if there have
 * been members written to the previous positions.) It then adds the found
 * member to the declarator stack and calls this function recursively. This
 * way, an array declarator inside a struct will be detected as well.
 *
 * If the parameter is a union, we test if it is a C-style union, which is
 * written as if it is a "normal" parameter. If it is an IDL-style union,
 * we have to use the same behaviour as with C-style unions. We do not determine
 * which of the members will be written, we simply go by the biggest member.
 * If all members are of the same size, we use the union as parameter. If the
 * members are of different size, we stil write the union as whole, which
 * will use the space of the maximum member. We simply have to behave as if
 * we wrote a parameter of this size.
 *
 * If a parameter is too big for the position we can mask the appropriate
 * part of the member and shift it, so it will fit. If there is something
 * left to marshal we return false too.
 */
    // try to write it
    // if returned false, we return false
    // if returned true, we try next parameter
    // return true (if at least one parameter has been written)
    // situations:
    // 1) nParamSize == nPosSize
    // 2) nParamSize < nPosSize
    // 3) nParamSize > nPosSize
    //
    // for 1): no problem, get 1st decl and write it
    // for 2): as long as nCurSize < nPosSize, marshal and shift
    // for 3): check if constructed or array
    //         - if array: find start (increase nCurSize until (nPosition-1)*nPosSize
    //         - if constructed: iterate over members and repeat above test (use stack)
bool CBEMarshaller::MarshalDeclaratorToPosition(CBEType *pType, int nStartSize, int nPosition, int nPosSize, bool bWrite, CBEContext *pContext)
{
    // if current size is already beyond position, we return false
    if (nPosition*nPosSize <= nStartSize)
        return false;
    // test for parameter
    assert(m_pParameter);
    // get current decl
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.back();
    if (!pCurrent->HasIndex())
    {
        // is first -> test for array
        if (pCurrent->pDeclarator->IsArray())
            return MarshalArrayToPosition(pType, nStartSize, nPosition, nPosSize, bWrite, pContext);
        // if there are size_is or length_is attribtes and the
        // decl has stars, then this is a variable sized array
        if ((pCurrent->pDeclarator->GetStars() > 0) &&
            (m_pParameter->FindAttribute(ATTR_SIZE_IS) ||
             m_pParameter->FindAttribute(ATTR_LENGTH_IS)))
            return MarshalArrayToPosition(pType, nStartSize, nPosition, nPosSize, bWrite, pContext);
        // since the declarator can also be a member of a struct
        // check the direct parent of the declarator
        if (!pCurrent->pDeclarator->GetSpecificParent<CBEFunction>() &&
             pCurrent->pDeclarator->GetSpecificParent<CBEStructType>() &&
            (pCurrent->pDeclarator->GetStars() > 0))
        {
            CBETypedDeclarator *pMember = pCurrent->pDeclarator->GetSpecificParent<CBEStructType>()->FindMember(pCurrent->pDeclarator->GetName());
            if (pMember &&
                (pMember->FindAttribute(ATTR_SIZE_IS) ||
                 pMember->FindAttribute(ATTR_LENGTH_IS)))
            {
                CBETypedDeclarator *pOldParam = m_pParameter;
                m_pParameter = pMember;
                bool bRet = MarshalArrayToPosition(pType, nStartSize, nPosition, nPosSize, bWrite, pContext);
                m_pParameter = pOldParam;
                return bRet;
            }
        }
        // a string is similar to an array, test for it
        if (m_pParameter->IsString())
        {
            CCompiler::Error("Cannot write string (%s) to asm register.", pCurrent->pDeclarator->GetName().c_str()); // var sized array
        }
    }

    // test user defined types (usually they are defined in a C header file and thus
    // not in the IDL namespace)
    vector<CBEExpression*> vBounds;
    while (dynamic_cast<CBEUserDefinedType*>(pType))
    {
        // search for original type and replace it
        CBERoot *pRoot = pType->GetSpecificParent<CBERoot>();
        assert(pRoot);
        CBETypedef *pUserType = pRoot->FindTypedef(((CBEUserDefinedType *) pType)->GetName());
        // if 0: not defined in IDL files -> use user-provided init function
        if (!pUserType || !pUserType->GetType())
            break;
        // assign aliased type
        pType = pUserType->GetType();
        // if alias is array?
        CBEDeclarator *pAlias = pUserType->GetAlias();
        // check if type has alias (it should, since the alias is
        // the name of the user defined type)
        if (pAlias && pAlias->IsArray())
        {
            // add array bounds of alias to temp vector
            vector<CBEExpression*>::iterator iterB = pAlias->GetFirstArrayBound();
            CBEExpression *pBound;
            while ((pBound = pAlias->GetNextArrayBound(iterB)) != 0)
            {
                vBounds.push_back(pBound);
            }
        }
    }

    // check if user defined type added array bounds
    if (vBounds.size() > 0)
    {
        vector<CBEExpression*>::iterator iterB;
        // add bounds to top declarator
        for (iterB = vBounds.begin(); iterB != vBounds.end(); iterB++)
        {
            pCurrent->pDeclarator->AddArrayBound(*iterB);
        }
        // call MarshalArray
        bool bRet = MarshalArrayToPosition(pType, nStartSize, nPosition, nPosSize, bWrite, pContext);
        // remove those array decls again
        for (iterB = vBounds.begin(); iterB != vBounds.end(); iterB++)
        {
            pCurrent->pDeclarator->RemoveArrayBound(*iterB);
        }
        // return (work done)
        return bRet;
    }

    // test for struct
    if (dynamic_cast<CBEStructType*>(pType))
        return MarshalStructToPosition((CBEStructType*)pType, nStartSize, nPosition, nPosSize, bWrite, pContext);

    // test for union
    if (dynamic_cast<CBEUnionType*>(pType))
    {
        // test for not C style union
        if (!((CBEUnionType*)pType)->IsCStyleUnion())
            return MarshalUnionToPosition((CBEUnionType*)pType, nStartSize, nPosition, nPosSize, bWrite, pContext);
        else
        {
            // get the biggest members
            vector<CBEUnionCase*>::iterator iterU = ((CBEUnionType*)pType)->GetFirstUnionCase();
            CBEUnionCase *pUnionCase, *pBiggestUnionCase = 0;
            while ((pUnionCase = ((CBEUnionType*)pType)->GetNextUnionCase(iterU)) != 0)
            {
                if (!pBiggestUnionCase)
                {
                    pBiggestUnionCase = pUnionCase;
                    continue;
                }
                if (pUnionCase->GetSize() > pBiggestUnionCase->GetSize())
                    pBiggestUnionCase = pUnionCase;
            }
            // marshal the biggest member
            return MarshalToPosition(m_pFile, pBiggestUnionCase, nPosition, nPosSize, nStartSize, bWrite, pContext);
        }

    }
    // if start size is beyond the start of this position, there is laready something
    // in here and we have to OR this parameter bitwise
    if (nStartSize > (nPosition-1)*nPosSize)
        m_pFile->Print(" | ");
    // get parameter's size
    int nParamSize = pCurrent->pDeclarator->GetSize()*pType->GetSize();
    // for a 4 byte position we can have for instance the following message:
    // byte 1 and 2: short a; byte 3 and 4: short b
    // because x86 uses little endian to encode variable, a "normal" marshal
    // would stuff the lower byte of a (a.lower) into buffer[0] and the
    // higher byte of a (a.higher) into buffer[1] =>
    // buffer[0] = a.lower;
    // buffer[1] = a.higher;
    // buffer[2] = b.lower;
    // buffer[3] = b.higher;
    // if we put this into one position using shift, ANDs and ORs, we have to
    // "emulate" this:
    // position = (((mword) a) & 0xffff) | ((((mword) b) << 16) & 0xffff0000)
    //
    //     nShiftBytes = nPosSize-(nStartSize-((nPosition-1)*nPosSize))
    //                   nPosSize-nStartSize+((nPosition-1)*nPosSize)
    //                   nPosition*nPosSize-nStartSize
    //                   -> nStartSize = (nPosition*nPosSize)+1 -> shift left by 1 byte
    //                   -> nStartSize = (nPosition*nPosSize)+2 -> shift left by 2 bytes
    //                -> nStartSize = (nPosition*nPosSize)+3 -> shift left by 3 bytes
    //                   -> nStartSize = (nPosition*nPosSize)+4 -> shift left by 4 bytes
    // Bitmask = x*"ff" + nShiftBytes*"00"
    // nPosSize = 4; nParamSize = 4; nShiftBytes = 0 -> x = 4 = MIN(nPosSize - nShiftBytes, nParamSize)
    // nPosSize = 4; nParamSize = 4; nShiftBytes = 1 -> x = 3 = MIN(nPosSize - nShiftBytes, nParamSize)
    // nPosSize = 4; nParamSize = 4; nShiftBytes = 2 -> x = 2 = MIN(nPosSize - nShiftBytes, nParamSize)
    // nPosSize = 4; nParamSize = 4; nShiftBytes = 3 -> x = 1 = MIN(nPosSize - nShiftBytes, nParamSize)
    //
    // nPosSize = 4; nParamSize = 2; nShiftBytes = 0 -> x = 2 = MIN(nPosSize - nShiftBytes, nParamSize)
    // nPosSize = 4; nParamSize = 2; nShiftBytes = 1 -> x = 2 = MIN(nPosSize - nShiftBytes, nParamSize)
    // nPosSize = 4; nParamSize = 2; nShiftBytes = 2 -> x = 2 = MIN(nPosSize - nShiftBytes, nParamSize)
    // nPosSize = 4; nParamSize = 2; nShiftBytes = 3 -> x = 1 = MIN(nPosSize - nShiftBytes, nParamSize)
    //
    // nPosSize = 3; nParamSize = 2; nShiftBytes = 0 -> x = 2 = MIN(nPosSize - nShiftBytes, nParamSize)
    // nPosSize = 3; nParamSize = 2; nShiftBytes = 1 -> x = 2 = MIN(nPosSize - nShiftBytes, nParamSize)
    // nPosSize = 3; nParamSize = 2; nShiftBytes = 2 -> x = 1 = MIN(nPosSize - nShiftBytes, nParamSize)

    // get shift-bits
    int nShiftBytes = nStartSize-(nPosition-1)*nPosSize;
    int nNumFF = 0;
    string sBitmask;
    if (nShiftBytes > 0)
    {
        nNumFF = ((nPosSize - nShiftBytes) < nParamSize) ? (nPosSize - nShiftBytes) : nParamSize;
        for (int i=0; i<nNumFF; i++)
            sBitmask += "ff";
        for (int i=0; i<nShiftBytes; i++)
            sBitmask += "00";
    }
    else if (nShiftBytes < 0)
    {
        nNumFF = ((nPosSize + nShiftBytes) < nParamSize) ? (nPosSize + nShiftBytes) : nParamSize;
        for (int i=0; i<-nShiftBytes; i++)
            sBitmask += "00";
        for (int i=0; i<nNumFF; i++)
            sBitmask += "ff";
    }
    else if (nParamSize < nPosSize)
    {
        nNumFF = nParamSize;
        for (int i=0; i<nNumFF; i++)
            sBitmask += "ff";
    }
    // if shift bits are the same as size of parameter, then parameter
    // will not be visible
    string sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, false, pContext, 0);
    bool bCast = pContext->GetSizes()->GetSizeOfType(TYPE_MWORD) != nParamSize;
    // cannot shift if we write to this parameter
    if (!bWrite)
    {
        if (!sBitmask.empty())
            m_pFile->Print("(");
        if (nShiftBytes != 0)
            m_pFile->Print("(");
        if (bCast)
            m_pFile->Print("((%s)", sMWord.c_str());
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack,
            m_pParameter->IsString(), false, pContext);
        if (bCast)
            m_pFile->Print(")");
        if (nShiftBytes > 0)
            m_pFile->Print("<<%d)", nShiftBytes*8);
        else if (nShiftBytes < 0)
            m_pFile->Print(">>%d)", -nShiftBytes*8);
        if (!sBitmask.empty())
            m_pFile->Print(" & 0x%s)", sBitmask.c_str());
    }
    else
    {
        // we don't care about bitshifted unmarshalling for now, because the
        // receiver is the server, which receives into message buffer anyways
        // and expects the parameters in the bit-stuffed order
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack,
            m_pParameter->IsString(), false, pContext);
    }
    return true;
}

/** \brief writes a array to a specific position in a (fictive) messsage buffer
 *  \param pType the type of the array elements
 *  \param nStartSize the size where to start in the array
 *  \param nPosition the position in the (fictive) message buffer where to marshal to
 *  \param nPosSize the size of the positions
 *  \param bWrite true if the array is written to (from the message buffer)
 *  \param iter the iterator pointing to the next array bound
 *  \param nLevel the number of the current array bound
 *  \param pContext the context of the write
 *  \return false if nothing could be written (didn't fit)
 */
bool
CBEMarshaller::MarshalConstArrayToPosition(CBEType *pType,
    int nStartSize,
    int nPosition,
    int nPosSize,
    bool bWrite,
    vector<CBEExpression*>::iterator iter,
    int nLevel,
    CBEContext *pContext)
{
    bool bRet = false;
    int nSize = nStartSize;
    int nTypeSize = pType->GetSize();
    // get current declarator
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.back();
    CBEDeclarator *pDeclarator = pCurrent->pDeclarator;
    // get current array boundary
    CBEExpression *pBound = pDeclarator->GetNextArrayBound(iter);
    int nBound = pBound->GetIntValue();
    // iterate over index
    for (int nIndex = 0; nIndex < nBound; nIndex++)
    {
        pCurrent->SetIndex(nIndex, nLevel);
        // if there are more array bounds, check them as well
        vector<CBEExpression*>::iterator iTemp = iter;
        if (pDeclarator->GetNextArrayBound(iTemp))
        {
            if (!MarshalConstArrayToPosition(pType, nStartSize, nPosition, nPosSize, bWrite, iter, nLevel+1, pContext))
                return bRet;
            bRet = true;
        }
        // otherwise try to find fitting index
        else
        {
            nSize += nTypeSize;
            // if current index does not fit, we skip
            if ((nPosition-1)*nPosSize > 0)
                continue;
            if (!MarshalDeclaratorToPosition(pType, nSize, nPosition, nPosSize, bWrite, pContext))
                return bRet;
            bRet = true;
        }
    }
    pCurrent->SetIndex(-1, nLevel);
    // return if we marshalled something
    return bRet;
}

/** \brief writes a array to a specific position in a (fictive) messsage buffer
 *  \param pType the type of the array elements
 *  \param nStartSize the size where to start in the array
 *  \param nPosition the position in the (fictive) message buffer where to marshal to
 *  \param nPosSize the size of the positions
 *  \param bWrite true if the array is written to (from the message buffer)
 *  \param pContext the context of the write
 *  \return false if nothing could be written (didn't fit)
 */
bool CBEMarshaller::MarshalArrayToPosition(CBEType *pType, int nStartSize, int nPosition, int nPosSize, bool bWrite, CBEContext *pContext)
{
    bool bRet = false;
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.back();
    CBEDeclarator *pDeclarator = pCurrent->pDeclarator;
    // if fixed size
    if ((pDeclarator->GetSize() >= 0) &&
        !(m_pParameter->FindAttribute(ATTR_SIZE_IS)) &&
        !(m_pParameter->FindAttribute(ATTR_LENGTH_IS)))
    {
        if (!MarshalConstArrayToPosition(pType, nStartSize, nPosition, nPosSize, bWrite, pDeclarator->GetFirstArrayBound(), 0, pContext))
            return bRet;
        bRet = true;
    }
    else
    {
        // FIXME: cannot marshal this here, because we cannot determine the correct index (might be to big)
        CCompiler::Error("Cannot marshal variable sized array (%s) to asm.", pDeclarator->GetName().c_str());
    }
    return bRet;
}

/** \brief writes a struct to a assembler marshal position
 *  \param pType the struct to be marshalled
 *  \param nStartSize the size in the struct from where to start
 *  \param nPosition the position in the message buffer (this is fictive and only describes, which parameter should be marshalled)
 *  \param nPosSize the size of all positions
 *  \param bWrite true if the parameter will be written to
 *  \param pContext the context of the write operation
 */
bool CBEMarshaller::MarshalStructToPosition(CBEStructType *pType, int nStartSize, int nPosition, int nPosSize, bool bWrite, CBEContext *pContext)
{
    // if this is a tagged struct without members, we have to find the original struct
    if ((pType->GetMemberCount() == 0) && (pType->GetFEType() == TYPE_TAGGED_STRUCT))
    {
        // search for tag
        CBERoot *pRoot = pType->GetSpecificParent<CBERoot>();
        assert(pRoot);
        CBEStructType *pTaggedType = (CBEStructType*)pRoot->FindTaggedType(TYPE_TAGGED_STRUCT, pType->GetTag());
        // if found, marshal this instead
        if ((pTaggedType) && (pTaggedType != pType))
            pType = pTaggedType;
        else
            return false;
    }
    // if whole struct is size of position it can be marshalled
    // by casting it to the position's type
    if (pType->GetSize() == nPosSize)
    {
        string sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, false, pContext, 0);
        m_pFile->Print("*((%s*)(&", sMWord.c_str());
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, false,
            false, pContext);
        m_pFile->Print("))");
        return true;
    }
    // check for bitfields
    vector<CBETypedDeclarator*>::iterator iterS = pType->GetFirstMember();
    CBETypedDeclarator *pMember;
    bool bBitfields = false;
    while (((pMember = pType->GetNextMember(iterS)) != 0) && !bBitfields)
    {
        if (pMember->GetSize() == 0)
            bBitfields = true;
    }
    if (bBitfields)
    {
        CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack, false,
            false, pContext);
        return true;
    }
    // marshal "normal" struct
    bool bRet = false;
    int nSize = nStartSize, nParamSize, nTypeSize;
    iterS = pType->GetFirstMember();
    CBETypedDeclarator *pOldParameter;
    while (((pMember = pType->GetNextMember(iterS)) != 0) &&
            // if we are past the current position, return
            (nPosition*nPosSize > nSize))
    {
        nParamSize = pMember->GetSize();
        // if we cross the border of the param position we want
        // to write, we actually do write it, otherwise we skip
        nSize += nParamSize;
        if ((nPosition-1)*nPosSize > nSize)
            continue;
        // remove param size (so we can test each decl)
        nSize -= nParamSize;
        // swap current parameter
        pOldParameter = m_pParameter;
        m_pParameter = pMember;
        // get type
        CBEType *pType = pMember->GetType();
        nTypeSize = pType->GetSize();
        // add declarator
        vector<CBEDeclarator*>::iterator iterM = pMember->GetFirstDeclarator();
        CBEDeclarator *pD;
        while (((pD = pMember->GetNextDeclarator(iterM)) != 0) &&
                // if we are past the current position, return
                (nPosition*nPosSize > nSize))
        {
            int nDeclSize = pD->GetSize();
            // first add decl's size, so we still add it if we skip
            nSize += nTypeSize*nDeclSize;
            // if parameter is before the current position, then skip it
            if ((nPosition-1)*nPosSize >= nSize)
                continue;
            // remove it again, so we have the real start size
            nSize -= nTypeSize*nDeclSize;
            // actually write it
            CDeclaratorStackLocation *pLoc = new CDeclaratorStackLocation(pD);
            m_vDeclaratorStack.push_back(pLoc);
            if (!MarshalDeclaratorToPosition(pType, nSize, nPosition, nPosSize, bWrite, pContext))
            {
                m_vDeclaratorStack.pop_back();
                delete pLoc;
                return bRet;
            }
            bRet = true;
            m_vDeclaratorStack.pop_back();
            delete pLoc;
            // increase size
            nSize += nTypeSize*nDeclSize;
        }
        m_pParameter = pOldParameter;
    }
    // we should only stop here if there are no members,
    // return true, for we did write 'nothing'
    return bRet;
}

/**    \brief marshals a union at a specified posintion in the message buffer
 *    \param pType the union type
 *    \param nStartSize the offset in the union where to start marshalling
 *    \param nPosition the index in the message buffer
 *    \param nPosSize the size of the element of the message buffer at that pos.
 *    \param bWrite true if we write, false if we read
 *    \param pContext the context of the write oepration
 *    \return true if the marshal/unmarshal was successful
 */
bool CBEMarshaller::MarshalUnionToPosition(CBEUnionType *pType, int nStartSize, int nPosition, int nPosSize, bool bWrite, CBEContext *pContext)
{
    // marshal switch variable
    CBETypedDeclarator *pSwitchVar = pType->GetSwitchVariable();
    if (nStartSize+pSwitchVar->GetSize() == (nPosition-1)*nPosSize)
    {
        // write union declarator
        if (pType->GetUnionName())
        {
        // FIXME: check union's size -> treat as if simple decl which is too big
            CDeclaratorStackLocation *pLoc =
                new CDeclaratorStackLocation(pType->GetUnionName());
            pLoc->SetIndex(-3);
            m_vDeclaratorStack.push_back(pLoc);
            CDeclaratorStackLocation::Write(m_pFile, &m_vDeclaratorStack,
                false, false, pContext);
            m_vDeclaratorStack.pop_back();
        }
        // return size
        // we actually return a size value -> this makes Marshal ignore the
        // temp offset * size of type addition.
        return true;
    }
    else
    {
        vector<CBEDeclarator*>::iterator iterS = pSwitchVar->GetFirstDeclarator();
        CBEDeclarator *pDecl = *iterS;
        CBEType *pSType = pSwitchVar->GetType();
        CDeclaratorStackLocation *pLoc = new CDeclaratorStackLocation(pDecl);
        m_vDeclaratorStack.push_back(pLoc);
        bool bRet = MarshalDeclaratorToPosition(pSType, nStartSize, nPosition, nPosSize, bWrite, pContext);
        m_vDeclaratorStack.pop_back();
        delete pLoc;
        return bRet;
    }
    return false;
}

/** \brief tests if the parameters of this function fit into the given position size
 *  \param pFunction the function to test
 *  \param nPosSize the position size to use for testing
 *  \param nDirection the direction to test for
 *  \param bAllowSmaller true if the parameter may be smaller than the position size
 *  \param bAllowLarger true if the parameter may be bigger than the position size
 *    \param nNumber the number of parameters to test
 *  \param pContext the context of this test
 *  \return true if function fits
 *
 * This test does not directly concern the marshalling code, but since all the algorithms
 * and heuristic on how to marshal a parameter are collected here, I added this function here
 * as well, since it uses these algorithms and heuristics too.
 */
bool CBEMarshaller::TestPositionSize(CBEFunction *pFunction, int nPosSize, int nDirection, bool bAllowSmaller, bool bAllowLarger, int nNumber, CBEContext *pContext)
{
    if (nDirection == 0)
    {
        return TestPositionSize(pFunction, nPosSize, pFunction->GetSendDirection(),
                                bAllowSmaller, bAllowLarger, nNumber, pContext) &&
               TestPositionSize(pFunction, nPosSize, pFunction->GetReceiveDirection(),
                                bAllowSmaller, bAllowLarger, nNumber, pContext);
    }

    CBETypedDeclarator *pParameter = 0;
    // if we have a return var and this is the receive direction,
    // i.e. OUT for this function, we have to use it
    if ((nDirection == pFunction->GetReceiveDirection()) &&
        ((pParameter = pFunction->GetReturnVariable()) != 0) &&
        !pFunction->GetReturnType()->IsVoid() &&
        (nNumber != 0))
    {
        if (!TestPositionSize(pParameter, nPosSize, bAllowSmaller, bAllowLarger, pContext))
            return false;
        nNumber--;
    }
    // now test the 'normal' parameters
    vector<CBETypedDeclarator*>::iterator iter = pFunction->GetFirstSortedParameter();
    while ((pParameter = pFunction->GetNextSortedParameter(iter)) != 0)
    {
        if (nNumber == 0)
            continue;
        if (!pParameter->IsDirection(nDirection))
            continue;
        if (!TestPositionSize(pParameter, nPosSize, bAllowSmaller, bAllowLarger, pContext))
            return false;
        nNumber--;
    }

    // if all of the parameters fit, return true
    return true;
}

/** \brief test if specific parameter fits into the position
 *  \param pParameter the parameter to tests
 *  \param nPosSize the size of a position
 *  \param bAllowSmaller true if smaller parameters are allowed
 *  \param bAllowLarger true if larger parameters are allowed
 *  \param pContext the context of this test
 *  \return true if parameter fits
 *
 * We iterate over the declarators of a parameter an test them seperately
 */
bool CBEMarshaller::TestPositionSize(CBETypedDeclarator* pParameter,
    int nPosSize,
    bool bAllowSmaller,
    bool bAllowLarger,
    CBEContext *pContext)
{
    bool bOut = (pParameter->FindAttribute(ATTR_OUT) != 0);
    CBEType *pType = pParameter->GetType();
    CBEAttribute *pAttr;
    if ((pAttr = pParameter->FindAttribute(ATTR_TRANSMIT_AS)) != 0)
        pType = pAttr->GetAttrType();
    bool bConstructedType = pType->IsConstructedType();
    bool bSetParam = false;
    // if m_pParameter is already set, then this is a recursive call;
    // if it is not set, than this is a plain call, and we have to set it now
    if (!m_pParameter)
    {
        m_pParameter = pParameter;
        bSetParam = true;
    }
    vector<CBEDeclarator*>::iterator iterD = pParameter->GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = pParameter->GetNextDeclarator(iterD)) != 0)
    {
        int nDeclSize = pDecl->GetSize();
        // if referenced OUT, the sizeis 1
        // if reference struct, the size is 1
        if ((nDeclSize == -1) &&
            (pDecl->GetStars() == 1) &&
             (bOut || bConstructedType))
        {
            nDeclSize = 1;
        }
        // if variables sized, return false
        if (nDeclSize < 0)
        {
            if (bSetParam)
                m_pParameter = 0;
            return false;
        }
        // push decl and check it
        CDeclaratorStackLocation *pLoc = new CDeclaratorStackLocation(pDecl);
        m_vDeclaratorStack.push_back(pLoc);
        if (!TestDeclaratorPositionSize(pType, nPosSize, bAllowSmaller, bAllowLarger, pContext))
        {
            if (bSetParam)
                m_pParameter = 0;
            return false;
        }
        m_vDeclaratorStack.pop_back();
        delete pLoc;
    }
    // if all declarators fit, return true
    if (bSetParam)
        m_pParameter = 0;
    return true;
}

/** \brief test if a specific declarator fits into the position
 *  \param pType the type of the declarator
 *    \param nPosSize the size of the position where to marshal the declarator
 *  \param bAllowSmaller true if declarator can be smaller than position
 *  \param bAllowLarger true if declarator can be larger than position
 *  \param pContext the context of the test
 *  \return true if declarator fits
 *
 */
bool CBEMarshaller::TestDeclaratorPositionSize(CBEType* pType, int nPosSize, bool bAllowSmaller, bool bAllowLarger, CBEContext *pContext)
{
    assert(m_pParameter);
    // get current decl
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.back();
    if (!pCurrent->HasIndex())
    {
        // is first -> test for array
        if (pCurrent->pDeclarator->IsArray())
            return TestArrayPositionSize(pType, nPosSize, bAllowSmaller, bAllowLarger, pContext);
        // test if parameter has size is attributes
        if ((pCurrent->pDeclarator->GetStars() > 0) &&
            (m_pParameter->FindAttribute(ATTR_SIZE_IS) ||
             m_pParameter->FindAttribute(ATTR_LENGTH_IS)))
            return TestArrayPositionSize(pType, nPosSize, bAllowSmaller, bAllowLarger, pContext);
        // since the declarator can also be a member of a struct
        // check the direct parent of the declarator
        if (!pCurrent->pDeclarator->GetSpecificParent<CBEFunction>() &&
             pCurrent->pDeclarator->GetSpecificParent<CBEStructType>() &&
            (pCurrent->pDeclarator->GetStars() > 0))
        {
            CBETypedDeclarator *pMember = pCurrent->pDeclarator->GetSpecificParent<CBEStructType>()->FindMember(pCurrent->pDeclarator->GetName());
            if (pMember &&
                (pMember->FindAttribute(ATTR_SIZE_IS) ||
                 pMember->FindAttribute(ATTR_LENGTH_IS)))
            {
                CBETypedDeclarator *pOldParam = m_pParameter;
                m_pParameter = pMember;
                bool bRet = TestArrayPositionSize(pType, nPosSize, bAllowSmaller, bAllowLarger, pContext);
                m_pParameter = pOldParam;
                return bRet;
            }
        }
        // a string is similar to an array, test for it
        if (m_pParameter->IsString())
            return false;
    }

    // test user defined types (usually they are defined in a C header file and thus
    // not in the IDL namespace)
    vector<CBEExpression*> vBounds;
    while (dynamic_cast<CBEUserDefinedType*>(pType))
    {
        // search for original type and replace it
        CBERoot *pRoot = pType->GetSpecificParent<CBERoot>();
        assert(pRoot);
        CBETypedef *pUserType = pRoot->FindTypedef(((CBEUserDefinedType *) pType)->GetName());
        // if 0: not defined in IDL files -> use user-provided init function
        if (!pUserType || !pUserType->GetType())
            break;
        // assign aliased type
        pType = pUserType->GetType();
        // if alias is array?
        CBEDeclarator *pAlias = pUserType->GetAlias();
        // check if type has alias (it should, since the alias is
        // the name of the user defined type)
        if (pAlias && pAlias->IsArray())
        {
            // add array bounds of alias to temp vector
            vector<CBEExpression*>::iterator iterB = pAlias->GetFirstArrayBound();
            CBEExpression *pBound;
            while ((pBound = pAlias->GetNextArrayBound(iterB)) != 0)
            {
                vBounds.push_back(pBound);
            }
        }
    }
    // check if user defined types added array bounds
    if (vBounds.size() > 0)
    {
        vector<CBEExpression*>::iterator iterB;
        // add bounds to top declarator
        for (iterB = vBounds.begin(); iterB != vBounds.end(); iterB++)
        {
            pCurrent->pDeclarator->AddArrayBound(*iterB);
        }
        // call MarshalArray
        bool bRet = TestArrayPositionSize(pType, nPosSize, bAllowSmaller, bAllowLarger, pContext);
        // remove those array decls again
        for (iterB = vBounds.begin(); iterB != vBounds.end(); iterB++)
        {
            pCurrent->pDeclarator->RemoveArrayBound(*iterB);
        }
        // return (work done)
        return bRet;
    }
    // test for struct
    if (dynamic_cast<CBEStructType*>(pType))
    {
        return TestStructPositionSize((CBEStructType*)pType, nPosSize, bAllowSmaller, bAllowLarger, pContext);
    }

    // test for union
    if (dynamic_cast<CBEUnionType*>(pType))
    {
        // test for not C style union
        if (!((CBEUnionType*)pType)->IsCStyleUnion())
        {
            return TestUnionPositionSize((CBEUnionType*)pType, nPosSize, bAllowSmaller, bAllowLarger, pContext);
        }
        else
        {
            // get the biggest members
            vector<CBEUnionCase*>::iterator iterU = ((CBEUnionType*)pType)->GetFirstUnionCase();
            CBEUnionCase *pUnionCase, *pBiggestUnionCase = 0;
            while ((pUnionCase = ((CBEUnionType*)pType)->GetNextUnionCase(iterU)) != 0)
            {
                if (!pBiggestUnionCase)
                {
                    pBiggestUnionCase = pUnionCase;
                    continue;
                }
                if (pUnionCase->GetSize() > pBiggestUnionCase->GetSize())
                    pBiggestUnionCase = pUnionCase;
            }
            // marshal the biggest member
            return TestPositionSize(pBiggestUnionCase, nPosSize, bAllowSmaller, bAllowLarger, pContext);
        }

    }

    // test size of declarator
    int nSize = pCurrent->pDeclarator->GetSize()*pType->GetSize();
    // if decl's size is -1 this can be an simple pointer to an OUT variable
    if ((pCurrent->pDeclarator->GetSize() == -1) &&
        (m_pParameter->FindAttribute(ATTR_OUT)))
        nSize = pType->GetSize();
    // if smaller (and allowed) -> true
    if (bAllowSmaller && (nSize < nPosSize))
        return true;
    // if bigger (and allowed) -> true
    if (bAllowLarger && (nSize > nPosSize))
        return true;
    // if declarator fits into this position -> true
    if (nSize == nPosSize)
        return true;
    // everything else does not fit -> false
    return false;
}

/** \brief test if array fits into the position
 *  \param pType the type of the array's elements
 *  \param nPosSize the size of the positions
 *  \param bAllowSmaller true if elements migt be smaller than position
 *  \param bAllowLarger true if elements might be larger than position
 *  \param iter the pointer to the current array boundary
 *  \param nLevel the number of the current level
 *  \param pContext the context of this tests
 *  \return true if array elements fit in there
 */
bool
CBEMarshaller::TestConstArrayPositionSize(CBEType* pType,
    int nPosSize,
    bool bAllowSmaller,
    bool bAllowLarger,
    vector<CBEExpression*>::iterator iter,
    int nLevel,
    CBEContext *pContext)
{
    // get current declarator
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.back();
    CBEDeclarator *pDeclarator = pCurrent->pDeclarator;
    // get current array boundary
    CBEExpression *pBound = pDeclarator->GetNextArrayBound(iter);
    int nBound = pBound->GetIntValue();
    // loop over elements
    for (int nIndex = 0; nIndex < nBound; nIndex++)
    {
        pCurrent->SetIndex(nIndex, nLevel);
        // check if we have more array bounds
        vector<CBEExpression*>::iterator iTemp = iter;
        if (pDeclarator->GetNextArrayBound(iTemp))
        {
            if (!TestConstArrayPositionSize(pType, nPosSize, bAllowSmaller, bAllowLarger, iter, nLevel+1, pContext))
                return false;
        }
        // otherwise check elements
        else
        {
            if (!TestDeclaratorPositionSize(pType, nPosSize, bAllowSmaller, bAllowLarger, pContext))
                return false;
        }
    }
    pCurrent->SetIndex(-1, nLevel);
    // all elements fit
    return true;
}

/** \brief test if array fits into the position
 *  \param pType the type of the array's elements
 *  \param nPosSize the size of the positions
 *  \param bAllowSmaller true if elements migt be smaller than position
 *  \param bAllowLarger true if elements might be larger than position
 *  \param pContext the context of this tests
 *  \return true if array elements fit in there
 */
bool CBEMarshaller::TestArrayPositionSize(CBEType* pType, int nPosSize, bool bAllowSmaller, bool bAllowLarger, CBEContext *pContext)
{
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.back();
    CBEDeclarator *pDeclarator = pCurrent->pDeclarator;
    // check if there is already somebody testing with arrays
    int nLevel = pCurrent->GetUsedIndexCount();
    // skip array bounds in used
    int i = nLevel;
    vector<CBEExpression*>::iterator iter = pDeclarator->GetFirstArrayBound();
    while (i-- > 0) pDeclarator->GetNextArrayBound(iter);
    // if fixed size
    if ((pDeclarator->GetSize() >= 0) &&
        !(m_pParameter->FindAttribute(ATTR_SIZE_IS)) &&
        !(m_pParameter->FindAttribute(ATTR_LENGTH_IS)))
    {
        if (!TestConstArrayPositionSize(pType, nPosSize, bAllowSmaller, bAllowLarger, iter, nLevel, pContext))
            return false;
    }
    else
        // we cannot marshal an array with size attribute to a specific position
        // (we don't know how to index it correctly)
        return false;
    return true;
}

/** \brief test if the struct fits into the position
 *  \param pType the struct to test
 *  \param nPosSize the size of a position
 *  \param bAllowSmaller true if the struct (or its elements) might be smaller than the position
 *  \param bAllowLarger true if the struct (or its elements) might be larger than the position
 *  \param pContext the context of the test
 *  \return true if struct fits
 */
bool CBEMarshaller::TestStructPositionSize(CBEStructType* pType, int nPosSize, bool bAllowSmaller, bool bAllowLarger, CBEContext *pContext)
{
    // if this is a tagged struct without members, we have to find the original struct
    if ((pType->GetMemberCount() == 0) && (pType->GetFEType() == TYPE_TAGGED_STRUCT))
    {
        // search for tag
        CBERoot *pRoot = pType->GetSpecificParent<CBERoot>();
        assert(pRoot);
        CBEStructType *pTaggedType = (CBEStructType*)pRoot->FindTaggedType(TYPE_TAGGED_STRUCT, pType->GetTag());
        // if found, marshal this instead
        if ((pTaggedType) && (pTaggedType != pType))
            pType = pTaggedType;
        else
            return false;
    }
    // If the whole struct is the size of the position it
    // might fit if casted to the type of the position
    if (pType->GetSize() == nPosSize)
        return true;
    // we ignore the bitfield case in the sense that every
    // element, which is a bitfield should fit into a position
    vector<CBETypedDeclarator*>::iterator iter = pType->GetFirstMember();
    CBETypedDeclarator *pMember;
    while ((pMember = pType->GetNextMember(iter)) != 0)
    {
        // test member
        if (!TestPositionSize(pMember, nPosSize, bAllowSmaller, bAllowLarger, pContext))
            return false;
    }
    // return true, if all members did fit
    return true;
}

/** \brief test if the union fits into the position
 *  \param pType the union to test
 *  \param nPosSize the size of the position
 *  \param bAllowSmaller true if the union might be smaller than the position
 *  \param bAllowLarger true if the union might be larger than the position
 *  \param pContext the context of the test
 *  \return true if union fits
 */
bool CBEMarshaller::TestUnionPositionSize(CBEUnionType* pType, int nPosSize, bool bAllowSmaller, bool bAllowLarger, CBEContext *pContext)
{
    // check switch variable
    CBETypedDeclarator *pSwitchVar = pType->GetSwitchVariable();
    if (!TestPositionSize(pSwitchVar, nPosSize, bAllowSmaller, bAllowLarger, pContext))
        return false;
    if (bAllowSmaller && (pType->GetSize() < nPosSize))
        return true;
    if (bAllowLarger && (pType->GetSize() > nPosSize))
        return true;
    if (pType->GetSize() == nPosSize)
        return true;
    return false;
}
