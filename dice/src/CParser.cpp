/**
 *	\file	dice/src/Parser.cpp
 *	\brief	contains the implementation of the class CParser
 *
 *	\date	Mon Jul 22 2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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
#include "fe/FEFile.h"
#include "Compiler.h"
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>

/** some external config variables */
extern const char* dice_configure_cpp;
extern const char* dice_compile_cpp;

/////////////////////////////////////////////////////////////////////////////////
// error handling
//@{
/** globale variables used by the parsers to count errors and warnings */
extern int errcount;
extern int erroccured;
extern int warningcount;
//@}

/**	debugging helper variable */
extern int nGlobalDebug;

//@{
/** globale pre-processor variables and function */
extern FILE *incin,*incout;
extern int inclex();
extern int nLineNbInc;
//@}

//@{
/** global variables and function of the DCE parser */
extern int nLineNbDCE;
extern int nIncludeLevelDCE;
extern FILE *dcein;
extern int dcedebug;

extern int dceparse();
//@}

//@{
/** global variables and function of the CORBA parser */
extern int nLineNbCORBA;
extern int nIncludeLevelCORBA;
extern FILE *corbain;
extern int corbadebug;

extern int corbaparse();
//@}

/** the name of the input file */
String sInFileName;
/** the name of the include path */
String sInPathName;
/** back up name of the top level input file - we need this when scanning included files */
String sTopLevelInFileName;

/////////////////////////////////////////////////////////////////////////////////
// file elements
/** a reference to the currently parsed file */
CFEFile *pCurFile;

CParser* CParser::m_pCurrentParser = 0;

CParser::CParser()
{
	m_pParent = 0;
    m_sCPPArgs = 0;
    m_nCPPArgCount = 0;
    char *sCPP = getenv("CPP");
    if (sCPP)
      m_sCPPProgram = strdup(sCPP);
    else if (TestCPP())
      m_sCPPProgram = strdup("cpp");
    else if (dice_configure_cpp)
      m_sCPPProgram = strdup(dice_configure_cpp);
    else if (dice_compile_cpp)
      m_sCPPProgram = strdup(dice_compile_cpp);
    else
      m_sCPPProgram = strdup("cpp");
    AddCPPArgument(m_sCPPProgram);
    // 1. argument: "-E"
    AddCPPArgument("-E");
    AddCPPArgument("-D__GNUC__");
    //AddCPPArgument("-P");
    for (int i=0; i<MAX_INCLUDE_PATHS; i++)
        m_sIncludePaths[i].Empty();
    m_nCurrentIncludePath = -1;
    m_nIDLType = 0;
    m_pBufferHead = m_pBufferTail = 0;
}

/** cleans up the parser object */
CParser::~CParser()
{
    for (int i=0; i<m_nCPPArgCount; i++)
        free(m_sCPPArgs[i]);
    if (m_fInput)
        fclose(m_fInput);
    if (m_sCPPProgram)
        free(m_sCPPProgram);
}

/** \brief parses an IDL file and inserts its elements into the pFEFile
 *  \param sFilename the name of the IDL file
 *  \param nIDL indicates the type of the IDL (DCE/CORBA)
 *  \param pFEFile the file to insert the elements to
 *  \param bVerbose true if verbose output should be written
 *  \return true if successful
 */
