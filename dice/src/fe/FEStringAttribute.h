/**
 *    \file    dice/src/fe/FEStringAttribute.h
 *    \brief   contains the declaration of the class CFEStringAttribute
 *
 *    \date    01/31/2001
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#ifndef __DICE_FE_FESTRINGATTRIBUTE_H__
#define __DICE_FE_FESTRINGATTRIBUTE_H__

#include "fe/FEAttribute.h"
#include <string>
using namespace std;

/**    \class CFEStringAttribute
 *    \ingroup frontend
 *    \brief represents all attributes, which contain a string information
 */
class CFEStringAttribute : public CFEAttribute
{

// standard constructor/destructor
public:
    /** \brief constructs string attribute object
     *  \param nType which type of attribute
     *  \param string the string contained in the attribute
     */
    CFEStringAttribute(ATTR_TYPE nType, string string);
    virtual ~CFEStringAttribute();

protected:
    /**    \brief copy constrcutor
     *    \param src the source to copy from
     */
    CFEStringAttribute(CFEStringAttribute & src);

// Operations
public:
    virtual void Serialize(CFile *pFile);
    virtual CObject* Clone();
    virtual string GetString();

// attributes
protected:
    /**    \var string m_String
     *    \brief the string
     */
    string m_String;
};

#endif /* __DICE_FE_FESTRINGATTRIBUTE_H__ */
