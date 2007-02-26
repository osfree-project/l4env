/**
 *  \file     dice/src/be/l4/v4/L4V4BEMsgBuffer.h
 *  \brief    contains the declaration of the class CL4V4BEMsgBuffer
 *
 *  \date     Tue Jun 15 2004
 *  \author   Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004 by
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
#ifndef L4V4MSGBUFFER_H
#define L4V4MSGBUFFER_H

#include <be/l4/L4BEMsgBuffer.h>

/** \class CL4V4BEMsgBuffer
 *  \ingroup backend
 *  \brief is the V4 specific message buffer
 */
class CL4V4BEMsgBuffer : public CL4BEMsgBuffer
{
public:
    /** creates an object of this class */
    CL4V4BEMsgBuffer();
    virtual ~CL4V4BEMsgBuffer();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CL4V4BEMsgBuffer(CL4V4BEMsgBuffer &src);

public:
    /** \brief creates a copy of this instance
     *  \return a reference to the copy
     */
    virtual CObject* Clone()
    { return new CL4V4BEMsgBuffer(*this); }
    virtual int GetPayloadOffset();
    virtual bool Sort(CBEStructType *pStruct);

    virtual void PostCreate(CBEClass *pClass, CFEInterface *pFEInterface);
    virtual void PostCreate(CBEFunction *pFunction, CFEOperation *pFEOperation);

    virtual void WriteInitialization(CBEFile *pFile, CBEFunction *pFunction,
	int nType, int nDirection);

protected:
    virtual bool AddPlatformSpecificMembers(CBEFunction *pFunction,
	CBEStructType *pStruct, int nDirection);
    virtual void WriteRcvFlexpageInitialization(CBEFile *pFile,	
	int nDirection);
    virtual bool WriteRefstringInitialization(CBEFile *pFile, int nDirection);
    virtual void WriteRefstringInitParameter(CBEFile *pFile,
	CBEFunction *pFunction, CBETypedDeclarator *pMember, int nIndex,
	int nDirection);
    virtual bool WriteRefstringInitFunction(CBEFile *pFile,
	CBEFunction *pFunction,	CBEClass *pClass, int nIndex, int nDirection);
    
    virtual bool AddMsgTagMember(CBEFunction *pFunction,
	CBEStructType *pStruct, int nDirection);
    virtual bool AddOpcodeMember(CBEFunction *pFunction,
	CBEStructType *pStruct, int nDirection);

    CBETypedDeclarator* GetMsgTagVariable(void);
    virtual void CheckConvertStruct(CBEStructType *pStruct);
    virtual CBETypedDeclarator* CheckConvertMember(CBEStructType *pStruct,
	vector<CBETypedDeclarator*>::iterator iter);
    virtual void ConvertMember(CBETypedDeclarator* pMember);
};

#endif