bool CParser::Parse(String sFilename, int nIDL, CFEFile *pFEFile, bool bVerbose)
{
    m_bVerbose = bVerbose;
    m_nIDLType = nIDL;

    if (!PreProcess(sFilename, false))
    {
		erroccured = true;
		errcount++;
		return false;
	}

    // if sFilename contains a path, add it to include paths
    int nPos;
    if ((nPos = sFilename.ReverseFind('/')) >= 0)
		AddIncludePath(sFilename.Left(nPos+1));

    // set new current file
    if (pCurFile != 0)
		pCurFile->AddChild(pFEFile);
    pCurFile = pFEFile;

    Verbose("Start parsing of input file ...\n");
    switch (m_nIDLType)
    {
    case USE_FE_DCE:
        Verbose("Run in DCE mode.\n");
        dcein = m_fInput;
        if (m_bVerbose)
            dcedebug = 1;
        if (dceparse())
        {
            if (m_fInput)
            {
                fclose(m_fInput);
                m_fInput = 0;
            }
            erroccured = true;
			return false;
        }
        break;
    case USE_FE_CORBA:
        Verbose("Run in CORBA mode.\n");
        corbain = m_fInput;
        if (m_bVerbose)
            corbadebug = 1;
        if (corbaparse())
        {
            if (m_fInput)
            {
                fclose(m_fInput);
                m_fInput = 0;
			}
            erroccured = true;
			return false;
        }
        break;
    case USE_FE_NONE:
	default:
            erroccured = true;
			errcount++;
			return false;
        break;
    }
    Verbose("... finished parsing input file.\n");

    // this is top level file, close it
    if (m_fInput)
    {
       fclose(m_fInput);
       m_fInput = 0;
    }

    // print error messages after finish of top level file
    if (erroccured)
    {
        if (errcount > 0)
            CCompiler::Error("%d Error(s) and %d Warning(s) occured.", errcount, warningcount);
        else
            CCompiler::Warning("%s: warning: %d Warning(s) occured.", (const char *) sFilename, warningcount);
    }
    return true;
}

/** \brief pre-process the file
 *  \param sFilename the filename of the file to pre-process
 *  \param bDefault true if the file to pre-process is a default file
 *  \return true if successful
 *
 * First we scan for include/import, and then we run CPP.
 */
bool CParser::PreProcess(String sFilename, bool bDefault)
{
    FILE *fInput = OpenFile(sFilename, bDefault);
    if (!fInput)
        return false; // open file already printed error message

    sInFileName = sFilename;
    if (sInFileName.IsEmpty())
        sInFileName = "<stdin>";
    // we add the current path to the file-name so it conformes to the stored file-name
    // generated by Gcc
    if (m_nCurrentIncludePath >= 0)
        sTopLevelInFileName = m_sIncludePaths[m_nCurrentIncludePath] + sFilename;
    else
        sTopLevelInFileName = sFilename;

    Verbose("Start preprocessing input file ...\n");
    // turn debugging on
    if (m_bVerbose)
        nGlobalDebug++;
    // search for import an dinclude statements
    FILE *fOutput = tmpfile();
//    String sFileN = sFilename.Right(sFilename.GetLength()-sFilename.ReverseFind('/')-1);
//    String s = "temp1-" +  sFileN;
//    FILE *fOutput = fopen((const char*)s, "w+");
    if (!fOutput)
    {
        fprintf(stderr, "could not create temporary file\n");
        if (fInput != 0)
            fclose(fInput);
        return false;
    }
    // search fInput for import statements
    incin = fInput;
    incout = fOutput;
    rewind(incin);
    // set line number count to start
    nLineNbInc = 1;
    inclex();
    // if one of the included files is an IDL file,
    // it has to be scanned as well, because itself might
    // include other IDL files, which will not be registered
    // with the Parser.
    // Example:
    // a.idl includes b.idl, which includes c.idl
    // a.idl is preprocessed, and scan reconizes b.idl as included file
    // gcc then preprocesses a.idl and includes b.idl AND c.idl, which is
    // later not recognized as included file.
    inc_bookmark_t *pCurrent;
    for (pCurrent = FirstIncludeBookmark(); pCurrent != 0; pCurrent = pCurrent->pNext)
    {
        // if no file-name, move on
        if (!(pCurrent->pFilename))
            continue;
        // if file name ends on .idl
        String sExtension = pCurrent->pFilename->Right(4);
        sExtension.MakeLower();
        if (sExtension == ".idl")
        {
            // open idl file
            FILE *fScanIn = OpenFile(*(pCurrent->pFilename), false);
            if (!fScanIn)
                return false; // OpenFile already printed error message
            // create temporary output file
            FILE *fScanOut = tmpfile();
            if (!fScanOut)
            {
                fprintf(stderr, "could not create temporary file\n");
                if (fScanIn != 0)
                    fclose(fScanIn);
                return false;
            }
            // scan file
            incin = fScanIn;
            incout = fScanOut;
            rewind(incin);
            nLineNbInc = 1;
            inclex();
            // close files
            fclose(fScanOut);
            fclose(fScanIn);
            // we don't need to reset bookmark, because
            // new includes will be added last, which is behind current
        }
    }

    // turn debugging off
    if (m_bVerbose)
        nGlobalDebug--;
    // close input of preprocess
    if (!sFilename.IsEmpty())	// _not_ stdin
        fclose(fInput);

    // now all includes and imports are scanned

    // get new temp file
    fInput = tmpfile();
//    s = "temp2-" + sFileN;
//    fInput = fopen((const char*)s, "w+");
    if (!fInput)
    {
        fprintf(stderr, "could not create temporary file\n");
        if (fOutput != 0)
            fclose(fOutput);
        return false;
    }
    // set input file handle to beginning of file
    rewind(fOutput);

    // run cpp preprocessor
    int iRet;
    if ((iRet = ExecCPP(fOutput, fInput)) > 0)
    {
        fprintf(stderr, "could not preprocess input file \"%s\" (returned %d).\n", (const char *) sInFileName, iRet);
        return false;
    }
    // close input file of cpp scanner
    fclose(fOutput);

    Verbose("... finished preprocessing input file.\n");

    // set input to beginning of file again
    fseek(fInput, 0, SEEK_SET);

    m_fInput = fInput;
    if (!m_fInput)
        CCompiler::Error("No input file.\n");

    return true;
}

