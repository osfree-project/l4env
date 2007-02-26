/**
 *    \file    dice/src/CParser.cpp
 *    \brief   contains the implementation of the class CParser
 *
 *    \date    Mon Jul 22 2002
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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "CParser.h"
#include "CPreProcess.h"
#include "CCParser.h"
#include "CCXXParser.h"
#include "CDCEParser.h"
#include "CCORBAParser.h"
#include "fe/FEFile.h"
#include "fe/FEFileComponent.h"
#include "Compiler.h"
#include "IncludeStatement.h"
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdarg.h>

/////////////////////////////////////////////////////////////////////////////////
// error handling
//@{
/** globale variables used by the parsers to count errors and warnings */
extern int errcount;
extern int erroccured;
extern int warningcount;
//@}

/**    debugging helper variable */
extern int nGlobalDebug;

/** the current line number (relative to the current file) */
int gLineNumber = 1;

/** the name of the input file */
string sInFileName;
/** the name of the include path */
string sInPathName;
/** back up name of the top level input file - we need this when scanning included files */
string sTopLevelInFileName;

/** indicates if the current file is a C header */
int c_inc = 0;

/** if we expect an file name return it when scanning, otherwise an ID */
bool bExpectFileName = false;

/////////////////////////////////////////////////////////////////////////////////
// file elements
/** a reference to the currently parsed file */
CFEFileComponent *pCurFileComponent = NULL;

CParser* CParser::m_pCurrentParser = 0;
CFEFile* CParser::m_pCurrentFile = 0;
bool CParser::m_bFirstRun = true;

CParser::CParser()
{
    m_pBuffer = 0;
    m_pFEFile = 0;
    m_nInputFileType = 0;
    m_nFiles = 0;
    m_pParent = 0;
}

/** cleans up the parser object */
CParser::~CParser()
{
}

/** \brief parses an IDL file and inserts its elements into the pFEFile
 *  \param scan_buffer the buffer to use for scanning
 *  \param sFilename the name of the file to parse
 *  \param nIDL indicates the type of the IDL (DCE/CORBA)
 *  \param bVerbose true if verbose output should be written
 *  \param bPreProcessOnly true if parser should staop after preprocessing
 *  \return true if successful
 */
bool CParser::Parse(void *scan_buffer, string sFilename, int nIDL, bool bVerbose, bool bPreProcessOnly)
{
    assert(false);
    return false;
}

/** \brief prints a message if verbose mode is on
 *  \param sMsg the message
 */
void CParser::Verbose(const char *sMsg, ...)
{
    // if verbose turned off: return
    if (!m_bVerbose)
        return;
    va_list args;
    va_start(args, sMsg);
    vprintf(sMsg, args);
    va_end(args);
}

/** \brief obtains a reference to the current parser
 *  \return a reference to the current parser
 */
CParser* CParser::GetCurrentParser()
{
    return m_pCurrentParser;
}

/** \brief sets the current parser
 *  \param pParser the new current parser
 */
CParser *CParser::SetCurrentParser(CParser *pParser)
{
    CParser *pOld = m_pCurrentParser;
    m_pCurrentParser = pParser;
    return pOld;
}

/** \brief creates a new parser object
 *  \param nFileType the type of the file to parse
 *  \return a reference to an appropriate parser object
 */
CParser* CParser::CreateParser(int nFileType)
{
    CParser *pRet = 0;
    switch (nFileType)
    {
    case USE_FE_DCE:
        pRet = new CDCEParser();
        break;
    case USE_FE_CORBA:
        pRet = new CCORBAParser();
        break;
    case USE_FILE_C:
        pRet = new CCParser();
        break;
    case USE_FILE_CXX:
        pRet = new CCXXParser();
        break;
    default:
        pRet = new CParser();
        break;
    }
    return pRet;
}

/** \brief imports a file
 *  \param sFilename the name of the file
 *  \return 0 if something went wrong and the parser should terminate
 */
unsigned char CParser::Import(string sFilename)
{
    assert(false);
    return false;
}

/** \brief returns a reference to the root of the current scope
 *  \return a reference to the root of the current scope
 *
 * The current scope is the last import statement.
 */
CFEFile* CParser::GetTopFileInScope()
{
    return m_pFEFile;
}


