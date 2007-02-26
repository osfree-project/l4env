/**
 *	\file	dice/src/be/BEMarshaller.cpp
 *	\brief	contains the implementation of the class CBEMarshaller
 *
 *	\date	05/08/2002
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
#include "Vector.h"

#include "fe/FEAttribute.h"
#include "fe/FETypeSpec.h"

IMPLEMENT_DYNAMIC(CBEMarshaller);

CBEMarshaller::CBEMarshaller()
{
    m_pFile = 0;
    m_pType = 0;
    m_bMarshal = true;
    m_pParameter = 0;
    m_pFunction = 0;
    IMPLEMENT_DYNAMIC_BASE(CBEMarshaller, CBEObject);
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
    return Marshal(pFile, pFunction, 0, nStartOffset, bUseConstOffset, pContext);
}

/** \brief only marshal parameter with specific type
 *  \param pFile the fiel to write to
 *  \param pFunction the function to marshal
 *  \param nFEType the type of the parameters to marshal (0 if all)
 *  \param nStartOffset the offset where to start in the message buffer
 *  \param bUseConstOffset true if nStartOffset can be used
 *  \param pContext the context of the write
 *  \return the size of the marshalled values
 */
int CBEMarshaller::Marshal(CBEFile * pFile, CBEFunction * pFunction, int nFEType, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext)
{
    int nSize = 0;
    m_pFunction = pFunction;
    VectorElement *pIter = pFunction->GetFirstSortedParameter();
    CBETypedDeclarator *pParam;
    while ((pParam = pFunction->GetNextSortedParameter(pIter)) != 0)
    {
        if (nFEType < 0)
        {
            // if type is negative, then marshal everything EXCEPT this type
            if (pParam->GetType()->IsOfType(-nFEType))
                continue;
        }
        else if (nFEType > 0)
        {
            // if type is positive, then marshal ONLY parameters with this type
            if (!pParam->GetType()->IsOfType(nFEType))
                continue;
        }
        // if type is 0, then marshal ALL parameters
        if ((m_bMarshal && (pFunction->DoMarshalParameter(pParam, pContext))) ||
            (!m_bMarshal && (pFunction->DoUnmarshalParameter(pParam, pContext))))
        {
            m_pParameter = pParam;
            nSize += Marshal(pFile, pParam, nStartOffset+nSize, bUseConstOffset, (pIter == 0), pContext);
        }
    }
    m_pFunction = 0;
    return nSize;
}

/** \brief marshals the given parameter
 *  \param pFile the file to marshal to
 *  \param pParameter the parameter to marshal
 *  \param nStartOffset the starting offset in the message buffer
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param bLastParameter true if this is the last parameter
 *  \param pContext the context of the marshalling operation
 *  \return the number of bytes this parameter uses for marshalling
 *
 * This function marshals a specific parameter. It iterates over the declarators of the
 * parameter and calls for each of them the MarshalDeclarator function. Before doing so,
 * the declarator is pushed on the declartor stack. This is done to be able to call the
 * MarshalDeclarator function recursively.
 */
int CBEMarshaller::Marshal(CBEFile *pFile, CBETypedDeclarator *pParameter, int nStartOffset, bool& bUseConstOffset, bool bLastParameter, CBEContext *pContext)
{
    m_pFile = pFile;
    if (!m_pFunction)
        m_pFunction = pParameter->GetFunction();

    int nSize = 0;
    // if m_pParameter is already set, then this is a recursive call;
    // if it is not set, than this is a plain call, and we have to set it now
    if (!m_pParameter)
        m_pParameter = pParameter;

    VectorElement *pIter = pParameter->GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = pParameter->GetNextDeclarator(pIter)) != 0)
    {
        m_vDeclaratorStack.Push(pDecl);
        nSize += MarshalDeclarator(pParameter->GetType(), nStartOffset+nSize, bUseConstOffset, true, bLastParameter && (pIter == 0), pContext);
        m_vDeclaratorStack.Pop();
    }

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
 * *(&lt;type&gt;*)(\&buffer[offset]) = [&lt;stars&gt;] [&lt;stack locations&gt;] &lt;name&gt; [&lt;index&gt;];
 * </code>
 */