/** \brief runs the C pre-processor
 *  \param fInput the input file
 *  \param fOutput the output file
 *  \return an error code if something went wrong, zero (0) otherwise
 */
int CParser::ExecCPP(FILE *fInput, FILE* fOutput)
{
     int pipes[2];
     if (pipe(pipes) == -1)
         return 1;

     int pid = fork();
     int status;
     if (pid == -1)
         return 2;

     // in child process call cpp
     if (pid == 0)
     {
         close(pipes[0]);
         // adjust stdin
         fclose(stdin);
         dup(fileno(fInput));
         // adjust stdout
         fclose(stdout);
         dup(fileno(fOutput));
         // child -> run cpp
         // last argument: the file (stdin)
         // function sets last argument always to 0
         AddCPPArgument("-");
         execvp(m_sCPPProgram, m_sCPPArgs);
         CPPErrorHandling();
         return 3;
     }
     // parent -> wait for cpp
     waitpid(pid, &status, 0);

     return status;
}

/** \brief does the error recognition and handling if CPP failed
 *
 * This function is called in the child process and only if CPP could not be started.
 * The child process will return with value 3 if something went wrong.
 */
void CParser::CPPErrorHandling()
{
    switch (errno)
    {
    case EACCES:
        CCompiler::Error("execvp(\"cpp\", cpp_args) returned error EACCES\n");
        break;
    case EPERM:
        CCompiler::Error("execvp(\"cpp\", cpp_args) returned error EPERM\n");
        break;
    case E2BIG:
        CCompiler::Error("execvp(\"cpp\", cpp_args) returned error E2BIG\n");
        break;
    case ENOEXEC:
        CCompiler::Error("execvp(\"cpp\", cpp_args) returned error ENOEXEC\n");
        break;
    case EFAULT:
        CCompiler::Error("execvp(\"cpp\", cpp_args) returned error EFAULT\n");
        break;
    case ENAMETOOLONG:
        CCompiler::Error("execvp(\"cpp\", cpp_args) returned error ENAMETOOLONG\n");
        break;
    case ENOENT:
        CCompiler::Error("execvp(\"cpp\", cpp_args) returned error ENOENT(%d)\n", ENOENT);
        break;
    case ENOMEM:
        CCompiler::Error("execvp(\"cpp\", cpp_args) returned error ENOMEM\n");
        break;
    case ENOTDIR:
        CCompiler::Error("execvp(\"cpp\", cpp_args) returned error ENOTDIR\n");
        break;
    case ELOOP:
        CCompiler::Error("execvp(\"cpp\", cpp_args) returned error ELOOP\n");
        break;
    case ETXTBSY:
        CCompiler::Error("execvp(\"cpp\", cpp_args) returned error ETXTBUSY\n");
        break;
    case EIO:
        CCompiler::Error("execvp(\"cpp\", cpp_args) returned error EIO\n");
        break;
    case ENFILE:
        CCompiler::Error("execvp(\"cpp\", cpp_args) returned error ENFILE\n");
        break;
    case EMFILE:
        CCompiler::Error("execvp(\"cpp\", cpp_args) returned error EMFILE\n");
        break;
    case EINVAL:
        CCompiler::Error("execvp(\"cpp\", cpp_args) returned error EINVAL\n");
        break;
    case EISDIR:
        CCompiler::Error("execvp(\"cpp\", cpp_args) returned error EISDIR\n");
        break;
#if !defined(_ALL_SOURCE)        
    case ELIBBAD:
        CCompiler::Error("execvp(\"cpp\", cpp_args) returned error ELIBBAD\n");
        break;
#endif        
    default:
        CCompiler::Error("execvp(\"cpp\", cpp_args) returned an unknown error\n");
        break;
    }
}

