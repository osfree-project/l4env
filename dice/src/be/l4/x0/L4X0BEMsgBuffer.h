/**
 *  \file    dice/src/be/l4/x0/L4X0BEMsgBuffer.h
 *  \brief   contains the declaration of the class CL4X0BEMsgBuffer
 *
 *  \date    08/03/2006
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006
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
#ifndef L4X0BEMSGBUFFER_H
#define L4X0BEMSGBUFFER_H

#include <be/l4/L4BEMsgBuffer.h>

/** \class CL4X0BEMsgBuffer
 *  \ingroup backend
 *  \brief represents a message buffer variable
 */
class CL4X0BEMsgBuffer : public CL4BEMsgBuffer
{
public:
    /** constructor */
    CL4X0BEMsgBuffer();
    ~CL4X0BEMsgBuffer();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CL4X0BEMsgBuffer(CL4X0BEMsgBuffer & src);

public: // public methods
    /** \brief creates a copy of the object
     *  \return a reference to the newly created instance
     */
    virtual CObject* Clone()
    { return new CL4X0BEMsgBuffer(*this); }
    virtual int GetPayloadOffset();

    virtual void WriteDopeShortInitialization(CBEFile *pFile, int nType,
	int nDirection);

    virtual int GetMemberPosition(string sName, int nDirection);

protected: // protected methods
    virtual void WriteRefstringInitParameter(CBEFile *pFile,
	CBEFunction *pFunction, CBETypedDeclarator *pMember, int nIndex,
	int nDirection);
    virtual bool WriteRefstringInitFunction(CBEFile *pFile,
	CBEFunction *pFunction,	CBEClass *pClass, int nIndex, int nDirection);
    virtual void WriteRcvFlexpageInitialization(CBEFile *pFile,	int nDirection);
	
    CBETypedDeclarator* GetRefstringMemberVariable(int nNumber);

    virtual bool AddPlatformSpecificMembers(CBEFunction *pFunction,
	CBEStructType *pStruct, int nDirection);
    virtual bool AddGenericStruct(CBEFunction *pFunction,
	CFEOperation *pFEOperation);
};

#endif

