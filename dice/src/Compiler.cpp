/**
 *    \file    dice/src/Compiler.cpp
 *    \brief   contains the implementation of the class CCompiler
 *
 *    \date    03/06/2001
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "Compiler.h"

#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/timeb.h>
#include <errno.h>
#include <limits.h> // needed for realpath
#include <stdlib.h>
#include <string.h>

#include <string>
#include <algorithm>
using namespace std;

#if defined(HAVE_GETOPT_H)
#include <getopt.h>
#endif

#include "File.h"
#include "CParser.h"
#include "CPreProcess.h"
#include "ProgramOptions.h"
#include "be/BERoot.h"
// L4 specific
#include "be/l4/L4BENameFactory.h"
// L4V2
#include "be/l4/v2/L4V2BEClassFactory.h"
// L4X0
#include "be/l4/x0/L4X0BEClassFactory.h"
// L4X0 Arm
#include "be/l4/x0/arm/X0ArmClassFactory.h"
// L4X0 IA32
#include "be/l4/x0/ia32/X0IA32ClassFactory.h"
// L4 X0 adaption (V2 user land to X0 kernel)
#include "be/l4/x0adapt/L4X0aBEClassFactory.h"
// L4V4
#include "be/l4/v4/L4V4BEClassFactory.h"
#include "be/l4/v4/ia32/L4V4IA32ClassFactory.h"
#include "be/l4/v4/L4V4BENameFactory.h"
// Sockets
#include "be/sock/SockBEClassFactory.h"
// CDR
#include "be/cdr/CCDRClassFactory.h"

#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"

//@{
/** some config variables */
extern const char* dice_version;
extern const char* dice_build;
extern const char* dice_user;
//@}

/** defines the number of characters for dependency output per line */
#define MAX_SHELL_COLS 80

/////////////////////////////////////////////////////////////////////////////////
// error handling
//@{
/** globale variables used by the parsers to count errors and warnings */
int errcount = 0;
int erroccured = 0;
int warningcount = 0;
//@}

/**    debugging helper variable */
int nGlobalDebug = 0;

//@{
/** global variables for argument parsing */
extern char *optarg;
extern int optind, opterr, optopt;
//@}

CCompiler::CCompiler()
{
    m_bVerbose = false;
    m_nUseFrontEnd = USE_FE_NONE;
    m_nOpcodeSize = 0;
    m_pContext = 0;
    m_pRootBE = 0;
    m_nWarningLevel = 0;
    m_sSymbols = 0;
    m_nSymbolCount = 0;
    m_nDumpMsgBufDwords = 0;
    for (int i=0; i<PROGRAM_OPTION_GROUPS; i++)
        m_nOptions[i] = 0;
    m_nBackEnd = 0;
}

/** cleans up the compiler object */
CCompiler::~CCompiler()
{
    for (int i=0; i<m_nSymbolCount-1; i++)
        free(m_sSymbols[i]);
    free(m_sSymbols);
    // we delete the preprocessor here, because ther should be only
    // one for the whole compiler run.
    CPreProcess *pPreProcess = CPreProcess::GetPreProcessor();
    delete pPreProcess;
}

/**
 *    \brief parses the arguments of the compiler call
 *    \param argc the number of arguments
 *    \param argv the arguments
 */
