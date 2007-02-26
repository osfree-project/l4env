/**
 *  \file    dice/src/Object.h
 *  \brief   contains the declaration of the class CObject
 *
 *  \date    01/31/2001
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef __DICE_OBJECT_H__
#define __DICE_OBJECT_H__

#include "defines.h"
#include <string>
using std::string;

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
// Constructor
  public:
    /** the constructor for this class */
    CObject(CObject * pParent = 0);
    virtual ~ CObject();

  protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CObject(const CObject & src);

// Operations
  public:
    string const GetSourceFileName() const;
    void SetSourceFileName(const string sFileName);
    int GetSourceLine() const;
    void SetSourceLine(const int nLineNb);
    int GetSourceLineEnd() const;
    void SetSourceLineEnd(const int nLineNb);
    virtual CObject * Clone();
    void SetParent(CObject * pParent = 0);
    CObject* GetParent() const;
    template< typename O > O* GetSpecificParent(unsigned nStart = 1);
    bool IsParent(CObject * pParent);
    /** \brief accept function for visitors
     */
    virtual void Accept(CVisitor&)
    { }

// Attributes
  protected:
    /** \var CObject *m_pParent
     *  \brief a reference to the parent object
     */
    CObject * m_pParent;
    /** \var int m_nSourceLineNb
     *  \brief the line number where this elements has been declared
     */
    int m_nSourceLineNb;
    /** \var int m_nSourceLineNbEnd
     *  \brief the line number where this element's declaration ends
     */
    int m_nSourceLineNbEnd;
    /** \var string m_sSourceFileName
     *  \brief the source file name
     */
    string m_sSourceFileName;
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

/** \brief sets the source line number of this element
 *  \param nLineNb the line this elements has been declared
 */
inline void
CObject::SetSourceLine(const int nLineNb)
{
    m_nSourceLineNb = nLineNb;
}

/** \brief retrieves the source code line number this elements was declared in
 *  \return the line number of declaration
 */
inline int
CObject::GetSourceLine(void) const
{
    return m_nSourceLineNb;
}

/** \brief sets the source line number of this element's end of declaration
 *  \param nLineNb the line this elements has been declared
 */
inline void
CObject::SetSourceLineEnd(const int nLineNb)
{
    m_nSourceLineNbEnd = nLineNb;
}

/** \brief retrieves the source code line number this element's declaration ends
 *  \return the line number of declaration
 */
inline int
CObject::GetSourceLineEnd(void) const
{
    return m_nSourceLineNbEnd;
}

/** \brief sets the file name of the source file
 *  \param sFileName the name of the source file
 */
inline void
CObject::SetSourceFileName(const string sFileName)
{
    m_sSourceFileName = sFileName;
}

/** \brief retrieves the name of the source file
 *  \return the name of the source file
 */
inline string const
CObject::GetSourceFileName(void) const
{
    return m_sSourceFileName;
}

#endif                // __DICE_OBJECT_H__
