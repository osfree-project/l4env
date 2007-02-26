/**
 *    \file    dice/src/CCXXParser.h
 *    \brief   contains the declaration of the class CCXXParser
 *
 *    \date    Sun Jul 27 2003
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
#ifndef CCXXPARSER_H
#define CCXXPARSER_H

#include <CParser.h>

//@{
/** helper functions to switch between input buffers */
void* GetCurrentBufferGccCxx();
void RestoreBufferGccCxx(void *buffer, bool bInit);
void StartBufferGccCxx(FILE* fInput);
//@}

/** \class CCXXParser
 *  \ingroup frontend
 *  \brief encapsulates the parser calls for the C++ parser
 */
class CCXXParser : public CParser
{
protected:
    /** creates new parser object */
    CCXXParser();

public:
    virtual ~CCXXParser();

    virtual bool Parse(void *scan_buffer, string sFilename, int nIDL, bool bVerbose, bool bPreProcessOnly = false);
    virtual unsigned char Import(string sFilename); // used by parser

    friend class CParser;
};

#endif