void CCompiler::ParseArguments(int argc, char *argv[])
{
    int c;
#if defined(HAVE_GETOPT_LONG)
    int index = 0;
#endif
    bool bHaveFileName = false;
    unsigned long nNoWarning = 0;

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
        {"with-cpp", 1, 0, 'x'},
        {"o", 1, 0, 'o'},
        {"M", 2, 0, 'M'},
        {"W", 1, 0, 'W'},
        {"D", 1, 0, 'D'},
        {"help", 0, 0, 'h'},
        {"help", 0, 0, '?'},
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
        c = getopt_long_only(argc, argv, "cstni::v::hF:p:CP:f:B:E::O::VI:NT::M::W:D:x:", long_options, &index);
#else
        // n has an optional parameter to recognize -nostdinc and -n (no-opcode)
        c = getopt(argc, argv, "cstn::i::v::hF:p:CP:f:B:E::O::VI:NT::M::W:D:x:o:");
#endif

        if (c == -1)
        {
            bHaveFileName = true;
            break;        // skip - evaluate later
        }

        Verbose("Read argument %c\n", c);

        switch (c)
        {
        case '?':
            // Error exits
#if defined(HAVE_GETOPT_LONG)
            Error("unrecognized option: %s (%d)\nUse \'--help\' to show valid options.\n", argv[optind - 1], optind);
#else
            Error("unrecognized option: %s (%d)\nUse \'-h\' to show valid options.\n", argv[optind], optind);
#endif
            break;
        case ':':
            // Error exits
            Error("missing argument for option: %s\n", argv[optind - 1]);
            break;
        case 'c':
            Verbose("Create client-side code.\n");
            SetOption(PROGRAM_GENERATE_CLIENT);
            break;
        case 's':
            Verbose("Create server/component-side code.\n");
            SetOption(PROGRAM_GENERATE_COMPONENT);
            break;
        case 't':
            Verbose("create skeletons enabled\n");
            SetOption(PROGRAM_GENERATE_TEMPLATE);
            break;
        case 'i':
            {
                // there may follow an optional argument stating whether this is "static" or "extern" inline
                SetOption(PROGRAM_GENERATE_INLINE);
                if (!optarg)
                {
                    Verbose("create client stub as inline\n");
                }
                else
                {
                    // make upper case
                    string sArg(optarg);
                    transform(sArg.begin(), sArg.end(), sArg.begin(), toupper);
                    while (sArg[0] == '=')
                        sArg.erase(sArg.begin());
                    if (sArg == "EXTERN")
                    {
                        SetOption(PROGRAM_GENERATE_INLINE_EXTERN);
                        UnsetOption(PROGRAM_GENERATE_INLINE_STATIC);
                        Verbose("create client stub as extern inline\n");
                    }
                    else if (sArg == "STATIC")
                    {
                        SetOption(PROGRAM_GENERATE_INLINE_STATIC);
                        UnsetOption(PROGRAM_GENERATE_INLINE_EXTERN);
                        Verbose("create client stub as static inline\n");
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
                    Verbose("create no opcodes\n");
                    SetOption(PROGRAM_NO_OPCODES);
                }
                else if (sArg == "ostdinc")
                {
                    Verbose("no standard include paths\n");
                    pPreProcess->AddCPPArgument(string("-nostdinc"));
                }
            }
#else
            Verbose("create no opcodes\n");
            SetOption(PROGRAM_NO_OPCODES);
#endif
            break;
        case 'v':
            {
                SetOption(PROGRAM_VERBOSE);
                m_bVerbose = true;
                if (!optarg)
                {
                    m_nVerboseLevel = 0;
                    Verbose("Verbose level %d enabled\n", m_nVerboseLevel);
                }
                else
                {
                    m_nVerboseLevel = atoi(optarg);
                    if ((m_nVerboseLevel < 0) || (m_nVerboseLevel > PROGRAM_VERBOSE_MAXLEVEL))
                    {
                        Warning("dice: Verbose level %d not supported in this version.", m_nVerboseLevel);
                        m_nVerboseLevel = 0;
                    }
                    Verbose("Verbose level %d enabled\n", m_nVerboseLevel);
                    switch (m_nVerboseLevel)
                    {
                    case 1:
                        m_nVerboseLevel = PROGRAM_VERBOSE_LEVEL_1;
                        break;
                    case 2:
                        m_nVerboseLevel = PROGRAM_VERBOSE_LEVEL_2;
                        break;
                    case 3:
                        m_nVerboseLevel = PROGRAM_VERBOSE_LEVEL_3;
                        break;
                    case 4:
                        m_nVerboseLevel = PROGRAM_VERBOSE_LEVEL_4;
                        break;
                    case 5:
                        m_nVerboseLevel = PROGRAM_VERBOSE_LEVEL_5;
                        break;
                    case 6:
                        m_nVerboseLevel = PROGRAM_VERBOSE_LEVEL_6;
                        break;
                    case 7:
                        m_nVerboseLevel = PROGRAM_VERBOSE_LEVEL_7;
                        m_bVerbose = false;
                        UnsetOption(PROGRAM_VERBOSE);
                        break;
                    default:
                        m_nVerboseLevel = 0;
                        break;
                    }
                }
            }
            break;
        case 'h':
            ShowHelp();
            break;
        case 'F':
            Verbose("file prefix %s used\n", optarg);
            m_sFilePrefix = optarg;
            break;
        case 'p':
            Verbose("include prefix %s used\n", optarg);
            m_sIncludePrefix = optarg;
            // remove tailing slashes
            while (*(m_sIncludePrefix.end()-1) == '/')
                m_sIncludePrefix.erase(m_sIncludePrefix.end()-1);
            break;
        case 'C':
            Verbose("use the CORBA frontend\n");
            m_nUseFrontEnd = USE_FE_CORBA;
            break;
        case 'P':
            Verbose("preprocessor option %s added\n", optarg);
            {
                // check for -I arguments, which we preprocess ourselves as well
                string sArg = optarg;
                pPreProcess->AddCPPArgument(sArg);
                if (sArg.substr(0, 2) == "-I")
                {
                    string sPath = sArg.substr(2);
                    pPreProcess->AddIncludePath(sPath);
                    Verbose("Added %s to include paths\n", sPath.c_str());
                }
                else if (sArg.substr(0, 2) == "-D")
                {
                    string sDefine = sArg.substr(2);
                    AddSymbol(sDefine);
                    Verbose("Found symbol \"%s\"\n", sDefine.c_str());
                }
            }
            break;
        case 'I':
            Verbose("add include path %s\n", optarg);
            {
                string sPath("-I");
                sPath += optarg;
                pPreProcess->AddCPPArgument(sPath);    // copies sPath
                // add to own include paths
                pPreProcess->AddIncludePath(optarg);
                Verbose("Added %s to include paths\n", optarg);
            }
            break;
        case 'N':
            Verbose("no standard include paths\n");
            pPreProcess->AddCPPArgument(string("-nostdinc"));
            break;
        case 'f':
            {
                // provide flags to compiler: this can be anything
                // make upper case
                string sArg = optarg;
                string sOrig = sArg;
                transform(sArg.begin(), sArg.end(), sArg.begin(), toupper);
                // test first letter
                char cFirst = sArg[0];
                switch (cFirst)
                {
                case 'A':
                    if (sArg == "ALIGN-TO-TYPE")
                    {
                        SetOption(PROGRAM_ALIGN_TO_TYPE);
                        Verbose("align parameters in message buffer to type\n");
                    }
                    break;
                case 'C':
                    if (sArg == "CTYPES")
                    {
                        SetOption(PROGRAM_USE_CTYPES);
                        Verbose("use C-style type names\n");
                    }
                    else if (sArg == "CONST-AS-DEFINE")
                    {
                        SetOption(PROGRAM_CONST_AS_DEFINE);
                        Verbose("print const declarators as define statements\n");
                    }
                    break;
                case 'F':
                    if (sArg == "FORCE-CORBA-ALLOC")
                    {
                        SetOption(PROGRAM_FORCE_CORBA_ALLOC);
                        Verbose("Force use of CORBA_alloc (instead of Environment->malloc).\n");
                        if (IsOptionSet(PROGRAM_FORCE_ENV_MALLOC))
                        {
                            UnsetOption(PROGRAM_FORCE_ENV_MALLOC);
                            Verbose("Overrides previous -fforce-env-malloc.\n");
                        }
                    }
                    else if (sArg == "FORCE-ENV-MALLOC")
                    {
                        SetOption(PROGRAM_FORCE_ENV_MALLOC);
                        Verbose("Force use of Environment->malloc (instead of CORBA_alloc).\n");
                        if (IsOptionSet(PROGRAM_FORCE_CORBA_ALLOC))
                        {
                            UnsetOption(PROGRAM_FORCE_CORBA_ALLOC);
                            Verbose("Overrides previous -fforce-corba-alloc.\n");
                        }
                    }
                    else if (sArg == "FORCE-C-BINDINGS")
                    {
                        SetOption(PROGRAM_FORCE_C_BINDINGS);
                        Verbose("Force use of L4 C bindings (instead of inline assembler).\n");
                    }
                    else if (sArg == "FREE-MEM-AFTER-REPLY")
                    {
                        SetOption(PROGRAM_FREE_MEM_AFTER_REPLY);
                        Verbose("Free memory pointers stored in environment after reply.\n");
                    }
                    else
                    {
                        // XXX FIXME: test for 'FILETYPE=' too
                        UnsetOption(PROGRAM_FILE_MASK);    // remove previous setting
                        if ((sArg == "FIDLFILE") || (sArg == "F1"))
                        {
                            SetOption(PROGRAM_FILE_IDLFILE);
                            Verbose("filetype is set to IDLFILE\n");
                        }
                        else if ((sArg == "FMODULE") || (sArg == "F2"))
                        {
                            SetOption(PROGRAM_FILE_MODULE);
                            Verbose("filetype is set to MODULE\n");
                        }
                        else if ((sArg == "FINTERFACE") || (sArg == "F3"))
                        {
                            SetOption(PROGRAM_FILE_INTERFACE);
                            Verbose("filetype is set to INTERFACE\n");
                        }
                        else if ((sArg == "FFUNCTION") || (sArg == "F4"))
                        {
                            SetOption(PROGRAM_FILE_FUNCTION);
                            Verbose("filetype is set to FUNCTION\n");
                        }
                        else if ((sArg == "FALL") || (sArg == "F5"))
                        {
                            SetOption(PROGRAM_FILE_ALL);
                            Verbose("filetype is set to ALL\n");
                        }
                        else
                        {
                            Error("\"%s\" is an invalid argument for option -ff\n", &optarg[1]);
                        }
                    }
                    break;
                case 'G':
                    {
                        // PROGRAM_GENERATE_LINE_DIRECTIVE
                        if (sArg == "GENERATE-LINE-DIRECTIVE")
                        {
                            SetOption(PROGRAM_GENERATE_LINE_DIRECTIVE);
                            Verbose("generating IDL file line directives in target code");
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
                            Verbose("User provides function \"%s\" to init indirect receive strings\n", sName.c_str());
                            m_sInitRcvStringFunc = sName;
                        }
                        else
                            Verbose("User provides function to init indirect receive strings\n");
                    }
                    break;
                case 'K':
                    if (sArg == "KEEP-TEMP-FILES")
                    {
                        pPreProcess->SetOption(PROGRAM_KEEP_TMP_FILES);
                        SetOption(PROGRAM_KEEP_TMP_FILES);
                        Verbose("Keep temporary files generated during preprocessing\n");
                    }
                    break;
                case 'L':
                    if (sArg == "L4TYPES")
                    {
                        SetOption(PROGRAM_USE_L4TYPES);
                        SetOption(PROGRAM_USE_CTYPES);
                        Verbose("use L4-style type names\n");
                    }
                    break;
                case 'N':
                    if (sArg == "NO-SEND-CANCELED-CHECK")
                    {
                        SetOption(PROGRAM_NO_SEND_CANCELED_CHECK);
                        Verbose("Do not check if the send part of a call was canceled.\n");
                    }
                    else if ((sArg == "NO-SERVER-LOOP") ||
                             (sArg == "NO-SRVLOOP"))
                    {
                        SetOption(PROGRAM_NO_SERVER_LOOP);
                        Verbose("Do not generate server loop.\n");
                    }
                    else if ((sArg == "NO-DISPATCH") ||
                             (sArg == "NO-DISPATCHER"))
                    {
                        SetOption(PROGRAM_NO_DISPATCHER);
                        Verbose("Do not generate dispatch function.\n");
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
                            Verbose("Set size of opcode type to %d bytes\n", m_nOpcodeSize);
                        }
                        else
                            Verbose("The opcode-size \"%s\" is not supported\n", sType.c_str());
                    }
                    break;
                case 'S':
                    if (sArg == "SERVER-PARAMETER")
                    {
                        SetOption(PROGRAM_SERVER_PARAMETER);
                        Verbose("User provides CORBA_Environment parameter to server loop\n");
                    }
                    break;
                case 'T':
                    if (sArg.substr(0, 12) == "TRACE-SERVER")
                    {
                        SetOption(PROGRAM_TRACE_SERVER);
                        Verbose("Trace messages received by the server loop\n");
                        if (sArg.length() > 12)
                        {
                            string sName = sOrig.substr(13);
                            Verbose("User sets trace function to \"%s\".\n", sName.c_str());
                            m_sTraceServerFunc = sName;
                        }
                    }
                    else if (sArg.substr(0, 12) == "TRACE-CLIENT")
                    {
                        SetOption(PROGRAM_TRACE_CLIENT);
                        Verbose("Trace messages send to the server and answers received from it\n");
                        if (sArg.length() > 12)
                        {
                            string sName = sOrig.substr(13);
                            Verbose("User sets trace function to \"%s\".\n", sName.c_str());
                            m_sTraceClientFunc = sName;
                        }
                    }
                    else if (sArg.substr(0, 17) == "TRACE-DUMP-MSGBUF")
                    {
                        SetOption(PROGRAM_TRACE_MSGBUF);
                        Verbose("Trace the message buffer struct\n");
                        if (sArg.length() > 17)
                        {
                            // check if lines are meant
                            if (sArg.substr(0, 24) == "TRACE-DUMP-MSGBUF-DWORDS")
                            {
                                if (sArg.length() > 24)
                                {
                                    string sNumber = sArg.substr(25);
                                    m_nDumpMsgBufDwords = atoi(sNumber.c_str());
                                    if (m_nDumpMsgBufDwords >= 0)
                                        SetOption(PROGRAM_TRACE_MSGBUF_DWORDS);
                                }
                                else
                                    Error("The option -ftrace-dump-msgbuf-dwords expects an argument (e.g. -ftrace-dump-msgbuf-dwords=10).\n");
                            }
                            else
                            {
                                string sName = sOrig.substr(18);
                                Verbose("User sets trace function to \"%s\".\n", sName.c_str());
                                m_sTraceMsgBuf = sName;
                            }
                        }
                    }
                    else if (sArg.substr(0, 14) == "TRACE-FUNCTION")
                    {
                        if (sArg.length() > 14)
                        {
                            string sName = sOrig.substr(15);
                            Verbose("User sets trace function to \"%s\".\n", sName.c_str());
                            if (m_sTraceServerFunc.empty())
                                m_sTraceServerFunc = sName;
                            if (m_sTraceClientFunc.empty())
                                m_sTraceClientFunc = sName;
                            if (m_sTraceMsgBuf.empty())
                                m_sTraceMsgBuf = sName;
                        }
                        else
                            Error("The option -ftrace-function expects an argument (e.g. -ftrace-function=LOGl).\n");
                    }
                    else if ((sArg == "TEST-NO-SUCCESS") || (sArg == "TEST-NO-SUCCESS-MESSAGE"))
                    {
                        Verbose("User does not want to see success messages of testsuite.\n");
                        SetOption(PROGRAM_TESTSUITE_NO_SUCCESS_MESSAGE);
                    }
                    else if (sArg == "TESTSUITE-SHUTDOWN")
                    {
                        Verbose("User wants to shutdown Fiasco after running the testsuite.\n");
                        SetOption(PROGRAM_TESTSUITE_SHUTDOWN_FIASCO);
                    }
                    break;
                case 'U':
                    if ((sArg == "USE-SYMBOLS") || (sArg == "USE-DEFINES"))
                    {
                        SetOption(PROGRAM_USE_SYMBOLS);
                        Verbose("Use the symbols/defines given as arguments\n");
                    }
                    break;
                case 'Z':
                    if (sArg == "ZERO-MSGBUF")
                    {
                        SetOption(PROGRAM_ZERO_MSGBUF);
                        Verbose("Zero message buffer before using.\n");
                    }
                    break;
                default:
                    // XXX FIXME: might be old-style file-types
                    Warning("unsupported argument \"%s\" for option -f\n", sArg.c_str());
                    break;
                }
            }
            break;
        case 'B':
            {
                // make upper case
                string sArg(optarg);
                transform(sArg.begin(), sArg.end(), sArg.begin(), toupper);

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
                        m_nBackEnd &= ~PROGRAM_BE_PLATFORM;
                        m_nBackEnd |= PROGRAM_BE_IA32;
                        Verbose("use back-end for IA32 platform\n");
                    }
                    else if (sArg == "IA64")
                    {
                        Warning("IA64 back-end not supported yet!");
                        m_nBackEnd &= ~PROGRAM_BE_PLATFORM;
                        m_nBackEnd |= PROGRAM_BE_IA32; // PROGRAM_BE_IA64;
                        Verbose("use back-end for IA64 platform\n");
                    }
                    else if (sArg == "ARM")
                    {
                        m_nBackEnd &= ~PROGRAM_BE_PLATFORM;
                        m_nBackEnd |= PROGRAM_BE_ARM;
                        Verbose("use back-end for ARM platform\n");
                    }
                    else if (sArg == "AMD64")
                    {
                        m_nBackEnd &= ~PROGRAM_BE_PLATFORM;
                        m_nBackEnd |= PROGRAM_BE_AMD64;
                        Verbose("use back-end for AMD64 platform\n");
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
                        m_nBackEnd &= ~PROGRAM_BE_INTERFACE;
                        m_nBackEnd |= PROGRAM_BE_V2;
                        Verbose("use back-end L4 version 2\n");
                    }
                    else if (sArg == "X0")
                    {
                        m_nBackEnd &= ~PROGRAM_BE_INTERFACE;
                        m_nBackEnd |= PROGRAM_BE_X0; // PROGRAM_BE_X0;
                        Verbose("use back-end L4 version X.0\n");
                    }
                    else if (sArg == "X0ADAPT")
                    {
                        m_nBackEnd &= ~PROGRAM_BE_INTERFACE;
                        m_nBackEnd |= PROGRAM_BE_X0ADAPT;
                        Verbose("use back-end Adaption of L4 V2 user land to X0 kernel\n");
                    }
                    else if ((sArg == "X2") || (sArg == "V4"))
                    {
                        m_nBackEnd &= ~PROGRAM_BE_INTERFACE;
                        m_nBackEnd |= PROGRAM_BE_V4;
                        Verbose("use back-end L4 version 4 (X.2)\n");
                    }
                    else if (sArg == "FLICK")
                    {
                        Warning("Flick compatibility mode is no longer supported");
                    }
                    else if ((sArg == "SOCKETS") || (sArg == "SOCK"))
                    {
                        m_nBackEnd &= ~PROGRAM_BE_INTERFACE;
                        m_nBackEnd |= PROGRAM_BE_SOCKETS;
                        Verbose("use sockets back-end\n");
                    }
                    else if (sArg == "CDR")
                    {
                        m_nBackEnd &= ~PROGRAM_BE_INTERFACE;
                        m_nBackEnd |= PROGRAM_BE_CDR;
                        Verbose("use CDR back-end\n");
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
                        m_nBackEnd &= ~PROGRAM_BE_LANGUAGE;
                        m_nBackEnd |= PROGRAM_BE_C;
                        Verbose("use back-end C language mapping\n");
                    }
                    else if (sArg == "CPP")
                    {
                        m_nBackEnd &= ~PROGRAM_BE_LANGUAGE;
                        m_nBackEnd |= PROGRAM_BE_CPP;
                        Verbose("use back-end C++ language mapping\n");
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
                Verbose("stop after preprocess option enabled.\n");
                SetOption(PROGRAM_STOP_AFTER_PRE);
            }
            else
            {
                string sArg(optarg);
                transform(sArg.begin(), sArg.end(), sArg.begin(), toupper);
                if (sArg == "XML")
                {
                    Verbose("stop after preprocessing and dump XML file.\n");
                    SetOption(PROGRAM_STOP_AFTER_PRE_XML);
                }
                else
                {
                    Warning("unrecognized argument \"%s\" to option -E.\n", sArg.c_str());
                    SetOption(PROGRAM_STOP_AFTER_PRE);
                }
            }
            break;
        case 'O':
            Warning("Option -O not supported anylonger.\n");
            break;
        case 'o':
            if (!optarg)
            {
                Error("Option -o requires argument\n");
            }
            else
            {
                Verbose("output directory set to %s\n", optarg);
                m_sOutputDir = optarg;
            }
            break;
        case 'T':
            if (optarg)
            {
                string sKind(optarg);
                transform(sKind.begin(), sKind.end(), sKind.begin(), tolower);
                if (sKind == "thread")
                    SetOption(PROGRAM_GENERATE_TESTSUITE_THREAD);
                else if (sKind == "task")
                    SetOption(PROGRAM_GENERATE_TESTSUITE_TASK);
                else
                {
                    Warning("dice: Testsuite option \"%s\" not supported. Using \"thread\".", optarg);
                    SetOption(PROGRAM_GENERATE_TESTSUITE_THREAD);
                }
            }
            else
                SetOption(PROGRAM_GENERATE_TESTSUITE_THREAD);
            Verbose("generate testsuite for IDL.\n");
            // need server stubs for that
            SetOption(PROGRAM_GENERATE_TEMPLATE);
            // need client and server files
            SetOption(PROGRAM_GENERATE_CLIENT);
            SetOption(PROGRAM_GENERATE_COMPONENT);
            break;
        case 'V':
            ShowVersion();
            break;
        case 'm':
            Verbose("generate message passing functions.\n");
            SetOption(PROGRAM_GENERATE_MESSAGE);
            break;
        case 'M':
            {
                UnsetOption(PROGRAM_DEPEND_MASK);
                // search for depedency options
                if (!optarg)
                {
                    SetOption(PROGRAM_DEPEND_M);
                    Verbose("Create dependencies without argument\n");
                }
                else
                {
                    // make upper case
                    string sArg = optarg;
                    transform(sArg.begin(), sArg.end(), sArg.begin(), toupper);
                    if (sArg == "M")
                    {
                        SetOption(PROGRAM_DEPEND_MM);
                    }
                    else if (sArg == "D")
                    {
                        SetOption(PROGRAM_DEPEND_MD);
                    }
                    else if (sArg == "MD")
                    {
                        SetOption(PROGRAM_DEPEND_MMD);
                    }
                    else
                    {
                        Warning("dice: Argument \"%s\" of option -M unrecognized: ignoring.", optarg);
                        SetOption(PROGRAM_DEPEND_M);
                    }
                    Verbose("Create dependencies with argument %s\n", optarg);
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
                transform(sArg.begin(), sArg.end(), sArg.begin(), toupper);
                if (sArg == "ALL")
                {
                    m_nWarningLevel |= PROGRAM_WARNING_ALL;
                    Verbose("All warnings on.\n");
                }
                else if (sArg == "NO-ALL")
                {
                    nNoWarning |= PROGRAM_WARNING_ALL;
                    Verbose("All warnings off.\n");
                }
                else if (sArg == "IGNORE-DUPLICATE-FID")
                {
                    m_nWarningLevel |= PROGRAM_WARNING_IGNORE_DUPLICATE_FID;
                    Verbose("Warn if duplicate function IDs are ignored.\n");
                }
                else if (sArg == "NO-IGNORE-DUPLICATE-FID")
                {
                    nNoWarning |= PROGRAM_WARNING_IGNORE_DUPLICATE_FID;
                    Verbose("Do not warn if duplicate function IDs are ignored.\n");
                }
                else if (sArg == "PREALLOC")
                {
                    m_nWarningLevel |= PROGRAM_WARNING_PREALLOC;
                    Verbose("Warn if CORBA_alloc is used.\n");
                }
                else if (sArg == "NO-PREALLOC")
                {
                    nNoWarning |= PROGRAM_WARNING_PREALLOC;
                    Verbose("Do not warn if CORBA_alloc is used.\n");
                }
                else if (sArg == "MAXSIZE")
                {
                    m_nWarningLevel |= PROGRAM_WARNING_NO_MAXSIZE;
                    Verbose("Warn if max-size attribute is not set for unbound variable sized arguments.\n");
                }
                else if (sArg == "NO-MAXSIZE")
                {
                    nNoWarning |= PROGRAM_WARNING_NO_MAXSIZE;
                    Verbose("Do not warn if max-size attribute is not set for unbound variable sized arguments.\n");
                }
                else if (sArg.substr(0, 2) == "P,")
                {
                    string sTmp(optarg);
                    // remove "p,"
                    string sCppArg = sTmp.substr(2);
                    pPreProcess->AddCPPArgument(sCppArg);
                    Verbose("preprocessor argument \"%s\" added\n", sCppArg.c_str());
                    // check if CPP argument has special meaning
                    if (sCppArg.substr(0, 2) == "-I")
                    {
                        string sPath = sCppArg.substr(2);
                        pPreProcess->AddIncludePath(sPath);
                        Verbose("Added %s to include paths\n", sPath.c_str());
                    }
                    else if (sCppArg.substr(0, 2) == "-D")
                    {
                        string sDefine = sCppArg.substr(2);
                        AddSymbol(sDefine);
                        Verbose("Found symbol \"%s\"\n", sDefine.c_str());
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
                AddSymbol(optarg);
                Verbose("Found symbol \"%s\"\n", optarg);
            }
            break;
        case 'x':
            {
                string sArg = optarg;
                if (!pPreProcess->SetCPP(sArg.c_str()))
                    Error("Preprocessor \"%s\" does not exist.\n", optarg);
                Verbose("Use \"%s\" as preprocessor\n", optarg);
            }
            break;
        default:
            Error("You used an obsolete parameter (%c).\nPlease use dice --help to check your parameters.\n", c);
        }
    }

    if (bHaveFileName && (argc > 1))
    {
        // infile left (should be last)
        // if argv is "-" the stdin should be used: set m_sInFileName to 0
        if (strcmp(argv[argc - 1], "-"))
            m_sInFileName = argv[argc - 1];
        else
            m_sInFileName = "";
        Verbose("Input file is: %s\n", m_sInFileName.c_str());
    }

    if (m_nUseFrontEnd == USE_FE_NONE)
        m_nUseFrontEnd = USE_FE_DCE;

    if (!IsOptionSet(PROGRAM_FILE_MASK))
        SetOption(PROGRAM_FILE_IDLFILE);

    if ((m_nBackEnd & PROGRAM_BE_INTERFACE) == 0)
    {
        if (m_nBackEnd & PROGRAM_BE_ARM)
            m_nBackEnd |= PROGRAM_BE_X0;
        else
            m_nBackEnd |= PROGRAM_BE_V2;
    }
    if ((m_nBackEnd & PROGRAM_BE_PLATFORM) == 0)
        m_nBackEnd |= PROGRAM_BE_IA32;
    if ((m_nBackEnd & PROGRAM_BE_LANGUAGE) == 0)
        m_nBackEnd |= PROGRAM_BE_C;
    if ((m_nBackEnd & PROGRAM_BE_ARM) &&
        ((m_nBackEnd & PROGRAM_BE_INTERFACE) != PROGRAM_BE_X0))
    {
        Warning("The Arm Backend currently works with X0 native only!");
        Warning("  -> Setting interface to X0 native.");
        m_nBackEnd &= ~PROGRAM_BE_INTERFACE;
        m_nBackEnd |= PROGRAM_BE_X0;
    }
    if ((m_nBackEnd & PROGRAM_BE_AMD64) &&
        ((m_nBackEnd & PROGRAM_BE_INTERFACE) != PROGRAM_BE_V2))
    {
        Warning("The AMD64 Backend currently works with V2 native only!");
        Warning("  -> Setting interface to V2 native.");
        m_nBackEnd &= ~PROGRAM_BE_INTERFACE;
        m_nBackEnd |= PROGRAM_BE_V2;
    }
    // with arm we *have to* marshal type aligned
    if (m_nBackEnd & PROGRAM_BE_ARM)
        SetOption(PROGRAM_ALIGN_TO_TYPE);
    // with AMD64 we *have to* use C bindings
    if (m_nBackEnd & PROGRAM_BE_AMD64)
        SetOption(PROGRAM_FORCE_C_BINDINGS);

    if (!IsOptionSet(PROGRAM_GENERATE_CLIENT) &&
        !IsOptionSet(PROGRAM_GENERATE_COMPONENT))
    {
        /* per default generate client AND server */
        SetOption(PROGRAM_GENERATE_CLIENT);
        SetOption(PROGRAM_GENERATE_COMPONENT);
    }

    if (IsOptionSet(PROGRAM_GENERATE_TESTSUITE_THREAD) ||
        IsOptionSet(PROGRAM_GENERATE_TESTSUITE_TASK))
    {
        UnsetOption(PROGRAM_NO_SERVER_LOOP);
        SetOption(PROGRAM_FORCE_CORBA_ALLOC);
    }

    if (nNoWarning != 0)
    {
        m_nWarningLevel &= ~nNoWarning;
    }
}

/** displays a copyright notice of this compiler */
void CCompiler::ShowCopyright()
{
    if (!m_bVerbose)
        return;
    printf("DICE (c) 2001-2003 Dresden University of Technology\n"
           "Author: Ronald Aigner <ra3@os.inf.tu-dresden.de>\n"
           "e-Mail: dice@os.inf.tu-dresden.de\n\n");
}

/** displays a help for this compiler */
void CCompiler::ShowHelp()
{
    ShowCopyright();
    printf("Usage: dice [<options>] <idl-file>\n"
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
                    " <level>      print verbose output\n"
    "    set <level> to 1 to let only the back-end generate output\n"
    "    set <level> to 2 to let back-end and data-representation generate\n"
    "                            output\n"
    "    set <level> to 3 to let back-end, data-representation and front-end\n"
    "                            generate output\n"
    "    set <level> to 4 to let the factories (class/name) generate output\n"
    "    set <level> to 5 to let all of the above generate output\n"
    "    set <level> to 6 to let all of the above plus parser generate output\n"
    "    set <level> to 7 to let only the parser generate output\n"
    "    if you don't specify a level, level 1 is assumed\n"
    " -C"
#if defined(HAVE_GETOPT_LONG)
    ", --corba"
#else
    "         "
#endif
    "                use the CORBA front-end (scan CORBA IDL)\n"
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
    " -EXML                      stop after parsing and dump an XML file of parsed\n"
    "                            input\n"
    " -M                         print included file tree and stop after doing that\n"
    " -MM                        print included file tree for files included with\n"
    "                            '#include \"file\"' and stop\n"
    " -MD                        print included file tree into .d file and compile\n"
    " -MMD                       print included file tree for files included with\n"
    "                            '#include \"file\"' into .d file and compile\n"
    " --with-cpp=<string>        use <string> as cpp\n"
    "    Dice (silently) checks if it can run <string> before using it.\n"

    "\nBack-End Options:\n"
    " -i"
#if defined(HAVE_GETOPT_LONG)
    ", --create-inline"
#endif
    " <mode>  generate client stubs as inline\n"
    "    set <mode> to \"static\" to generate static inline\n"
    "    set <mode> to \"extern\" to generate extern inline\n"
    "    <mode> is optional\n"
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
    "                 specify an output directory (default is .)\n"
    "\n"
    "Compiler Flags:\n"
    " -f<string>                   supply flags to compiler\n"
    "    if <string> starts with 'F'/'f' (filetype):\n"
    "      set <string> to \"Fidlfile\" for one client C file per IDL file (default)\n"
    "      set <string> to \"Fmodule\" for one client C file per module\n"
    "      set <string> to \"Finterface\" for one client C file per interface\n"
    "      set <string> to \"Ffunction\" for one client C file per function\n"
    "      set <string> tp \"Fall\" for one client C file for everything\n"
    "      alternatively you can set -fF1, -fF2, -fF3, -fF4, -fF5 respectively\n"
    "    set <string> to 'ctypes' to use C-style type names instead of CORBA type\n"
    "      names\n"
    "    set <string> to 'l4types' to use L4-style type names instead of CORBA type\n"
    "      names (this implies 'ctypes' for all non-l4 types)\n"
    "    set <string> to 'opcodesize=<size>' to set the size of the opcode\n"
    "      set <size> to \"byte\" or 1 if opcode should use only 1 byte\n"
    "      set <size> to \"short\" or 2 if opcode should use 2 bytes\n"
    "      set <size> to \"long\" or 4 if opcode should use 4 bytes (default)\n"
    "      set <size> to \"longlong\" or 8 if opcode should use 8 bytes\n"
    "    set <string> to 'server-parameter' to provide a CORBA_Environment\n"
    "      parameter to the server loop; otherwise you may use NULL when calling the\n"
    "      loop\n"
    "    set <string> to 'init-rcvstring[=<func-name>]' to make the server-loop use\n"
    "       a user-provided function to initialize the receive buffers of indirect\n"
    "       strings\n"
    "    set <string> to 'align-to-type' to align parameters in the message buffer\n"
    "       by the size of their type (or word) (default for ARM)\n"
    "\n"
    "    set <string> to 'force-corba-alloc' to force the usage of the CORBA_alloc\n"
    "       function instead of the CORBA_Environment's malloc member\n"
    "    set <string> to 'force-env-malloc' to force the usage of CORBA_Environment's\n"
    "       malloc member instead of CORBA_alloc\n"
    "    depending on their order they may override each other.\n"
    "    set <string> to 'free-mem-after-reply' to force freeing of memory pointers\n"
    "       stored in the environment variable using the free function of the\n"
    "       environment or CORBA_free\n"
    "\n"
    "    set <string> to 'force-c-bindings' to force the usage of L4's C-bindings\n"
    "       instead of inline assembler\n"
    "    set <string> to 'no-server-loop' to not generate the server loop function\n"
    "    set <string> to 'no-dispatcher' to not generate the dispatcher function\n"
    "\n"
    "  Debug Options:\n"
    "    set <string> to 'trace-server' to trace all messages received by the\n"
    "       server-loop\n"
    "    set <string> to 'trace-client' to trace all messages send to the server and\n"
    "       answers received from it\n"
    "    set <string> to 'trace-dump-msgbuf' to dump the message buffer, before a\n"
    "       call, right after it, and after each wait\n"
    "    set <string> to 'trace-dump-msgbuf-dwords=<number>' to restrict the number\n"
    "       of dumped dwords. <number> can be any positive integer including zero.\n"
    "    set <string> to 'trace-function=<function>' to use <function> instead of\n"
    "       'printf' to print trace messages. This option sets the function for\n"
    "       client and server.\n"
    "       If you like to specify different functions for client, server, or msgbuf,\n"
    "       you can use -ftrace-client=<function1>, -ftrace-server=<function2>, or\n"
    "       -ftrace-dump-msgbuf=<function3> respectively\n"
    "\n"
    "    set <string> to 'zero-msgbuf' to zero out the message buffer before each\n"
    "       and wait/reply-and-wait\n"
    "    if <string> is set to 'use-symbols' or 'use-defines' the symbols given with\n"
    "       '-D' or '-P-D' are not just handed down to the preprocessor, but also\n"
    "       used to simplify the generated code. USE FOR DEBUGGING ONLY!\n"
    "    if <string> is set to 'test-no-success-message' the tesuite will only print\n"
    "       error messages (only useful in association with -T)\n"
    "    if <string> is set to 'no-send-canceled-check' the client call will not\n"
    "       retry to send if it was canceled or aborted by another thread.\n"
    "    if <string> is set to 'const-as-define' all const declarations will be\n"
    "       printed as #define statements\n"
    "    if <string> is set to 'keep-temp-files' Dice will not delete the temporarely\n"
    "       during preprocessing created files. This options should be used to\n"
    "       debug dice only.\n"
    "\n"
    " -B"
#if defined(HAVE_GETOPT_LONG)
    ", --back-end"
#endif
    " <string>     defines the back-end to use\n"
    "    <string> starts with a letter specifying platform, kernel interface or\n"
    "    language mapping\n"
    "    p - specifies the platform (IA32, IA64, ARM, AMD64) (default: IA32)\n"
    "    i - specifies the kernel interface (v2, x0, x0adapt, v4, sock)(default:v2)\n"
    "    m - specifies the language mapping (C, CPP) (default: C)\n"
    "    example: -Bpia32 -Biv2 -BmC - which is default\n"
    "    (currently only the default values are supported)\n"
    "    For debugging purposes you can use the \"kernel\" interface 'sockets',\n"
    "    which uses sockets to communicate (-Bisock[ets]).\n"
    " -T"
#if defined(HAVE_GETOPT_LONG)
    ", --testsuite"
#endif
    " <string>    generates a testsuite for the stubs.\n"
    "                             (overwrites server skeleton file!)\n"
    "    set <string> to \"thread\" if testsuite servers should run as seperate\n"
    "                             threads\n"
    "    set <string> to \"task\"   if testsuite servers should run as seperate\n"
    "                             tasks\n"
    "    if <string> is empty \"thread\" is chosen\n"
    " -m"
#if defined(HAVE_GETOPT_LONG)
    ", --message-passing"
#else
    "                   "
#endif
    "       generate MP functions for RPC functions as well\n"
    "\nGeneral Compiler Options\n"
    " -Wall                       ignore all warnings\n"
    " -Wignore-duplicate-fids     duplicate function ID are no errors, but warnings\n"
    " -Wprealloc                  warn if memory has to be allocated using\n"
    "                             CORBA_alloc\n"
    " -Wno-maxsize                warn if a variable sized parameter has no maximum\n"
    "                             size to bound its required memory use\n"
    "\n\nexample: dice -v -i test.idl\n\n");
    exit(0);
}

/** displays a short help for this compiler */
void CCompiler::ShowShortHelp()
{
    printf("Usage: dice [<options>] <idl-file>\n"
    //23456789+123456789+123456789+123456789+123456789+123456789+123456789+123456789+
    "\nPre-Processor/Front-End Options:\n"
    " -h"
#if defined(HAVE_GETOPT_LONG)
        ", --help"
#else
        "        "
#endif
                 "                 shows verbose help\n"
    " -v"
#if defined(HAVE_GETOPT_LONG)
        ", --verbose"
#endif
                    " <level>      print verbose output\n"
    " -C"
#if defined(HAVE_GETOPT_LONG)
    ", --corba"
#else
    "         "
#endif
    "                use the CORBA front-end (scan CORBA IDL)\n"
    " -P"
#if defined(HAVE_GETOPT_LONG)
    ", --preprocess"
#endif
    " <string>  hand <string> as argument to preprocessor\n"
    " -I<string>                 same as -P-I<string>\n"
    " -D<string>                 same as -P-D<string>\n"
    " -nostdinc                  same as -P-nostdinc\n"
    " -E                         stop after pre-processing and output pre-processed\n"
    "                            IDL file\n"
    " -M                         print included file tree and stop after doing that\n"
    " -MM                        print included file tree for files included with\n"
    "                            '#include \"file\"' and stop\n"
    " -MD                        print included file tree into .d file and compile\n"
    " -MMD                       print included file tree for files included with\n"
    "                            '#include \"file\"' into .d file and compile\n"

    "\nBack-End Options:\n"
    " -i"
#if defined(HAVE_GETOPT_LONG)
    ", --create-inline"
#endif
    " <mode>  generate client stubs as inline\n"
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
    "                 specify an output directory (default is .)\n"
    " -f<string>                   supply flags to compiler\n"
    " -B"
#if defined(HAVE_GETOPT_LONG)
    ", --back-end"
#endif
    " <string>     defines the back-end to use\n"
    " -T"
#if defined(HAVE_GETOPT_LONG)
    ", --testsuite"
#endif
    " <string>    generates a testsuite for the stubs.\n"
    " -m"
#if defined(HAVE_GETOPT_LONG)
    ", --message-passing"
#else
    "                   "
#endif
    "       generate MP functions for RPC functions as well\n"
    " -W<string>                  some warning options\n\n");
    exit(0);
}

/** display the compiler's version (requires a defines for the version number) */
void CCompiler::ShowVersion()
{
    ShowCopyright();
    printf("DICE %s - built on %s", dice_version, dice_build);
    if (dice_user)
        printf(" by %s", dice_user);
    printf(".\n");
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
        Verbose("Added %s to include paths\n", m_sInFileName.substr(0, nPos).c_str());
    }
    // set namespace to uninitialized
    CParser *pParser = CParser::CreateParser(m_nUseFrontEnd);
    CParser::SetCurrentParser(pParser);
    if (!pParser->Parse(0 /* no exitsing scan buffer*/, m_sInFileName,
        m_nUseFrontEnd, ((m_nVerboseLevel & PROGRAM_VERBOSE_PARSER) > 0),
        IsOptionSet(PROGRAM_STOP_AFTER_PRE)))
    {
        if (!erroccured)
            Error("other parser error.");
    }
    // get file
    m_pRootFE = pParser->GetTopFileInScope();
    // now that parsing is over, get rid of it
    delete pParser;
    // if errors, print them and abort
    if (erroccured)
    {
        if (errcount > 0)
            Error("%d Error(s) and %d Warning(s) occured.", errcount, warningcount);
        else
            Warning("%s: warning: %d Warning(s) occured while parsing.", m_sInFileName.c_str(), warningcount);
    }
    // if we should stop after preprocessing we write the intermediate output
    if (IsOptionSet(PROGRAM_STOP_AFTER_PRE_XML))
    {
        Verbose("Start generating XML output ...\n");
        string sXMLFilename;
        if (!m_sOutputDir.empty())
            sXMLFilename = m_sOutputDir;
        sXMLFilename += m_pRootFE->GetFileNameWithoutExtension() + ".xml";
        CFile *pXML = new CFile();
        pXML->Open(sXMLFilename, CFile::Write);
        pXML->Print("<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n");
        m_pRootFE->Serialize(pXML);
        pXML->Close();
        Verbose("... finished generating XML output\n");
    }
}

/**
 *    \brief prepares the write operation
 *
 * This method performs any tasks necessary before the write operation. E.g. it
 * creates the class and name factory depending on the compiler arguments, it creates
 * the context and finally creates the backend.
 *
 * First thing this function does is to force an consistency check for the whole
 * hierarchy of the IDL file. If this check fails the compile run is aborted.
 *
 * Next the transmittable data is optimized.
 *
 * Finally the preparations for the write operation are made.
 */
void CCompiler::PrepareWrite()
{
    // if we should stop after preprocessing skip this function
    if (IsOptionSet(PROGRAM_STOP_AFTER_PRE) ||
        IsOptionSet(PROGRAM_STOP_AFTER_PRE_XML))
        return;

    Verbose("Check consistency of parsed input ...\n");
    // consistency check
    if (!m_pRootFE)
        Error("Internal Error: Current file not set");
    if (!m_pRootFE->CheckConsistency())
    {
        exit(1);
    }
    Verbose("... finished consistency check.\n");

    // create context
    CBEClassFactory *pCF;
    CBENameFactory *pNF;
    Verbose("Create Context...\n");

    // set class factory depending on arguments
    if (m_nBackEnd & PROGRAM_BE_V2)
        pCF = new CL4V2BEClassFactory((m_nVerboseLevel & PROGRAM_VERBOSE_CLASSFACTORY) > 0);
    else if (m_nBackEnd & PROGRAM_BE_X0)
    {
        if (m_nBackEnd & PROGRAM_BE_ARM)
            pCF = new CX0ArmClassFactory((m_nVerboseLevel & PROGRAM_VERBOSE_CLASSFACTORY) > 0);
        else if (m_nBackEnd & PROGRAM_BE_IA32)
            pCF = new CX0IA32ClassFactory((m_nVerboseLevel & PROGRAM_VERBOSE_CLASSFACTORY) > 0);
        else
            pCF = new CL4X0BEClassFactory((m_nVerboseLevel & PROGRAM_VERBOSE_CLASSFACTORY) > 0);
    }
    else if (m_nBackEnd & PROGRAM_BE_X0ADAPT)
        pCF = new CL4X0aBEClassFactory((m_nVerboseLevel & PROGRAM_VERBOSE_CLASSFACTORY) > 0);
    else if (m_nBackEnd & PROGRAM_BE_V4)
    {
        if (m_nBackEnd & PROGRAM_BE_IA32)
            pCF = new CL4V4IA32ClassFactory((m_nVerboseLevel & PROGRAM_VERBOSE_CLASSFACTORY) > 0);
        else
            pCF = new CL4V4BEClassFactory((m_nVerboseLevel & PROGRAM_VERBOSE_CLASSFACTORY) > 0);
    }
    else if (m_nBackEnd & PROGRAM_BE_SOCKETS)
        pCF = new CSockBEClassFactory((m_nVerboseLevel & PROGRAM_VERBOSE_CLASSFACTORY) > 0);
    else if (m_nBackEnd & PROGRAM_BE_CDR)
        pCF = new CCDRClassFactory((m_nVerboseLevel & PROGRAM_VERBOSE_CLASSFACTORY) > 0);
    else
        pCF = new CBEClassFactory((m_nVerboseLevel & PROGRAM_VERBOSE_CLASSFACTORY) > 0);

    // set name factory depending on arguments
    if ((m_nBackEnd & PROGRAM_BE_V2) || (m_nBackEnd & PROGRAM_BE_X0) || (m_nBackEnd & PROGRAM_BE_X0ADAPT))
        pNF = new CL4BENameFactory((m_nVerboseLevel & PROGRAM_VERBOSE_NAMEFACTORY) > 0);
    else if (m_nBackEnd & PROGRAM_BE_V4)
        pNF = new CL4V4BENameFactory((m_nVerboseLevel & PROGRAM_VERBOSE_NAMEFACTORY) > 0);
    else
        pNF = new CBENameFactory((m_nVerboseLevel & PROGRAM_VERBOSE_NAMEFACTORY) > 0);

    // create context
    m_pContext = new CBEContext(pCF, pNF);
    Verbose("...done.\n");

    Verbose("Set context parameters...\n");
    for (int i=0; i<PROGRAM_OPTION_GROUPS; i++)
        m_pContext->ModifyOptions((i << PROGRAM_OPTION_OPTION_BITS) | m_nOptions[i]);
    m_pContext->ModifyBackEnd(m_nBackEnd);
    m_pContext->SetFilePrefix(m_sFilePrefix);
    m_pContext->SetIncludePrefix(m_sIncludePrefix);
    m_pContext->ModifyWarningLevel(m_nWarningLevel);
    m_pContext->SetOpcodeSize(m_nOpcodeSize);
    m_pContext->SetInitRcvStringFunc(m_sInitRcvStringFunc);
    m_pContext->SetTraceClientFunc(m_sTraceClientFunc);
    m_pContext->SetTraceServerFunc(m_sTraceServerFunc);
    m_pContext->SetTraceMsgBufFunc(m_sTraceMsgBuf);
    m_pContext->SetOutputDir(m_sOutputDir);
    if (m_pContext->IsOptionSet(PROGRAM_USE_SYMBOLS))
    {
        for (int i=0; i<m_nSymbolCount-1; i++)
            m_pContext->AddSymbol(m_sSymbols[i]);
    }
    m_pContext->SetTraceMsgBufDwords(m_nDumpMsgBufDwords);
    Verbose("...done.\n");

    /** Prepare write for the back-end does some intialization which cannot be performed
     * during the write run.
     */
    Verbose("Create backend...\n");
    m_pRootBE = pCF->GetNewRoot();
    if (!m_pRootBE->CreateBE(m_pRootFE, m_pContext))
    {
        Verbose("Back-End creation failed\n");
        delete m_pRootBE;
        m_pRootBE = 0;
        Error("Creation of the back-end failed. compilation aborted.\n");
    }
    Verbose("...done.\n");

    // print dependency tree
    Verbose("Print dependencies...\n");
    if (IsOptionSet(PROGRAM_DEPEND_MASK))
        PrintDependencies();
    Verbose("... dependencies done.\n");

    // if we should stop after printing the dependencies, stop here
    if (IsOptionSet(PROGRAM_DEPEND_M) || IsOptionSet(PROGRAM_DEPEND_MM))
    {
         delete m_pContext;
         delete m_pRootBE;
         delete pNF;
         delete pCF;
         return;
    }
}

/**
 *    \brief prints the target files
 *
 * This method calls the write operation of the backend
 */
void CCompiler::Write()
{
    // if we should stop after preprocessing skip this function
    if (IsOptionSet(PROGRAM_STOP_AFTER_PRE) ||
        IsOptionSet(PROGRAM_STOP_AFTER_PRE_XML) ||
        IsOptionSet(PROGRAM_DEPEND_M) ||
        IsOptionSet(PROGRAM_DEPEND_MM))
        return;

    // write backend
    Verbose("Write backend...\n");
    m_pRootBE->Write(m_pContext);
    Verbose("...done.\n");
}

/**
 *    \brief helper function
 *  \param sMsg the format string of the message
 *
 * This method prints a message if the ciompiler runs in verbose mode.
 */
void CCompiler::Verbose(const char *sMsg, ...)
{
    // if verbose turned off: return
    if (!m_bVerbose)
        return;
    va_list args;
    va_start(args, sMsg);
    vprintf(sMsg, args);
    va_end(args);
}

/**
 *    \brief helper function
 *    \param sMsg the error message to print before exiting
 *
 * This method prints an error message and exits the compiler. Any clean up should be
 * performed  BEFORE this method is called.
 */
void CCompiler::Error(const char *sMsg, ...)
{
    fprintf(stderr, "dice: ");
    va_list args;
    va_start(args, sMsg);
    vfprintf(stderr, sMsg, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(1);
}

/**    \brief helper function
 *    \param pFEObject the front-end object where the error occurred
 *    \param nLinenb the linenumber of the error
 *    \param sMsg the error message
 *
 * The given front-end object can be used to determine the file this object belonged to and the line, where this
 * object has been declared. This is useful if we do not have a current line number available (any time after the
 * parsing finished).
 */
void CCompiler::GccError(CFEBase * pFEObject, int nLinenb, const char *sMsg, ...)
{
    va_list args;
    va_start(args, sMsg);
    GccErrorVL(pFEObject, nLinenb, sMsg, args);
    va_end(args);
}

/**    \brief helper function
 *    \param pFEObject the front-end object where the error occurred
 *    \param nLinenb the linenumber of the error
 *    \param sMsg the error message
 *  \param vl the argument list to be printed
 *
 * The given front-end object can be used to determine the file this object belonged to and the line, where this
 * object has been declared. This is useful if we do not have a current line number available (any time after the
 * parsing finished).
 */
void CCompiler::GccErrorVL(CFEBase * pFEObject, int nLinenb, const char *sMsg, va_list vl)
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
                fprintf(stderr, "In file included ");
            vector<CFEFile*>::iterator iter;
            for (iter = pStack->begin();
                (iter != pStack->end()) && (iter != pStack->end()-1); iter++)
            {
                // we do not use GetFullFileName, because the "normal" filename already includes the whole path
                // it is the filename generated by Gcc
                CFEFile *pFEFile = *iter;
                int nIncludeLine = 1;
                if (iter != pStack->end()-1)
                    nIncludeLine = (*(iter+1))->GetIncludedOnLine();
                fprintf(stderr, "from %s:%d", pFEFile->GetFileName().c_str(), nIncludeLine);
                if (iter + 2 != pStack->end())
                    fprintf(stderr, ",\n                 ");
                else
                    fprintf(stderr, ":\n");
            }
            if (*iter)
            {
                // we do not use GetFullFileName, because the "normal" filename already includes the whole path
                // it is the filename generated by Gcc
                fprintf(stderr, "%s:%d: ", (*iter)->GetFileName().c_str(), nLinenb);
            }
        }
        // cleanup
        delete pStack;
    }
    vfprintf(stderr, sMsg, vl);
    fprintf(stderr, "\n");
}

