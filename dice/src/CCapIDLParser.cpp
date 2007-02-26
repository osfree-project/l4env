/**
 *    \file    dice/src/CCapIDLParser.cpp
 *    \brief   contains the implementation of the class CCapIDLParser
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
#include "CCapIDLParser.h"
#include "CPreProcess.h"
#include "Compiler.h"
#include "fe/FEFile.h"

#include <cctype>
#include <algorithm>

//@{
/** global variables and function of the CapIDL parser */
extern FILE *capidlin;
extern int capidldebug;
extern int capidl_flex_debug;
extern int capidlparse();
//@}

/////////////////////////////////////////////////////////////////////////////////
// error handling
//@{
/** globale variables used by the parsers to count errors and warnings */
extern int errcount;
extern int erroccured;
extern int warningcount;
//@}

/////////////////////////////////////////////////////////////////////////////////
// file elements
/** a reference to the currently parsed file */

/** the current line number (relative to the current file) */
extern int gLineNumber;

/** indicates if the current file is a C header */
extern int c_inc;

/** the name of the input file */
extern string sInFileName;
/** the name of the include path */
extern string sInPathName;
/** back up name of the top level input file - we need this when scanning included files */
extern string sTopLevelInFileName;

CCapIDLParser::CCapIDLParser()
 : CParser()
{
}

/** destroy this object */
CCapIDLParser::~CCapIDLParser()
{
}

/** \brief imports a file
 *  \param sFilename the name of the file
 *  \return false if something went wrong and the parser should terminate
 */
unsigned char CCapIDLParser::Import(string sFilename)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CapIDL::Import(%s) called: sInFileName = %s\n",
        sFilename.c_str(), sInFileName.c_str());
    // 1. save state
    // - scanner's buffer
    // - line number
    // - c_inc
    // - sTopLevelInFileName
    // - sFilename
    m_pBuffer = GetCurrentBufferCapIDL();
    m_nLineNumber = gLineNumber;
    m_c_inc = c_inc;
    m_sPrevTopLevelFileName = sTopLevelInFileName;
    m_sFilename = sInFileName;

    // 2. determine file type
    // set c_inc to indicate if this is a C header file or not
    int iDot = sFilename.rfind('.');
    FrontEnd_Type nFileType = m_nInputFileType;
    if (iDot > 0)
    {
        string sExt = sFilename.substr (iDot + 1);
        transform(sExt.begin(), sExt.end(), sExt.begin(), _tolower);
	nFileType = DetermineFileType(sExt);
        if ((nFileType == USE_FILE_C) ||
            (nFileType == USE_FILE_CXX))
            c_inc = 1;
        else
            c_inc = 0;
    }
    else
        c_inc = 0;

    unsigned char nRet = 2;
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"CapIDL::Import(%s) check if filetypes equal (%d == %d)?\n",
        sFilename.c_str(), m_nInputFileType, nFileType);
    // only create new parser if file type is different
    if (m_nInputFileType != nFileType)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CapIDL::Import(%s) requires a new parser (%d)\n",
	    sFilename.c_str(), nFileType);
        // 3. create new Parser
        CParser *pParser = CParser::CreateParser(nFileType);
        CParser *pOldParser = CParser::SetCurrentParser(pParser);
        pParser->SetParent(pOldParser);
        // scanner set a new FEFile as my current file

        // 4. call it's Parse method
        erroccured = !pParser->Parse(m_pBuffer, sFilename, nFileType);
        delete pParser;

        // restore old parser
        CParser::SetCurrentParser(pOldParser);

        // if error while parsing, return
        if (erroccured)
            return 0;

        // 5. restore state
        c_inc = m_c_inc;
        sTopLevelInFileName = m_sPrevTopLevelFileName;
        sInFileName = m_sFilename;
        gLineNumber = m_nLineNumber;
        RestoreBufferCapIDL(m_pBuffer, false);
    }
    else
    {
        m_nFiles++;
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	    "CapIDL::Import(%s) doesn't require a new parser, inc file cnt to %d\n",
            sFilename.c_str(), m_nFiles);
        nRet = 1;
    }

    // everything alright, continue
    return nRet;
}

/** \brief parses an IDL file and inserts its elements into the pFEFile
 *  \param scan_buffer the buffer of the scanner to use
 *  \param sFilename the filename of the file to import
 *  \param nIDL indicates the type of the IDL (DCE/CORBA/CapIDL)
 *  \param bPreProcessOnly true if we stop after pre-process
 *  \return true if successful
 */
bool 
CCapIDLParser::Parse(void *scan_buffer, 
    string sFilename, 
    FrontEnd_Type nIDL, 
    bool bPreProcessOnly)
{
//     TRACE("CapIDL::Parse(%p, %s, %d, %s, %s) called\n", scan_buffer,
//         sFilename.c_str(), nIDL,
//         bPreProcessOnly?"true":"false");

    m_nInputFileType = nIDL;
    bool bFirst = (scan_buffer == 0);

    // 1. call preprocess -> opens input file
    CPreProcess *pPreProcess = CPreProcess::GetPreProcessor();
    FILE *fInput = 0;
    if (!scan_buffer)
    {
        fInput = pPreProcess->PreProcess(sFilename, false);
        if (!fInput)
        {
            erroccured = true;
            errcount++;
            return false;
        }

        if (bPreProcessOnly)
        {
            bool bRet = CopyFile(fInput, stdout);
            fclose(fInput);
            return bRet;
        }
    }

    // 2. create an FEFile
    // get path, include level and whether this is a standard include
    string sPath = pPreProcess->GetIncludePath(sFilename);
    bool bIsStandard = pPreProcess->IsStandardInclude(sFilename, gLineNumber);
    string sOrigName = pPreProcess->GetOriginalIncludeForFile(sFilename, 
	gLineNumber);
    if (sOrigName.empty())
        sOrigName = sFilename;
    // new file
    m_pFEFile = new CFEFile(sOrigName, sPath, gLineNumber, bIsStandard);

    // 3. set new current file
    CParser::SetCurrentFile(m_pFEFile);

    // 4. set gLineNumber
    // set line number to start
    gLineNumber = 1;
    sInFileName = sFilename;

    // 5. init the scanner
    if (CCompiler::IsVerboseLevel(PROGRAM_VERBOSE_PARSER))
        capidldebug = 1;
    else
        capidldebug = 0;
    if (CCompiler::IsVerboseLevel(PROGRAM_VERBOSE_SCANNER))
        capidl_flex_debug = 1;
    else
        capidl_flex_debug = 0;

    if (!scan_buffer)
    {
        capidlin = fInput;
        StartBufferCapIDL(fInput);
    }
    else
        RestoreBufferCapIDL(scan_buffer, true);

    // 6. call capidlparse
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "Parse CapIDL IDL file (%d).\n", nIDL);
    if (capidlparse())
    {
        erroccured = true;
        return false;
    }
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "... finished parsing file.\n");

    // flush buffer state
    GetCurrentBufferCapIDL();

    // 7. close file
    if (fInput)
        fclose(fInput);

    // switch current file
    CParser::SetCurrentFileParent();

    // finish the environment
    if (bFirst)
        FinishEnvironment();

    return true;
}