/** \brief set the new current file
 *  \param pFEFile the new current file
 *
 * This function first tests if there is already a current
 * file and adds the new file if so. Then the current file
 * is set to the new file.
 */
void CParser::SetCurrentFile(CFEFile *pFEFile)
{
    if (!pFEFile)
        return;
    if (m_pCurrentFile)
        m_pCurrentFile->AddChild(pFEFile);
    m_pCurrentFile = pFEFile;
}

/** \brief set the current file to the parent of the current file
 *
 * Test and set the current file to its parent.
 */
void CParser::SetCurrentFileParent()
{
    if (m_pCurrentFile && m_pCurrentFile->GetParent())
        m_pCurrentFile = (CFEFile*)m_pCurrentFile->GetParent();
}

/** \brief access the current front end file
 *  \return a reference to the current front-end file
 */
CFEFile *CParser::GetCurrentFile()
{
    return m_pCurrentFile;
}

/** \brief prepare the environment of the parsed files
 *  \param sFilename the name of the file to prepare the environment for
 *  \param fIn the file handle where the input file is opened
 *  \param fOut the file handle to the preprocessed output (to be)
 *
 * This function creates types, which should be known
 * when parsing IDL files, but cannot be imported.
 */
bool CParser::PrepareEnvironment(string sFilename, FILE*& fIn, FILE*& fOut)
{
    // if we run for the very first time, we import the
    // dice header file
    if (m_bFirstRun)
    {
        m_bFirstRun = false;
        // we have to make the include statement come first in the
        // in-file, therefore we have to copy the in-file into a
        // temporary file and prepend the include statement
        FILE *fTmp = tmpfile();
        if (!fTmp)
            return false;
        fprintf(fTmp, "#include \"dice/dice-corba-types.h\"\n");
        fprintf(fTmp, "#line 1 \"%s\"\n", sFilename.c_str()); // reset line number
        if (!CopyFile(fIn, fTmp))
            return false;
        fclose(fIn);
        fIn = fTmp;
    }
    rewind(fIn);
    return true;
}

/** \brief does some finializing work
 *  \return true if successful
 *
 * We "remove" the "dice-corba-types.h" include.
 */
void CParser::FinishEnvironment()
{
    CFEFile *pRoot = dynamic_cast<CFEFile*>(GetTopFileInScope()->GetRoot());
    vector<CIncludeStatement*>::iterator iter = pRoot->GetFirstInclude();
    CIncludeStatement *pInclude;
    while ((pInclude = pRoot->GetNextInclude(iter)) != 0)
    {
        if (pInclude->GetIncludedFileName() == "dice/dice-corba-types.h")
            pInclude->SetPrivate(true);
    }
}

/** \brief test if we have to switch parsers
 *  \return true if we have to close this parser and switch to the parser owning us
 *
 * We switch parser when we hit a line directive stating the end of a file
 * and we have no more managed files in here.
 */
bool CParser::DoEndImport()
{
    m_nFiles--;
    return (m_nFiles < 0);
}

/** \brief simply copy the input file into the outpt file
 *  \param fInput the file to copy from
 *  \param fOutput the file to copy to
 *  \return true if copy was successful
 */
bool CParser::CopyFile(FILE *fInput, FILE* fOutput)
{
    char buffer[1024];
    size_t len;
    do
    {
        len = fread((void*)buffer, sizeof(char), sizeof(buffer), fInput);
        fwrite((const void*)buffer, sizeof(char), len, fOutput);
    } while (len == sizeof(buffer));
    return true;
}

/** \brief update saved state information
 *  \param sFileName the name of the file with updated info
 *  \param nLineNumber the new line number
 *
 * Because end of file line directives (the ones ending on "2")
 * appear in nested files (and thus parsers), the outer file
 * looses updated state information contained in those lines.
 * Thus we have to tell the outer file about the updates.
 */
void CParser::UpdateState(string sFileName, int nLineNumber)
{
    // if its not me, try parent (because it always called for
    // the currently active parser)
    if (sFileName != m_sFilename)
    {
        if (m_pParent)
            m_pParent->UpdateState(sFileName, nLineNumber);
        return;
    }
//     TRACE("UpdateState for %s from %d to %d\n",
//         sFileName.c_str(), m_nLineNumber, nLineNumber);
    // update line number
    if (nLineNumber > m_nLineNumber)
        m_nLineNumber = nLineNumber;
}