/**
 *    \brief helper function
 *    \param sMsg the warning message to print
 *
 * This method prints an error message and returns.
 */
void CCompiler::Warning(const char *sMsg, ...)
{
    va_list args;
    va_start(args, sMsg);
    vfprintf(stderr, sMsg, args);
    va_end(args);
    fprintf(stderr, "\n");
}

/**    \brief print warning messages
 *    \param pFEObject the object where the warning occured
 *    \param nLinenb the line in the file where the object originated
 *    \param sMsg the warning message
 */
void CCompiler::GccWarning(CFEBase * pFEObject, int nLinenb, const char *sMsg, ...)
{
    va_list args;
    va_start(args, sMsg);
    GccWarningVL(pFEObject, nLinenb, sMsg, args);
    va_end(args);
}

/**    \brief print warning messages
 *    \param pFEObject the object where the warning occured
 *    \param nLinenb the line in the file where the object originated
 *    \param sMsg the warning message
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
        CFEFile *pCur = (CFEFile*)pFEObject->GetSpecificParent<CFEFile>();
        vector<CFEFile*> *pStack = new vector<CFEFile*>();
        while (pCur)
        {
            pStack->insert(pStack->begin(), pCur);
            // start with parent of current, because GetFile starts with "this"
            pCur = pCur->GetSpecificParent<CFEFile>(1);
        }
        // need at least one file
        if (!pStack->empty())
        {
            // walk down
            if (pStack->size() > 1)
                fprintf(stderr, "In file included ");
            vector<CFEFile*>::iterator iter;
            for (iter = pStack->begin();
                (iter != pStack->end()) && (iter != pStack->end()-1); iter++)
            {
                fprintf(stderr, "from %s:%d", (*iter)->GetFullFileName().c_str(), 1);
                if (iter+2 != pStack->end())
                    fprintf(stderr, ",\n                 ");
                else
                    fprintf(stderr, ":\n");
            }
            if (*iter)
            {
                fprintf(stderr, "%s:%d: warning: ", (*iter)->GetFullFileName().c_str(), nLinenb);
            }
        }
        // cleanup
        delete pStack;
    }
    vfprintf(stderr, sMsg, vl);
    fprintf(stderr, "\n");
}

/**    \brief print the dependency tree
 *
 * The dependency tree contains the files the top IDL file depends on. These are the included files.
 */
