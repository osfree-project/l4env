/**
 *  \file    dice/src/be/l4/L4BEMarshaller.h
 *  \brief   contains the declaration of the class CL4BEMarshaller
 *
 *  \date    01/26/2005
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
#ifndef L4BEMARSHALLER_H
#define L4BEMARSHALLER_H

#include "be/BEMarshaller.h"
#include "be/BEMsgBufferType.h"

/** \class CL4BEMarshaller
 *  \ingroup backend
 *  \brief contains the marshalling code
 *
 * Requirements:
 * # add local variables, which are needed for marshalling, to function
 * # marshal all parameters of a function
 * # write access to single words in the message buffer (or from function as \
 *   if marshalled)
 * # test if certain number of parameters fits into registers
 * # set certain members of message buffer (e.g. opcode, zero fpage, \
 *   exception) to zero
 * # marshal certain members of message buffer seperately (e.g. opcode, ...)
 */
class CL4BEMarshaller : public CBEMarshaller
{
public:
	/** constructor */
	CL4BEMarshaller();
	virtual ~CL4BEMarshaller();

	virtual void MarshalFunction(CBEFile& pFile, CBEFunction *pFunction,
		CMsgStructType nType);
	virtual bool MarshalWordMember(CBEFile& pFile, CBEFunction *pFunction,
		CMsgStructType nType, int nPosition, bool bReference, bool bLValue);
	virtual void MarshalParameter(CBEFile& pFile, CBEFunction *pFunction,
		CBETypedDeclarator *pParameter, bool bMarshal, int nPosition);

protected:
	virtual bool MarshalSpecialMember(CBEFile& pFile, CBETypedDeclarator *pMember);
	virtual bool MarshalRcvFpage(CBETypedDeclarator *pMember);
	virtual bool MarshalSendDope(CBETypedDeclarator *pMember);
	virtual bool MarshalSizeDope(CBETypedDeclarator *pMember);
	virtual bool MarshalZeroFlexpage(CBEFile& pFile, CBETypedDeclarator *pMember);

	virtual void MarshalParameterIntern(CBEFile& pFile, CBETypedDeclarator *pParameter,
		CDeclStack* pStack);
	virtual bool MarshalRefstring(CBEFile& pFile, CBETypedDeclarator *pParameter,
		CDeclStack* pStack);
	virtual void WriteMember(CBEFile& pFile, CMsgStructType nType, CBEMsgBuffer *pMsgBuffer,
		CBETypedDeclarator *pMember, CDeclStack* pStack);
	virtual void WriteRefstringCastMember(CBEFile& pFile, CMsgStructType nType, CBEMsgBuffer *pMsgBuffer,
		CBETypedDeclarator *pMember);

	virtual void MarshalArrayIntern(CBEFile& pFile, CBETypedDeclarator *pParameter,
		CBEType *pType, CDeclStack* pStack);

	virtual bool DoSkipParameter(CBEFunction *pFunction,
		CBETypedDeclarator *pParameter, CMsgStructType nType);

protected:
	/** \var int m_nSkipSize
	 *  \brief internal counter to know if the parameter should be skipped
	 */
	int m_nSkipSize;

private:
	/** \class PositionMarshaller
	 *  \ingroup backend
	 *  \brief class to marshal members to specific positions
	 *
	 * This class wraps the methods required to marshal a specific member of a
	 * specific position to a word sized location. The methods only write the
	 * access to the member. What is done to the member is not of concern to
	 * this class.
	 */
	class PositionMarshaller
	{
		/** default constructor */
		PositionMarshaller(CL4BEMarshaller *pParent);
		~PositionMarshaller();

	public:
		bool Marshal(CBEFile& pFile, CBEFunction *pFunction, CMsgStructType nType,
			int nPosition, bool bReference, bool bLValue);
	private:
		CBEMsgBufferType* GetMessageBufferType(CBEFunction *pFunction);
		CBETypedDeclarator* GetMemberAt(CBEMsgBufferType *pType,
			CBEStructType *pStruct, int nPosition);
		int GetMemberSize(CBETypedDeclarator *pMember);
		void WriteParameter(CBEFile& pFile, CBETypedDeclarator *pParameter,
			bool bReference, bool bLValue);
		void WriteSpecialMember(CBEFile& pFile, CBEFunction *pFunction,
			CBETypedDeclarator *pMember, CMsgStructType nType, bool bReference,
			bool bLValue);

	protected:
		/** \var CBEMarshaller *m_pParent
		 *  \brief reference to calling marshaller
		 */
		CL4BEMarshaller* m_pParent;
		/** \var int m_nPosSize
		 *  \brief the size of the position to marshal to
		 *
		 * This is the size of a word type, but to spare the permanent lookup
		 * of the size or the passing as parameter on the stack, we store it
		 * here.
		 */
		int m_nPosSize;
		/** \var bool m_bReference
		 *  \brief true if a reference to the member is needed
		 *
		 * Stored here to avoid passing as parameter (value does not change).
		 */
		bool m_bReference;

		friend class CL4BEMarshaller;
	};

	friend class CL4BEMarshaller::PositionMarshaller;
};

#endif
