/**
 *    \file    dice/src/be/l4/v4/L4V4BEMsgBufferType.h
 *    \brief    contains the declaration of the class CL4V4BEMsgBufferType
 *
 *    \date    01/08/2004
 *    \author    Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
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
#ifndef L4V4BEMSGBUFFERTYPE_H
#define L4V4BEMSGBUFFERTYPE_H

#include <be/l4/L4BEMsgBufferType.h>

/** \class CL4V4BEMsgBufferType
 *  \ingroup backend
 *  \brief contains the V4 specific message buffer
 */
class CL4V4BEMsgBufferType : public CL4BEMsgBufferType
{

public:
    /** creates a new message buffer object */
    CL4V4BEMsgBufferType();
    virtual ~CL4V4BEMsgBufferType();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CL4V4BEMsgBufferType(CL4V4BEMsgBufferType &src);

public:
    virtual CObject* Clone();
    virtual void InitCounts(CBEClass* pClass,  CBEContext* pContext);
    virtual void InitCounts(CBEFunction* pFunction,  CBEContext* pContext);
    virtual CFETypeSpec* GetMsgBufferType(CFEInterface* pFEInterface,  CFEDeclarator* &pFEDeclarator,  CBEContext* pContext);
    virtual CFETypeSpec* GetMsgBufferType(CFEOperation* pFEOperation,  CFEDeclarator* &pFEDeclarator,  CBEContext* pContext);

protected:
    virtual void WriteSizeDopeInit(CBEFile* pFile,  CBEContext* pContext);
    virtual void WriteSendDopeInit(CBEFile* pFile,  CBEContext* pContext);
    virtual void WriteSendDopeInit(CBEFile* pFile,  int nSendDirection,  CBEContext* pContext);
};

#endif