void CCompiler::PrintDependencies()
{
    if (!m_pRootBE)
        return;

    // if file, open file
    FILE *output = stdout;
    if (IsOptionSet(PROGRAM_DEPEND_MD) || IsOptionSet(PROGRAM_DEPEND_MMD))
    {
        string sOutName;
        if (!m_sOutputDir.empty())
            sOutName = m_sOutputDir;
        sOutName += m_pRootFE->GetFileNameWithoutExtension() + ".d";
        output = fopen(sOutName.c_str(), "w");
        if (!output)
        {
            Warning("Could not open %s, use <stdout>\n", sOutName.c_str());
            output = stdout;
        }
    }

    m_nCurCol = 0;
    m_pRootBE->PrintTargetFiles(output, m_nCurCol, MAX_SHELL_COLS);
    fprintf(output, ": ");
    m_nCurCol += 2;
    PrintDependentFile(output, m_pRootFE->GetFullFileName());
    PrintDependencyTree(output, m_pRootFE);
    fprintf(output, "\n");

    // if file, close file
    if (IsOptionSet(PROGRAM_DEPEND_MD) || IsOptionSet(PROGRAM_DEPEND_MMD))
        fclose(output);
}

/**    \brief prints the included files
 *    \param output the output stream
 *    \param pFEFile the current front-end file
 *
 * This implementation print the file names of the included files. It first prints the names and then
 * iterates over the included files.
 *
 * If the options PROGRAM_DEPEND_MM or PROGRAM_DEPEND_MMD are specified only files included with
 * '#include "file"' are printed.
 */