/** \brief adds another C pre-processor argument
 *  \param sNewArgument the new argument
 */
void CParser::AddCPPArgument(String sNewArgument)
{
    if (sNewArgument.IsEmpty())
        return;
    AddCPPArgument((const char*)sNewArgument);
}

/** \brief adds another C pre-processor argument
 *  \param sNewArgument the new argument
 */
void CParser::AddCPPArgument(const char* sNewArgument)
{
    if (!sNewArgument)
        return;
    if (!m_sCPPArgs)
        m_nCPPArgCount = 2;
    else
        m_nCPPArgCount++;
    m_sCPPArgs = (char **) realloc(m_sCPPArgs, m_nCPPArgCount * sizeof(char *));
    m_sCPPArgs[m_nCPPArgCount - 2] = strdup(sNewArgument);
    m_sCPPArgs[m_nCPPArgCount - 1] = 0;
}

/** \brief prints a message if verbose mode is on
 *  \param sMsg the message
 */
void CParser::Verbose(char *sMsg, ...)
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
void CParser::SetCurrentParser(CParser *pParser)
{
	m_pCurrentParser = pParser;
}

/** \brief allows access to the cpp arguments
 *  \return a reference to the arguments
 */
char ** CParser::GetCPPArguments()
{
	return m_sCPPArgs;
}

/** \brief tries to open a file (and use the search paths)
 *  \param sName the name of the file
 *  \param bDefault true if the file is a deafult file ('<' file '>')
 *  \return a file descriptior if file found
 */
