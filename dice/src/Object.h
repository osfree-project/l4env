/**
 *  \file    dice/src/Object.h
 *  \brief   contains the declaration of the class CObject
 *
 *  \date    01/31/2001
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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
#ifndef __DICE_OBJECT_H__
#define __DICE_OBJECT_H__

#include "defines.h"
#include "Location.h"

class CVisitor;

/** \class CObject
 *  \ingroup backend
 *  \brief base class for all classes
 *
 * The base class CObject is used to store information common
 * for all classes, such as source file and line number, and is
 * also used to build class hierarchies using references to parents.
 */
class CObject
{
public:
    /** the constructor for this class */
    CObject(CObject * pParent = 0);
    virtual ~ CObject();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CObject(const CObject & src);

public:
    virtual CObject * Clone();
    void SetParent(CObject * pParent = 0);
    CObject* GetParent() const;
    template< typename O > O* GetSpecificParent(unsigned nStart = 1);
    bool IsParent(CObject * pParent);
    virtual void Accept(CVisitor&);


    /** \var location sourceLoc
     *  \brief contains the begin and end position of this object
     */
    location m_sourceLoc;

protected:
    /** \var CObject *m_pParent
     *  \brief a reference to the parent object
     */
    CObject * m_pParent;
};

/** \brief obtain a reference to a specifc parent
 *  \param nStart the parent where to start (1 for parent, 0 for this)
 *  \return reference to parent of given type
 */
template< typename O >
O* CObject::GetSpecificParent(unsigned nStart)
{
    CObject *pParent = this;
    unsigned nCur = 0;
    for (; pParent; pParent = pParent->GetParent())
    {
        if (++nCur <= nStart)
            continue;

        O *tmp = dynamic_cast<O*>(pParent);
        if (tmp)
            return tmp;
    }
    return 0;
}

/** \brief retrieves a reference to the parent object
 *  \return a reference to the parent object
 */
inline CObject*
CObject::GetParent(void) const
{
    return m_pParent;
}

/** \brief sets the new parent object
 *  \param pParent a reference to the new parent object
 */
inline void
CObject::SetParent(CObject * pParent)
{
    m_pParent = pParent;
}

#endif                // __DICE_OBJECT_H__
