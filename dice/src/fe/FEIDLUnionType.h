/**
 *    \file    dice/src/fe/FEIDLUnionType.h
 *    \brief   contains the declaration of the class CFEIDLUnionType
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
#ifndef __DICE_FE_FEIDLUNIONTYPE_H__
#define __DICE_FE_FEIDLUNIONTYPE_H__

#include "fe/FEUnionType.h"
#include <string>
#include <vector>

class CFEUnionCase;

/** \class CFEIDLUnionType
 *  \ingroup frontend
 *  \brief represents a union
 */
class CFEIDLUnionType : public CFEUnionType
{

// standard constructor/destructor
public:
    /** \brief constructor for object of union type
     *  \param pSwitchType the tyoe of the switch variable
     *  \param sSwitchVar the name of the switch variable
     *  \param pUnionBody the "real" union
     *  \param sUnionName the name of the union
     *  \param sTag the tag if one
     */
    CFEIDLUnionType(std::string sTag,
        vector<CFEUnionCase*> *pUnionBody,
	CFETypeSpec *pSwitchType,
        std::string sSwitchVar,
        std::string sUnionName);
    virtual ~CFEIDLUnionType();

// Operations
public:
    virtual CObject* Clone();

    /** retrieves the type of the switch variable
     *  \return the type of the switch variable
     */
    CFETypeSpec *GetSwitchType()
    { return m_pSwitchType; }
    /** retrieves the name of the switch variable
     *  \return the name of the switch variable
     */
    std::string GetSwitchVar()
    { return m_sSwitchVar; }
    /** retrieves the name of the union
     *  \return an identifier containing the name of the union
     */
    std::string GetUnionName()
    { return m_sUnionName; }

protected:
    /** a copy construtor used for the tagged union class */
    CFEIDLUnionType(CFEIDLUnionType& src); // copy constructor for tagged union

// attribute
protected:
    /** \var CFETypeSpec *m_pSwitchType
     *  \brief the type of the switch argument
     */
    CFETypeSpec *m_pSwitchType;
    /** \var std::string m_sSwitchVar
     *  \brief the name of the switch variable
     */
    std::string m_sSwitchVar;
    /** \var std::string m_sUnionName
     *  \brief the name of the union
     */
    std::string m_sUnionName;
};

#endif /* __DICE_FE_FEIDLUNIONTYPE_H__ */

