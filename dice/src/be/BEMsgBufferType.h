/**
 *  \file    dice/src/be/BEMsgBufferType.h
 *  \brief   contains the declaration of the class CBEMsgBufferType
 *
 *  \date    11/10/2004
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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
#include "BEFunction.h"

class CFEOperation;
class CFEInterface;
class CFETypedDeclarator;

/** \class CMsgStructType
 *  \ingroup backend
 *  \brief helper class to determine struct in msg-buf type
 */
class CMsgStructType
{
public:
    /** \enum Type
     *  \brief contains the valid message buffer struct types
     */
    enum Type { Generic, In, Out, Exc };

    /** \brief constructor
     *  \param type a message buffer struct type initializer
     */
    CMsgStructType(Type type)
    {
	nType = type;
    }

    /** \brief constructor
     *  \param nDir a DIRECTION_TYPE initializer
     */
    CMsgStructType(DIRECTION_TYPE nDir)
    {
	switch (nDir) {
	case 0:
	case DIRECTION_INOUT:
	    nType = Generic;
	    break;
	case DIRECTION_IN:
	    nType = In;
	    break;
	case DIRECTION_OUT:
	    nType = Out;
	    break;
	default:
	    nType = Exc;
	}
    }

    /** \brief cast operator
     *  \return an int value
     */
    operator int() const
    {
	return nType;
    }

    /** \brief cast operator
     *  \return a DIRECTION_TYPE value
     */
    operator DIRECTION_TYPE() const
    {
	switch (nType) {
	case In:
	    return DIRECTION_IN;
	case Out:
	    return DIRECTION_OUT;
	default:
	    break;
	}
	return DIRECTION_NONE;
    }

    friend bool operator== (const CMsgStructType&, const CMsgStructType&);
    friend bool operator== (const CMsgStructType::Type&, const CMsgStructType&);
    friend bool operator== (const CMsgStructType&, const CMsgStructType::Type&);
    friend bool operator!= (const CMsgStructType&, const CMsgStructType&);
    friend bool operator!= (const CMsgStructType::Type&, const CMsgStructType&);
    friend bool operator!= (const CMsgStructType&, const CMsgStructType::Type&);
private:
    /** \var Type nType
     *  \brief the message buffer struct type
     */
    Type nType;
};

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
    ~CBEMsgBufferType();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CBEMsgBufferType(CBEMsgBufferType & src);

public: // public methods
    virtual CObject* Clone();
    virtual void CreateBackEnd(CFEOperation *pFEOperation);
    virtual void CreateBackEnd(CFEInterface *pFEInterface);
    CBEStructType* GetStruct(string sFuncName, string sClassName, 
	CMsgStructType nType);
    vector<CBETypedDeclarator*>::iterator 
	GetStartOfPayload(CBEStructType* pStruct);

    virtual bool AddGenericStruct(CFEBase *pFERefObj);

protected:
    void AddStruct(CFEOperation *pFEOperation, CMsgStructType nType);
    void AddStruct(CFEInterface *pFEInterface);
    void AddStruct(CBEStructType *pStruct, CMsgStructType nType, string sFunctionName,
	string sClassName);
    virtual void AddElements(CFEOperation *pFEOperation, CMsgStructType nType);
    virtual void AddElement(CFETypedDeclarator *pFEParameter, CMsgStructType nType);
    virtual void AddElement(CBEStructType *pStruct, CBETypedDeclarator *pParameter);
    virtual void FlattenElement(CBETypedDeclarator *pParameter, CBEStructType *pStruct);
    virtual void FlattenConstructedElement(CBETypedDeclarator *pParameter,
	CDeclStack* pStack, CBEStructType *pStruct);
    void CheckElementForString(CBETypedDeclarator *pParameter,
	CBEFunction *pFunction, CBEStructType *pStruct, CDeclStack* pStack);
    void CheckConstructedElementForVariableSize(CBETypedDeclarator *pParameter,
	CBEFunction *pFunction, CBEStructType *pStruct, CDeclStack* pStack);
    string CreateInitStringForString(CBEFunction *pFunction, CDeclStack* pStack);

    friend class CBEMsgBuffer;

private:
    bool CreateInitStringForStringIDLUnion(CBETypedDeclarator*& pParameter, string& sUnionStrPre,
	string& sUnionStrSuf, CDeclStack::iterator& iter, CDeclStack& vStack);
};

#endif
