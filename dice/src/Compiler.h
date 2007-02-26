/**
 *	\file	dice/src/Compiler.h 
 *	\brief	contains the declaration of the class CCompiler
 *
 *	\date	01/31/2001
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

/** preprocessing symbol to check header file */
#ifndef __DICE_COMPILER_H__
#define __DICE_COMPILER_H__

#include <stdio.h>
#include "defines.h"
#include "CString.h"

#include "be/BEContext.h" // needed for ProgramOptionType

class CBERoot;
class CFEBase;
class CFEFile;
class CFELibrary;
class CFEInterface;
class CFEOperation;

#define USE_FE_NONE		0x0	/**< defines that no front-end is used */
#define USE_FE_DCE		0x1	/**< defines that the DCE front-end is used */
#define USE_FE_CORBA	0x2	/**< defines that the CORBA front-end is used */

/**
 *	\class CCompiler
 *	\brief is the place where all the problems start ;)
 *
 * This class contains the "logic" of the compiler. The main function creates an object
 * of this class and calls its methods.
 */
class CCompiler
{
  public:
	/** creates a compiler object */
    CCompiler();
    virtual ~ CCompiler();

// Operations
  public:
    virtual void Write();
    virtual void PrepareWrite();
    virtual void ShowHelp();
    virtual void ShowShortHelp();
    virtual void ShowCopyright();
    virtual void ParseArguments(int argc, char *argv[]);
    static void Error(const char *sMsg, ...);
    static void Warning(const char *sMsg, ...);
    static void GccError(CFEBase * pFEObject, int nLinenb, const char *sMsg, ...);
    static void GccErrorVL(CFEBase * pFEObject, int nLinenb, const char *sMsg, va_list vl);
    static void GccWarning(CFEBase * pFEObject, int nLinenb, const char *sMsg, ...);
    static void GccWarningVL(CFEBase * pFEObject, int nLinenb, const char *sMsg, va_list vl);
	virtual void Parse();

  protected:
    virtual void Optimize();
    virtual void Verbose(const char *sMsg, ...);
    virtual void ShowVersion();
    virtual void PrintDependencyTree(FILE * output, CFEFile * pFEFile);
    virtual void PrintDependencies();
    virtual void PrintGeneratedFiles(FILE * output, CFEFile * pFEFile);
    virtual void PrintGeneratedFiles4File(FILE * output, CFEFile * pFEFile);
    virtual void PrintGeneratedFiles4Library(FILE * output, CFEFile * pFEFile);
    virtual void PrintGeneratedFiles4Library(FILE * output, CFELibrary * pFELibrary);
    virtual void PrintGeneratedFiles4Library(FILE * output, CFEInterface * pFEInterface);
    virtual void PrintGeneratedFiles4Interface(FILE * output, CFEFile * pFEFile);
    virtual void PrintGeneratedFiles4Interface(FILE * output, CFELibrary * pFEpFELibrary);
    virtual void PrintGeneratedFiles4Interface(FILE * output, CFEInterface * pFEInterface);
    virtual void PrintGeneratedFiles4Operation(FILE * output, CFEFile * pFEFile);
    virtual void PrintGeneratedFiles4Operation(FILE * output, CFELibrary * pFELibrary);
    virtual void PrintGeneratedFiles4Operation(FILE * output, CFEInterface * pFEInterface);
    virtual void PrintGeneratedFiles4Operation(FILE * output, CFEOperation * pFEOperation);
    virtual void PrintDependentFile(FILE * output, String sFileName);
    virtual void AddSymbol(const char *sNewSymbol);
    virtual void AddSymbol(String sNewSymbol);

// Attributes
  protected:
	/**	\var int m_nVerboseLevel
	 *	\brief the verbose level
	 */
    int m_nVerboseLevel;
	/**	\var int m_nOptimizeLevel
	 *	\brief the optimization level
	 */
    int m_nOptimizeLevel;
    /** \var unsigned long m_nWarningLevel
     *  \brief the warning level
     */
    unsigned long m_nWarningLevel;
	/**	\var int m_nUseFrontEnd
	 *	\brief contains the front-end to use
	 */
    int m_nUseFrontEnd;
	/**	\var CBERoot* m_pRootBE
	 *	\brief the root point of the back end
	 */
    CBERoot *m_pRootBE;
    /** \var CFEFile *m_pRootFE
     *  \brief a reference to the top most file of the front-end
     */
    CFEFile *m_pRootFE;
	/**	\var CBEContext* m_pContext
	 *	\brief the context of all write operations of the back-end
	 */
    CBEContext *m_pContext;
	/**	\var String m_sIncludePrefix
	 *	\brief the include prefix, which is used to prefix each include statement
	 */
    String m_sIncludePrefix;
	/**	\var String m_sFilePrefix
	 *	\brief the file prefix, which all files are supposed to receive
	 */
    String m_sFilePrefix;
	/**	\var String m_sOutFileName
	 *	\brief the name of the output file
	 */
    String m_sOutFileName;
	/**	\var String m_sInFileName
	 *	\brief the name of the input file, NULL if stdin is used
	 */
    String m_sInFileName;
	/**	\var ProgramOptionType m_nOptions
	 *	\brief the options which are specified with the compiler call
	 */
    ProgramOptionType m_nOptions;
    /** \var unsigned m_nBackEnd
     *  \brief the back-end, which are specified with the compiler call
     */
    unsigned m_nBackEnd;
    /** \var int m_nOpcodeSize
     *  \brief contains the size of the opcode type in bytes
     */
    int m_nOpcodeSize;
	/**	\var bool m_bVerbose
	 *	\brief is the same as (m_nOptions & PROGRAM_VERBOSE)
	 */
    bool m_bVerbose;
	/**	\var int m_nCurCol
	 *	\brief is the current column for dependency output
	 */
    int m_nCurCol;
    /** \var String m_sIncludePaths[MAX_INCLUDE_PATHS]
     *  \brief contains the include paths
     */
    String m_sIncludePaths[MAX_INCLUDE_PATHS];
    /** \var String m_sInitRcvStringFunc
     *  \brief contains the name of the init-rcv-string function
     */
    String m_sInitRcvStringFunc;
    /** \var String m_sTraceClientFunc
     *  \brief contains the name of the Trace function for the client
     */
    String m_sTraceClientFunc;
    /** \var String m_sTraceServerFunc
     *  \brief contains the name of the Trace function for the server
     */
    String m_sTraceServerFunc;
    /** \var String m_sTraceMsgBuf
     *  \brief contains the name of the Trace function for the message buffer
     */
    String m_sTraceMsgBuf;
	/** \var int m_nSymbolCount
	 *  \brief number of currently defined symbols
	 */
	int m_nSymbolCount;
	/** \var char **m_sSymbols
	 *  \brief list of symbols defined when calling dice
	 */
	char **m_sSymbols;
	/** \var int m_nDumpMsgBufDwords
	 *  \brief contains the number of dwords to dump
	 */
	int m_nDumpMsgBufDwords;
};

#endif				// __DICE_COMPILER_H__
