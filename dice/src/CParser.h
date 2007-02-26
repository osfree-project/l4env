/**
 *    \file    dice/src/CParser.h
 *    \brief   contains the declaration of the class CParser
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

/** preprocessing symbol to check header file */
#ifndef PARSER_H
#define PARSER_H

#include <string>
using namespace std;
#include <stdio.h>
#include <string.h> // needed for memset

class CFEFile;

#define USE_FILE_C      (USE_FE_CORBA + 1)  /**< defines that a C header file is currently parsed */
#define USE_FILE_CXX    (USE_FE_CORBA + 2)  /**< defines that a C++ header file is currently parsed */

/** \class CParser
 *  \ingroup frontend
 *  \brief encapsulates the parser calls
 */
class CParser
{
protected:
    /** creates a new parser object */
    CParser();

public:
    virtual ~CParser();

public: // Public methods
    static CParser* CreateParser(int nFileType);

    static CParser* GetCurrentParser(); // used by CCompiler, parser, scanner
    static CParser* SetCurrentParser(CParser *pParser); // used by CParser-derived, CCompiler

    static void SetCurrentFile(CFEFile *pFEFile); // used by scanner, parser, CParser-derived
    static void SetCurrentFileParent(); // used by scanner, parser, CParser-derived
    static CFEFile* GetCurrentFile(); // used by scanner, parser, CParser-derived

    virtual bool Parse(void *scan_buffer, string sFilename, int nIDL, bool bVerbose, bool bPreProcessOnly = false);
    virtual bool DoEndImport();
    virtual unsigned char Import(string sFilename); // used by parser

    virtual CFEFile *GetTopFileInScope(); // used by scanner, parser
    virtual bool PrepareEnvironment(string sFilename, FILE*& fIn, FILE*& fOut); // used by preprocessing
    virtual void FinishEnvironment(); // used by parser

    virtual void UpdateState(string sFileName, int nLineNumber);
    void SetParent(CParser *pParser)
    { m_pParent = pParser; }

protected: // Protected methods
    virtual void Verbose(const char *sMsg, ...);
    bool CopyFile(FILE *fInput, FILE *fOutput);

protected: // Protected attributes
    /** \var CParser *m_pCurrentParser
     *  \brief a reference to the current paser
     */
    static CParser* m_pCurrentParser;
    /** \var CFEFile *m_pCurrentFile
     *  \brief a reference to the current FE file to add data to
     */
    static CFEFile* m_pCurrentFile;
    /** \var bool m_bFirstRun
     *  \brief true if the parser is run the firt time
     */
    static bool m_bFirstRun;
    /** \var bool m_bVerbose
     *  \brief true if we should print verbose output
     */
    bool m_bVerbose;
    /** \var int m_nInputFileType
     *  \brief defines the IDL parser (DCE or CORBA)
     */
    int m_nInputFileType;
    /** \var void* m_pBuffer
     *  \brief is the pointer to the saved scan-buffer
     */
    void *m_pBuffer;
    /** \var string m_sFilename
     *  \brief contains the filename of the file
     */
    string m_sFilename;
    /** \var string m_sPrevTopLevelFileName
     *  \brief contains the top level file-name of the previos level
     */
    string m_sPrevTopLevelFileName;
    /** \var CFEFile *m_pFEFile
     *  \brief a reference to the top level file object in the scope of this parser
     */
    CFEFile *m_pFEFile;
    /** \var int m_nLineNumber
     *  \brief contains the line-number where the scan of the file was interrupted
     */
    int m_nLineNumber;
    /** \var int m_c_inc
     *  \brief is 1 if it is a C header file, 0 if IDL header file
     */
    int m_c_inc;
    /** \var int m_nFiles
     *  \brief number of new files in the same parser
     */
    int m_nFiles;
    /** \var CParser *m_pParent
     *  \brief reference to outer parser
     */
    CParser *m_pParent;
};

#endif
