/**
 *  \file   dice/src/be/BEOpcodeType.h
 *  \brief  contains the declaration of the class CBEOpcodeType
 *
 *  \date   01/21/2002
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
#ifndef __DICE_BEOPCODETYPE_H__
#define __DICE_BEOPCODETYPE_H__

#include "be/BEType.h"

class CBETypedDeclarator;

/** \class CBEOpcodeType
 *  \ingroup backend
 *  \brief the back-end struct type
 */
class CBEOpcodeType : public CBEType
{
// Constructor
public:
    /** \brief constructor
     */
    CBEOpcodeType();
    virtual ~CBEOpcodeType();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CBEOpcodeType(CBEOpcodeType &src);

public:
    virtual CObject* Clone();
    virtual void CreateBackEnd();
};

#endif // !__DICE_BEOPCODETYPE_H__