void CCompiler::PrintDependencyTree(FILE * output, CFEFile * pFEFile)
{
    if (!pFEFile)
        return;
    // print names
    vector<CFEFile*>::iterator iterF = pFEFile->GetFirstChildFile();
    CFEFile *pIncFile;
    while ((pIncFile = pFEFile->GetNextChildFile(iterF)) != 0)
    {
        if (pIncFile->IsStdIncludeFile() &&
            (IsOptionSet(PROGRAM_DEPEND_MM) || IsOptionSet(PROGRAM_DEPEND_MMD)))
            continue;
        PrintDependentFile(output, pIncFile->GetFullFileName());
    }
    // ierate over included files
    iterF = pFEFile->GetFirstChildFile();
    while ((pIncFile = pFEFile->GetNextChildFile(iterF)) != 0)
    {
        if (pIncFile->IsStdIncludeFile() &&
            (IsOptionSet(PROGRAM_DEPEND_MM) || IsOptionSet(PROGRAM_DEPEND_MMD)))
            continue;
        PrintDependencyTree(output, pIncFile);
    }
}

/** \brief prints a list of generated files for the given options
 *  \param output the output stream
 *  \param pFEFile the file to scan for generated files
 */
void CCompiler::PrintGeneratedFiles(FILE * output, CFEFile * pFEFile)
{
    if (!pFEFile->IsIDLFile())
        return;

    if (IsOptionSet(PROGRAM_FILE_IDLFILE) || IsOptionSet(PROGRAM_FILE_ALL))
    {
        PrintGeneratedFiles4File(output, pFEFile);
    }
    else if (IsOptionSet(PROGRAM_FILE_MODULE))
    {
        PrintGeneratedFiles4Library(output, pFEFile);
    }
    else if (IsOptionSet(PROGRAM_FILE_INTERFACE))
    {
        PrintGeneratedFiles4Interface(output, pFEFile);
    }
    else if (IsOptionSet(PROGRAM_FILE_FUNCTION))
    {
        PrintGeneratedFiles4Operation(output, pFEFile);
    }

    CBENameFactory *pNF = m_pContext->GetNameFactory();
    string sName;
    // create client header file
    if (IsOptionSet(PROGRAM_GENERATE_CLIENT))
    {
        m_pContext->SetFileType(FILETYPE_CLIENTHEADER);
        sName = pNF->GetFileName(pFEFile, m_pContext);
        PrintDependentFile(output, sName);
    }
    // create server header file
    if (IsOptionSet(PROGRAM_GENERATE_COMPONENT))
    {
        // component file
        m_pContext->SetFileType(FILETYPE_COMPONENTHEADER);
        sName = pNF->GetFileName(pFEFile, m_pContext);
        PrintDependentFile(output, sName);
    }
    // opcodes
    if (!IsOptionSet(PROGRAM_NO_OPCODES))
    {
        m_pContext->SetFileType(FILETYPE_OPCODE);
        sName = pNF->GetFileName(pFEFile, m_pContext);
        PrintDependentFile(output, sName);
    }

    if (IsOptionSet(PROGRAM_FILE_ALL))
    {
        vector<CFEFile*>::iterator iter = pFEFile->GetFirstChildFile();
        CFEFile *pFile;
        while ((pFile = pFEFile->GetNextChildFile(iter)) != 0)
            PrintGeneratedFiles(output, pFile);
    }
}

