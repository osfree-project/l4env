/**
 *  \file    dice/src/Compiler.cpp
 *  \brief   contains the implementation of the class CCompiler
 *
 *  \date    03/06/2001
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "Compiler.h"

#include <unistd.h>
#include <stdarg.h>
#include <cctype>
#include <sys/wait.h>
#include <sys/timeb.h>
#include <limits.h> // needed for realpath
#include <stdlib.h>

#include <cassert>
#include <string>
#include <algorithm>
#include <iostream>

#if defined(HAVE_GETOPT_H)
#include <getopt.h>
#endif

#include "File.h"
#include "CParser.h"
#include "CPreProcess.h"
#include "Dependency.h"
#include "ProgramOptions.h"
#include "be/BERoot.h"
#include "be/BESizes.h"
#include "be/BEContext.h"
// L4 specific
#include "be/l4/L4BENameFactory.h"
// L4V2
#include "be/l4/v2/L4V2BEClassFactory.h"
#include "be/l4/v2/L4V2BENameFactory.h"
// L4V2 AMD64
#include "be/l4/v2/amd64/V2AMD64ClassFactory.h"
#include "be/l4/v2/amd64/V2AMD64NameFactory.h"
// L4V2 IA32
#include "be/l4/v2/ia32/V2IA32ClassFactory.h"
// L4X0
#include "be/l4/x0/L4X0BEClassFactory.h"
// L4V4
#include "be/l4/v4/L4V4BEClassFactory.h"
#include "be/l4/v4/ia32/L4V4IA32ClassFactory.h"
#include "be/l4/v4/L4V4BENameFactory.h"
// Sockets
#include "be/sock/SockBEClassFactory.h"
// Language C
//#include "be/lang/c/LangCClassFactory.h"
// Language C++
//#include "be/lang/cxx/LangCPPClassFactory.h"

#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"

// consitency checker
#include "fe/ConsistencyVisitor.h"
#include "fe/PostParseVisitor.h"

// dynamic loadable modules
#include <dlfcn.h>

//@{
/** some config variables */
extern const char* dice_version;
extern const char* dice_build;
extern const char* dice_user;
extern const char* dice_svnrev;
//@}

/////////////////////////////////////////////////////////////////////////////////
// error handling
//@{
/** globale variables used by the parsers to count errors and warnings */
int errcount = 0;
int erroccured = 0;
int warningcount = 0;
//@}

//@{
/** global variables for argument parsing */
extern char *optarg;
extern int optind, opterr, optopt;
//@}

//@{
/** static members */
bitset<PROGRAM_OPTIONS_MAX> CCompiler::m_Options;
bitset<PROGRAM_DEPEND_MAX> CCompiler::m_Depend;
ProgramFile_Type CCompiler::m_FileOptions = PROGRAM_FILE_IDLFILE;
BackEnd_Interface_Type CCompiler::m_BackEndInterface = PROGRAM_BE_NONE_I;
BackEnd_Platform_Type CCompiler::m_BackEndPlatform = PROGRAM_BE_NONE_P;
BackEnd_Language_Type CCompiler::m_BackEndLanguage = PROGRAM_BE_NONE_L;
int CCompiler::m_nVerboseInd = -1;
ProgramVerbose_Type CCompiler::m_VerboseLevel = PROGRAM_VERBOSE_NONE;
bitset<PROGRAM_WARNING_MAX> CCompiler::m_WarningLevel;
int CCompiler::m_nDumpMsgBufDwords = 0;
CBENameFactory *CCompiler::m_pNameFactory = 0;
CBEClassFactory *CCompiler::m_pClassFactory = 0;
CBESizes *CCompiler::m_pSizes = 0;
/* they have empty constructors because we test for this */
string CCompiler::m_sTraceMsgBufFunc = string();
string CCompiler::m_sTraceServerFunc = string();
string CCompiler::m_sTraceClientFunc = string();
string CCompiler::m_sTraceLib = string();
string CCompiler::m_sInitRcvStringFunc;
string CCompiler::m_sFilePrefix;
string CCompiler::m_sOutputDir;
string CCompiler::m_sIncludePrefix;
//@}

CCompiler::CCompiler()
{
    m_nUseFrontEnd = USE_FE_NONE;
    m_nOpcodeSize = 0;
    m_pRootBE = 0;
    m_sDependsFile = string();
}

/** cleans up the compiler object */
CCompiler::~CCompiler()
{
    // we delete the preprocessor here, because ther should be only
    // one for the whole compiler run.
    CPreProcess *pPreProcess = CPreProcess::GetPreProcessor();
    delete pPreProcess;
}

/**
 *  \brief parses the arguments of the compiler call
 *  \param argc the number of arguments
 *  \param argv the arguments
 */
