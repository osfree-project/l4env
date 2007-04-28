/**
 *    \file    dice/src/be/l4/v4/L4V4BEMarshaller.cpp
 *    \brief   contains the implementation of the class CL4V4BEMarshaller
 *
 *    \date    06/01/2006
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006-2007
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
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEFunction.h"
#include "be/BEType.h"
#include "be/BEStructType.h"
#include "be/BEMsgBuffer.h"
#include "be/BEClassFactory.h"
#include "be/l4/L4BENameFactory.h"
#include "Compiler.h"
#include "Messages.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"
#include "be/BEFile.h"
#include <sstream>
using std::ostringstream;
#include <cassert>

CL4V4BEMarshaller::CL4V4BEMarshaller()
 : CL4BEMarshaller()
{
}

/** destroys the object */
CL4V4BEMarshaller::~CL4V4BEMarshaller()
{
}

/** \brief tests if this parameter should be marshalled
 *  \param pFunction the function the parameter belongs to
 *  \param pParameter the parameter (!) to be tested
 *  \param nDirection the direction of the marshalling
 *  \return true if this parameter should be skipped
 *
 * Do not skip any parameters for V4 (all parameters have to be stuffed into
 * the message buffer).
 */
bool
CL4V4BEMarshaller::DoSkipParameter(CBEFunction *pFunction,
    CBETypedDeclarator *pParameter,
    DIRECTION_TYPE nDirection)
{
    return CBEMarshaller::DoSkipParameter(pFunction, pParameter, nDirection);
}

/** \brief marshal an indirect string parameter
 *  \param pParameter the member to test for refstring marshalling
 *  \param pStack the declarator stack so far
 *  \return true if we marshalled a refstring
 *
 * A refstring parameter can be identified by its ATTR_REF attribute. A
 * pointer to this parameter has then to be assigned to the snd_str element of
 * the indirect part member. The size has to be assigned to the snd_size
 * element (for sending). For receiving the address of the receive buffer can
 * be used to set the address of the receive buffer. 
 *
 * We have to use the rcv_str member to set incoming strings, because snd_str
 * is not se properly.
 */
bool
CL4V4BEMarshaller::MarshalRefstring(CBETypedDeclarator *pParameter,
    CDeclStack* pStack)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	"CL4BEMarshaller::%s called for %s\n", __func__, 
	pParameter->m_Declarators.First()->GetName().c_str());

    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer(m_pFunction);
    CBETypedDeclarator *pMember = FindMarshalMember(pStack);
    if (!pMember)
    {
	CMessages::Warning("%s: could not find member for parameter %s\n",
	    __func__, pParameter->m_Declarators.First()->GetName().c_str());
    }
    assert(pMember);
    // check the member for the ref attribute, because we might have made some
    // parameters to ref's after adding them to the message buffer
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"CL4BEMarshaller::%s member with%s [ref] attribute\n", __func__,
	pMember->m_Attributes.Find(ATTR_REF) ? "" : "out");
    if (!pMember->m_Attributes.Find(ATTR_REF))
	return false;
    CBEType *pType = pParameter->GetType();
    // try to find respective member and assign
    if (m_bMarshal)
    {
	*m_pFile << "\t";
	WriteMember(m_pFunction->GetSendDirection(), pMsgBuffer, pMember,
	    pStack);
	// write access to snd_str part of indirect string
	*m_pFile << " = L4_StringItem ( (";
	// size with max constraint
	pParameter->WriteGetSize(m_pFile, pStack, m_pFunction);
	if (!pParameter->m_Attributes.Find(ATTR_LENGTH_IS) &&
	    !pParameter->m_Attributes.Find(ATTR_SIZE_IS) &&
	    pParameter->m_Attributes.Find(ATTR_STRING))
	    *m_pFile << "+1"; // tranmist terminating zero
	*m_pFile << " > ";
	pParameter->WriteGetMaxSize(m_pFile, pStack, m_pFunction);
	*m_pFile << ") ? ";
	pParameter->WriteGetMaxSize(m_pFile, pStack, m_pFunction);
	*m_pFile << " : ";
	pParameter->WriteGetSize(m_pFile, pStack, m_pFunction);
	if (!pParameter->m_Attributes.Find(ATTR_LENGTH_IS) &&
	    !pParameter->m_Attributes.Find(ATTR_SIZE_IS) &&
	    pParameter->m_Attributes.Find(ATTR_STRING))
	    *m_pFile << "+1"; // tranmist terminating zero
	*m_pFile << ", (void *)(";
	// if type of member and parameter are different, cast to member type
	WriteParameter(pParameter, pStack, true);
	*m_pFile << ") );\n";
    }
    else if (
	!(pParameter->m_Attributes.Find(ATTR_PREALLOC_CLIENT) &&
	    m_pFile->IsOfFileType(FILETYPE_CLIENT)) &&
	!(pParameter->m_Attributes.Find(ATTR_PREALLOC_SERVER) &&
	    m_pFile->IsOfFileType(FILETYPE_COMPONENT)) )
    {
	// only unmarshal refstrings if not preallocated, because preallocated
	// refstrings are already assigned to rcv_str.
	// also: check if parameter is return parameter, because we cannot
	// make a pointer from it. Instead, dereference the refstring.
	bool bReturn = pParameter == m_pFunction->GetReturnVariable();
	*m_pFile << "\t";
	WriteParameter(pParameter, pStack, !bReturn);
	*m_pFile << " = ";
	if (bReturn)
	    *m_pFile << "*";
	// cast to type of parameter
	pType->WriteCast(m_pFile, true);
	// access message buffer
	WriteMember(m_pFunction->GetReceiveDirection(), pMsgBuffer, pMember, 
	    pStack);
	// append receive member
	*m_pFile << ".X.str.string_ptr;\n";

	// We do unmarshal the size parameter, because the actually
	// transmitted size might be smaller than the size of the receive
	// buffer we provided.
	// But we only do this, if the size parameter is a parameter of the
	// function.
    }

    return true;
}

