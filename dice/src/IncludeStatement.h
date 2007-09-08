/**
 *  \file    dice/src/IncludeStatement.h
 *  \brief   contains the declaration of the class CIncludeStatement
 *
 *  \date    10/22/2004
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
#ifndef __DICE_INCLUDESTATEMENT_H__
#define __DICE_INCLUDESTATEMENT_H__

#include "Object.h"
#include <string>

/** \class CIncludeStatement
 *  \ingroup base
 *  \brief helper class to manage included files
 */
class CIncludeStatement : public CObject
{
    /** hidden empty constructor */
    CIncludeStatement()
    { }

public:
    /** default constructor */
    CIncludeStatement(bool bIDLFile, bool bStdInclude, bool bImport,
	std::string sFileName, std::string sFromFile, std::string sPath, int nLineNb);
    /** copy constructor
     * \param src the source to copy from
     */
    CIncludeStatement(const CIncludeStatement &src);
    /** destroys the object */
    ~CIncludeStatement();

    /** \brief creates a copy of this object
     *  \return reference to new instance
     */
    CObject* Clone()
    { return new CIncludeStatement(*this); }

    /** \brief tries to make a simple match
     *  \param sName the file name to match against
     *  \return true if filenames match
     */
    bool Match(std::string sName)
    { return m_sFilename == sName; }

    /** \var bool m_bIDLFile
     *  \brief true if this is an IDL file
     */
    bool m_bIDLFile;
    /** \var bool m_bStandard
     *  \brief true if this is included as a standard include file \
     *         (using "<" and ">")
     */
    bool m_bStandard;
    /** \var bool m_bImport
     *  \brief true if this is an import statement
     */
    bool m_bImport;
    /** \var std::string m_sFilename
     *  \brief the name of the file to include
     */
    std::string m_sFilename;
    /** \var std::string m_sFromFile
     *  \brief the name of the file with the include statement
     */
    std::string m_sFromFile;
    /** \var std::string m_sPath
     *  \brief path on which the file was opened
     */
    std::string m_sPath;
    /** \var int m_nLineNb
     *  \brief line number of the include statement
     */
    unsigned int m_nLineNb;
};

#endif                // __DICE_INCLUDESTATEMENT_H__