void CCompiler::ParseArguments(int argc, char *argv[])
{
    int c;
#if defined(HAVE_GETOPT_LONG)
    int index = 0;
#endif
    bool bHaveFileName = false;
    bitset<PROGRAM_WARNING_MAX> nNoWarning;
    nNoWarning.reset();
    bool bShowHelp = false;

#if defined(HAVE_GETOPT_LONG)
    static struct option long_options[] = {
        {"client", 0, 0, 'c'},
        {"server", 0, 0, 's'},
        {"create-skeleton", 0, 0, 't'},
        {"template", 0, 0, 't'},
        {"no-opcodes", 0, 0, 'n'},
        {"create-inline", 2, 0, 'i'},
        {"filename-prefix", 1, 0, 'F'},
        {"include-prefix", 1, 0, 'p'},
        {"verbose", 2, 0, 'v'},
        {"corba", 0, 0, 'C'},
        {"preprocess", 1, 0, 'P'},
        {"f", 1, 0, 'f'},
        {"B", 1, 0, 'B'},
        {"stop-after-preprocess", 2, 0, 'E'},
        {"include", 1, 0, 'I'},
        {"nostdinc", 0, 0, 'N'},
        {"optimize", 2, 0, 'O'},
        {"testsuite", 2, 0, 'T'},
        {"version", 0, 0, 'V'},
        {"message-passing", 0, 0, 'm'},
        {"with-cpp", 1, 0, 'w'},
        {"o", 1, 0, 'o'},
        {"M", 2, 0, 'M'},
        {"W", 1, 0, 'W'},
        {"D", 1, 0, 'D'},
	{"x", 1, 0, 'x'},
        {"help", 0, 0, 'h'},
        {0, 0, 0, 0}
    };
#endif

    // prevent getopt from writing error messages
    opterr = 0;
    // obtain a reference to the pre-processor
    // and create one if not existent
    CPreProcess *pPreProcess = CPreProcess::GetPreProcessor();


    while (1)
    {
#if defined(HAVE_GETOPT_LONG)
        c = getopt_long_only(argc, argv, "cstni::v::hF:p:CP:f:B:E::O::VI:NT::M::W:D:x:o:", long_options, &index);
#else
        // n has an optional parameter to recognize -nostdinc and -n (no-opcode)
        c = getopt(argc, argv, "cstn::i::v::hF:p:CP:f:B:E::O::VI:NT::M::W:D:x:o:");
#endif

        if (c == -1)
        {
            bHaveFileName = true;
            break;        // skip - evaluate later
        }

        Verbose(PROGRAM_VERBOSE_OPTIONS, "Read argument %c\n", c);

        switch (c)
        {
        case '?':
            // Unknown options might be used by plugins
#if defined(HAVE_GETOPT_LONG)
            Warning("unrecognized option: %s (%d)\n", argv[optind - 1],
		optind);
	    Warning("Use \'--help\' to show valid options.\n");
	    Warning("However, plugins might process this option.\n");
#else
            Warning("unrecognized option: %s (%d)\n", argv[optind], optind);
	    Warning("Use \'-h\' to show valid options.\n");
	    Warning("However, plugins might process this option.\n");
#endif
            break;
        case ':':
            // Error exits
            Error("missing argument for option: %s\n", argv[optind - 1]);
            break;
        case 'c':
            Verbose(PROGRAM_VERBOSE_OPTIONS, "Create client-side code.\n");
            SetOption(PROGRAM_GENERATE_CLIENT);
            break;
        case 's':
            Verbose(PROGRAM_VERBOSE_OPTIONS, "Create server/component-side code.\n");
            SetOption(PROGRAM_GENERATE_COMPONENT);
            break;
        case 't':
            Verbose(PROGRAM_VERBOSE_OPTIONS, "create skeletons enabled\n");
            SetOption(PROGRAM_GENERATE_TEMPLATE);
            break;
        case 'i':
            {
                // there may follow an optional argument stating whether this is "static" or "extern" inline
                SetOption(PROGRAM_GENERATE_INLINE);
                if (!optarg)
                {
                    Verbose(PROGRAM_VERBOSE_OPTIONS, "create client stub as inline\n");
                }
                else
                {
                    // make upper case
                    string sArg(optarg);
                    transform(sArg.begin(), sArg.end(), sArg.begin(), _toupper);
                    while (sArg[0] == '=')
                        sArg.erase(sArg.begin());
                    if (sArg == "EXTERN")
                    {
                        SetOption(PROGRAM_GENERATE_INLINE_EXTERN);
                        UnsetOption(PROGRAM_GENERATE_INLINE_STATIC);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "create client stub as extern inline\n");
                    }
                    else if (sArg == "STATIC")
                    {
                        SetOption(PROGRAM_GENERATE_INLINE_STATIC);
                        UnsetOption(PROGRAM_GENERATE_INLINE_EXTERN);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "create client stub as static inline\n");
                    }
                    else
                    {
                        Warning("dice: Inline argument \"%s\" not supported. (assume none)", optarg);
                        UnsetOption(PROGRAM_GENERATE_INLINE_EXTERN);
                        UnsetOption(PROGRAM_GENERATE_INLINE_STATIC);
                    }
                }
            }
            break;
        case 'n':
#if !defined(HAVE_GETOPT_LONG)
            {
                // check if -nostdinc is meant
                string sArg(optarg);
                if (sArg.empty())
                {
                    Verbose(PROGRAM_VERBOSE_OPTIONS, "create no opcodes\n");
                    SetOption(PROGRAM_NO_OPCODES);
                }
                else if (sArg == "ostdinc")
                {
                    Verbose(PROGRAM_VERBOSE_OPTIONS, "no standard include paths\n");
                    pPreProcess->AddCPPArgument(string("-nostdinc"));
                }
            }
#else
            Verbose(PROGRAM_VERBOSE_OPTIONS, "create no opcodes\n");
            SetOption(PROGRAM_NO_OPCODES);
#endif
            break;
        case 'v':
            {
		int nVerboseLevel = PROGRAM_VERBOSE_NORMAL;
                if (!optarg)
                    Verbose(PROGRAM_VERBOSE_OPTIONS, "Verbose level %d enabled\n", nVerboseLevel);
                else
                {
                    nVerboseLevel = atoi(optarg);
                    if ((nVerboseLevel < 0) ||
			(nVerboseLevel > PROGRAM_VERBOSE_MAXLEVEL))
                    {
                        Warning("dice: Verbose level %d not supported in this version.", nVerboseLevel);
                        nVerboseLevel = std::max(std::min(nVerboseLevel, 
				(int)PROGRAM_VERBOSE_MAXLEVEL), 0);
                    }
                    Verbose(PROGRAM_VERBOSE_OPTIONS, "Verbose level %d enabled\n", nVerboseLevel);
                }
		CCompiler::SetVerboseLevel((ProgramVerbose_Type)nVerboseLevel);
            }
            break;
        case 'h':
            bShowHelp = true;
            break;
        case 'F':
            Verbose(PROGRAM_VERBOSE_OPTIONS, "file prefix %s used\n", optarg);
            SetFilePrefix(optarg);
            break;
        case 'p':
	    {
		Verbose(PROGRAM_VERBOSE_OPTIONS, "include prefix %s used\n", optarg);
		string sPrefix = optarg;
		// remove trailing slashes
		while (*(sPrefix.end()-1) == '/')
		    sPrefix.erase(sPrefix.end()-1);
		SetIncludePrefix(sPrefix);
	    }
            break;
        case 'C':
	    Warning("Option -C is deprecated. Use '-x corba' instead.\n");
            Verbose(PROGRAM_VERBOSE_OPTIONS, "use the CORBA frontend\n");
            m_nUseFrontEnd = USE_FE_CORBA;
            break;
        case 'P':
            Verbose(PROGRAM_VERBOSE_OPTIONS, "preprocessor option %s added\n", optarg);
            {
                // check for -I arguments, which we preprocess ourselves as well
                string sArg = optarg;
                pPreProcess->AddCPPArgument(sArg);
                if (sArg.substr(0, 2) == "-I")
                {
                    string sPath = sArg.substr(2);
                    pPreProcess->AddIncludePath(sPath);
                    Verbose(PROGRAM_VERBOSE_OPTIONS, "Added %s to include paths\n", sPath.c_str());
                }
            }
            break;
        case 'I':
            Verbose(PROGRAM_VERBOSE_OPTIONS, "add include path %s\n", optarg);
            {
                string sPath("-I");
                sPath += optarg;
                pPreProcess->AddCPPArgument(sPath);    // copies sPath
                // add to own include paths
                pPreProcess->AddIncludePath(optarg);
                Verbose(PROGRAM_VERBOSE_OPTIONS, "Added %s to include paths\n", optarg);
            }
            break;
        case 'N':
            Verbose(PROGRAM_VERBOSE_OPTIONS, "no standard include paths\n");
            pPreProcess->AddCPPArgument(string("-nostdinc"));
            break;
        case 'f':
            {
                // provide flags to compiler: this can be anything
                // make upper case
                string sArg = optarg;
                string sOrig = sArg;
                transform(sArg.begin(), sArg.end(), sArg.begin(), _toupper);
                // test first letter
                char cFirst = sArg[0];
                switch (cFirst)
                {
                case 'A':
                    if (sArg == "ALIGN-TO-TYPE")
                    {
                        SetOption(PROGRAM_ALIGN_TO_TYPE);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "align parameters in message buffer to type\n");
                    }
                    break;
                case 'C':
                    if (sArg == "CORBATYPES")
                    {
			SetOption(PROGRAM_USE_CORBA_TYPES);
			Verbose(PROGRAM_VERBOSE_OPTIONS, "Use CORBA types.");
                    }
		    else if (sArg == "CTYPES")
                    {
			Error("Option -fctypes is deprecated.\n");
                    }
                    else if (sArg == "CONST-AS-DEFINE")
                    {
                        SetOption(PROGRAM_CONST_AS_DEFINE);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "print const declarators as define statements\n");
                    }
                    break;
                case 'F':
                    if (sArg == "FORCE-CORBA-ALLOC")
                    {
                        SetOption(PROGRAM_FORCE_CORBA_ALLOC);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "Force use of CORBA_alloc (instead of Environment->malloc).\n");
                        if (IsOptionSet(PROGRAM_FORCE_ENV_MALLOC))
                        {
                            UnsetOption(PROGRAM_FORCE_ENV_MALLOC);
                            Verbose(PROGRAM_VERBOSE_OPTIONS, "Overrides previous -fforce-env-malloc.\n");
                        }
                    }
                    else if (sArg == "FORCE-ENV-MALLOC")
                    {
                        SetOption(PROGRAM_FORCE_ENV_MALLOC);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "Force use of Environment->malloc (instead of CORBA_alloc).\n");
                        if (IsOptionSet(PROGRAM_FORCE_CORBA_ALLOC))
                        {
                            UnsetOption(PROGRAM_FORCE_CORBA_ALLOC);
                            Verbose(PROGRAM_VERBOSE_OPTIONS, "Overrides previous -fforce-corba-alloc.\n");
                        }
                    }
                    else if (sArg == "FORCE-C-BINDINGS")
                    {
                        SetOption(PROGRAM_FORCE_C_BINDINGS);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "Force use of L4 C bindings (instead of inline assembler).\n");
                    }
                    else if (sArg == "FREE-MEM-AFTER-REPLY")
                    {
                        SetOption(PROGRAM_FREE_MEM_AFTER_REPLY);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "Free memory pointers stored in environment after reply.\n");
                    }
                    else
                    {
                        // XXX FIXME: test for 'FILETYPE=' too
                        if ((sArg == "FIDLFILE") || (sArg == "F1"))
                        {
                            SetFileOption(PROGRAM_FILE_IDLFILE);
                            Verbose(PROGRAM_VERBOSE_OPTIONS, "filetype is set to IDLFILE\n");
                        }
                        else if ((sArg == "FMODULE") || (sArg == "F2"))
                        {
                            SetFileOption(PROGRAM_FILE_MODULE);
                            Verbose(PROGRAM_VERBOSE_OPTIONS, "filetype is set to MODULE\n");
                        }
                        else if ((sArg == "FINTERFACE") || (sArg == "F3"))
                        {
                            SetFileOption(PROGRAM_FILE_INTERFACE);
                            Verbose(PROGRAM_VERBOSE_OPTIONS, "filetype is set to INTERFACE\n");
                        }
                        else if ((sArg == "FFUNCTION") || (sArg == "F4"))
                        {
                            SetFileOption(PROGRAM_FILE_FUNCTION);
                            Verbose(PROGRAM_VERBOSE_OPTIONS, "filetype is set to FUNCTION\n");
                        }
                        else if ((sArg == "FALL") || (sArg == "F5"))
                        {
                            SetFileOption(PROGRAM_FILE_ALL);
                            Verbose(PROGRAM_VERBOSE_OPTIONS, "filetype is set to ALL\n");
                        }
                        else
                        {
                            Error("\"%s\" is an invalid argument for option -ff\n",
				&optarg[1]);
                        }
                    }
                    break;
                case 'G':
                    {
                        // PROGRAM_GENERATE_LINE_DIRECTIVE
                        if (sArg == "GENERATE-LINE-DIRECTIVE")
                        {
                            SetOption(PROGRAM_GENERATE_LINE_DIRECTIVE);
                            Verbose(PROGRAM_VERBOSE_OPTIONS, "generating IDL file line directives in target code");
                        }
                        else
                        {
                            Error("\"%s\" is an invalid argument for option -f\n", optarg);
                        }
                    }
                case 'I':
                    if (sArg.substr(0,14) == "INIT-RCVSTRING")
                    {
                        SetOption(PROGRAM_INIT_RCVSTRING);
                        if (sArg.length() > 14)
                        {
                            string sName = sOrig.substr(15);
                            Verbose(PROGRAM_VERBOSE_OPTIONS, "User provides function \"%s\" to init indirect receive strings\n", sName.c_str());
                            SetInitRcvStringFunc(sName);
                        }
                        else
                            Verbose(PROGRAM_VERBOSE_OPTIONS, "User provides function to init indirect receive strings\n");
                    }
                    break;
                case 'K':
                    if (sArg == "KEEP-TEMP-FILES")
                    {
                        SetOption(PROGRAM_KEEP_TMP_FILES);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "Keep temporary files generated during preprocessing\n");
                    }
                    break;
                case 'L':
                    if (sArg == "L4TYPES")
                    {
			Error("Option \"%s\" is deprecated.", sOrig.c_str());
                    }
                    break;
                case 'N':
                    if (sArg == "NO-SEND-CANCELED-CHECK")
                    {
                        SetOption(PROGRAM_NO_SEND_CANCELED_CHECK);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "Do not check if the send part of a call was canceled.\n");
                    }
                    else if ((sArg == "NO-SERVER-LOOP") ||
                             (sArg == "NO-SRVLOOP"))
                    {
                        SetOption(PROGRAM_NO_SERVER_LOOP);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "Do not generate server loop.\n");
                    }
                    else if ((sArg == "NO-DISPATCH") ||
                             (sArg == "NO-DISPATCHER"))
                    {
                        SetOption(PROGRAM_NO_DISPATCHER);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "Do not generate dispatch function.\n");
                    }
                    break;
                case 'O':
                    if (sArg.substr(0, 12) == "OPCODE-SIZE=")
                    {
                        string sType = sArg.substr(12);
                        if (isdigit(sArg[13]))
                        {
                            m_nOpcodeSize = atoi(sType.c_str());
                            if (m_nOpcodeSize > 8)
                                m_nOpcodeSize = 8;
                            else if (m_nOpcodeSize > 4)
                                m_nOpcodeSize = 4;
                            else if (m_nOpcodeSize > 2)
                                m_nOpcodeSize = 2;
                        }
                        else
                        {
                            if ((sType == "BYTE") || (sType == "CHAR"))
                                m_nOpcodeSize = 1;
                            else if (sType == "SHORT")
                                m_nOpcodeSize = 2;
                            else if ((sType == "INT") || (sType == "LONG"))
                                m_nOpcodeSize = 4;
                            else if (sType == "LONGLONG")
                                m_nOpcodeSize = 8;
                            else
                                m_nOpcodeSize = 0;
                        }
                        if (m_nOpcodeSize > 0)
                        {
                            Verbose(PROGRAM_VERBOSE_OPTIONS, "Set size of opcode type to %d bytes\n", m_nOpcodeSize);
                        }
                        else
                            Verbose(PROGRAM_VERBOSE_OPTIONS, "The opcode-size \"%s\" is not supported\n", sType.c_str());
                    }
                    break;
                case 'S':
                    if (sArg == "SERVER-PARAMETER")
                    {
			Error("Option \"%s\" is deprecated.", sOrig.c_str());
                    }
                    break;
                case 'T':
                    if (sArg.substr(0, 12) == "TRACE-SERVER")
                    {
                        SetOption(PROGRAM_TRACE_SERVER);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "Trace messages received by the server loop\n");
                        if (sArg.length() > 12)
                        {
                            string sName = sOrig.substr(13);
                            Verbose(PROGRAM_VERBOSE_OPTIONS, "User sets trace function to \"%s\".\n", sName.c_str());
                            SetTraceServerFunc(sName);
                        }
                    }
                    else if (sArg.substr(0, 12) == "TRACE-CLIENT")
                    {
                        SetOption(PROGRAM_TRACE_CLIENT);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "Trace messages send to the server and answers received from it\n");
                        if (sArg.length() > 12)
                        {
                            string sName = sOrig.substr(13);
                            Verbose(PROGRAM_VERBOSE_OPTIONS, "User sets trace function to \"%s\".\n", sName.c_str());
                            SetTraceClientFunc(sName);
                        }
                    }
                    else if (sArg.substr(0, 17) == "TRACE-DUMP-MSGBUF")
                    {
                        SetOption(PROGRAM_TRACE_MSGBUF);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "Trace the message buffer struct\n");
                        if (sArg.length() > 17)
                        {
                            // check if lines are meant
                            if (sArg.substr(0, 24) == "TRACE-DUMP-MSGBUF-DWORDS")
                            {
                                if (sArg.length() > 24)
                                {
                                    string sNumber = sArg.substr(25);
                                    int nDwords = atoi(sNumber.c_str());
                                    if (nDwords >= 0)
                                        SetOption(PROGRAM_TRACE_MSGBUF_DWORDS);
				    SetTraceMsgBufDwords(nDwords);
                                }
                                else
                                    Error("The option -ftrace-dump-msgbuf-dwords expects an argument (e.g. -ftrace-dump-msgbuf-dwords=10).\n");
                            }
                            else
                            {
                                string sName = sOrig.substr(18);
                                Verbose(PROGRAM_VERBOSE_OPTIONS, "User sets trace function to \"%s\".\n", sName.c_str());
                                SetTraceMsgBufFunc(sName);
                            }
                        }
                    }
                    else if (sArg.substr(0, 14) == "TRACE-FUNCTION")
                    {
                        if (sArg.length() > 14)
                        {
                            string sName = sOrig.substr(15);
                            Verbose(PROGRAM_VERBOSE_OPTIONS, "User sets trace function to \"%s\".\n", sName.c_str());
                            if (GetTraceServerFunc().empty())
                                SetTraceServerFunc(sName);
                            if (GetTraceClientFunc().empty())
                                SetTraceClientFunc(sName);
                            if (GetTraceMsgBufFunc().empty())
                                SetTraceMsgBufFunc(sName);
                        }
                        else
                            Error("The option -ftrace-function expects an argument (e.g. -ftrace-function=LOGl).\n");
                    }
		    else if (sArg.substr(0, 9) == "TRACE-LIB")
		    {
			if (sArg.length() > 9)
			{
			    string sName = sOrig.substr(10);
			    Verbose(PROGRAM_VERBOSE_OPTIONS, "User sets tracing lib to \"%s\".\n",
				sName.c_str());
			    SetTraceLib(sName);
			}
			else
			    Error("The option -ftrace-lib expects an argument.\n");
		    }
                    else if ((sArg == "TEST-NO-SUCCESS") || 
			     (sArg == "TEST-NO-SUCCESS-MESSAGE"))
                    {
			Error("Option \"%s\" is deprecated.", sOrig.c_str());
                    }
                    else if (sArg == "TESTSUITE-SHUTDOWN")
                    {
			Error("Option \"%s\" is deprecated.", sOrig.c_str());
                    }
                    break;
                case 'U':
                    if ((sArg == "USE-SYMBOLS") || (sArg == "USE-DEFINES"))
                    {
			Error("Option \"%s\" is deprecated.", sOrig.c_str());
                    }
                    break;
                case 'Z':
                    if (sArg == "ZERO-MSGBUF")
                    {
                        SetOption(PROGRAM_ZERO_MSGBUF);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "Zero message buffer before using.\n");
                    }
                    break;
                default:
                    Warning("unsupported argument \"%s\" for option -f\n", sArg.c_str());
                    break;
                }
            }
            break;
        case 'B':
            {
                // make upper case
                string sArg(optarg);
                transform(sArg.begin(), sArg.end(), sArg.begin(), _toupper);

                // test first letter
                // p - platform
                // i - kernel interface
                // m - language mapping
                char sFirst = sArg[0];
                switch (sFirst)
                {
                case 'P':
                    sArg = sArg.substr(1);
                    if (sArg == "IA32")
                    {
			SetBackEndPlatform(PROGRAM_BE_IA32);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "use back-end for IA32 platform\n");
                    }
                    else if (sArg == "IA64")
                    {
                        Warning("IA64 back-end not supported yet!");
			SetBackEndPlatform(PROGRAM_BE_IA32);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "use back-end for IA64 platform\n");
                    }
                    else if (sArg == "ARM")
                    {
			SetBackEndPlatform(PROGRAM_BE_ARM);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "use back-end for ARM platform\n");
                    }
                    else if (sArg == "AMD64")
                    {
			SetBackEndPlatform(PROGRAM_BE_AMD64);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "use back-end for AMD64 platform\n");
                    }
                    else
                    {
                        Error("\"%s\" is an invalid argument for option -B/--back-end\n", optarg);
                    }
                    break;
                case 'I':
                    sArg = sArg.substr(1);
                    if (sArg == "V2")
                    {
			SetBackEndInterface(PROGRAM_BE_V2);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "use back-end L4 version 2\n");
                    }
                    else if (sArg == "X0")
                    {
			SetBackEndInterface(PROGRAM_BE_X0);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "use back-end L4 version X.0\n");
                    }
                    else if (sArg == "X0ADAPT")
                    {
			Error("X0adapt Back-End deprecated.\n");
                    }
                    else if ((sArg == "X2") || (sArg == "V4"))
                    {
			SetBackEndInterface(PROGRAM_BE_V4);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "use back-end L4 version 4 (X.2)\n");
                    }
                    else if (sArg == "FLICK")
                    {
                        Error("Flick compatibility mode is no longer supported");
                    }
                    else if ((sArg == "SOCKETS") || (sArg == "SOCK"))
                    {
			SetBackEndInterface(PROGRAM_BE_SOCKETS);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "use sockets back-end\n");
                    }
                    else
                    {
                        Error("\"%s\" is an invalid argument for option -B/--back-end\n", optarg);
                    }
                    break;
                case 'M':
                    sArg = sArg.substr(1);
                    if (sArg == "C")
                    {
			SetBackEndLanguage(PROGRAM_BE_C);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "use back-end C language mapping\n");
                    }
                    else if (sArg == "CPP")
                    {
			SetBackEndLanguage(PROGRAM_BE_CPP);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "use back-end C++ language mapping\n");
                    }
                    else
                    {
                        Error("\"%s\" is an invalid argument for option -B/--back-end\n", optarg);
                    }
                    break;
                default:
                    // unknown option
                    Error("\"%s\" is an invalid argument for option -B/--back-end\n", optarg);
                }
            }
            break;
        case 'E':
            if (!optarg)
            {
                Verbose(PROGRAM_VERBOSE_OPTIONS, "stop after preprocess option enabled.\n");
                SetOption(PROGRAM_STOP_AFTER_PRE);
            }
            else
            {
                string sArg(optarg);
                transform(sArg.begin(), sArg.end(), sArg.begin(), _toupper);
                if (sArg == "XML")
                {
		    Error("Option -EXML is deprecated.\n"); 
                }
                else
                {
                    Warning("unrecognized argument \"%s\" to option -E.\n", sArg.c_str());
                    SetOption(PROGRAM_STOP_AFTER_PRE);
                }
            }
            break;
        case 'o':
            if (!optarg)
            {
                Error("Option -o requires argument\n");
            }
            else
            {
                Verbose(PROGRAM_VERBOSE_OPTIONS, "output directory set to %s\n", optarg);
		string sDir = optarg;
		if (!sDir.empty())
		{
		    if (sDir[sDir.length()-1] != '/')
			sDir += "/";
		}
		SetOutputDir(sDir);
            }
            break;
        case 'T':
	    Error("Option \"-T%s\" is deprecated.", optarg ? optarg : "");
            break;
        case 'V':
            ShowVersion();
            break;
        case 'm':
            Verbose(PROGRAM_VERBOSE_OPTIONS, "generate message passing functions.\n");
            SetOption(PROGRAM_GENERATE_MESSAGE);
            break;
        case 'M':
            {
                // search for depedency options
                if (!optarg)
                {
		    SetDependsOption(PROGRAM_DEPEND_M);
                    Verbose(PROGRAM_VERBOSE_OPTIONS, "Create dependencies without argument\n");
                }
                else
                {
                    // make upper case
                    string sArg = optarg;
                    transform(sArg.begin(), sArg.end(), sArg.begin(), _toupper);
                    if (sArg == "M")
                        SetDependsOption(PROGRAM_DEPEND_MM);
                    else if (sArg == "D")
                        SetDependsOption(PROGRAM_DEPEND_MD);
                    else if (sArg == "MD")
                        SetDependsOption(PROGRAM_DEPEND_MMD);
		    /* do not unset DEPEND_MASK1 generally, because then -MP
		     * might delete -M. Nor unset it for the following option
		     * for the same reason.
		     */
		    else if (sArg == "F")
		    {
			SetDependsOption(PROGRAM_DEPEND_MF);
			// checking if parameter after -MF starts with a '-'
			// which would make it a likely argument
			if (argv[optind] && (argv[optind])[0] != '-')
			{
			    m_sDependsFile = argv[optind++];
			    Verbose(PROGRAM_VERBOSE_OPTIONS, "Set depends file name to \"%s\".\n", 
				m_sDependsFile.c_str());
			}
		    }
		    else if (sArg == "P")
			SetDependsOption(PROGRAM_DEPEND_MP);
                    else
                    {
                        Warning("dice: Argument \"%s\" of option -M unrecognized: ignoring.", optarg);
                        SetDependsOption(PROGRAM_DEPEND_M);
                    }
                    Verbose(PROGRAM_VERBOSE_OPTIONS, "Create dependencies with argument %s\n", optarg);
                }
            }
            break;
        case 'W':
            if (!optarg)
            {
                Error("The option '-W' has to be used with parameters (see --help for details)\n");
            }
            else
            {
                string sArg = optarg;
                transform(sArg.begin(), sArg.end(), sArg.begin(), _toupper);
                if (sArg == "ALL")
                {
		    SetWarningLevel(PROGRAM_WARNING_ALL);
                    Verbose(PROGRAM_VERBOSE_OPTIONS, "All warnings on.\n");
                }
                else if (sArg == "NO-ALL")
                {
                    nNoWarning.reset();
                    Verbose(PROGRAM_VERBOSE_OPTIONS, "All warnings off.\n");
                }
                else if (sArg == "IGNORE-DUPLICATE-FID")
                {
		    SetWarningLevel(PROGRAM_WARNING_IGNORE_DUPLICATE_FID);
                    Verbose(PROGRAM_VERBOSE_OPTIONS, "Warn if duplicate function IDs are ignored.\n");
                }
                else if (sArg == "NO-IGNORE-DUPLICATE-FID")
                {
                    nNoWarning.set(PROGRAM_WARNING_IGNORE_DUPLICATE_FID);
                    Verbose(PROGRAM_VERBOSE_OPTIONS, "Do not warn if duplicate function IDs are ignored.\n");
                }
                else if (sArg == "PREALLOC")
                {
		    SetWarningLevel(PROGRAM_WARNING_PREALLOC);
                    Verbose(PROGRAM_VERBOSE_OPTIONS, "Warn if CORBA_alloc is used.\n");
                }
                else if (sArg == "NO-PREALLOC")
                {
                    nNoWarning.set(PROGRAM_WARNING_PREALLOC);
                    Verbose(PROGRAM_VERBOSE_OPTIONS, "Do not warn if CORBA_alloc is used.\n");
                }
                else if (sArg == "MAXSIZE")
                {
		    SetWarningLevel(PROGRAM_WARNING_NO_MAXSIZE);
                    Verbose(PROGRAM_VERBOSE_OPTIONS, "Warn if max-size attribute is not set for unbound variable sized arguments.\n");
                }
                else if (sArg == "NO-MAXSIZE")
                {
                    nNoWarning.set(PROGRAM_WARNING_NO_MAXSIZE);
                    Verbose(PROGRAM_VERBOSE_OPTIONS, "Do not warn if max-size attribute is not set for unbound variable sized arguments.\n");
                }
                else if (sArg.substr(0, 2) == "P,")
                {
                    string sTmp(optarg);
                    // remove "p,"
                    string sCppArg = sTmp.substr(2);
                    pPreProcess->AddCPPArgument(sCppArg);
                    Verbose(PROGRAM_VERBOSE_OPTIONS, "preprocessor argument \"%s\" added\n", sCppArg.c_str());
                    // check if CPP argument has special meaning
                    if (sCppArg.substr(0, 2) == "-I")
                    {
                        string sPath = sCppArg.substr(2);
			pPreProcess->AddIncludePath(sPath);
                        Verbose(PROGRAM_VERBOSE_OPTIONS, "Added %s to include paths\n", sPath.c_str());
                    }
                }
                else
                {
                    Warning("dice: warning \"%s\" not supported.\n", optarg);
                }
            }
            break;
        case 'D':
            if (!optarg)
            {
                Error("There has to be an argument for the option '-D'\n");
            }
            else
            {
                string sArg("-D");
                sArg += optarg;
                pPreProcess->AddCPPArgument(sArg);
                Verbose(PROGRAM_VERBOSE_OPTIONS, "Found symbol \"%s\"\n", 
		    optarg);
            }
            break;
        case 'w':
            {
                string sArg = optarg;
                if (!pPreProcess->SetCPP(sArg.c_str()))
                    Error("Preprocessor \"%s\" does not exist.\n", optarg);
                Verbose(PROGRAM_VERBOSE_OPTIONS,
		    "Use \"%s\" as preprocessor\n", optarg);
            }
            break;
	case 'x':
	    {
		string sArg = optarg;
                transform(sArg.begin(), sArg.end(), sArg.begin(), _toupper);
		if (sArg == "CORBA")
		{
		    Verbose(PROGRAM_VERBOSE_OPTIONS,
			"use the CORBA frontend\n");
		    m_nUseFrontEnd = USE_FE_CORBA;
		}
		else if (sArg == "CAPIDL" || sArg == "CAP")
		{
		    Verbose(PROGRAM_VERBOSE_OPTIONS,
			"use the CapIDL frontend\n");
		    m_nUseFrontEnd = USE_FE_CAPIDL;
		}
		else if (sArg == "DCE")
		{
		    Verbose(PROGRAM_VERBOSE_OPTIONS,
			"use the DCE frontend\n");
		    m_nUseFrontEnd = USE_FE_DCE;
		}
	    }
	    break;
        default:
            Error("You used an obsolete parameter (%c).\nPlease use dice --help to check your parameters.\n", c);
        }
    }

    if (optind < argc)
    {
	Verbose(PROGRAM_VERBOSE_OPTIONS, "Arguments not processed: ");
	while (optind < argc)
	    Verbose(PROGRAM_VERBOSE_OPTIONS, "%s ", argv[optind++]);
	Verbose(PROGRAM_VERBOSE_OPTIONS, "\n");
    }

    if (bHaveFileName && (argc > 1))
    {
        // infile left (should be last)
        // if argv is "-" the stdin should be used: set m_sInFileName to 0
        if (strcmp(argv[argc - 1], "-"))
            m_sInFileName = argv[argc - 1];
        else
            m_sInFileName = "";
        Verbose(PROGRAM_VERBOSE_OPTIONS, "Input file is: %s\n", m_sInFileName.c_str());
    }

    if (m_nUseFrontEnd == USE_FE_NONE)
        m_nUseFrontEnd = USE_FE_DCE;

    if (!IsBackEndInterfaceSet(PROGRAM_BE_INTERFACE))
    {
        if (IsBackEndPlatformSet(PROGRAM_BE_ARM))
	    SetBackEndInterface(PROGRAM_BE_X0);
        else
	    SetBackEndInterface(PROGRAM_BE_V2);
    }
    if (!IsBackEndPlatformSet(PROGRAM_BE_PLATFORM))
	SetBackEndPlatform(PROGRAM_BE_IA32);
    if (!IsBackEndLanguageSet(PROGRAM_BE_LANGUAGE))
	SetBackEndLanguage(PROGRAM_BE_C);
    if (IsBackEndPlatformSet(PROGRAM_BE_ARM) &&
        !IsBackEndInterfaceSet(PROGRAM_BE_X0))
    {
        Warning("The Arm Backend currently works with X0 native only!");
        Warning("  -> Setting interface to X0 native.");
	SetBackEndInterface(PROGRAM_BE_X0);
    }
    if (IsBackEndPlatformSet(PROGRAM_BE_AMD64) &&
        !IsBackEndInterfaceSet(PROGRAM_BE_V2))
    {
        Warning("The AMD64 Backend currently works with V2 native only!");
        Warning("  -> Setting interface to V2 native.");
	SetBackEndInterface(PROGRAM_BE_V2);
    }
    // with arm we *have to* marshal type aligned
    if (IsBackEndPlatformSet(PROGRAM_BE_ARM))
        SetOption(PROGRAM_ALIGN_TO_TYPE);
    // with AMD64 we *have to* use C bindings
    if (IsBackEndPlatformSet(PROGRAM_BE_AMD64))
        SetOption(PROGRAM_FORCE_C_BINDINGS);

    if (!IsOptionSet(PROGRAM_GENERATE_CLIENT) &&
        !IsOptionSet(PROGRAM_GENERATE_COMPONENT))
    {
        /* per default generate client AND server */
        SetOption(PROGRAM_GENERATE_CLIENT);
        SetOption(PROGRAM_GENERATE_COMPONENT);
    }

    // check if tracing functions are set
    if (IsOptionSet(PROGRAM_TRACE_SERVER) &&
	GetTraceServerFunc().empty())
	SetTraceServerFunc("printf");
    if (IsOptionSet(PROGRAM_TRACE_CLIENT) &&
	GetTraceClientFunc().empty())
	SetTraceClientFunc("printf");
    if (IsOptionSet(PROGRAM_TRACE_MSGBUF) &&
	GetTraceMsgBufFunc().empty())
	SetTraceMsgBufFunc("printf");

    if (nNoWarning.any())
	m_WarningLevel &= ~nNoWarning;

    // check if dependency options are set correctly
    if (IsDependsOptionSet(PROGRAM_DEPEND_MF) ||
	IsDependsOptionSet(PROGRAM_DEPEND_MP))
    {
	if (!IsDependsOptionSet(PROGRAM_DEPEND_M) &&
	    !IsDependsOptionSet(PROGRAM_DEPEND_MM) && 
	    !IsDependsOptionSet(PROGRAM_DEPEND_MD) &&
	    !IsDependsOptionSet(PROGRAM_DEPEND_MMD))
	{
	    Error("Option %s requires one of -M,-MM,-MD,-MMD.\n",
		IsDependsOptionSet(PROGRAM_DEPEND_MP) ? "-MP" : "-MF");
	}
    }
    if (IsDependsOptionSet(PROGRAM_DEPEND_MF) &&
	m_sDependsFile.empty())
    {
	Error("Option -MF requires argument.\n");
    }
	

    // init plugins
    InitTraceLib(argc, argv);
    
    if (bShowHelp)
	ShowHelp();
}

