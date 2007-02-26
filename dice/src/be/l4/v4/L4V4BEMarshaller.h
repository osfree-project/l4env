/**
 *    \file    dice/src/be/l4/v4/L4V4BEMarshaller.h
 *    \brief    contains the declaration of the class CL4V4BEMarshaller
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
#ifndef L4V4BEO1MARSHALLER_H
#define L4V4BEO1MARSHALLER_H

#include <be/l4/L4BEMarshaller.h>

/** \class CL4V4BEMarshaller
 *  \ingroup backend
 *  \brief V4 specific marshalling routines
 */
class CL4V4BEMarshaller : public CL4BEMarshaller
{

public:
    /** creates an instance of this class */
    CL4V4BEMarshaller();
    virtual ~CL4V4BEMarshaller();

protected:
    void WriteAssignment(CBEType *pType, int nStartOffset, bool& bUseConstOffset, int nAlignment, CBEContext *pContext);
    int MarshalValue(int nBytes, int nValue, int nStartOffset, bool & bUseConstOffset, bool bIncOffsetVariable, CBEContext * pContext);
};

#endif
