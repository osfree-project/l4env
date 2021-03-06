/**
 *  \file    dice/src/be/l4/L4BEMsgBuffer.h
 *  \brief   contains the declaration of the class CL4BEMsgBuffer
 *
 *  \date    02/02/2005
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
#ifndef L4BEMSGBUFFER_H
#define L4BEMSGBUFFER_H

#include <be/BEMsgBuffer.h>
#include "L4BEMarshaller.h"

/** \class CL4BEMsgBuffer
 *  \ingroup backend
 *  \brief represents a message buffer variable
 */
class CL4BEMsgBuffer : public CBEMsgBuffer
{
public:
	/** constructor */
	CL4BEMsgBuffer();
	~CL4BEMsgBuffer();

	/** \brief anonymous enum to contain definitions for CheckProperty
	 */
	enum
	{
		MSGBUF_PROP_SHORT_IPC = 1,	/**< check if short IPC can be used */
		MSGBUF_PROP_UTCB_IPC  = 2,	/**< check if UTCB IPC can be used */
		MSGBUF_L4_MAX,				/**< maximum value */
	};

protected:
	/** \brief copy constructor
	 *  \param src the source to copy from
	 */
	CL4BEMsgBuffer(CL4BEMsgBuffer* src);

public: // public methods
	virtual void WriteInitialization(CBEFile& pFile, CBEFunction *pFunction,
		int nType, CMsgStructType nStructType);
	virtual CL4BEMsgBuffer* Clone();
	virtual bool HasWordMembers(CBEFunction *pFunction, CMsgStructType nType);
	virtual bool HasProperty(int nProperty, CMsgStructType nType);

protected: // protected methods
	virtual CBETypedDeclarator* GetFlexpageVariable();
	virtual CBETypedDeclarator* GetSizeDopeVariable();
	virtual CBETypedDeclarator* GetSendDopeVariable();

	virtual bool WriteRefstringInitialization(CBEFile& pFile, CMsgStructType nType);
	virtual bool WriteRefstringInitFunction(CBEFile& pFile,
		CBEFunction *pFunction, CBEClass *pClass, int nIndex, CMsgStructType nType);
	virtual void WriteRcvFlexpageInitialization(CBEFile& pFile, CMsgStructType nType);
	virtual void WriteRefstringInitParameter(CBEFile& pFile,
		CBEFunction *pFunction, CBETypedDeclarator *pMember, int nIndex,
		CMsgStructType nType);
	virtual void WriteMaxRefstringSize(CBEFile& pFile, CBEFunction *pFunction,
		CBETypedDeclarator *pMember, CBETypedDeclarator *pParameter,
		int nIndex);

	virtual void Sort(CBEStructType *pStruct);
	virtual bool DoExchangeMembers(CBETypedDeclarator *pFirst, CBETypedDeclarator *pSecond);

	virtual void Pad();
	virtual void PadRefstringToPosition(int nPosition);
	virtual void PadRefstringToPosition(CBEStructType *pStruct, int nPosition);
	virtual int GetMaxPosOfRefstringInMsgBuffer();
	virtual bool InsertPadMember(int nFEType, int nSize,
		CBETypedDeclarator *pMember, CBEStructType *pStruct);

	virtual int GetWordMemberCountFunction();
	virtual int GetWordMemberCountClass();
	CBETypedDeclarator* GetRefstringMemberVariable(int nNumber);

	using CBEMsgBuffer::GetMemberSize;
	virtual int GetMemberSize(int nType, CBETypedDeclarator *pMember, bool bMax);

	virtual void AddGenericStructMembersClass(CBEStructType *pStruct);

	virtual void CheckConvertStruct(CBEStructType *pStruct, CBEFunction *pFunction);
	virtual CBETypedDeclarator* CheckConvertMember(CBEStructType *pStruct,
		vector<CBETypedDeclarator*>::iterator iter);
	virtual void ConvertMember(CBETypedDeclarator* pMember, CBEFunction *pFunction);
};

#endif

