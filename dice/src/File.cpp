/**
 *    \file    dice/src/File.cpp
 *  \brief   contains the implementation of the class CFile
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

#include "File.h"

CFile::CFile()
{
    m_nIndent = 0;
}

CFile::CFile(CFile& src)
: CObject(src)
{
}

/** \brief specializaion of stream operator for strings
 *  \param s string parameter
 *
 * Here we implement the special treatment for indentation. To make it right
 * we have to check if there are tabs after line-breaks.
 */
template<>
CFile& CFile::operator<<(string s)
{
    if (s[0] == '\t')
    {
	PrintIndent();
	*this << s.substr(1);
	return *this;
    }

    string::size_type pos = s.find('\n');
    if (pos != string::npos &&
	pos != s.length())
    {
	/* first print everything up to \n */
	m_file << s.substr(0, pos + 1);

	/* then call ourselves with the rest */
	*this << s.substr(pos + 1);
	return *this;
    }

    /* simple string */
    m_file << s;
    return *this;
}

/** \brief specialization of stream operator for character arrays
 *  \param s character array to print
 */
template<>
CFile& CFile::operator<<(char const * s)
{
    this->operator<<(string(s));
    return *this;
}

/** \brief specialization of stream operator for character arrays
 *  \param s character array to print
 */
template<>
CFile& CFile::operator<<(char* s)
{
    this->operator<<(string(s));
    return *this;
}

/**
 *  \brief opens a file with a specific type
 *  \param sFileName the file name
 *  \return true if open was siccessful, false otherwise
 */
bool CFile::Open(string sFileName)
{
    if (m_file.is_open())
	return false;
    m_file.open(sFileName.c_str());
    if (!m_file)
	return false;
    if (m_sFilename.empty())
	m_sFilename = sFileName;
    m_nIndent = 0;
    m_nLastIndent = 0;
    return true;
}

/** \brief prints the indentation
 */
void CFile::PrintIndent(void)
{
    for (unsigned int i = 0; i < m_nIndent; i++)
	m_file << " ";
}

/**
 *  \brief increases the ident for this file
 *  \param by the number of characters, the ident should be increased.
 *
 * The standard value to increase the ident is specified in the constant
 * STD_INDENT.  If the ident reaches the values specified in MAX_IDENT it
 * ignores the ident increase.
 */
void CFile::IncIndent(int by)
{
    m_nLastIndent = (m_nIndent + by > MAX_INDENT) ? MAX_INDENT - m_nIndent : by;
    m_nIndent = std::min(m_nIndent + by, MAX_INDENT);
}

/**
 *  \brief decreases the ident
 *  \param by the number of character, by which the ident should be decreased
 *
 * The standard value for the decrement operation is STD_IDENT. If the ident
 * reaches zero (0) the operation ignores the decrement.  If by is -1 the
 * indent is decremented by the value of the last increment.
 */
void CFile::DecIndent(int by)
{
    m_nIndent = std::max((int)m_nIndent - ((by == -1) ? m_nLastIndent : by), 0);
    m_nLastIndent = 0;
}

