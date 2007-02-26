/**
 *    \file    dice/src/IncludeStatement.h
 *    \brief   contains the declaration of the class CIncludeStatement
 *
 *    \date    10/22/2004
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
#ifndef __DICE_INCLUDESTATEMENT_H__
#define __DICE_INCLUDESTATEMENT_H__

#include "Object.h"
#include <string>
using namespace std;

/** \class IncludeFile
 *  \ingroup base
 *  \brief helper class to manage included files
 */
class CIncludeStatement : public CObject
{
public:
    /** default constructor */
    CIncludeStatement(bool bIDLFile, bool bStdInclude, bool bPrivate, string sFileName);
    /** copy constructor
     * \param src the source to copy from
     */
    CIncludeStatement(CIncludeStatement &src);
    /** destroys the object */
    ~CIncludeStatement();

public: // Methods
    virtual CObject* Clone();
    virtual bool IsIDLFile();
    virtual bool IsStdInclude();
    virtual bool IsPrivate();
    virtual void SetPrivate(bool bPrivate);
    virtual string GetIncludedFileName();

protected: // Members
    /** \var bool bIDLFile
     *  \brief true if this is an IDL file
     */
    bool m_bIDLFile;
    /** \var bool bIsStandardInclude
     *  \brief true if this is included as a standard include file (using '<' and '>')
     */
    bool m_bIsStandardInclude;
    /** \var bool bPrivate
     *  \brief true if this include statement should not appear in target file
     */
    bool m_bPrivate;
    /** \var string sFileName
     *  \brief the name of the file to include
     */
    string m_sFileName;
};

/** \brief creates a copy of this object
 *  \return reference to new instance
 */
inline CObject*
CIncludeStatement::Clone(void)
{
    return new CIncludeStatement(*this);
}

/** \brief return value of m_bIDLFile
 *  \return value of m_bIDLFile
 */
inline bool
CIncludeStatement::IsIDLFile()
{
    return m_bIDLFile;
}

/** \brief return value of m_bIsStandardInclude
 *  \return value of m_bIsStandardInclude
 */
inline bool
CIncludeStatement::IsStdInclude()
{
    return m_bIsStandardInclude;
}

/** \brief return value of m_bPrivate
 *  \return value of m_bPrivate
 */
inline bool
CIncludeStatement::IsPrivate()
{
    return m_bPrivate;
}

/** \brief sets the value of m_bPrivate
 *  \param bPrivate the new value of m_bPrivate
 */
inline void
CIncludeStatement::SetPrivate(bool bPrivate)
{
    m_bPrivate = bPrivate;
}

/** \brief returns value of m_sFileName
 *  \return value of m_sFileName
 */
inline string
CIncludeStatement::GetIncludedFileName()
{
    return m_sFileName;
}

#endif                // __DICE_INCLUDESTATEMENT_H__
