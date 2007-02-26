/**
 *	\file	dice/src/CCORBAParser.cpp
 *	\brief	contains the implementation of the class CCORBAParser
 *
 *	\date	Sun Jul 27 2003
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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
#include "CCORBAParser.h"
#include "CPreProcess.h"
#include "Compiler.h"
#include "fe/FEFile.h"

//@{
/** global variables and function of the CORBA parser */
extern FILE *corbain;
extern int corbadebug;
extern int corba_flex_debug;
extern int corbaparse();
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
extern int c_inc_old;

/** the name of the input file */
extern String sInFileName;
/** the name of the include path */
extern String sInPathName;
/** back up name of the top level input file - we need this when scanning included files */
extern String sTopLevelInFileName;

CCORBAParser::CCORBAParser()
 : CParser()
{
}

CCORBAParser::~CCORBAParser()
{
}

/** \brief imports a file
 *  \param sFilename the name of the file
 *  \return false if something went wrong and the parser should terminate
 */
unsigned char CCORBAParser::Import(String sFilename)
{
    // 1. save state
	// - scanner's buffer
	// - line number
	// - c_inc
	// - sTopLevelInFileName
	// - sFilename
	m_pBuffer = GetCurrentBufferCorba();
	m_nLineNumber = gLineNumber;
	m_c_inc = c_inc;
	m_sPrevTopLevelFileName = sTopLevelInFileName;
	m_sFilename = sInFileName;

	// 2. determine file type
    // set c_inc to indicate if this is a C header file or not
    int iDot = sFilename.ReverseFind('.');
	int nFileType = m_nInputFileType;
    if (iDot > 0)
    {
        String sExt = sFilename.Mid (iDot + 1);
        sExt.MakeLower();
		if (sExt == "h")
		    nFileType = USE_FILE_C;
		if ((sExt == "hh") ||
		    (sExt == "H") ||
			(sExt == "hpp") ||
			(sExt == "hxx"))
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
	    Verbose("CORBA::Import(%s) requires a new parser (%d)\n", (const char*)sFilename, nFileType);
		// 3. create new Parser
		CParser *pParser = CParser::CreateParser(nFileType);
		CParser *pOldParser = CParser::SetCurrentParser(pParser);
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
		gLineNumber = m_nLineNumber;
		c_inc = m_c_inc;
		sTopLevelInFileName = m_sPrevTopLevelFileName;
		sInFileName = m_sFilename;
		RestoreBufferCorba(m_pBuffer, true);
	}
	else
	{
	    m_nFiles++;
	    Verbose("CORBA::Import(%s) does not require a new parser, increment file count to %d\n",
		    (const char*)sFilename, m_nFiles);
		nRet = 1;
	}

	// everything alright, continue
    return nRet;
}


/** \brief parses an IDL file and inserts its elements into the pFEFile
 *  \param scan_buffer the buffer of the scanner to use
 *  \param nIDL indicates the type of the IDL (DCE/CORBA)
 *  \param bVerbose true if verbose output should be written
 *  \return true if successful
 */
bool CCORBAParser::Parse(void *scan_buffer, String sFilename, int nIDL, bool bVerbose, bool bPreProcessOnly)
{
//     TRACE("CORBA::Parse(%p, %s, %d, %s, %s) called\n", scan_buffer,
// 	    (const char*)sFilename, nIDL, bVerbose?"true":"false",
// 		bPreProcessOnly?"true":"false");

    m_bVerbose = bVerbose;
    m_nInputFileType = nIDL;
	bool bFirst = (scan_buffer == 0);

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
	String sPath = pPreProcess->GetCurrentIncludePath();
    bool bIsStandard = pPreProcess->IsStandardInclude(sFilename, gLineNumber);
	String sOrigName = pPreProcess->GetOriginalIncludeForFile(sFilename, gLineNumber);
	if (sOrigName.IsEmpty())
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
		corbadebug = 1;
		corba_flex_debug = 1;
	}
	else
	{
		corbadebug = 0;
		corba_flex_debug = 0;
	}

	if (!scan_buffer)
	{
		corbain = fInput;
		StartBufferCorba(fInput);
    }
	else
		RestoreBufferCorba(scan_buffer, false);

	// 6. call gcc_cparse
	Verbose("Import CORBA IDL file (%d).\n", nIDL);
	if (corbaparse())
	{
		erroccured = true;
		return false;
	}
    Verbose("... finished parsing file.\n");

	// flush buffer state
	GetCurrentBufferCorba();

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
