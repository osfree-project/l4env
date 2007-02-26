/**
 *  \file    dice/src/be/BEMsgBufferType.h
 *  \brief   contains the declaration of the class CBEMsgBufferType
 *
 *  \date    11/10/2004
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef __DICE_BE_BEMSGBUFFERTYPE_H__
#define __DICE_BE_BEMSGBUFFERTYPE_H__

#include <be/BEUnionType.h>

class CFEOperation;
class CFEInterface;
class CFETypedDeclarator;

/** \class CBEMsgBufferType
 *  \ingroup backend
 *  \brief encapsulates the message buffer type
 */
class CBEMsgBufferType : public CBEUnionType
{
public:
    /** constructor */
    CBEMsgBufferType();
    /** destroys instance of this class */
    ~CBEMsgBufferType()
    { }

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CBEMsgBufferType(CBEMsgBufferType & src);

public: // public methods
    /** \brief creates a new instance of this class
     *  \return a reference to the copy of this instance
     */
    virtual CObject* Clone()
    { return new CBEMsgBufferType(*this); }

    virtual void CreateBackEnd(CFEOperation *pFEOperation);
    virtual void CreateBackEnd(CFEInterface *pFEInterface);
    CBEStructType* GetStruct(string sFuncName, string sClassName, 
	int nDirection);
    vector<CBETypedDeclarator*>::iterator 
	GetStartOfPayload(CBEStructType* pStruct);

    virtual bool AddGenericStruct(CFEBase *pFERefObj);

protected:
    void AddStruct(CFEOperation *pFEOperation, int nDirection);
    void AddStruct(CFEInterface *pFEInterface);
    void AddStruct(CBEStructType *pStruct, int nDirection, string sFunctionName,
	string sClassName);
    virtual void AddElements(CFEOperation *pFEOperation, int nDirection);
    virtual void AddElement(CFETypedDeclarator *pFEParameter, int nDirection);
    virtual void AddElement(CBEStructType *pStruct, CBETypedDeclarator *pParameter);
    virtual void FlattenElement(CBETypedDeclarator *pParameter, 
	CBEStructType *pStruct);
    virtual void FlattenConstructedElement(CBETypedDeclarator *pParameter,
	vector<CDeclaratorStackLocation*> *pStack, CBEStructType *pStruct);
    void CheckElementForString(CBETypedDeclarator *pParameter,
	CBEFunction *pFunction, CBEStructType *pStruct, 
	vector<CDeclaratorStackLocation*> *pStack);
    void CheckConstructedElementForVariableSize(CBETypedDeclarator *pParameter,
	CBEFunction *pFunction, CBEStructType *pStruct, 
	vector<CDeclaratorStackLocation*> *pStack);
    string CreateInitStringForString(CBEFunction *pFunction,
	vector<CDeclaratorStackLocation*> *pStack);

    friend class CBEMsgBuffer;
};

#endif
