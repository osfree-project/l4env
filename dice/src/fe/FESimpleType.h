/**
 *  \file   dice/src/fe/FESimpleType.h
 *  \brief  contains the declaration of the class CFESimpleType
 *
 *  \date   01/31/2001
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
#ifndef __DICE_FE_FESIMPLETYPE_H__
#define __DICE_FE_FESIMPLETYPE_H__

#include "fe/FETypeSpec.h"

/** \class CFESimpleType
 *  \ingroup frontend
 *  \brief represents a simple scalar type
 */
class CFESimpleType : public CFETypeSpec
{

// standard constructor/destructor
public:
    /** \brief CFESimpleType constructor
     *  \param nType the type of the type
     *  \param bUnSigned true if unsigned
     *  \param bUnsignedFirst true if the unsigned string appears first
     *  \param nSize the size of the type (number of bytes)
     *  \param bShowType true if the basic type is shown
     */
    CFESimpleType(TYPESPEC_TYPE nType, bool bUnSigned = false, bool bUnsignedFirst = true, int nSize = 0, bool bShowType = true);
    virtual ~CFESimpleType();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFESimpleType(CFESimpleType &src);

// Operations
public:
    virtual void Serialize(CFile *pFile);
    virtual void SetUnsigned(bool bUnsigned);
    virtual bool CheckConsistency();
    virtual bool IsUnsigned();
    virtual CObject* Clone();
    virtual int GetSize();

// attributes
protected:
/** \name Some Flags */
//@{
    /** some variables, which contain flags, whether keywords are used with this
     * type, e.g. unsigned, hyper, and flags to decide which order the keywords appear in
     */
    bool m_bUnSigned;
    bool m_bUnsignedFirst;
    bool m_bShowType;
//@}
    /** \var int m_nSize
     *  \brief the size of the type in bytes
     */
    int m_nSize;
};

#endif /* __DICE_FE_FESIMPLETYPE_H__ */