/**    \brief prints the file-name generated for the front-end file
 *    \param output the target stream
 *    \param pFEFile the current front-end file
 */
void CCompiler::PrintGeneratedFiles4File(FILE * output, CFEFile * pFEFile)
{
    CBENameFactory *pNF = m_pContext->GetNameFactory();
    string sName;

    // client
    if (IsOptionSet(PROGRAM_GENERATE_CLIENT))
    {
        // client implementation file
        m_pContext->SetFileType(FILETYPE_CLIENTIMPLEMENTATION);
        sName = pNF->GetFileName(pFEFile, m_pContext);
        PrintDependentFile(output, sName);
    }
    // server
    if (IsOptionSet(PROGRAM_GENERATE_COMPONENT))
    {
        // component file
        m_pContext->SetFileType(FILETYPE_COMPONENTIMPLEMENTATION);
        sName = pNF->GetFileName(pFEFile, m_pContext);
        PrintDependentFile(output, sName);
    }
    // testsuite
    if (IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
    {
        m_pContext->SetFileType(FILETYPE_TESTSUITE);
        sName = pNF->GetFileName(pFEFile, m_pContext);
        PrintDependentFile(output, sName);
    }
}

/** \brief prints a list of generated files for the library granularity
 *  \param output the output stream
 *  \param pFEFile the front-end file to scan for generated files
 */
void CCompiler::PrintGeneratedFiles4Library(FILE * output, CFEFile * pFEFile)
{
    // iterate over libraries
    vector<CFELibrary*>::iterator iterL = pFEFile->GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = pFEFile->GetNextLibrary(iterL)) != 0)
    {
        PrintGeneratedFiles4Library(output, pLibrary);
    }
    // iterate over interfaces
    vector<CFEInterface*>::iterator iterI = pFEFile->GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = pFEFile->GetNextInterface(iterI)) != 0)
    {
        PrintGeneratedFiles4Library(output, pInterface);
    }

    // if testsuite
    if (m_pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
    {
        m_pContext->SetFileType(FILETYPE_TESTSUITE);
        string sTestName = m_pContext->GetNameFactory()->GetFileName(pFEFile, m_pContext);
        PrintDependentFile(output, sTestName);
    }
}