/** \brief writes the access to a refstring member in the message buffer
 *  \param nDirection the direction of the parameter
 *  \param pMsgBuffer the message buffer containing the members
 *  \param pMember the member to access
 *
 * For derived interfaces the offset into the message buffer where indirect
 * strings may start can vary greatly. For marshalling we cannot directly use
 * the indirect part member, but have to use an offset into the word buffer
 * (size dope's word value) and cast that location to a stringdope. (This only
 * applies when marshalling at server side.)
 *
 * To allow multiple indirect strings, we first have to know at which position
 * in the indirect string list the current member is.
 */
void
CL4V4BEMarshaller::WriteRefstringCastMember(DIRECTION_TYPE nDirection,
    CBEMsgBuffer *pMsgBuffer,
    CBETypedDeclarator *pMember)
{
    assert(pMember);
    assert(pMsgBuffer);

    // get index in refstring field
    CMsgStructType nType = nDirection;
    CBEStructType *pStruct = GetStruct(m_pFunction, nType);
    assert(pStruct);
    // iterate members of struct, when member of struct matches pMember, then
    // stop counting, otherwise: if member of struct is of type refstring
    // increment counter
    int nIndex = -1;
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = pStruct->m_Members.begin();
	 iter != pStruct->m_Members.end();
	 iter++)
    {
	/* increment first, so nIndex is at zero if we have only one member */
	if ((*iter)->m_Attributes.Find(ATTR_REF))
	    nIndex++;
	if ((*iter) == pMember)
	    break;
    }
    assert (nIndex >= 0);

    // dereference pointer to L4_StringItem
    *m_pFile << "(*";

    // write type cast for restring
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBEType *pType = pCF->GetNewType(TYPE_REFSTRING);
    pType->CreateBackEnd(true, 0, TYPE_REFSTRING);
    pType->WriteCast(m_pFile, true);
    delete pType;

    // access to the strings is done using the generic struct, the word member
    // using the size-dope's word count plus the index from above as index
    // into the word member and casting the result to a string dope.
    //
    // ((L4_Msg_t*)<msgbuf>)->msg[((L4_Msg_t*)<msgbuf>)->tag.X.u + nIndex]
    *m_pFile << "&( ((L4_Msg_t*)";
    pMsgBuffer->WriteAccessToVariable(m_pFile, m_pFunction, true);
    *m_pFile << ")->msg[((L4_Msg_t*)";
    pMsgBuffer->WriteAccessToVariable(m_pFile, m_pFunction, true);
    *m_pFile << ")->tag.X.u";
    ostringstream os;
    os << nIndex;
    if (nIndex > 0)
	*m_pFile << " + " << os.str();
    *m_pFile << "] ))";
}

/** \brief test if zero flexpage and marshal if so
 *  \param pMember the parameter to marshal
 *  \return true if zero flexpage marshalled
 */
bool
CL4V4BEMarshaller::MarshalZeroFlexpage(CBETypedDeclarator *pMember)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sName = pNF->GetString(CL4BENameFactory::STR_ZERO_FPAGE);
    if (!pMember->m_Declarators.Find(sName))
	return false;

    // get message buffer
    CBEMsgBuffer *pMsgBuffer = pMember->GetSpecificParent<CBEMsgBuffer>();
    assert(pMsgBuffer);
    
    if (m_bMarshal)
    {
	// zero raw member
	*m_pFile << "\t";
	WriteMember(m_pFunction->GetSendDirection(), pMsgBuffer, pMember, NULL);
	*m_pFile << ".raw = 0;\n";
    }

    return true;
}

