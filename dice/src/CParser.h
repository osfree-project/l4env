/**
 *  \file    dice/src/CParser.h
 *  \brief   contains the declaration of the class CParser
 *
 *  \date    Mon Jul 22 2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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
#include <cstdio>
#include <cstring> // for std::memset
#include "Compiler.h" // FrontEnd_Type

class CFEFile;

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
    static CParser* CreateParser(FrontEnd_Type nFileType);

    // used by CCompiler, parser, scanner
    static CParser* GetCurrentParser();
    // used by CParser-derived, CCompiler
    static CParser* SetCurrentParser(CParser *pParser);

    // used by scanner, parser, CParser-derived
    static void SetCurrentFile(CFEFile *pFEFile);
    static void SetCurrentFileParent();
    static CFEFile* GetCurrentFile();

    /** \brief parses an IDL file and inserts its elements into the pFEFile
     *  \param scan_buffer the buffer to use for scanning
     *  \param sFilename the name of the file to parse
     *  \param nIDL indicates the type of the IDL (DCE/CORBA)
     *  \param bPreProcessOnly true if parser should staop after preprocessing
     *  \return true if successful
     */
    virtual bool Parse(void *scan_buffer, string sFilename, FrontEnd_Type nIDL,
	    bool bPreProcessOnly = false) = 0;
    virtual bool DoEndImport();
    /** \brief imports a file
     *  \param sFilename the name of the file
     *  \return 0 if something went wrong and the parser should terminate
     */
    virtual unsigned char Import(string sFilename) = 0;

    virtual CFEFile *GetTopFileInScope(); // used by scanner, parser

    virtual void UpdateState(string sFileName, int nLineNumber);

    /** \brief sets parent of parser
     *  \param pParser the parent parser
     */
    void SetParent(CParser *pParser)
    { m_pParent = pParser; }

protected: // Protected methods
    bool CopyFile(FILE *fInput, FILE *fOutput);
    FrontEnd_Type DetermineFileType(string sExt);

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
    /** \var int m_nInputFileType
     *  \brief defines the IDL parser (DCE or CORBA)
     */
    FrontEnd_Type m_nInputFileType;
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
     *  \brief a reference to the top level file object in the scope of this \
     *         parser
     */
    CFEFile *m_pFEFile;
    /** \var int m_nLineNumber
     *  \brief contains the line-number where the scan of the file was \
     *         interrupted
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