/** \brief print the list of generated files for the library granularity
 *  \param output the output stream to print to
 *  \param pFELibrary the library to scan
 */
void CCompiler::PrintGeneratedFiles4Library(FILE * output, CFELibrary * pFELibrary)
{
    CBENameFactory *pNF = m_pContext->GetNameFactory();
    string sName;
    // client file
    if (IsOptionSet(PROGRAM_GENERATE_CLIENT))
    {
        // implementation
        m_pContext->SetFileType(FILETYPE_CLIENTIMPLEMENTATION);
        sName = pNF->GetFileName(pFELibrary, m_pContext);
        PrintDependentFile(output, sName);
    }
    // component file
    if (IsOptionSet(PROGRAM_GENERATE_COMPONENT))
    {
        // implementation
        m_pContext->SetFileType(FILETYPE_COMPONENTIMPLEMENTATION);
        sName = pNF->GetFileName(pFELibrary, m_pContext);
        PrintDependentFile(output, sName);
    }
    // nested libraries
    vector<CFELibrary*>::iterator iterL = pFELibrary->GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = pFELibrary->GetNextLibrary(iterL)) != 0)
    {
        PrintGeneratedFiles4Library(output, pLibrary);
    }
}

/** \brief print the list of generated files for the library granularity
 *  \param output the output stream to print to
 *  \param pFEInterface the interface to scan
 */