/** \brief initializes the trace lib
 *  \param argc the number of arguments
 *  \param argv the arguments of the program
 */
void CCompiler::InitTraceLib(int argc, char *argv[])
{
    // if trace lib was set, then open it and let it parse the arguments. This
    // way it might interpret some arguments that we ignored
    string sTraceLib = CCompiler::GetTraceLib();
    if (sTraceLib.empty())
	return;

    void* lib = dlopen(sTraceLib.c_str(), RTLD_NOW);
    if (lib == NULL)
    {
	fprintf(stderr, "%s\n", dlerror());
	// error exists
	Error("Could not load tracing library \"%s\".\n", sTraceLib.c_str());

	return;
    }

    // get symbol for init function
    void (*init)(int, char**);
    init = (void (*)(int, char**))dlsym(lib, "dice_tracing_init");
    // use error message as error indicator
    const char* errmsg = dlerror();
    if (errmsg != NULL)
    {
	fprintf(stderr, "%s\n", errmsg);
	Error("Could not find symbol for init function.\n");
	return;
    }

    // call init function
    (*init) (argc, argv);
}

/** displays a copyright notice of this compiler */
void CCompiler::ShowCopyright()
{
    if (!IsVerboseLevel(PROGRAM_VERBOSE_OPTIONS))
        return;
    std::cout << "DICE (c) 2001-2006 Dresden University of Technology" << std::endl;
    std::cout << "Author: Ronald Aigner <ra3@os.inf.tu-dresden.de>" << std::endl;
    std::cout << "e-Mail: dice@os.inf.tu-dresden.de" << std::endl << std::endl;
}

