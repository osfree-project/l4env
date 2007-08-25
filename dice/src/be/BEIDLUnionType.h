/**
 *    \file    dice/src/be/BEIDLUnionType.h
 *    \brief   contains the declaration of the class CBEIDLUnionType
 *
 *    \date    03/14/2006
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
#ifndef __DICE_BEIDLUNIONTYPE_H__
#define __DICE_BEIDLUNIONTYPE_H__

#include "be/BEStructType.h"

/** \class CBEIDLUnionType
 *  \ingroup backend
 *  \brief the back-end struct type
 */
class CBEIDLUnionType : public CBEStructType
{
// Constructor
public:
    /** \brief constructor
     */
    CBEIDLUnionType();
    virtual ~CBEIDLUnionType();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CBEIDLUnionType(CBEIDLUnionType &src);

public:
    virtual void CreateBackEnd(CFETypeSpec *pFEType);
    virtual CObject* Clone(void);

    CBETypedDeclarator* GetSwitchVariable(void);
    CBETypedDeclarator* GetUnionVariable(void);

protected:
    /** \var std::string m_sSwitchName
     *  \brief internal name to find switch member
     */
    std::string m_sSwitchName;
    /** \var std::string m_sUnionName
     *  \brief internal name to find union member
     */
    std::string m_sUnionName;
};

#endif // !__DICE_BESTRUCTTYPE_H__
