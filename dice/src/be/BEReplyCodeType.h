/**
 *  \file   dice/src/be/BEReplyCodeType.h
 *  \brief  contains the declaration of the class CBEReplyCodeType
 *
 *  \date   10/10/2003
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
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
#ifndef BEREPLYCODETYPE_H
#define BEREPLYCODETYPE_H

#include "be/BEType.h"

class CBEContext;
class CBETypedDeclarator;

/** \class CBEReplyCodeType
 *  \ingroup backend
 *  \brief the back-end struct type
 */
class CBEReplyCodeType : public CBEType
{
// Constructor
public:
    /** \brief constructor
     */
    CBEReplyCodeType();
    virtual ~CBEReplyCodeType();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CBEReplyCodeType(CBEReplyCodeType &src);

public:
    virtual CObject* Clone();
    virtual bool CreateBackEnd(CBEContext *pContext);

protected:
};

#endif