/** displays a help for this compiler */
void CCompiler::ShowHelp(bool bShort)
{
    ShowCopyright();
    std::cout << "Usage: dice [<options>] <idl-file>\n"
    //23456789+123456789+123456789+123456789+123456789+123456789+123456789+123456789+
    "\nPre-Processor/Front-End Options:\n"
    " -h"
#if defined(HAVE_GETOPT_LONG)
        ", --help"
#else
        "        "
#endif
                 "                 shows this help\n"
    " -v"
#if defined(HAVE_GETOPT_LONG)
        ", --verbose"
#endif
                    " [<level>]    print verbose output\n"
    "                              with optional verboseness level\n"
    "                              (-v is the same as -v" << PROGRAM_VERBOSE_NORMAL << ")\n"
    " -x <language>              specify the language of the input file\n"
    "                            permissable languages are: dce corba\n"
    "                            If none is given 'dce' is assumed.\n"
    " -P"
#if defined(HAVE_GETOPT_LONG)
    ", --preprocess"
#endif
    " <string>  hand <string> as argument to preprocessor\n"
    " -I<string>                 same as -P-I<string>\n"
    " -D<string>                 same as -P-D<string>\n"
    " -nostdinc                  same as -P-nostdinc\n"
    " -Wp,<string>               same as -P<string>\n"
    "    Arguments given to '-P' and '-Wp,' are checked for '-D' and '-I'\n"
    "    as well.\n"
    " -E                         stop processing after pre-processing\n"
    " -M                         print included file tree and stop after\n"
    "                            doing that\n"
    " -MM                        print included file tree for files included\n"
    "                            with '#include \"file\"' and stop\n"
    " -MD                        print included file tree into .d file and\n"
    "                            compile\n"
    " -MMD                       print included file tree for files included\n"
    "                            with '#include \"file\"' into .d file and\n"
    "                            compile\n"
    " -MF <string>               generates dependency output into the\n"
    "                            file specified by <string>\n"
    " --with-cpp=<string>        use <string> as cpp\n"
    "    Dice checks if it can run <string> before using it.\n"
    "\n"
    "\nBack-End Options:\n"
    " -i"
#if defined(HAVE_GETOPT_LONG)
    ", --create-inline"
#endif
    " <mode>  generate client stubs as inline\n";
    if (!bShort)
	std::cout <<
    "    set <mode> to \"static\" to generate static inline\n"
    "    set <mode> to \"extern\" to generate extern inline\n"
    "    <mode> is optional\n";
    std::cout <<
    " -n"
#if defined(HAVE_GETOPT_LONG)
    ", --no-opcodes"
#else
    "              "
#endif
    "            do not generate opcodes\n"
    " -c"
#if defined(HAVE_GETOPT_LONG)
    ", --client"
#else
    "          "
#endif
    "                generate client-side code\n"
    " -s"
#if defined(HAVE_GETOPT_LONG)
    ", --server"
#else
    "          "
#endif
    "                generate server/component-side code\n"
    "    if none of the two is specified both are set\n"
    " -t"
#if defined(HAVE_GETOPT_LONG)
    ", --template, --create-skeleton\n"
    "   "
#endif
    "              generate skeleton/templates for server side functions\n"
    " -F"
#if defined(HAVE_GETOPT_LONG)
    ", --filename-prefix"
#endif
    " <string>\n"
    "                 prefix each filename with the string\n"
    " -p"
#if defined(HAVE_GETOPT_LONG)
    ", --include-prefix"
#endif
    " <string>\n"
    "                 prefix each included file with string\n"
    " -o <string>\n"
    "                 specify an output directory (default is .)\n";
    if (bShort)
	std::cout <<
    " -f<string>      supply flags to compiler\n";
    else
	std::cout <<
    "\n"
    "Compiler Flags:\n"
    " -f<string>                   supply flags to compiler\n"
    "    if <string> starts with 'F'/'f' (filetype):\n"
    "      set <string> to \"Fidlfile\" for one client C file per IDL file\n"
    "      set <string> to \"Fmodule\" for one client C file per module\n"
    "      set <string> to \"Finterface\" for one client C file per interface\n"
    "      set <string> to \"Ffunction\" for one client C file per function\n"
    "      set <string> tp \"Fall\" for one client C file for everything\n"
    "      alternatively set -fF1, -fF2, -fF3, -fF4, -fF5 respectively\n"
    "      default is -fFidlfile\n"
    "    set <string> to 'corbatypes' to use CORBA-style type names\n"
    "    set <string> to 'opcodesize=<size>' to set the size of the opcode\n"
    "      set <size> to \"byte\" or 1 if opcode should use only 1 byte\n"
    "      set <size> to \"short\" or 2 if opcode should use 2 bytes\n"
    "      set <size> to \"long\" or 4 if opcode should use 4 bytes (default)\n"
    "      set <size> to \"longlong\" or 8 if opcode should use 8 bytes\n"
    "    set <string> to 'init-rcvstring[=<func-name>]' to make the\n"
    "       server-loop use a user-provided function to initialize the\n"
    "       receive buffers of indirect strings\n"
    "    set <string> to 'align-to-type' to align parameters in the message\n"
    "       buffer by the size of their type (or word) (default for ARM)\n"
    "\n"
    "    set <string> to 'force-corba-alloc' to force the usage of the\n"
    "       CORBA_alloc function instead of the CORBA_Environment's malloc\n"
    "       member\n"
    "    set <string> to 'force-env-malloc' to force the usage of\n"
    "       CORBA_Environment's malloc member instead of CORBA_alloc\n"
    "    depending on their order they may override each other.\n"
    "    set <string> to 'free-mem-after-reply' to force freeing of memory\n"
    "       pointers stored in the environment variable using the free\n"
    "       function of the environment or CORBA_free\n"
    "\n"
    "    set <string> to 'force-c-bindings' to force the usage of L4's\n"
    "       C-bindings instead of inline assembler\n"
    "    set <string> to 'no-server-loop' to not generate the server loop\n"
    "       function\n"
    "    set <string> to 'no-dispatcher' to not generate the dispatcher\n"
    "       function\n"
    "\n"
    "  Debug Options:\n"
    "    set <string> to 'trace-server' to trace all messages received by the\n"
    "       server-loop\n"
    "    set <string> to 'trace-client' to trace all messages send to the\n"
    "       server and answers received from it\n"
    "    set <string> to 'trace-dump-msgbuf' to dump the message buffer,\n"
    "       before a call, right after it, and after each wait\n"
    "    set <string> to 'trace-dump-msgbuf-dwords=<number>' to restrict the\n"
    "       number of dumped dwords. <number> can be any positive integer\n"
    "       including zero.\n"
    "    set <string> to 'trace-function=<function>' to use <function>\n"
    "       instead of 'printf' to print trace messages. This option sets\n"
    "       the function for client and server.\n"
    "       If you like to specify different functions for client, server,\n"
    "       or msgbuf, you can use -ftrace-client=<function1>,\n"
    "       -ftrace-server=<function2>, or -ftrace-dump-msgbuf=<function3>\n"
    "       respectively\n"
    "    set <string> to 'trace-lib=<libname>' to use a dynamic loadable \n"
    "       library that implements the tracing hooks. (see documentation for\n"
    "       details)\n"
    "    set <string> to 'zero-msgbuf' to zero out the message buffer before\n"
    "       each and wait/reply-and-wait\n"
    "    if <string> is set to 'use-symbols' or 'use-defines' the symbols\n"
    "       given with '-D' or '-P-D' are not just handed down to the\n"
    "       preprocessor, but also used to simplify the generated code.\n"
    "       USE FOR DEBUGGING ONLY!\n"
    "    if <string> is set to 'no-send-canceled-check' the client call will\n"
    "       not retry to send if it was canceled or aborted by another thread.\n"
    "    if <string> is set to 'const-as-define' all const declarations will\n"
    "       be printed as #define statements\n"
    "    if <string> is set to 'keep-temp-files' Dice will not delete the\n"
    "       temporarely during preprocessing created files.\n"
    "       USE FOR DEBUGGING ONLY!\n"
    "\n";
    std::cout <<
    " -B"
#if defined(HAVE_GETOPT_LONG)
    ", --back-end"
#endif
    " <string>     defines the back-end to use\n";
    if (!bShort)
	std::cout <<
    "    <string> starts with a letter specifying platform, kernel interface or\n"
    "    language mapping\n"
    "    p - specifies the platform (IA32, IA64, ARM, AMD64)\n"
    "    i - specifies the kernel interface (v2, x0, v4, sock)\n"
    "    m - specifies the language mapping (C, CPP)\n"
    "    example: -Bpia32 -Biv2 -BmC - which is default\n";
    std::cout <<
    " -m"
#if defined(HAVE_GETOPT_LONG)
    ", --message-passing"
#else
    "                   "
#endif
    "       generate MP functions for RPC functions as well\n"
    "\nGeneral Compiler Options:\n"
    " -Wall                       ignore all warnings\n"
    " -Wignore-duplicate-fids     duplicate function ID are no errors, but\n"
    "                             warnings\n"
    " -Wprealloc                  warn if memory has to be allocated using\n"
    "                             CORBA_alloc\n"
    " -Wno-maxsize                warn if a variable sized parameter has no\n"
    "                             maximum size to bound its required memory\n"
    "                             use\n"
    "\n\nexample: dice -v -i test.idl\n\n";
    exit(0);
}

