/**
 *    \file    dice/src/IncludeStatement.cpp
 *    \brief   contains the implementation of the class CIncludeStatement
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

 #include "IncludeStatement.h"

CIncludeStatement::CIncludeStatement(bool bIDLFile,
    bool bStdInclude,
    bool bPrivate,
    string sFileName)
{
    m_bIDLFile = bIDLFile;
    m_bIsStandardInclude = bStdInclude;
    m_bPrivate = bPrivate;
    m_sFileName = sFileName;
}

CIncludeStatement::CIncludeStatement(CIncludeStatement &src)
{
    m_bIDLFile = src.m_bIDLFile;
    m_bIsStandardInclude = src.m_bIsStandardInclude;
    m_bPrivate = src.m_bPrivate;
    m_sFileName = src.m_sFileName;
}

CIncludeStatement::~CIncludeStatement()
{ }
