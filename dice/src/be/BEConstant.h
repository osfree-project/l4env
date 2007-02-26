/**
 *    \file    dice/src/be/BEConstant.h
 *  \brief   contains the declaration of the class CBEConstant
 *
 *    \date    01/18/2002
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

/** preprocessing symbol to check header file */
#ifndef __DICE_BECONSTANT_H__
#define __DICE_BECONSTANT_H__

#include "be/BEObject.h"

class CFEConstDeclarator;

class CBEType;
class CBEExpression;
class CBEFile;
class CBEHeaderFile;
class CBEImplementationFile;

/** \class CBEConstant
 *  \ingroup backend
 *  \brief the back-end constant
 */
class CBEConstant : public CBEObject
{
public:
// Constructor
    /** \brief constructor
     */
    CBEConstant();
    ~CBEConstant();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CBEConstant(CBEConstant &src);

public:
    virtual void Write(CBEHeaderFile *pFile);
    virtual void CreateBackEnd(CBEType* pType, string sName, 
	CBEExpression* pValue, bool bAlwaysDefine);
    virtual void CreateBackEnd(CFEConstDeclarator *pFEConstDeclarator);
    virtual bool AddToFile(CBEHeaderFile *pHeader);
    virtual string GetName();
    virtual CBEExpression *GetValue();

    /** \brief tries to match against a given name
     *  \param sName the name to match against
     *  \return true if given name matches internal name
     */
    bool Match(string sName)
    { return GetName() == sName; }

protected:
    /** \var bool m_bAlwaysDefine
     *  \brief true if this const has to be printed as define always
     */
     bool m_bAlwaysDefine;
    /** \var string m_sName
     *  \brief the name of the constant
     */
    string m_sName;
    /** \var CBEType *m_pType
     *  \brief the type of the constant
     *
     * A C constant usually does not have a type, but to be able to check
     * integrity later on, we keep it.
     */
    CBEType *m_pType;
    /** \var CBEExpression *m_pValue
     *  \brief the value of the constant
     */
    CBEExpression *m_pValue;
};

#endif // !__DICE_BECONSTANT_H__