/** display the compiler's version (requires a defines for the version number) */
void CCompiler::ShowVersion()
{
    ShowCopyright();
    std::cout << "DICE " << dice_version;
    if (dice_svnrev)
	std::cout << " (" << dice_svnrev << ")";
    std::cout << " - built on " << dice_build;
    if (dice_user)
	std::cout << " by " << dice_user;
    std::cout << "." << std::endl;
    exit(0);
}

/** \brief creates a parser object and initializes it
 *
 * The parser object pre-processes and parses the input file.
 */
void CCompiler::Parse()
{
    // if sFilename contains a path, add it to include paths
    CPreProcess *pPreProcess = CPreProcess::GetPreProcessor();
    int nPos;
    if ((nPos = m_sInFileName.rfind('/')) >= 0)
    {
        string sPath("-I");
        sPath += m_sInFileName.substr(0, nPos);
        pPreProcess->AddCPPArgument(sPath);    // copies sPath
        pPreProcess->AddIncludePath(m_sInFileName.substr(0, nPos));
        Verbose(PROGRAM_VERBOSE_OPTIONS, "Added %s to include paths\n",
	    m_sInFileName.substr(0, nPos).c_str());
    }
    // set namespace to uninitialized
    CParser *pParser = CParser::CreateParser(m_nUseFrontEnd);
    CParser::SetCurrentParser(pParser);
    if (!pParser->Parse(0 /* no exitsing scan buffer*/, m_sInFileName,
        m_nUseFrontEnd, IsOptionSet(PROGRAM_STOP_AFTER_PRE)))
    {
        if (!erroccured)
            Error("other parser error.");
    }
    // get file
    m_pRootFE = pParser->GetTopFileInScope();
    // now that parsing is over, get rid of it
    delete pParser;
    // post parse processing
    CPostParseVisitor v;
    try
    {
	if (m_pRootFE)
	    m_pRootFE->Accept(v);
    }
    catch (...)
    {
	Error("Post-Parse Processing failed.\n");
    }
    // if errors, print them and abort
    if (erroccured)
    {
        if (errcount > 0)
            Error("%d Error(s) and %d Warning(s) occured.", errcount, 
		warningcount);
        else
            Warning("%s: warning: %d Warning(s) occured while parsing.", 
		m_sInFileName.c_str(), warningcount);
    }
}