FILE* CParser::OpenFile(String sName, bool bDefault)
{
	String sCurPath;
	if (m_nCurrentIncludePath < 0)
		sCurPath.Empty();
	else
		sCurPath = m_sIncludePaths[m_nCurrentIncludePath];
    /* try to open included file */
    if (m_bVerbose)
    {
        if (sCurPath.IsEmpty())
            printf("try to open include \"%s\"\n",(const char*)sName);
        else
            printf("try to open include \"%s\" in path \"%s\"\n",(const char*)sName,(const char*)sCurPath);
    }
    FILE *fReturn = 0;
    if (sCurPath.IsEmpty() || bDefault)
    {
        // check if this is a file
        if (CheckName(sName))
            fReturn = NULL;
        else
            fReturn = fopen((const char*)sName, "r");
    }
    else
    {
        String s = sCurPath + sName;
        if (CheckName(s))
            fReturn = NULL;
        else
            fReturn = fopen((const char*)s, "r");
    }
    if ((fReturn != 0) && m_bVerbose)
        printf("success\n");

    if (!fReturn)
    {
        /* not in current directory -> search paths */
        int i = 0;
        while (!fReturn && (i < MAX_INCLUDE_PATHS))
        {
            if (!(m_sIncludePaths[i].IsEmpty()))
            {
                if (m_bVerbose)
                    printf("Search file in path \"%s\".\n", (const char*)m_sIncludePaths[i]);
                String s = m_sIncludePaths[i];
                if (s.Right(1) != "/")
                    s += "/";
                s += sName;
                if (CheckName(s) == 0)
                {
                    m_nCurrentIncludePath = i;
                    if (m_bVerbose)
                        printf("try to open with path %s\n",(const char*)s);
                    // it's there
                    fReturn = fopen((const char*)s, "r");
                }
            }
            i++;
        }
    }
    else
        m_nCurrentIncludePath = -1;

    if (m_bVerbose)
        printf("Found file \"%s\" in path \"%s\".\n", (const char*)sName, (const char*)sCurPath);

    if (!fReturn)
    {
        if (m_bVerbose)
            printf("Couldn't find file\n");
        if (!pCurFile)
        {
			fprintf(stderr, "dice: %s: No such file or directory.\n", (const char*)sName);
			return 0;
		}
        /* if not open by now, couldn't find file */
        if (pCurFile->GetParent() != 0)
        {
            // run to top
            CFEFile *pFile = (CFEFile*)pCurFile->GetParent();
            Vector vStack(RUNTIME_CLASS(CFEFile));
            while (pFile != 0)
            {
				vStack.AddHead(pFile);
				pFile = (CFEFile*)pFile->GetParent();
			}
            // down in line
            fprintf(stderr, "In file included ");
            VectorElement *pIter = vStack.GetFirst();
            while (pIter != 0)
            {
				CFEFile *pFEFile = (CFEFile*)(pIter->GetElement());
				// get line number
				int nLine = 1;
                String sFileName = pFEFile->GetFullFileName();
                if (sFileName.IsEmpty())
                    sFileName = sTopLevelInFileName;
				if (pFEFile->GetParent() != 0)
					nLine = FindLineNbOfInclude(sFileName, ((CFEFile*)(pFEFile->GetParent()))->GetFullFileName());
				else
					nLine = FindLineNbOfInclude(sFileName, pFEFile->GetFullFileName());
                fprintf(stderr, "from %s:%d", (const char*)sFileName, nLine);
                if (pIter->GetNext() != 0)
                    fprintf(stderr, ",\n                 ");
                else
                    fprintf(stderr, ":\n");
                pIter = pIter->GetNext();
            }
        }
        String sFileName = pCurFile->GetFullFileName();
        if (sFileName.IsEmpty())
            sFileName = sTopLevelInFileName;
		int nLine = FindLineNbOfInclude(sName, sFileName);
        fprintf(stderr, "%s:%d: %s: No such file or directory.\n", (const char*)sFileName, nLine, (const char*)sName);
		return 0;
    }

    if (m_nCurrentIncludePath >= 0)
    {
        if (!m_sIncludePaths[m_nCurrentIncludePath].IsEmpty())
        {
            if (m_sIncludePaths[m_nCurrentIncludePath].Right(1) != "/")
                m_sIncludePaths[m_nCurrentIncludePath] += "/";
        }
    }

    /* switch to buffer */
	return (fReturn != 0)? fReturn : stdin;
}

/** \brief adds a include path
 *  \param sPath the path to add
 *  \return the index of the new path, or -1 if max limit reached
 */
int CParser::AddIncludePath(String sPath)
{
	// search for last
	int nCurrent = 0;
	while (!m_sIncludePaths[nCurrent].IsEmpty()) nCurrent++;
	if (nCurrent < MAX_INCLUDE_PATHS)
	{
        // add trailing slash if not present
        if (sPath.Right(1) != "/")
            sPath += "/";
		m_sIncludePaths[nCurrent] = sPath;
		return nCurrent;
	}
	return -1;
}

/** \brief adds a include path
 *  \param sNewPath the path to add
 *  \return the index of the new path, or -1 if max limit reached
 */
int CParser::AddIncludePath(const char* sNewPath)
{
    return AddIncludePath(String(sNewPath));
}

/** indicates if the current file is a C header */
extern int c_inc;

/** \brief imports a file
 *  \param sFilename the name of the file
 *  \return false if something went wrong and the parser should terminate
 */
