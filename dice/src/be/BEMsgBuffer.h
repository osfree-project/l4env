/**
 *    \file    dice/src/be/BEMsgBuffer.h
 *    \brief   contains the declaration of the class CBEMsgBuffer
 *
 *    \date    11/09/2004
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef BEMSGBUFFER_H
#define BEMSGBUFFER_H

#include <be/BETypedef.h>
#include "BEMsgBufferType.h"

class CFEOperation;
class CFEInterface;
class CBEClass;

/** \class CBEMsgBuffer
 *  \ingroup backend
 *  \brief represents a message buffer variable
 */
class CBEMsgBuffer : public CBETypedef
{
public:
    /** constructor */
    CBEMsgBuffer();
    ~CBEMsgBuffer();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CBEMsgBuffer(CBEMsgBuffer & src);

public: // public methods
    virtual CObject* Clone();
    virtual bool IsVariableSized(CMsgStructType nType);
    virtual int GetCount(int nFEType, CMsgStructType nType);
    virtual int GetCountAll(int nFEType, CMsgStructType nType);
    virtual int GetPayloadOffset();

    using CBETypedef::CreateBackEnd;
    virtual void CreateBackEnd(CFEOperation *pFEOperation);
    virtual void CreateBackEnd(CFEInterface *pFEInterface);
    virtual void PostCreate(CBEClass *pClass, CFEInterface *pFEInterface);
    virtual void PostCreate(CBEFunction *pFunction, CFEOperation *pFEOperation);

    virtual bool AddReturnVariable(CBEFunction *pFunction,
	CBETypedDeclarator *pReturn = 0);
    virtual bool AddPlatformSpecificMembers(CBEFunction *pFunction);
    virtual bool AddPlatformSpecificMembers(CBEClass *pClass);

    virtual bool Sort(CBEClass *pClass);
    virtual bool Sort(CBEFunction *pFunction);
    virtual bool Sort(CBEStructType *pStruct);

    virtual void WriteAccess(CBEFile& pFile, CBEFunction *pFunction,
	CMsgStructType nType, CDeclStack* pStack);
    virtual void WriteMemberAccess(CBEFile& pFile, CBEFunction *pFunction,
	CMsgStructType nType, int nFEType, int nIndex);
    virtual void WriteMemberAccess(CBEFile& pFile, CBEFunction *pFunction,
	CMsgStructType nType, int nFEType, std::string sIndex);
    virtual void WriteGenericMemberAccess(CBEFile& pFile, int nIndex);
    void WriteAccessToStruct(CBEFile& pFile, CBEFunction *pFunction,
	CMsgStructType nType);
    CBETypedDeclarator* WriteAccessToVariable(CBEFile& pFile,
	CBEFunction *pFunction, bool bPointer);
    virtual void WriteInitialization(CBEFile& pFile, CBEFunction *pFunction,
	int nType, CMsgStructType nStructType);
    virtual void WriteDump(CBEFile& pFile);

    virtual bool HasProperty(int nProperty, CMsgStructType nType);

    virtual CBETypedDeclarator* FindMember(std::string sName, CMsgStructType nType);
    virtual CBETypedDeclarator* FindMember(std::string sName,
	CBEFunction *pFunction, CMsgStructType nType);
    virtual int GetMemberPosition(std::string sName, CMsgStructType nType);

    virtual int GetMemberSize(int nType, CBEFunction *pFunction,
	CMsgStructType nStructType, bool bMax);
    virtual int GetMemberSize(int nType);

    virtual bool IsEarlier(CBEFunction *pFunction, CMsgStructType nType,
	std::string sName1, std::string sName2);

protected: // protected methods
    virtual CBEType* CreateType(CFEOperation *pFEOperation);
    virtual CBEType* CreateType(CFEInterface *pFEInterface);

    virtual bool AddPlatformSpecificMembers(CBEFunction *pFunction,
	CBEStructType *pStruct, CMsgStructType nType);
    virtual bool AddOpcodeMember(CBEFunction *pFunction,
	CBEStructType *pStruct, CMsgStructType nType);
    virtual bool AddExceptionMember(CBEFunction *pFunction,
	CBEStructType *pStruct, CMsgStructType nType);
    virtual bool AddGenericStruct(CBEFunction *pFunction,
	CFEOperation *pFEOperation);
    virtual bool AddGenericStruct(CBEClass *pClass, CFEInterface *pFEInterface);
    virtual bool AddGenericStructMembersFunction(CBEStructType *pStruct);
    virtual bool AddGenericStructMembersClass(CBEStructType *pStruct);

    virtual int GetWordMemberCountFunction(void);
    virtual int GetWordMemberCountClass(void);

    virtual int GetMemberSize(int nType, CBETypedDeclarator *pMember,
	bool bMax);

    CBETypedDeclarator* GetOpcodeVariable(void);
    CBETypedDeclarator* GetExceptionVariable(void);
    CBETypedDeclarator* GetReturnVariable(CBEFunction *pFunction);
    CBETypedDeclarator* GetWordMemberVariable(int nNumber);
    CBETypedDeclarator* GetMemberVariable(int nFEType, bool bUnsigned,
	std::string sName, int nArray);

    CBEStructType* GetStruct(CMsgStructType nType);
    CBEStructType* GetStruct(CBEFunction *pFunction, CMsgStructType nType);

    virtual bool Pad();
    virtual bool SortPayload(CBEStructType *pStruct);
    virtual bool DoExchangeMembers(CBETypedDeclarator *pFirst,
	    CBETypedDeclarator *pSecond);

    void WriteAccess(CBEFile& pFile, CBEFunction *pFunction, CMsgStructType nType,
	CBETypedDeclarator *pMember);

    CBEFunction* GetAnyFunctionFromClass(CBEClass *pClass);

    friend class CBEMsgBufferType; // accesses GetStruct
};

#endif