/**
 *  \brief prepares the write operation
 *
 * This method performs any tasks necessary before the write operation. E.g.
 * it creates the class and name factory depending on the compiler arguments,
 * it creates the context and finally creates the backend.
 *
 * First thing this function does is to force an consistency check for the
 * whole hierarchy of the IDL file. If this check fails the compile run is
 * aborted.
 *
 * Next the transmittable data is optimized.
 *
 * Finally the preparations for the write operation are made.
 */
void CCompiler::PrepareWrite()
{
    // if we should stop after preprocessing skip this function
    if (IsOptionSet(PROGRAM_STOP_AFTER_PRE))
        return;

    Verbose(PROGRAM_VERBOSE_NORMAL, "Check consistency of parsed input ...\n");
    if (!m_pRootFE)
        Error("Internal Error: Current file not set");
    // consistency check
    CConsistencyVisitor v;
    try
    {
	m_pRootFE->Accept(v);
    }
    catch (...)
    {
	Error("Consistency check failed.\n");
    }
    Verbose(PROGRAM_VERBOSE_NORMAL, "... finished consistency check.\n");

    // create context
    Verbose(PROGRAM_VERBOSE_NORMAL, "Create Context...\n");

    // set platform class factory depending on arguments
    CBEClassFactory *pCF = NULL;
    if (IsBackEndInterfaceSet(PROGRAM_BE_V2))
    {
	if (IsBackEndPlatformSet(PROGRAM_BE_AMD64))
	    pCF = new CL4V2AMD64BEClassFactory();
	else if (IsBackEndPlatformSet(PROGRAM_BE_IA32))
	    pCF = new CL4V2IA32BEClassFactory();
	else
	    pCF = new CL4V2BEClassFactory();
    }
    else if (IsBackEndInterfaceSet(PROGRAM_BE_X0))
	pCF = new CL4X0BEClassFactory();
    else if (IsBackEndInterfaceSet(PROGRAM_BE_V4))
    {
        if (IsBackEndPlatformSet(PROGRAM_BE_IA32))
            pCF = new CL4V4IA32ClassFactory();
        else
            pCF = new CL4V4BEClassFactory();
    }
    else if (IsBackEndInterfaceSet(PROGRAM_BE_SOCKETS))
        pCF = new CSockBEClassFactory();
    assert(pCF);
    SetClassFactory(pCF);

    // set name factory depending on arguments
    CBENameFactory *pNF = NULL;
    if (IsBackEndInterfaceSet(PROGRAM_BE_V2))
    {
	if (IsBackEndPlatformSet(PROGRAM_BE_AMD64))
	    pNF = new CL4V2AMD64BENameFactory();
	else
	    pNF = new CL4V2BENameFactory();
    }
    else if (IsBackEndInterfaceSet(PROGRAM_BE_X0))
	pNF = new CL4BENameFactory();
    else if (IsBackEndInterfaceSet(PROGRAM_BE_V4))
        pNF = new CL4V4BENameFactory();
    else
        pNF = new CBENameFactory();
    assert(pNF);
    SetNameFactory(pNF);

    // set sizes
    CBESizes *pSizes = pCF->GetNewSizes();
    if (m_nOpcodeSize > 0)
	pSizes->SetOpcodeSize(m_nOpcodeSize);
    SetSizes(pSizes);

    /** Prepare write for the back-end does some intialization which cannot be
     * performed during the write run.
     */
    Verbose(PROGRAM_VERBOSE_NORMAL, "Create backend...\n");
    m_pRootBE = pCF->GetNewRoot();
    try
    {
	m_pRootBE->CreateBE(m_pRootFE);
    }
    catch (CBECreateException *e)
    {
	Verbose(PROGRAM_VERBOSE_NORMAL, "Back-End creation failed\n");
        delete m_pRootBE;
        m_pRootBE = 0;
	e->Print();
	delete e;
	Error("Creating back-end failed.\n");
    }
    Verbose(PROGRAM_VERBOSE_NORMAL, "...done.\n");

    // print dependency tree
    Verbose(PROGRAM_VERBOSE_NORMAL, "Print dependencies...\n");
    if (m_Depend.any())
    {
	CDependency *pD = new CDependency(m_sDependsFile, m_pRootFE, m_pRootBE);
        pD->PrintDependencies();
	delete pD;
    }
    Verbose(PROGRAM_VERBOSE_NORMAL, "... dependencies done.\n");

    // if we should stop after printing the dependencies, stop here
    if (IsDependsOptionSet(PROGRAM_DEPEND_M) ||
	IsDependsOptionSet(PROGRAM_DEPEND_MM))
    {
         delete m_pRootBE;
	 m_pRootBE = 0;
         delete pNF;
         delete pCF;
         return;
    }
}