bool CParser::Import(String sFilename)
{
	// create new buffer
	buffer_t *pNew = new_buffer();
	PushBuffer(pNew);
	pNew->pFilename = new String(sFilename);
	pNew->c_inc = c_inc;
	// search for the include statement to find out
	// if this is a default import or not
	bool bDefault = false;
    // save top level file-name
    pNew->pPrevTopLevelFileName = new String(sTopLevelInFileName);
	// pre-process file
    // sets sTopLevelInFileName to sFilename
    if (!PreProcess(sFilename, bDefault))
    {
		// errcount will be increased after above call to dceparse/corbaparse
		erroccured = true;
        errcount++;
        return false;
    }

    // set c_inc to indicate if this is a C header file or not
	// check extension of file
    int iDot = sFilename.ReverseFind('.');
    if (iDot > 0)
    {
        String sExt = sFilename.Mid (iDot + 1);
        sExt.MakeLower();
        if ((sExt == "h") || (sExt == "hh"))
    		c_inc = 1;
        else
    		c_inc = 2;
    }
    else
		c_inc = 2;

	// get path, include level and whether this is a standard include
	int nLineNb = 0, nIncludeLevel = 0;
	switch(m_nIDLType)
	{
   	case USE_FE_DCE:
		nLineNb = nLineNbDCE;
		nIncludeLevel = nIncludeLevelDCE;
		break;
   	case USE_FE_CORBA:
		nLineNb = nLineNbCORBA;
		nIncludeLevel = nIncludeLevelCORBA;
		break;
	}
	String sPath;
	if (m_nCurrentIncludePath >= 0) // set by PreProcess (or rather OpenFile)
		sPath = m_sIncludePaths[m_nCurrentIncludePath];

   	// new file
    bool bIsStandard = IsStandardInclude(sFilename, nLineNb);
   	CFEFile *pFEFile = new CFEFile(sFilename, sPath, nIncludeLevel, bIsStandard);
    // set new current file
    if (pCurFile != 0)
		pCurFile->AddChild(pFEFile);
    pCurFile = pFEFile;

    // save the lexer context
    SaveLexContext();

    // everything alright, continue
    return true;
}

/** \brief does the clean up when an imported file ends
 *  \return true if the parser should terminate
 */
bool CParser::EndImport()
{
	int nIncludeLevel = 0;
    switch (m_nIDLType)
    {
    case USE_FE_DCE:
    	nIncludeLevel = nIncludeLevelDCE;
    	break;
    case USE_FE_CORBA:
    	nIncludeLevel = nIncludeLevelCORBA;
    	break;
    default:
    	break;
    }
	if (nIncludeLevel <= 0)
		return true;
	// restore the lexer context
	RestoreLexContext();
    // restore top level file name
    buffer_t *pCurr = PopBuffer();
    sTopLevelInFileName = *(pCurr->pPrevTopLevelFileName);
	// pop buffer state
	del_buffer(pCurr);
	// switch current file
	if (pCurFile->GetParent() != 0)
		pCurFile = (CFEFile*)pCurFile->GetParent();
	// do not terminate                             
	return false;
}

/** \brief saves the context of the lexer
 */
void CParser::SaveLexContext()
{
	buffer_t* pCurrent = CurrentBuffer();
    pCurrent->nLineNo = 0;
    pCurrent->buffer = 0;
	// save line number
	// switch input buffer
	switch (m_nIDLType)
	{
	case USE_FE_DCE:
		pCurrent->nLineNo = nLineNbDCE;
		pCurrent->buffer = SwitchToFileDce(m_fInput);
		break;
	case USE_FE_CORBA:
		pCurrent->nLineNo = nLineNbCORBA;
		pCurrent->buffer = SwitchToFileCorba(m_fInput);
		break;
	}
}

/** \brief restores the context of the lexer
 */
void CParser::RestoreLexContext()
{
	buffer_t* pCurrent = CurrentBuffer();
	// restore input buffer
	if (pCurrent != 0)
	{
    	switch (m_nIDLType)
    	{
    	case USE_FE_DCE:
			nLineNbDCE = pCurrent->nLineNo;
			RestoreBufferDce(pCurrent->buffer);
    		break;
    	case USE_FE_CORBA:
			nLineNbCORBA = pCurrent->nLineNo;
			RestoreBufferCorba(pCurrent->buffer);
    		break;
    	}
		pCurrent->buffer = 0;
		c_inc = pCurrent->c_inc;
	}
}

/** \brief pushes a new buffer into the list
 *  \param pNew the buffer to push
 */