int CBEMarshaller::MarshalDeclarator(CBEType *pType, int nStartOffset, bool& bUseConstOffset, bool bIncOffsetVariable, bool bLastParameter, CBEContext *pContext)
{
    if (pType->IsVoid())
	    return 0;

    ASSERT(m_pParameter);
    // get current decl
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    if (!pCurrent->HasIndex())
    {
        // is first -> test for array
        if (pCurrent->pDeclarator->IsArray())
        {
            return MarshalArray(pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
        }
        // a string is similar to an array, test for it
        if (m_pParameter->IsString())
        {
            return MarshalString(pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
        }
    }

    // test user defined types (usually they are defined in a C header file and thus
    // not in the IDL namespace)
    bool bUseUserMarshal = false;
    while ((pType->IsKindOf(RUNTIME_CLASS(CBEUserDefinedType))) && !bUseUserMarshal)
    {
        // search for original type and replace it
        CBERoot *pRoot = pType->GetRoot();
        ASSERT(pRoot);
        CBETypedef *pUserType = pRoot->FindTypedef(((CBEUserDefinedType *) pType)->GetName());
        // if 0: not defined in IDL files -> use user-provided init function
        if (pUserType)
        {
            if (pUserType->GetType())
                pType = pUserType->GetType();
        }
        else
            bUseUserMarshal = true;
        // if alias is array?
        CBEDeclarator *pAlias = 0;
        if (pUserType)
            pAlias = pUserType->GetAlias();
        // check if type has alias (it should, since the alias is
        // the name of the user defined type)
        if (pAlias && pAlias->IsArray())
        {
            // add array bounds of alias to temp vector
            Vector vBounds(RUNTIME_CLASS(CBEExpression));
            VectorElement *pI = pAlias->GetFirstArrayBound();
            CBEExpression *pBound;
            while ((pBound = pAlias->GetNextArrayBound(pI)) != 0)
            {
                vBounds.Add(pBound);
            }
            // add bounds to top declarator
            for (pI = vBounds.GetFirst(); pI; pI = pI->GetNext())
            {
                pCurrent->pDeclarator->AddArrayBound((CBEExpression*)pI->GetElement());
            }
            // call MarshalArray
            int nSize = MarshalArray(pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
            // remove those array decls again
            for (pI = vBounds.GetFirst(); pI; pI = pI->GetNext())
            {
                pCurrent->pDeclarator->RemoveArrayBound((CBEExpression*)pI->GetElement());
            }
            // return (work done)
            return nSize;
        }
    }

    // test for struct
    if (pType->IsKindOf(RUNTIME_CLASS(CBEStructType)))
    {
        return MarshalStruct((CBEStructType*)pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
    }

    // test for union
    if (pType->IsKindOf(RUNTIME_CLASS(CBEUnionType)))
    {
        // test for C style union
        if (!((CBEUnionType*)pType)->IsCStyleUnion())
        {
            return MarshalUnion((CBEUnionType*)pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
        }
    }

    // marshal
    if (m_bMarshal)
    {
        m_pFile->PrintIndent("");
        WriteBuffer(pType, nStartOffset, bUseConstOffset, true, pContext);
        m_pFile->Print(" = ");
        m_vDeclaratorStack.Write(m_pFile, m_pParameter->IsString(), false, pContext);
        m_pFile->Print(";\n");
    }
    else
    {
        m_pFile->PrintIndent("");
        m_vDeclaratorStack.Write(m_pFile, m_pParameter->IsString(), false, pContext);
        m_pFile->Print(" = ");
        WriteBuffer(pType, nStartOffset, bUseConstOffset, true, pContext);
        m_pFile->Print(";\n");
    }
    if (!bUseConstOffset && bIncOffsetVariable)
    {
        // add either the temporary offset, which was used for counting the members
        // or we add the size we just marshalled. Either way: we need the size of the
        // (base) type
        String sOffsetVar = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        //String sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        m_pFile->PrintIndent("%s += sizeof", (const char*)sOffsetVar);
        pType->WriteCast(m_pFile, false, pContext);
        m_pFile->Print(";\n");
    }

    return pType->GetSize();
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
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    CBEDeclarator *pDecl = pCurrent->pDeclarator;
    // if fixed size
    if ((pDecl->GetSize() >= 0) &&
        !(m_pParameter->FindAttribute(ATTR_SIZE_IS)) &&
        !(m_pParameter->FindAttribute(ATTR_LENGTH_IS)))
    {
        nSize = MarshalConstArray(pType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
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
 *  \param pContext the context of the marshalling operation
 *  \return the size of the array in the message buffer
 */
int CBEMarshaller::MarshalConstArray(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext)
{
    int nSize = 0;
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    CBEDeclarator *pDecl = pCurrent->pDeclarator;
    // for each dimension
    VectorElement *pIter = pDecl->GetFirstArrayBound();
    CBEExpression *pBound;
    while ((pBound = pDecl->GetNextArrayBound(pIter)) != 0)
    {
        // now loop for this bound
        for (pCurrent->nIndex = 0; pCurrent->nIndex < pBound->GetIntValue(); pCurrent->nIndex++)
            nSize += MarshalDeclarator(pType, nStartOffset+nSize, bUseConstOffset, true, bLastParameter, pContext);
    }
    // return size
    return nSize;
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
    // allocate memory for unmarshalling
    if (!m_bMarshal)
    {
        // var = (type*)malloc(max-size)
    }

    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    // marshal size
    // write iteration code
    // for (tmp = 0; tmp < size; tmp++)
    // {
    //     ...
    // }

    String sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
    // write for loop
    m_pFile->PrintIndent("for (%s = 0; %s < ", (const char*)sTmpVar, (const char*)sTmpVar);
    m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
    m_pFile->Print("; %s++)\n", (const char*)sTmpVar);
    m_pFile->PrintIndent("{\n");
    m_pFile->IncIndent();

    // set index to -2 (var size) => uses tmp offset as index
    pCurrent->SetIndex(sTmpVar);
    MarshalDeclarator(pType, nStartOffset, bUseConstOffset, false, bLastParameter, pContext);

    // close loop
    m_pFile->DecIndent();
    m_pFile->PrintIndent("}\n");

    // we increment the offset ourselves
    String sOffsetVar = pContext->GetNameFactory()->GetOffsetVariable(pContext);
    m_pFile->PrintIndent("%s ", (const char*)sOffsetVar);
    if (bUseConstOffset)
        m_pFile->Print(" = %d +", nStartOffset);
    else
        m_pFile->Print(" += ");
    m_pFile->Print("%s", (const char*)sTmpVar);
    if (pType->GetSize() > 1)
        m_pFile->Print(" * %d", pType->GetSize());
    m_pFile->Print(";\n");

    // we have to turn off the const offset (and thus turn on usage of offset variable)
    bUseConstOffset = false;
    // return arbitrary size, since it doesn't matter anymore    
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
        CBERoot *pRoot = pType->GetRoot();
        ASSERT(pRoot);
        CBEStructType *pTaggedType = (CBEStructType*)pRoot->FindTaggedType(TYPE_TAGGED_STRUCT, pType->GetTag());
        // if found, marshal this instead
        if ((pTaggedType) && (pTaggedType != pType))
            return MarshalStruct(pTaggedType, nStartOffset, bUseConstOffset, bLastParameter, pContext);
    }
    // check for bitfields
    VectorElement *pIter = pType->GetFirstMember();
    CBETypedDeclarator *pMember;
    bool bBitfields = false;
    while (((pMember = pType->GetNextMember(pIter)) != 0) && !bBitfields)
    {
		if (pMember->GetSize() == 0)
		    bBitfields = true;
	}
	if (bBitfields)
		return MarshalBitfieldStruct(pType, nStartOffset, bUseConstOffset, pContext);
	// marshal "normal" struct
    int nSize = 0;
    pIter = pType->GetFirstMember();
    CBETypedDeclarator *pOldParameter;
    while ((pMember = pType->GetNextMember(pIter)) != 0)
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
 */
int CBEMarshaller::MarshalUnion(CBEUnionType *pType, int nStartOffset, bool& bUseConstOffset, bool bLastParameter, CBEContext *pContext)
{
    int nSize = 0, nCaseSize = 0, nMaxCaseSize = 0;
    // marshal switch variable
    ASSERT(pType->GetSwitchVariable());
    nSize = Marshal(m_pFile, pType->GetSwitchVariable(), nStartOffset, bUseConstOffset, bLastParameter, pContext);

    // write switch statement
    String sSwitchVar = pType->GetSwitchVariableName();
    m_pFile->PrintIndent("switch(");
    m_vDeclaratorStack.Write(m_pFile, false, false, pContext);
    m_pFile->Print(".%s)\n", (const char*)sSwitchVar);
    m_pFile->PrintIndent("{\n");
    // we have to add the union declarator by hand
    if (pType->GetUnionName())
    {
        m_vDeclaratorStack.Push(pType->GetUnionName());
        m_vDeclaratorStack.GetTop()->SetIndex(-3);
    }
    // write the cases
    VectorElement *pIter = pType->GetFirstUnionCase();
    CBEUnionCase *pCase;
    CBETypedDeclarator *pOldParameter;
    // we have to make the usage of the const offset for every branch
    // as if it is the first. Therefor we use the value of bUseConstOffset
    // over and over in each loop. To set it correctly after the loop
    // we need the bChangedConstOffset variable.
    bool bUnionConstOffset, bChangedConstOffset = false;
    while ((pCase = pType->GetNextUnionCase(pIter)) != 0)
    {
        bUnionConstOffset = bUseConstOffset;
        if (pCase->IsDefault())
        {
            m_pFile->PrintIndent("default: \n");
        }
        else
        {
            VectorElement *pIterL = pCase->GetFirstLabel();
            CBEExpression *pLabel;
            while ((pLabel = pCase->GetNextLabel(pIterL)) != 0)
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
            String sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
            m_pFile->PrintIndent("%s = %d;\n", (const char*)sOffset, nCaseSize+nStartOffset+nSize);
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
        m_vDeclaratorStack.Pop();
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
 *  \param pContext the context of the write operation
 */
void CBEMarshaller::WriteBuffer(CBEType *pType, int nStartOffset, bool& bUseConstOffset, bool bDereferencePosition, CBEContext *pContext)
{
    // for the variable sized arrays we have to add the size var to the offset
    // get current
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    ASSERT(pCurrent);
    // get strings
    if (pCurrent->nIndex == -2)
        m_pFile->Print("(");
    else
    {
        if (bDereferencePosition)
            m_pFile->Print("*");
    }
    pType->WriteCast(m_pFile, true, pContext);
    // write the buffer
    CBEMsgBufferType *pMsgBuffer = m_pFunction->GetMessageBuffer();
    m_pFile->Print("(&(");
    pMsgBuffer->WriteMemberAccess(m_pFile, pType->GetFEType(), pContext);
    // write the offset in the buffer
    if (bUseConstOffset)
        m_pFile->Print("[%d]))", nStartOffset);
    else
    {
        String sOffsetVar = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        m_pFile->Print("[%s]))", (const char*)sOffsetVar);
    }
    if (pCurrent->nIndex == -2)
        m_pFile->Print(")[%s]", (const char*)pCurrent->sIndex);
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
    return Unmarshal(pFile, pFunction, 0, nStartOffset, bUseConstOffset, pContext);
}

/** \brief unmarshals a function
 *  \param pFile the file to write to
 *  \param pFunction the function to unmarshal
 *  \param nFEType the type of the parameters to be unmarshalled
 *  \param nStartOffset the start in the message buffer where to unmarshal
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param pContext the context of the marshalling
 *  \return the number of bytes unmarshalled
 *
 * The unmarshalling is basically the same as the marshalling, Only the last sequence, when writing the
 * unmarshalling line is different. This can be differentiated when setting a boolean variable.
 */
int CBEMarshaller::Unmarshal(CBEFile * pFile, CBEFunction * pFunction, int nFEType, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
    m_bMarshal = false;
    return Marshal(pFile, pFunction, nFEType, nStartOffset, bUseConstOffset, pContext);
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
        // we need a stack location, so WriteBuffer is working
        m_vDeclaratorStack.Push(0);
        m_pFile->PrintIndent("");
        WriteBuffer(pType, nStartOffset, bUseConstOffset, true, pContext);
        m_pFile->Print(" = %d;\n", nValue);
        m_vDeclaratorStack.Pop();
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
 * A string is simply marshalled by copying each of its bytes into the message buffer until a 0 (zero) is
 * encountered. Something like:
 *
 * <code>do { buffer[offset+tmp] = &lt,var&gt;[tmp]; } while(&lt;var&gt;[tmp++] != 0); offset += tmp;</code>
 *
 * If unmarshalling, we need to allocate memory for the string.
 */
int CBEMarshaller::MarshalString(CBEType *pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext *pContext)
{
    // marshal size of string
    String sTemp = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
    // if size attribute then attributes declarator is marshalled as well, we don't care about it here
    // if no size attribute and string attribute we marshal strlen size
    if (!(m_pParameter->HasSizeAttr(ATTR_SIZE_IS)) &&
        !(m_pParameter->HasSizeAttr(ATTR_LENGTH_IS)) &&
        !(m_pParameter->HasSizeAttr(ATTR_MAX_IS)) &&
        (m_pParameter->FindAttribute(ATTR_STRING)))
    {
        if (m_bMarshal)
        {
            m_pFile->PrintIndent("");
            WriteBuffer(String("CORBA_long"), nStartOffset, bUseConstOffset, true, pContext);
            m_pFile->Print(" = strlen(");
            m_vDeclaratorStack.Write(m_pFile, false, false, pContext);
            m_pFile->Print(");\n");
        }
        else
        {
            m_pFile->PrintIndent("%s = ", (const char*)sTemp);
            WriteBuffer(String("CORBA_long"), nStartOffset, bUseConstOffset, true, pContext);
            m_pFile->Print(";\n");
        }
        nStartOffset += pContext->GetSizes()->GetSizeOfType(TYPE_INTEGER, 4);
    }
    // allocate memory
	// we don't need to allocate memory if we init this parameter with the [in] value
    if (!m_bMarshal && !m_pParameter->FindAttribute(ATTR_INIT_WITH_IN))
    {
        if (pContext->IsWarningSet(PROGRAM_WARNING_PREALLOC))
        {
            CDeclaratorStackLocation *pCur = m_vDeclaratorStack.GetTop();
            if (m_pFunction)
                CCompiler::Warning("CORBA_alloc is used to allocate memory for %s in %s.",
                                   (const char*)pCur->pDeclarator->GetName(), (const char*)m_pFunction->GetName());
            else 
                CCompiler::Warning("CORBA_alloc is used to allocate memory for %s.", (const char*)pCur->pDeclarator->GetName());
        }
        // var = (type*)malloc(size)
        m_pFile->PrintIndent("");
        m_vDeclaratorStack.Write(m_pFile, true, false, pContext);
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
        if (m_pParameter->HasSizeAttr(ATTR_SIZE_IS) ||
            m_pParameter->HasSizeAttr(ATTR_LENGTH_IS) ||
            m_pParameter->HasSizeAttr(ATTR_MAX_IS))
            m_pParameter->WriteGetSize(m_pFile, NULL, pContext);
        else
            m_pFile->Print("%s", (const char*)sTemp);
        m_pFile->Print(");\n");
    }
    // marshal/unmarshal
    m_pFile->PrintIndent("for (%s=0; ", (const char*)sTemp);
    m_vDeclaratorStack.Write(m_pFile, true, false, pContext);
    m_pFile->Print("[%s] != 0; %s++)\n", (const char*)sTemp, (const char*)sTemp);
    m_pFile->PrintIndent("{\n");
    m_pFile->IncIndent();
    m_pFile->PrintIndent("");
    // set index to -2 (var size)
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    pCurrent->SetIndex(sTemp);
    MarshalDeclarator(pType, nStartOffset, bUseConstOffset, false, bLastParameter, pContext);
    m_pFile->DecIndent();
    m_pFile->PrintIndent("}\n");

    // increment or set offset variable
    String sOffsetVar = pContext->GetNameFactory()->GetOffsetVariable(pContext);
    m_pFile->PrintIndent("%s ", (const char*)sOffsetVar);
    if (bUseConstOffset)
        // used const offset until now
        m_pFile->Print("= %d +", nStartOffset);
    else
        // already use offset variable
        m_pFile->Print("+=");
    m_pFile->Print("%s", (const char*)sTemp);
    if (pType->GetSize() > 1)
        m_pFile->Print(" * %d", pType->GetSize());
    m_pFile->Print(";\n");

    // because we have to use the offset variable now, we have to turn off the const offset
    bUseConstOffset = false;
    // write terminating zero
    MarshalValue(1, 0, nStartOffset, bUseConstOffset, false, pContext);
    // return arbitrary size
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
        WriteBuffer(pType, nStartOffset, bUseConstOffset, true, pContext);
        m_pFile->Print(" = ");
        m_vDeclaratorStack.Write(m_pFile, false, false, pContext);
        m_pFile->Print(";\n");
    }
    else
    {
        m_pFile->PrintIndent("");
        m_vDeclaratorStack.Write(m_pFile, false, false, pContext);
        m_pFile->Print(" = ");
        WriteBuffer(pType, nStartOffset, bUseConstOffset, true, pContext);
        m_pFile->Print(";\n");
    }
    return pType->GetSize();
}

/** \brief print the part with the message buffer
 *  \param sTypeName the name of the type to cast the message buffer to
 *  \param nStartOffset which index in the message buffer to use
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param bDereferencePosition true if the message buffer position shall be dereferenced
 *  \param pContext the context of the write operation
 */
void CBEMarshaller::WriteBuffer(String sTypeName, int nStartOffset, bool & bUseConstOffset, bool bDereferencePosition, CBEContext * pContext)
{
    // for the variable sized arrays we have to add the size var to the offset
    // get current
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    ASSERT(pCurrent);
    // get strings
    if (pCurrent->nIndex == -2)
        m_pFile->Print("(");
    else
    {
        if (bDereferencePosition)
            m_pFile->Print("*");
    }
    m_pFile->Print("(%s*)", (const char*)sTypeName);
    // write the buffer
    CBEMsgBufferType *pMsgBuffer = m_pFunction->GetMessageBuffer();
    m_pFile->Print("(&(");
    pMsgBuffer->WriteMemberAccess(m_pFile, TYPE_INTEGER, pContext);
    // write the offset in the buffer
    if (bUseConstOffset)
        m_pFile->Print("[%d]))", nStartOffset);
    else
    {
        String sOffsetVar = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        m_pFile->Print("[%s]))", (const char*)sOffsetVar);
    }
    if (pCurrent->nIndex == -2)
        m_pFile->Print(")[%s]", (const char*)pCurrent->sIndex);
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
    m_pFile = pFile;
    // find parameter
    int nCurSize = 0, nParamSize;
    VectorElement *pIter = pFunction->GetFirstSortedParameter();
	CBETypedDeclarator *pParameter;
	while ((pParameter = pFunction->GetNextSortedParameter(pIter)) != 0)
	{
	    if (!pParameter->IsDirection(nDirection))
		    continue;
	    nParamSize = pParameter->GetSize();
		// if we cross the border of the param position we want
		// to write, we stop
		if ((nPosition-1)*nPosSize < nCurSize+nParamSize)
			break;
	    nCurSize += nParamSize;
	}
	// if no parameter is found, nothing will be written at the position
	if (!pParameter)
	    return false;
    // set function
	if (!m_pFunction)
        m_pFunction = pFunction;

    // if m_pParameter is already set, then this is a recursive call;
    // if it is not set, than this is a plain call, and we have to set it now
    if (!m_pParameter)
        m_pParameter = pParameter;

	return MarshalToPosition(pParameter, nPosition, nPosSize, nCurSize, bWrite, pContext);
}

bool CBEMarshaller::MarshalToPosition(CBETypedDeclarator *pParameter,  int nPosition,  int nPosSize,  int nCurSize,  bool bWrite,  CBEContext * pContext)
{
	bool bWrote = false;
	bool bOut = (pParameter->FindAttribute(ATTR_OUT) != 0);
	bool bConstructedType = pParameter->GetType()->IsConstructedType();
	int nTypeSize = pParameter->GetType()->GetSize();
    VectorElement *pIter = pParameter->GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = pParameter->GetNextDeclarator(pIter)) != 0)
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
            return bWrote;
		// if current decl does not fit, we skip
		if ((nPosition-1)*nPosSize >= nCurSize+(nTypeSize*nDeclSize))
			continue;
        m_vDeclaratorStack.Push(pDecl);
        if (!MarshalDeclaratorToPosition(pParameter->GetType(), nCurSize, nPosition, nPosSize, bWrite, pContext))
		    return bWrote;
        m_vDeclaratorStack.Pop();
		bWrote = true;
		// add type size here, because call to MarshalDeclaratorToPosition requires
		// position befor this declarator.
	    nCurSize += nTypeSize;
    }

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
    ASSERT(m_pParameter);
    // get current decl
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    if (!pCurrent->HasIndex())
    {
        // is first -> test for array
        if (pCurrent->pDeclarator->IsArray())
        {
		    return MarshalArrayToPosition(pType, nStartSize, nPosition, nPosSize, bWrite, pContext);
        }
        // a string is similar to an array, test for it
        if (m_pParameter->IsString())
        {
    		CCompiler::Error("cannot do this (string)"); // var sized array
        }
    }

    // test user defined types (usually they are defined in a C header file and thus
    // not in the IDL namespace)
    bool bUseUserMarshal = false;
    while ((pType->IsKindOf(RUNTIME_CLASS(CBEUserDefinedType))) && !bUseUserMarshal)
    {
        // search for original type and replace it
        CBERoot *pRoot = pType->GetRoot();
        ASSERT(pRoot);
        CBETypedef *pUserType = pRoot->FindTypedef(((CBEUserDefinedType *) pType)->GetName());
        // if 0: not defined in IDL files -> use user-provided init function
        if (pUserType)
        {
            if (pUserType->GetType())
                pType = pUserType->GetType();
        }
        else
            bUseUserMarshal = true;
        // if alias is array?
        CBEDeclarator *pAlias = 0;
        if (pUserType)
            pAlias = pUserType->GetAlias();
        // check if type has alias (it should, since the alias is
        // the name of the user defined type)
        if (pAlias && pAlias->IsArray())
        {
            // add array bounds of alias to temp vector
            Vector vBounds(RUNTIME_CLASS(CBEExpression));
            VectorElement *pI = pAlias->GetFirstArrayBound();
            CBEExpression *pBound;
            while ((pBound = pAlias->GetNextArrayBound(pI)) != 0)
            {
                vBounds.Add(pBound);
            }
            // add bounds to top declarator
            for (pI = vBounds.GetFirst(); pI; pI = pI->GetNext())
            {
                pCurrent->pDeclarator->AddArrayBound((CBEExpression*)pI->GetElement());
            }
            // call MarshalArray
            bool bRet = MarshalArrayToPosition(pType, nStartSize, nPosition, nPosSize, bWrite, pContext);
            // remove those array decls again
            for (pI = vBounds.GetFirst(); pI; pI = pI->GetNext())
            {
                pCurrent->pDeclarator->RemoveArrayBound((CBEExpression*)pI->GetElement());
            }
            // return (work done)
            return bRet;
        }
    }
    // test for struct
    if (pType->IsKindOf(RUNTIME_CLASS(CBEStructType)))
    {
        return MarshalStructToPosition((CBEStructType*)pType, nStartSize, nPosition, nPosSize, bWrite, pContext);
    }

    // test for union
    if (pType->IsKindOf(RUNTIME_CLASS(CBEUnionType)))
    {
        // test for not C style union
        if (!((CBEUnionType*)pType)->IsCStyleUnion())
        {
            return MarshalUnionToPosition((CBEUnionType*)pType, nStartSize, nPosition, nPosSize, bWrite, pContext);
        }
		else
		{
		    // get the biggest members
			VectorElement *pIter = ((CBEUnionType*)pType)->GetFirstUnionCase();
			CBEUnionCase *pUnionCase, *pBiggestUnionCase = 0;
			while ((pUnionCase = ((CBEUnionType*)pType)->GetNextUnionCase(pIter)) != 0)
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
			return MarshalToPosition(pBiggestUnionCase, nPosition, nPosSize, nStartSize, bWrite, pContext);
		}
			    
    }

	// get shift-bits
	int nShiftBits = nPosition*nPosSize-nStartSize-(pCurrent->pDeclarator->GetSize()*pType->GetSize());
	// cannot shift if we write to this file
	if ((nShiftBits > 0) && !bWrite)
	    m_pFile->Print("(");
    m_vDeclaratorStack.Write(m_pFile, m_pParameter->IsString(), false, pContext);
	if ((nShiftBits > 0) && !bWrite)
	    m_pFile->Print("<<%d)", nShiftBits);

    return true;
}

bool CBEMarshaller::MarshalArrayToPosition(CBEType *pType, int nStartSize, int nPosition, int nPosSize, bool bWrite, CBEContext *pContext)
{
    bool bRet;
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    CBEDeclarator *pDeclarator = pCurrent->pDeclarator;
    // if fixed size
    if ((pDeclarator->GetSize() >= 0) &&
        !(m_pParameter->FindAttribute(ATTR_SIZE_IS)) &&
        !(m_pParameter->FindAttribute(ATTR_LENGTH_IS)))
    {
		int nSize = nStartSize;
		int nTypeSize = pType->GetSize();
		bool bRet;
		CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
		CBEDeclarator *pDecl = pCurrent->pDeclarator;
		// for each dimension
		VectorElement *pIter = pDecl->GetFirstArrayBound();
		CBEExpression *pBound;
		while ((pBound = pDecl->GetNextArrayBound(pIter)) != 0)
		{
			// now loop for this bound
			for (pCurrent->nIndex = 0; pCurrent->nIndex < pBound->GetIntValue(); pCurrent->nIndex++)
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
		// return size
		return bRet;
    }
    else
    {
		// FIXME: cannot marshal this here, because we cannot determine the correct index (might be to big)
		CCompiler::Error("cannot do this");
    }
    return bRet;
}

bool CBEMarshaller::MarshalStructToPosition(CBEStructType *pType, int nStartSize, int nPosition, int nPosSize, bool bWrite, CBEContext *pContext)
{
    // if this is a tagged struct without members, we have to find the original struct
    if ((pType->GetMemberCount() == 0) && (pType->GetFEType() == TYPE_TAGGED_STRUCT))
    {
        // search for tag
        CBERoot *pRoot = pType->GetRoot();
        ASSERT(pRoot);
        CBEStructType *pTaggedType = (CBEStructType*)pRoot->FindTaggedType(TYPE_TAGGED_STRUCT, pType->GetTag());
        // if found, marshal this instead
        if ((pTaggedType) && (pTaggedType != pType))
		    pType = pTaggedType;
        else
		    return false;
    }
    // check for bitfields
    VectorElement *pIter = pType->GetFirstMember();
    CBETypedDeclarator *pMember;
    bool bBitfields = false;
    while (((pMember = pType->GetNextMember(pIter)) != 0) && !bBitfields)
    {
		if (pMember->GetSize() == 0)
		    bBitfields = true;
	}
	if (bBitfields)
	{
	    m_vDeclaratorStack.Write(m_pFile, false, false, pContext);
		return true;
	}
	// marshal "normal" struct
	bool bRet = false;
    int nSize = nStartSize, nParamSize, nTypeSize;
    pIter = pType->GetFirstMember();
    CBETypedDeclarator *pOldParameter;
    while (((pMember = pType->GetNextMember(pIter)) != 0) &&
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
		VectorElement *pI = pMember->GetFirstDeclarator();
		CBEDeclarator *pD;
		while (((pD = pMember->GetNextDeclarator(pI)) != 0) &&
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
		    m_vDeclaratorStack.Push(pD);
            if (!MarshalDeclaratorToPosition(pType, nSize, nPosition, nPosSize, bWrite, pContext))
			{
			    m_vDeclaratorStack.Pop();
			    return bRet;
			}
		    bRet = true;
			m_vDeclaratorStack.Pop();
			// increase size
		    nSize += nTypeSize*nDeclSize;
		}
        m_pParameter = pOldParameter;
	}
	// we should only stop here if there are no members,
	// return true, for we did write 'nothing'
    return bRet;
}

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
			m_vDeclaratorStack.Push(pType->GetUnionName());
			m_vDeclaratorStack.GetTop()->SetIndex(-3);
			m_vDeclaratorStack.Write(m_pFile, false, false, pContext);
			m_vDeclaratorStack.Pop();
		}
		// return size
		// we actually return a size value -> this makes Marshal ignore the
		// temp offset * size of type addition.
		return true;
	}
	else
	{
	    VectorElement *pIter = pSwitchVar->GetFirstDeclarator();
		CBEDeclarator *pDecl = pSwitchVar->GetNextDeclarator(pIter);
		CBEType *pType = pSwitchVar->GetType();
	    m_vDeclaratorStack.Push(pDecl);
		bool bRet = MarshalDeclaratorToPosition(pType, nStartSize, nPosition, nPosSize, bWrite, pContext);
		m_vDeclaratorStack.Pop();
		return bRet;
	}
    return false;
}
