/**
 *	\file	BEO1Marshaller.h
 *	\brief	contains the implementation of the class CBEAttribute
 *
 *	\date	05/16/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

#ifndef BEO1MARSHALLER_H
#define BEO1MARSHALLER_H

#include "be/BEMarshaller.h"

/** \class CBEO1Marshaller
 *  \brief the class contains the first optimization level of the marshaller code
 */
class CBEO1Marshaller : public CBEMarshaller
{
DECLARE_DYNAMIC(CBEO1Marshaller);
public:
    /** \brief constructs a marshaller object */
	CBEO1Marshaller();
	virtual ~CBEO1Marshaller();

protected: // Protected methods
    virtual int MarshalConstArray(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext);
    virtual int MarshalVariableArray(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext);
    virtual int MarshalString(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext);
    virtual int MarshalStruct(CBEStructType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext);
};

#endif