void CParser::PushBuffer(buffer_t *pNew)
{
	pNew->pNext = m_pBufferHead;
	pNew->pPrev = 0;
	if (m_pBufferHead != 0)
		m_pBufferHead->pPrev = pNew;
	m_pBufferHead = pNew;
	if (!m_pBufferTail)
		m_pBufferTail = pNew;
}

/** \brief pops a buffer from the list
 *  \return a reference to the top-most buffer
 */
buffer_t* CParser::PopBuffer()
{
	if (!m_pBufferHead)
		return 0;
	if (m_pBufferHead->pNext != 0)
		m_pBufferHead->pNext->pPrev = 0;
	buffer_t *tmp = m_pBufferHead;
	m_pBufferHead = m_pBufferHead->pNext;
	tmp->pNext = 0;
	return tmp;
}

/** \brief access the current buffer
 *  \return a reference to the current buffer
 */
buffer_t* CParser::CurrentBuffer()
{
	return m_pBufferHead;
}

/** \brief adds include bookmark to parser
 *  \param sFile the filename of the include statement
 *  \param sFromFile the file where the include statement is located
 *  \param nLineNb the line number of the include statement
 *  \param bImport true if the file is imported
 *  \param bStandard true if the include is a standard include ('<'file'>')
 */
void CParser::AddInclude(String sFile, String sFromFile, int nLineNb, bool bImport, bool bStandard)
{
	inc_bookmark_t* pNew = new_include_bookmark();
	pNew->pFilename = new String(sFile);
	pNew->pFromFile = new String(sFromFile);
	pNew->nLineNb = nLineNb;
	pNew->bImport = bImport;
	pNew->bStandard = bStandard;
	PushIncludeBookmark(pNew);
}

/** \brief tries to find the suitable path for a given file name
 *  \param sFilename the given filename
 *  \param nLineNb
 *  \return the path to the file
 *
 * The function searches the stored include statements to find one, where the stored filename
 * is part of the given filename and the linenumbers match. It also checks if the resulting string
 * is one of the given include paths.
 */
String CParser::FindPathToFile(String sFilename, int nLineNb)
{
	if (sFilename.IsEmpty())
		return String();

	inc_bookmark_t *pCurrent;
	for (pCurrent = FirstIncludeBookmark(); pCurrent != 0; pCurrent = pCurrent->pNext)
	{
		// if no file-name, move on
		if (!(pCurrent->pFilename))
			continue;
		// test line numbers
		if (pCurrent->nLineNb != nLineNb)
			continue;
		// test for file name
		int nPos;
		if ((nPos = sFilename.ReverseFind(*(pCurrent->pFilename))) < 0)
			continue;
		// file name found, now extract path
		String sPath = sFilename.Left(nPos);
		// now search if path is in include paths
		for (int i=0; !m_sIncludePaths[i].IsEmpty(); i++)
		{
			if (m_sIncludePaths[i] == sPath)
				return sPath;
		}
	}
	return String();
}

/** \brief tries to find the original include statement for the given file name
 *  \param sFilename the name to search for
 *  \param nLineNb the line number where the include statemenet appeared on
 *  \return the text of the original include statement.
 */
String CParser::GetOriginalIncludeForFile(String sFilename, int nLineNb)
{
	if (sFilename.IsEmpty())
		return String();

	inc_bookmark_t *pCurrent;
	for (pCurrent = FirstIncludeBookmark(); pCurrent != 0; pCurrent = pCurrent->pNext)
	{
		// if no file-name, move on
		if (!(pCurrent->pFilename))
			continue;
		// test line numbers
		if (pCurrent->nLineNb != nLineNb)
			continue;
		// test for file name
		int nPos;
		if ((nPos = sFilename.ReverseFind(*(pCurrent->pFilename))) < 0)
			continue;
		// file name found, return it
        return *(pCurrent->pFilename);
	}
	return String();
}

/** \brief pushes a include bookmark to the top of the list
 *  \param pNew the new bookmark
 */
void CParser::PushIncludeBookmark(inc_bookmark_t* pNew)
{
	pNew->pNext = m_pBookmarkHead;
	pNew->pPrev = 0;
	if (m_pBookmarkHead != 0)
		m_pBookmarkHead->pPrev = pNew;
	m_pBookmarkHead = pNew;
	if (!m_pBookmarkTail)
		m_pBookmarkTail = pNew;
}

