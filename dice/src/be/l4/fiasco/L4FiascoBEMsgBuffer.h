/**
 *  \file    dice/src/be/l4/fiasco/L4FiascoBEMsgBuffer.h
 *  \brief   contains the declaration of the class CL4FiascoBEMsgBuffer
 *
 *  \date    08/24/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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
#ifndef L4FIASCOBEMSGBUFFER_H
#define L4VFIASCOEMSGBUFFER_H

#include <be/l4/L4BEMsgBuffer.h>

/** \class CL4FiascoBEMsgBuffer
 *  \ingroup backend
 *  \brief represents a message buffer variable
 */
class CL4FiascoBEMsgBuffer : public CL4BEMsgBuffer
{
public:
	/** constructor */
	CL4FiascoBEMsgBuffer();
	~CL4FiascoBEMsgBuffer();

protected:
	/** \brief copy constructor
	 *  \param src the source to copy from
	 */
	CL4FiascoBEMsgBuffer(CL4FiascoBEMsgBuffer* src);

public: // public methods
	virtual int GetPayloadOffset();
	virtual CL4FiascoBEMsgBuffer* Clone();
	virtual void WriteDopeShortInitialization(CBEFile& pFile, CBEFunction *pFunction,
		int nType, CMsgStructType nStructType);
	virtual int GetMemberPosition(std::string sName, CMsgStructType nType);
	virtual bool HasProperty(int nProperty, CMsgStructType nType);

protected: // protected methods
	virtual void PostCreate(CBEFunction *pFunction, CFEOperation *pFEOperation);
	virtual void WriteRefstringInitParameter(CBEFile& pFile,
		CBEFunction *pFunction, CBETypedDeclarator *pMember, int nIndex,
		CMsgStructType nType);
	virtual bool WriteRefstringInitFunction(CBEFile& pFile,
		CBEFunction *pFunction,	CBEClass *pClass, int nIndex, CMsgStructType nType);
	virtual void WriteRcvFlexpageInitialization(CBEFile& pFile,	CMsgStructType nType);

	CBETypedDeclarator* GetRefstringMemberVariable(int nNumber);

	virtual void AddPlatformSpecificMembers(CBEFunction *pFunction,	CBEStructType *pStruct);
	virtual void AddGenericStruct(CBEFunction *pFunction, CFEOperation *pFEOperation);

	bool IsUtcb();

protected:
	/** \var bool m_bIsUtcb
	 *  \brief cached value if this message buffer fits into UTCB IPC
	 */
	bool m_bIsUtcb;
};

#endif

