/**
 *    \file    dice/src/File.h
 *  \brief   contains the declaration of the class CFile
 *
 *    \date    07/05/2001
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
#ifndef __DICE_FILE_H__
#define __DICE_FILE_H__

#include "Object.h"
#include <string>
#include <fstream>
using std::ofstream;

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

/** maimum possible indent */
const unsigned int MAX_INDENT = 80;
/** the standard indentation value */
const unsigned int STD_INDENT = 4;

/** \class CFile
 *  \ingroup base
 *  \brief base class for all file classes
 *
 * Because we only write files, this class owns an ostream only.
 */
class CFile : public CObject
{
// Constructors
protected:
    /** copy constructor */
    CFile(CFile&src);

public:
    /** the constructor for this class */
    CFile();

// Operations
public:
    void DecIndent(int by = STD_INDENT);
    void IncIndent(int by = STD_INDENT);

    bool Open(string sFileName);

    /** \brief return the name of the file
     *  \return the name of the currently open file, 0 if no file is open
     */
    string GetFileName() const
    { return m_sFilename; }
    /** \brief test whether or not file is open
     *  \return true if file is open
     */
    bool IsOpen()
    { return m_file.is_open(); }
    /** flushes the content of the file stream to disk */
    void Flush()
    { m_file.flush(); }
    /** \brief return the current indent
     *  \return the current indent
     */
    unsigned int GetIndent() const
    { return m_nIndent; }
    /** \brief closes a file
     *  \return true if close was successful, false otherwise
     */
    void Close()
    { m_file.close(); }

    template <typename T> CFile& operator<< (T a);

  protected:
     virtual void PrintIndent(void);

  protected:
    /** \var string m_sFilename
     *  \brief the file's name
     */
    string m_sFilename;
    /** \var int m_nIndent
     *  \brief the current valid indent, when printing to the file
     */
    unsigned int m_nIndent;
    /** \var ofstream m_file
     *  \brief the file handle
     */
    ofstream m_file;
    /** \var m_nLastIndent
     *  \brief remembers last increment
     */
    int m_nLastIndent;
};

/** \brief output data to the file
 *  \param a the data to by printed into the file
 *  \return a reference to the file
 */
template <typename T>
CFile& CFile::operator<< (T a)
{
    m_file << a;
    return *this;
}

#endif                // __DICE_FILE_H__