/**
 *  \brief prints the target files
 *
 * This method calls the write operation of the backend
 */
void CCompiler::Write()
{
    // if we should stop after preprocessing skip this function
    if (IsOptionSet(PROGRAM_STOP_AFTER_PRE) ||
        IsDependsOptionSet(PROGRAM_DEPEND_M) ||
        IsDependsOptionSet(PROGRAM_DEPEND_MM))
        return;

    // We could load the tracing lib here. However, we want it to parse our
    // arguments. So we have to open it right at the beginning.

    // write backend
    Verbose(PROGRAM_VERBOSE_NORMAL, "Write backend...\n");
    m_pRootBE->Write();
    Verbose(PROGRAM_VERBOSE_NORMAL, "...done.\n");
}

/**
 *  \brief helper function
 *  \param sMsg the error message to print before exiting
 *
 * This method prints an error message and exits the compiler. Any clean up
 * should be performed  BEFORE this method is called.
 */
void CCompiler::Error(const char *sMsg, ...)
{
    std::cerr << "dice: ";
    va_list args;
    va_start(args, sMsg);
    vfprintf(stderr, sMsg, args);
    va_end(args);
    std::cerr << "\n";

    exit(1);
}

/** \brief helper function
 *  \param pFEObject the front-end object where the error occurred
 *  \param nLinenb the linenumber of the error
 *  \param sMsg the error message
 *
 * The given front-end object can be used to determine the file this object
 * belonged to and the line, where this object has been declared. This is
 * useful if we do not have a current line number available (any time after
 * the parsing finished).
 */
