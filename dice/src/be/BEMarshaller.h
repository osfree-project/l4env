/**
 *    \file    dice/src/be/BEMarshaller.h
 *  \brief   contains the declaration of the class CBEMarshaller
 *
 *    \date    11/18/2004
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

/** preprocessing symbol to check header file */
#ifndef __DICE_BE_BEMARSHALLER_H__
#define __DICE_BE_BEMARSHALLER_H__

#include <be/BEObject.h>
#include <vector>
using std::vector;

class CBETypedDeclarator;
class CBEFunction;
class CBEMsgBuffer;
class CBEType;
class CDeclaratorStackLocation;

/** \class CBEMarshaller
 *  \ingroup backend
 *  \brief contains the marshalling code
 *
 * Requirements:
 * # add local variables to function which are needed for marshalling
 * # marshal whole function
 * # write access to single words in the message buffer (or from function as \
 *   if marshalled)
 * # test if certain number of parameters fits into registers
 * # set certain members of message buffer (e.g. opcode, zero fpage, \
 *   exception) to zero
 * # marshal certain members of message buffer seperately (e.g. opcode, ...)
 */
class CBEMarshaller : public CBEObject
{
public:
    /** constructor */
    CBEMarshaller();
    virtual ~CBEMarshaller();

public:
    virtual void MarshalFunction(CBEFile *pFile, int nDirection);
    virtual void MarshalFunction(CBEFile *pFile, CBEFunction *pFunction, 
	int nDirection);
    virtual void MarshalParameter(CBEFile *pFile, CBEFunction *pFunction, 
	CBETypedDeclarator *pParameter, bool bMarshal);
    virtual void MarshalValue(CBEFile *pFile, CBEFunction *pFunction, 
	CBETypedDeclarator *pParameter, int nValue);

    virtual bool AddLocalVariable(CBEFunction *pFunction);

protected:
    CBEStructType* GetStruct(int& nDirection);
    CBEStructType* GetStruct(CBEFunction *pFunction, int& nDirection);
    CBEMsgBuffer* GetMessageBuffer(CBEFunction *pFunction);

    virtual void MarshalParameterIntern(CBETypedDeclarator *pParameter,
	vector<CDeclaratorStackLocation*> *pStack);
    virtual bool MarshalSpecialMember(CBETypedDeclarator *pMember);
    virtual bool MarshalOpcode(CBETypedDeclarator *pMember);
    virtual bool MarshalException(CBETypedDeclarator *pMember);
    virtual bool MarshalReturn(CBETypedDeclarator *pMember);
    virtual void MarshalGenericMember(int nPosition,
	CBETypedDeclarator *pParameter,
	vector<CDeclaratorStackLocation*> *pStack);
    virtual void MarshalGenericValue(int nPosition, int nValue);
    virtual bool MarshalString(CBETypedDeclarator *pParameter, 
	vector<CDeclaratorStackLocation*> *pStack);
    virtual bool MarshalArray(CBETypedDeclarator *pParameter, 
	vector<CDeclaratorStackLocation*> *pStack);
    virtual void MarshalArrayIntern(CBETypedDeclarator *pParameter, 
	CBEType *pType, vector<CDeclaratorStackLocation*> *pStack);
    virtual void MarshalArrayInternRef(CBETypedDeclarator *pParameter,
	vector<CDeclaratorStackLocation*> *pStack);
    virtual bool MarshalStruct(CBETypedDeclarator *pParameter, 
	vector<CDeclaratorStackLocation*> *pStack);
    virtual bool MarshalUnion(CBETypedDeclarator *pParameter,
	vector<CDeclaratorStackLocation*> *pStack);

    virtual bool DoSkipParameter(CBEFunction *pFunction, 
	CBETypedDeclarator *pParameter, int nDirection);
    virtual CBETypedDeclarator* FindMarshalMember(
	vector<CDeclaratorStackLocation*> *pSack);
    
    virtual void WriteMember(int nDir, CBEMsgBuffer *pMsgBuffer,
	CBETypedDeclarator *pMember, vector<CDeclaratorStackLocation*> *pStack);
    virtual void WriteParameter(CBETypedDeclarator *pParameter, 
	vector<CDeclaratorStackLocation*> *pStack, bool bPointer);
    virtual void WriteAssignment(CBETypedDeclarator *pParameter,
	vector<CDeclaratorStackLocation*> *pStack);

protected:
    /** \var bool m_bMarshal
     *  \brief true if marshaling, false if unmarshaling
     */
    bool m_bMarshal;
    /** \var CBEFile *m_pFile
     *  \brief reference to the file to write to
     */
    CBEFile *m_pFile;
    /** \var CBEFunction *m_pFunction
     *  \brief reference to the function the parameters belong to
     */
    CBEFunction *m_pFunction;
};

#endif
