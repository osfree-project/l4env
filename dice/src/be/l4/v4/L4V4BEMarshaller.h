/**
 *    \file    dice/src/be/l4/v4/L4V4BEMarshaller.h
 *    \brief   contains the declaration of the class CL4V4BEMarshaller
 *
 *    \date    06/01/2006
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef L4V4BEMARSHALLER_H
#define L4V4BEMARSHALLER_H

#include <be/l4/L4BEMarshaller.h>

/** \class CL4V4BEMarshaller
 *  \ingroup backend
 *  \brief contains the marshalling code
 */
class CL4V4BEMarshaller : public CL4BEMarshaller
{
public:
    /** constructor */
    CL4V4BEMarshaller();
    virtual ~CL4V4BEMarshaller();

protected:
    virtual bool DoSkipParameter(CBEFunction *pFunction, 
	CBETypedDeclarator *pParameter, int nDirection);
    virtual bool MarshalRefstring(CBETypedDeclarator *pParameter, 
	vector<CDeclaratorStackLocation*> *pStack);
    virtual void WriteRefstringCastMember(int nDir, CBEMsgBuffer *pMsgBuffer,
	CBETypedDeclarator *pMember);

protected:
    virtual bool MarshalZeroFlexpage(CBETypedDeclarator *pMember);
    
};

#endif