void CCompiler::GccError(CFEBase * pFEObject, int nLinenb, const char *sMsg, ...)
{
    va_list args;
    va_start(args, sMsg);
    GccErrorVL(pFEObject, nLinenb, sMsg, args);
    va_end(args);
}

/** \brief helper function
 *  \param pFEObject the front-end object where the error occurred
 *  \param nLinenb the linenumber of the error
 *  \param sMsg the error message
 *  \param vl the argument list to be printed
 *
 * The given front-end object can be used to determine the file this object
 * belonged to and the line, where this object has been declared. This is
 * useful if we do not have a current line number available (any time after
 * the parsing finished).
 */
void 
CCompiler::GccErrorVL(CFEBase * pFEObject,
    int nLinenb,
    const char *sMsg,
    va_list vl)
{
    if (pFEObject)
    {
        // check line number
        if (nLinenb == 0)
            nLinenb = pFEObject->GetSourceLine();
        // iterate through include hierarchy
        CFEFile *pCur = (CFEFile*)pFEObject->GetSpecificParent<CFEFile>(0);
        vector<CFEFile*> *pStack = new vector<CFEFile*>();
        while (pCur)
        {
            pStack->insert(pStack->begin(), pCur);
            // get file of parent because GetFile start with "this"
            pCur = pCur->GetSpecificParent<CFEFile>(1);
        }
        // need at least one file
        if (!pStack->empty())
        {
            // walk down
            if (pStack->size() > 1)
                std::cerr << "In file included ";
            vector<CFEFile*>::iterator iter;
            for (iter = pStack->begin();
                (iter != pStack->end()) && (iter != pStack->end()-1); iter++)
            {
		// we do not use GetFullFileName, because the "normal"
		// filename already includes the whole path it is the filename
		// generated by Gcc
                CFEFile *pFEFile = *iter;
                int nIncludeLine = 1;
                if (iter != pStack->end()-1)
                    nIncludeLine = (*(iter+1))->GetIncludedOnLine();
		std::cerr << "from " << pFEFile->GetFileName() << ":" << nIncludeLine;
                if (iter + 2 != pStack->end())
                    std::cerr << ",\n                 ";
                else
                    std::cerr << ":\n";
            }
            if (*iter)
            {
		// we do not use GetFullFileName, because the "normal"
		// filename already includes the whole path it is the filename
		// generated by Gcc
                std::cerr << (*iter)->GetFileName() << ":" << nLinenb << ": ";
            }
        }
        // cleanup
        delete pStack;
    }
    vfprintf(stderr, sMsg, vl);
    std::cerr << std::endl;
}

/**
 *  \brief helper function
 *  \param sMsg the warning message to print
 *
 * This method prints an error message and returns.
 */
void CCompiler::Warning(const char *sMsg, ...)
{
    va_list args;
    va_start(args, sMsg);
    vfprintf(stderr, sMsg, args);
    va_end(args);
    std::cerr << std::endl;
}

/** \brief print verbose message
 *  \param level verboseness level of message
 *  \param format the format string
 */
void CCompiler::Verbose(ProgramVerbose_Type level, const char *format, ...)
{
    if (!IsVerboseLevel(level))
	return;
    
    if (m_nVerboseInd >= 0)
	fprintf(stdout, "[%02d] ", m_nVerboseInd);
    
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    fflush (stdout);
}

/** \brief print verbose message
 *  \param level verboseness level of message
 *  \param format the format string
 */
void CCompiler::VerboseI(ProgramVerbose_Type level, const char *format, ...)
{
    if (!IsVerboseLevel(level))
	return;
    
    m_nVerboseInd++;
    if (m_nVerboseInd >= 0)
	fprintf(stdout, "[%02d] ", m_nVerboseInd);
    
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    fflush (stdout);
}

/** \brief print verbose message
 *  \param level verboseness level of message
 *  \param format the format string
 */
void CCompiler::VerboseD(ProgramVerbose_Type level, const char *format, ...)
{
    if (!IsVerboseLevel(level))
	return;
    
    if (m_nVerboseInd >= 0)
	fprintf(stdout, "[%02d] ", m_nVerboseInd);
    if (m_nVerboseInd > 0)
	m_nVerboseInd--;
    
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    fflush (stdout);
}

/** \brief print warning messages
 *  \param pFEObject the object where the warning occured
 *  \param nLinenb the line in the file where the object originated
 *  \param sMsg the warning message
 */
void CCompiler::GccWarning(CFEBase * pFEObject, int nLinenb, const char *sMsg, ...)
{
    va_list args;
    va_start(args, sMsg);
    GccWarningVL(pFEObject, nLinenb, sMsg, args);
    va_end(args);
}

/** \brief print warning messages
 *  \param pFEObject the object where the warning occured
 *  \param nLinenb the line in the file where the object originated
 *  \param sMsg the warning message
 *  \param vl teh variable argument list
 */
void CCompiler::GccWarningVL(CFEBase * pFEObject, int nLinenb, const char *sMsg, va_list vl)
{
    erroccured++;
    warningcount++;
    if (pFEObject)
    {
        // check line number
        if (nLinenb == 0)
            nLinenb = pFEObject->GetSourceLine();
        // iterate through include hierarchy
        CFEFile *pCur = (CFEFile*)pFEObject->GetSpecificParent<CFEFile>(0);
        vector<CFEFile*> *pStack = new vector<CFEFile*>();
        while (pCur)
        {
            pStack->insert(pStack->begin(), pCur);
            // start with parent of current, because GetFile starts with "this"
            pCur = pCur->GetSpecificParent<CFEFile>();
        }
        // need at least one file
        if (!pStack->empty())
        {
            // walk down
            if (pStack->size() > 1)
                std::cerr << "In file included ";
            vector<CFEFile*>::iterator iter;
            for (iter = pStack->begin();
                (iter != pStack->end()) && (iter != pStack->end()-1); iter++)
            {
                std::cerr << "from " << (*iter)->GetFileName() << ":1";
                if (iter+2 != pStack->end())
                    std::cerr << ",\n                 ";
                else
                    std::cerr << ":\n";
            }
            if (*iter)
            {
                std::cerr << (*iter)->GetFileName() << ":" << nLinenb << ": warning: ";
            }
        }
        // cleanup
        delete pStack;
    }
    vfprintf(stderr, sMsg, vl);
    std::cerr << "\n";
}

