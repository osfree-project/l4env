/**
 *    \file    dice/src/CCParser.cpp
 *    \brief   contains the implementation of the class CCParser
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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "CCParser.h"
#include "CPreProcess.h"
#include "Compiler.h"
#include "fe/FEFile.h"

#include <ctype.h>
#include <algorithm>
using namespace std;

//@{
/** global variables and function of the GCC C parser */
extern FILE *gcc_cin;
extern int gcc_cdebug;
extern int gcc_c_flex_debug;
extern int gcc_cparse();
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

CCParser::CCParser()
 : CParser()
{
}

/** destroys the object */
CCParser::~CCParser()
{
}

/** \brief imports a file
 *  \param sFilename the name of the file
 *  \return false if something went wrong and the parser should terminate
 *
 * When importing a file we have the following sequence of commands:
 * -# save state of parser (everything which has to do with the scanner and can be modified by it)
 * -# determine the file type
 * -# create a new parser
 * -# call it's Parse method
 * -# restore state
 */
unsigned char CCParser::Import(string sFilename)
{
    // 1. save state
    // - scanner's buffer
    // - line number
    // - c_inc
    // - sTopLevelInFileName
    // - sFilename
    m_pBuffer = GetCurrentBufferGccC();
    m_nLineNumber = gLineNumber;
    m_c_inc = c_inc;
    m_sPrevTopLevelFileName = sTopLevelInFileName;
    m_sFilename = sInFileName;

    // 2. determine file type
    // set c_inc to indicate if this is a C header file or not
    int iDot = sFilename.rfind('.');
    int nFileType = m_nInputFileType;
    if (iDot > 0)
    {
        string sExt = sFilename.substr (iDot + 1);
        transform(sExt.begin(), sExt.end(), sExt.begin(), tolower);
        if ((sExt == "h") ||
	    (sExt == "c"))
            nFileType = USE_FILE_C;
        if ((sExt == "hh") ||
            (sExt == "H") ||
            (sExt == "hpp") ||
            (sExt == "hxx") ||
	    (sExt == "cpp") ||
	    (sExt == "cc"))
            nFileType = USE_FILE_CXX;
        if ((nFileType == USE_FILE_C) ||
            (nFileType == USE_FILE_CXX))
            c_inc = 1;
        else
            c_inc = 0;
    }
    else
        c_inc = 0;

    unsigned char nRet = 2;
    if (m_nInputFileType != nFileType)
    {
        Verbose("C::Import(%s) requires a new parser (%d)\n", sFilename.c_str(), nFileType);
        // 3. create new Parser
        CParser *pParser = CParser::CreateParser(nFileType);
        CParser *pOldParser = CParser::SetCurrentParser(pParser);
        pParser->SetParent(pOldParser);
        // scanner set a new FEFile as my current file

        // 4. call it's Parse method
        erroccured = !pParser->Parse(m_pBuffer, sFilename, nFileType, m_bVerbose);
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
        RestoreBufferGccC(m_pBuffer, false);
    }
    else
    {
        m_nFiles++;
        Verbose("C::Import(%s) does not require a new parser, increment file count to %d\n",
            sFilename.c_str(), m_nFiles);
        nRet = 1;
    }

    // everything alright, continue
    return nRet;
}

/** \brief parses an IDL file and inserts its elements into the pFEFile
 *  \param scan_buffer the buffer of the scanner to use
 *    \param sFilename the name of the file to parse
 *  \param nIDL indicates the type of the IDL (DCE/CORBA)
 *  \param bVerbose true if verbose output should be written
 *    \param bPreProcessOnly true if we stop after pre-processing
 *  \return true if successful
 *
 * Performs the following steps:
 * -# call PreProcess -> opens input file
 * -# create an FEFile
 * -# add FEFile to current file and set current file
 * -# set gLineNumber
 * -# init the scanner (set buffer to zero, set input file, call restart)
 * -# call parse method
 * -# close input file
 */
bool CCParser::Parse(void *scan_buffer, string sFilename, int nIDL, bool bVerbose, bool bPreProcessOnly)
{
//     TRACE("C::Parse(%p, %s, %d, %s, %s) called\n", scan_buffer,
//         sFilename.c_str(), nIDL, bVerbose?"true":"false",
//         bPreProcessOnly?"true":"false");

    m_bVerbose = bVerbose;
    m_nInputFileType = nIDL;

    // 1. call preprocess -> opens input file
    CPreProcess *pPreProcess = CPreProcess::GetPreProcessor();
    FILE *fInput = 0;
    if (!scan_buffer)
    {
        fInput = pPreProcess->PreProcess(sFilename, false, bVerbose);
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

    // cannot use GetCurrentIncludePath, because it relies on OpenFile setting
    // the current include path. This is only done if PreProcess and therefore
    // OpenFile is called.
    string sPath;
    if (!scan_buffer)
        sPath = pPreProcess->GetCurrentIncludePath();
    else
        sPath = pPreProcess->FindPathToFile(sFilename, gLineNumber);
    bool bIsStandard = pPreProcess->IsStandardInclude(sFilename, gLineNumber);
    string sOrigName = pPreProcess->GetOriginalIncludeForFile(sFilename, gLineNumber);
    // if path is set and origname is empty, then sNewFileNameGccC is with full path
    // and FindPathToFile returned an include path that matches the beginning of the
    // string. Now we get the original name by cutting off the beginning of the string,
    // which is the path
    if (!sPath.empty() && sOrigName.empty())
        sOrigName = sFilename.substr(sPath.length());
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
    if (m_bVerbose)
    {
        gcc_cdebug = 1;
        gcc_c_flex_debug = 1;
    }
    else
    {
        gcc_cdebug = 0;
        gcc_c_flex_debug = 0;
    }

    if (!scan_buffer)
    {
        gcc_cin = fInput;
        StartBufferGccC(fInput);
    }
    else
        RestoreBufferGccC(scan_buffer, true);

    // 6. call gcc_cparse
    Verbose("Import C header file (%d).\n", nIDL);
    if (gcc_cparse())
    {
        erroccured = true;
        return false;
    }
    Verbose("... finished parsing file.\n");

    // flush buffer state
    GetCurrentBufferGccC();

    // 7. close file
    if (fInput)
        fclose(fInput);

    // switch current file
    CParser::SetCurrentFileParent();

    return true;
}