void CCompiler::PrintGeneratedFiles4Library(FILE * output, CFEInterface * pFEInterface)
{
    CBENameFactory *pNF = m_pContext->GetNameFactory();
    string sName;
    // client file
    if (IsOptionSet(PROGRAM_GENERATE_CLIENT))
    {
        // implementation
        m_pContext->SetFileType(FILETYPE_CLIENTIMPLEMENTATION);
        sName = pNF->GetFileName(pFEInterface, m_pContext);
        PrintDependentFile(output, sName);
    }
    // component file
    if (IsOptionSet(PROGRAM_GENERATE_COMPONENT))
    {
        // implementation
        m_pContext->SetFileType(FILETYPE_COMPONENTIMPLEMENTATION);
        sName = pNF->GetFileName(pFEInterface, m_pContext);
        PrintDependentFile(output, sName);
    }
}

/** \brief print the list of generated files for interface granularity
 *  \param output the output stream to print to
 *  \param pFEFile the front-end file to be scanned
 */
void CCompiler::PrintGeneratedFiles4Interface(FILE * output, CFEFile * pFEFile)
{
     // iterate over interfaces
     vector<CFEInterface*>::iterator iterI = pFEFile->GetFirstInterface();
     CFEInterface *pInterface;
     while ((pInterface = pFEFile->GetNextInterface(iterI)) != 0)
     {
         PrintGeneratedFiles4Interface(output, pInterface);
     }
     // iterate over libraries
     vector<CFELibrary*>::iterator iterL = pFEFile->GetFirstLibrary();
     CFELibrary *pLibrary;
     while ((pLibrary = pFEFile->GetNextLibrary(iterL)) != 0)
     {
         PrintGeneratedFiles4Interface(output, pLibrary);
     }

     // if testsuite
     if (m_pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
     {
         m_pContext->SetFileType(FILETYPE_TESTSUITE);
         string sTestName = m_pContext->GetNameFactory()->GetFileName(pFEFile, m_pContext);
         PrintDependentFile(output, sTestName);
     }
}

/** \brief print the list of generated files for interface granularity
 *  \param output the output stream to print to
 *  \param pFELibrary the front-end library to be scanned
 */
void CCompiler::PrintGeneratedFiles4Interface(FILE * output, CFELibrary * pFELibrary)
{
    // iterate over interfaces
    vector<CFEInterface*>::iterator iterI = pFELibrary->GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = pFELibrary->GetNextInterface(iterI)) != 0)
    {
        PrintGeneratedFiles4Interface(output, pInterface);
    }
    // iterate over nested libraries
    vector<CFELibrary*>::iterator iterL = pFELibrary->GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = pFELibrary->GetNextLibrary(iterL)) != 0)
    {
        PrintGeneratedFiles4Interface(output, pLibrary);
    }
}

/** \brief print the list of generated files for interface granularity
 *  \param output the output stream to print to
 *  \param pFEInterface the front-end interface to be scanned
 */
void CCompiler::PrintGeneratedFiles4Interface(FILE * output, CFEInterface * pFEInterface)
{
    CBENameFactory *pNF = m_pContext->GetNameFactory();
    string sName;
    // client file
    if (IsOptionSet(PROGRAM_GENERATE_CLIENT))
    {
        // implementation
        m_pContext->SetFileType(FILETYPE_CLIENTIMPLEMENTATION);
        sName = pNF->GetFileName(pFEInterface, m_pContext);
        PrintDependentFile(output, sName);
    }
    // component file
    if (IsOptionSet(PROGRAM_GENERATE_COMPONENT))
    {
        // implementation
        m_pContext->SetFileType(FILETYPE_COMPONENTIMPLEMENTATION);
        sName = pNF->GetFileName(pFEInterface, m_pContext);
        PrintDependentFile(output, sName);
    }
}

/** \brief print the list of generated files for operation granularity
 *  \param output the output stream to print to
 *  \param pFEFile the front-end file to be scanned
 */
void CCompiler::PrintGeneratedFiles4Operation(FILE * output, CFEFile * pFEFile)
{
    // iterate over interfaces
    vector<CFEInterface*>::iterator iterI = pFEFile->GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = pFEFile->GetNextInterface(iterI)) != 0)
    {
        PrintGeneratedFiles4Operation(output, pInterface);
    }
    // iterate over libraries
    vector<CFELibrary*>::iterator iterL = pFEFile->GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = pFEFile->GetNextLibrary(iterL)) != 0)
    {
        PrintGeneratedFiles4Operation(output, pLibrary);
    }

    // if testsuite
    if (m_pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
    {
        m_pContext->SetFileType(FILETYPE_TESTSUITE);
        string sTestName = m_pContext->GetNameFactory()->GetFileName(pFEFile, m_pContext);
        PrintDependentFile(output, sTestName);
    }
}

/** \brief print the list of generated files for operation granularity
 *  \param output the output stream to print to
 *  \param pFELibrary the front-end library to be scanned
 */
void CCompiler::PrintGeneratedFiles4Operation(FILE * output, CFELibrary * pFELibrary)
{
    // iterate over interfaces
    vector<CFEInterface*>::iterator iterI = pFELibrary->GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = pFELibrary->GetNextInterface(iterI)) != 0)
    {
        PrintGeneratedFiles4Operation(output, pInterface);
    }
    // iterate over nested libraries
    vector<CFELibrary*>::iterator iterL = pFELibrary->GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = pFELibrary->GetNextLibrary(iterL)) != 0)
    {
        PrintGeneratedFiles4Operation(output, pLibrary);
    }
}

/** \brief print the list of generated files for operation granularity
 *  \param output the output stream to print to
 *  \param pFEInterface the front-end interface to be scanned
 */
void CCompiler::PrintGeneratedFiles4Operation(FILE * output, CFEInterface * pFEInterface)
{
    // iterate over operations
    vector<CFEOperation*>::iterator iter = pFEInterface->GetFirstOperation();
    CFEOperation *pOperation;
    while ((pOperation = pFEInterface->GetNextOperation(iter)) != 0)
    {
        PrintGeneratedFiles4Operation(output, pOperation);
    }
}

/** \brief print the list of generated files for operation granularity
 *  \param output the output stream to print to
 *  \param pFEOperation the front-end operation to be scanned
 */
void CCompiler::PrintGeneratedFiles4Operation(FILE * output, CFEOperation * pFEOperation)
{
     CBENameFactory *pNF = m_pContext->GetNameFactory();
     string sName;
     // client file
     if (IsOptionSet(PROGRAM_GENERATE_CLIENT))
     {
         // implementation
         m_pContext->SetFileType(FILETYPE_CLIENTIMPLEMENTATION);
         sName = pNF->GetFileName(pFEOperation, m_pContext);
         PrintDependentFile(output, sName);
     }
     // component file
     if (IsOptionSet(PROGRAM_GENERATE_COMPONENT))
     {
         // implementation
         m_pContext->SetFileType(FILETYPE_COMPONENTIMPLEMENTATION);
         sName = pNF->GetFileName(pFEOperation, m_pContext);
         PrintDependentFile(output, sName);
     }
}

/**    \brief prints a filename to the dependency tree
 *    \param output the stream to write to
 *    \param sFileName the name to print
 *
 * This implementation adds a spacer after each file name and also checks before writing
 * if the maximum column number would be exceeded by this file. If it would a new line is started
 * The length of the filename is added to the columns.
 */
void CCompiler::PrintDependentFile(FILE * output, string sFileName)
{
     int nLength = sFileName.length();
     if ((m_nCurCol + nLength + 1) >= MAX_SHELL_COLS)
     {
         fprintf(output, "\\\n ");
         m_nCurCol = 1;
     }
     fprintf(output, "%s ", sFileName.c_str());
     m_nCurCol += nLength + 1;
}

/** \brief adds another symbol to the internal list
 *  \param sNewSymbol the symbol to add
 */
void CCompiler::AddSymbol(const char *sNewSymbol)
{
    if (!sNewSymbol)
        return;
    if (!m_sSymbols)
        m_nSymbolCount = 2;
    else
        m_nSymbolCount++;
    m_sSymbols = (char **) realloc(m_sSymbols, m_nSymbolCount * sizeof(char *));
    assert(m_sSymbols);
    m_sSymbols[m_nSymbolCount - 2] = strdup(sNewSymbol);
    m_sSymbols[m_nSymbolCount - 1] = 0;
}

/** \brief adds another symbol to the internal list
 *  \param sNewSymbol the symbol to add
 */
void CCompiler::AddSymbol(string sNewSymbol)
{
    if (sNewSymbol.empty())
        return;
    AddSymbol(sNewSymbol.c_str());
}

/**    \brief set the option
 *    \param nRawOption the option in raw format
 */
void CCompiler::SetOption(unsigned int nRawOption)
{
    m_nOptions[PROGRAM_OPTION_GROUP_INDEX(nRawOption)] |=
        PROGRAM_OPTION_OPTION(nRawOption);
}

/**    \brief set the option
 *    \param nOption the option in ProgramOptionType format
 */
void CCompiler::SetOption(ProgramOptionType nOption)
{
    m_nOptions[nOption._s.group] |= nOption._s.option;
}

/**    \brief unset an option
 *    \param nRawOption the option in raw format
 */
void CCompiler::UnsetOption(unsigned int nRawOption)
{
    m_nOptions[PROGRAM_OPTION_GROUP_INDEX(nRawOption)] &=
        ~PROGRAM_OPTION_OPTION(nRawOption);
}

/**    \brief tests if an option is set
 *    \param nRawOption the option in raw format
 */
bool CCompiler::IsOptionSet(unsigned int nRawOption)
{
    return m_nOptions[PROGRAM_OPTION_GROUP_INDEX(nRawOption)] &
        PROGRAM_OPTION_OPTION(nRawOption);
}