/** \brief removes a include bookmark from the top of the list
 *  \return the top element
 */
inc_bookmark_t* CParser::PopIncludeBookmark()
{
	if (!m_pBookmarkHead)
		return 0;
	if (m_pBookmarkHead->pNext != 0)
		m_pBookmarkHead->pNext->pPrev = 0;
	inc_bookmark_t *tmp = m_pBookmarkHead;
	m_pBookmarkHead = m_pBookmarkHead->pNext;
	tmp->pNext = 0;
	return tmp;
}

/** \brief returns a reference to the current include bookmark
 *  \return a reference to the current include bookmark
 */
inc_bookmark_t* CParser::FirstIncludeBookmark()
{
	return m_pBookmarkHead;
}

/** \brief checks if the given file is included as standard include
 *  \param sFilename the name of the file
 *  \param nLineNb the line number of the include statement
 *  \return true if standard include
 */
bool CParser::IsStandardInclude(String sFilename, int nLineNb)
{
	if (sFilename.IsEmpty())
		return false;

	inc_bookmark_t *pCurrent;
	for (pCurrent = FirstIncludeBookmark(); pCurrent != 0; pCurrent = pCurrent->pNext)
	{
		// if no file-name, move on
		if (!(pCurrent->pFilename))
			continue;
		// test line numbers
		if (pCurrent->nLineNb != nLineNb)
			continue;
		// test for file name
		int nPos;
		if ((nPos = sFilename.ReverseFind(*(pCurrent->pFilename))) < 0)
			continue;
		// file name found, now extract standard include
		return pCurrent->bStandard;
	}
	return false;
}

/** \brief tries to find the line number where the given file was included
 *  \param sFilename the name of the included file
 *  \param sFromFile the name of the file, which included the other file
 *  \return the line-number of the include-statement
 */
int CParser::FindLineNbOfInclude(String sFilename, String sFromFile)
{
	if (sFilename.IsEmpty())
		return 0;
    if (sFromFile.IsEmpty() && !sTopLevelInFileName.IsEmpty())
        sFromFile = sTopLevelInFileName;
	if (sFromFile.IsEmpty()) // if no from file, this may be top???
		return 1;

	inc_bookmark_t *pCurrent;
	for (pCurrent = FirstIncludeBookmark(); pCurrent != 0; pCurrent = pCurrent->pNext)
	{
		// if no file-name, move on
		if (!(pCurrent->pFilename))
			continue;
		// if no from file-name, move on
		if (!(pCurrent->pFromFile))
			continue;
		// test for file name
		if (sFilename.ReverseFind(*(pCurrent->pFilename)) < 0)
			continue;
		// test for from file name
		if (sFromFile.ReverseFind(*(pCurrent->pFromFile)) < 0)
			continue;
		// file name found, now return line number
		return pCurrent->nLineNb;
	}
	return 1;
}

/** \brief checks if the file-name really points to a file
 *  \param sPathToFile the full name of the file
 *  \return 0 if it is a correct name, -1 otherwise
 */
int CParser::CheckName(String sPathToFile)
{
    struct stat st;
    if (stat((const char*)sPathToFile, &st))
        return -1;
    if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode))
        return -1;
    return 0;
}

/** \brief try to execute cpp
 *  \return true if cpp could be executed
 *
 * If we can execute cpp, it is found in the PATH.
 */
bool CParser::TestCPP()
{
	int pid = fork();
	int status;
	if (pid == -1)
		return false;

	// in child process call cpp
	if (pid == 0)
	{
	    char *sArgs[3];
		sArgs[0] = "cpp";
		sArgs[1] = "--version";
		sArgs[2] = 0;
		// close stdout
		int fd = open("/dev/null", O_APPEND);
		fclose(stdout);
		dup2(fd, 1 /* stdout */);
		// child -> run cpp
		// last argument: the file (stdin)
		// function sets last argument always to 0
		execvp("cpp", sArgs);
		return errno;
	}
	// parent -> wait for cpp
	waitpid(pid, &status, 0);

	return (status == 0);
}
