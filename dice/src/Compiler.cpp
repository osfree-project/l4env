/**
 *	\file	dice/src/Compiler.cpp
 *	\brief	contains the implementation of the class CCompiler
 *
 *	\date	03/06/2001
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

/**	debugging helper variable */
int nGlobalDebug = 0;

//@{
/** global variables for argument parsing */
extern char *optarg;
extern int optind, opterr, optopt;
//@}

CCompiler::CCompiler()
{
    m_nOptions = 0;
    m_bVerbose = false;
    m_nUseFrontEnd = USE_FE_NONE;
    m_nOptimizeLevel = PROGRAM_OPTIMIZE_MAXLEVEL;
    m_nOpcodeSize = 0;
    m_pContext = 0;
    m_pRootBE = 0;
    m_nWarningLevel = 0;
	m_sSymbols = 0;
	m_nSymbolCount = 0;
	m_nDumpMsgBufDwords = 0;
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
 *	\brief parses the arguments of the compiler call
 *	\param argc the number of arguments
 *	\param argv the arguments
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
        c = getopt(argc, argv, "cstn::i::v::hF:p:CP:f:B:E::O::VI:NT::M::W:D:x:");
#endif

        if (c == -1)
        {
            bHaveFileName = true;
            break;		// skip - evaluate later
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
            m_nOptions |= PROGRAM_GENERATE_CLIENT;
            break;
        case 's':
            Verbose("Create server/component-side code.\n");
            m_nOptions |= PROGRAM_GENERATE_COMPONENT;
            break;
        case 't':
            Verbose("create skeletons enabled\n");
            m_nOptions |= PROGRAM_GENERATE_TEMPLATE;
            break;
        case 'i':
            {
                // there may follow an optional argument stating whether this is "static" or "extern" inline
                m_nOptions |= PROGRAM_GENERATE_INLINE;
                if (!optarg)
                {
                    Verbose("create client stub as inline\n");
                }
                else
                {
                    // make upper case
                    String sArg(optarg);
                    sArg.MakeUpper();
					sArg.TrimLeft('=');
                    if (sArg == "EXTERN")
                    {
                        m_nOptions |= PROGRAM_GENERATE_INLINE_EXTERN;
                        m_nOptions &= ~PROGRAM_GENERATE_INLINE_STATIC;
                        Verbose("create client stub as extern inline\n");
                    }
                    else if (sArg == "STATIC")
                    {
                        m_nOptions |= PROGRAM_GENERATE_INLINE_STATIC;
                        m_nOptions &= ~PROGRAM_GENERATE_INLINE_EXTERN;
                        Verbose("create client stub as static inline\n");
                    }
                    else
                    {
                        Warning("dice: Inline argument \"%s\" not supported. (assume none)", optarg);
                        m_nOptions &= ~PROGRAM_GENERATE_INLINE_EXTERN;
                        m_nOptions &= ~PROGRAM_GENERATE_INLINE_STATIC;
                    }
                }
            }
            break;
        case 'n':
#if !defined(HAVE_GETOPT_LONG)
            {
                // check if -nostdinc is meant
                String sArg(optarg);
                if (sArg.IsEmpty())
                {
                    Verbose("create no opcodes\n");
                    m_nOptions |= PROGRAM_NO_OPCODES;
                }
                else if (sArg == "ostdinc")
                {
                    Verbose("no standard include paths\n");
                    pPreProcess->AddCPPArgument(String("-nostdinc"));
                }
            }
#else
            Verbose("create no opcodes\n");
            m_nOptions |= PROGRAM_NO_OPCODES;
#endif
            break;
        case 'v':
            {
                m_nOptions |= PROGRAM_VERBOSE;
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
                        m_nOptions &= ~PROGRAM_VERBOSE;
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
            m_sIncludePrefix.TrimRight('/');
            break;
        case 'C':
            Verbose("use the CORBA frontend\n");
            m_nUseFrontEnd = USE_FE_CORBA;
            break;
        case 'P':
            Verbose("preprocessor option %s added\n", optarg);
            {
                // check for -I arguments, which we preprocess ourselves as well
                String sArg(optarg);
                pPreProcess->AddCPPArgument(sArg);
                if (sArg.Left(2) == "-I")
                {
                    String sPath = sArg.Right(sArg.GetLength()-2);
                    pPreProcess->AddIncludePath(sPath);
                    Verbose("Added %s to include paths\n", (const char*)sPath);
                }
				else if (sArg.Left(2) == "-D")
				{
				    String sDefine = sArg.Right(sArg.GetLength()-2);
				    AddSymbol(sDefine);
					Verbose("Found symbol \"%s\"\n", (const char*)sDefine);
				}
            }
            break;
        case 'I':
            Verbose("add include path %s\n", optarg);
            {
                String sPath("-I");
                sPath += optarg;
                pPreProcess->AddCPPArgument(sPath);	// copies sPath
                // add to own include paths
                pPreProcess->AddIncludePath(optarg);
                Verbose("Added %s to include paths\n", optarg);
            }
            break;
        case 'N':
            Verbose("no standard include paths\n");
            pPreProcess->AddCPPArgument(String("-nostdinc"));
            break;
        case 'f':
            {
                // provide flags to compiler: this can be anything
                // make upper case
                String sArg(optarg);
                String sOrig = sArg;
                sArg.MakeUpper();
                // test first letter
                char cFirst = sArg[0];
                switch (cFirst)
                {
                case 'F':
				    if (sArg == "FORCE-CORBA-ALLOC")
					{
					    m_nOptions |= PROGRAM_FORCE_CORBA_ALLOC;
						Verbose("Force use of CORBA_alloc (instead of Environment->malloc).\n");
						if (m_nOptions & PROGRAM_FORCE_ENV_MALLOC)
						{
						    m_nOptions &= ~PROGRAM_FORCE_ENV_MALLOC;
							Verbose("Overrides previous -fforce-env-malloc.\n");
						}
					}
					else if (sArg == "FORCE-ENV-MALLOC")
					{
					    m_nOptions |= PROGRAM_FORCE_ENV_MALLOC;
						Verbose("Force use of Environment->malloc (instead of CORBA_alloc).\n");
						if (m_nOptions & PROGRAM_FORCE_CORBA_ALLOC)
						{
						    m_nOptions &= ~PROGRAM_FORCE_CORBA_ALLOC;
							Verbose("Overrides previous -fforce-corba-alloc.\n");
						}
					}
					else if (sArg == "FORCE-C-BINDINGS")
					{
					    m_nOptions |= PROGRAM_FORCE_C_BINDINGS;
						Verbose("Force use of L4 C bindings (instead of inline assembler).\n");
					}
					else
					{
						// XXX FIXME: test for 'FILETYPE=' too
						m_nOptions &= ~PROGRAM_FILE_MASK;	// remove previous setting
						if ((sArg == "FIDLFILE") || (sArg == "F1"))
						{
							m_nOptions |= PROGRAM_FILE_IDLFILE;
							Verbose("filetype is set to IDLFILE\n");
						}
						else if ((sArg == "FMODULE") || (sArg == "F2"))
						{
							m_nOptions |= PROGRAM_FILE_MODULE;
							Verbose("filetype is set to MODULE\n");
						}
						else if ((sArg == "FINTERFACE") || (sArg == "F3"))
						{
							m_nOptions |= PROGRAM_FILE_INTERFACE;
							Verbose("filetype is set to INTERFACE\n");
						}
						else if ((sArg == "FFUNCTION") || (sArg == "F4"))
						{
							m_nOptions |= PROGRAM_FILE_FUNCTION;
							Verbose("filetype is set to FUNCTION\n");
						}
						else if ((sArg == "FALL") || (sArg == "F5"))
						{
							m_nOptions |= PROGRAM_FILE_ALL;
							Verbose("filetype is set to ALL\n");
						}
						else
						{
							Error("\"%s\" is an invalid argument for option -ff\n", &optarg[1]);
						}
					}
                    break;
                case 'C':
                    if (sArg == "CTYPES")
                    {
                        m_nOptions |= PROGRAM_USE_CTYPES;
                        Verbose("use C-style type names\n");
                    }
					else if (sArg == "CONST-AS-DEFINE")
					{
					    m_nOptions |= PROGRAM_CONST_AS_DEFINE;
						Verbose("print const declarators as define statements\n");
					}
                    break;
                case 'I':
                    if (sArg.Left(14) == "INIT-RCVSTRING")
                    {
                        m_nOptions |= PROGRAM_INIT_RCVSTRING;
                        if (sArg.GetLength() > 14)
                        {
                            String sName = sOrig.Right(sOrig.GetLength()-15);
                            Verbose("User provides function \"%s\" to init indirect receive strings\n", (const char*)sName);
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
					    m_nOptions |= PROGRAM_KEEP_TMP_FILES;
						Verbose("Keep temporary files generated during preprocessing\n");
					}
					break;
                case 'L':
                    if (sArg == "L4TYPES")
                    {
                        m_nOptions |= PROGRAM_USE_L4TYPES | PROGRAM_USE_CTYPES;
                        Verbose("use L4-style type names\n");
                    }
                    break;
				case 'N':
				    if (sArg == "NO-SEND-CANCELED-CHECK")
					{
					    m_nOptions |= PROGRAM_NO_SEND_CANCELED_CHECK;
						Verbose("Do not check if the send part of a call was canceled.\n");
					}
					else if ((sArg == "NO-SERVER-LOOP") ||
					         (sArg == "NO-SRVLOOP"))
					{
					    m_nOptions |= PROGRAM_NO_SERVER_LOOP;
						Verbose("Do not generate server loop.\n");
					}
					else if ((sArg == "NO-DISPATCH") ||
					         (sArg == "NO-DISPATCHER"))
					{
					    m_nOptions |= PROGRAM_NO_DISPATCHER;
						Verbose("Do not generate dispatch function.\n");
					}
					break;
                case 'O':
                    if (sArg.Left(12) == "OPCODE-SIZE=")
                    {
                        String sType = sArg.Right(sArg.GetLength()-12);
                        if (isdigit(sArg[13]))
                        {
                            m_nOpcodeSize = atoi(sType);
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
                            Verbose("The opcode-size \"%s\" is not supported\n", (const char*)sType);
                    }
                    break;
                case 'S':
                    if (sArg == "SERVER-PARAMETER")
                    {
                        m_nOptions |= PROGRAM_SERVER_PARAMETER;
                        Verbose("User provides CORBA_Environment parameter to server loop\n");
                    }
                    break;
                case 'T':
					if (sArg.Left(12) == "TRACE-SERVER")
					{
					    m_nOptions |= PROGRAM_TRACE_SERVER;
						Verbose("Trace messages received by the server loop\n");
					    if (sArg.GetLength() > 12)
						{
						    String sName = sOrig.Right(sOrig.GetLength()-13);
							Verbose("User sets trace function to \"%s\".\n", (const char*)sName);
							m_sTraceServerFunc = sName;
						}
					}
					else if (sArg.Left(12) == "TRACE-CLIENT")
					{
					    m_nOptions |= PROGRAM_TRACE_CLIENT;
						Verbose("Trace messages send to the server and answers received from it\n");
					    if (sArg.GetLength() > 12)
						{
						    String sName = sOrig.Right(sOrig.GetLength()-13);
							Verbose("User sets trace function to \"%s\".\n", (const char*)sName);
							m_sTraceClientFunc = sName;
						}
					}
					else if (sArg.Left(17) == "TRACE-DUMP-MSGBUF")
					{
					    m_nOptions |= PROGRAM_TRACE_MSGBUF;
						Verbose("Trace the message buffer struct\n");
					    if (sArg.GetLength() > 17)
						{
						    // check if lines are meant
							if (sArg.Left(24) == "TRACE-DUMP-MSGBUF-DWORDS")
							{
							    if (sArg.GetLength() > 24)
								{
								    String sNumber = sArg.Right(sArg.GetLength()-25);
									m_nDumpMsgBufDwords = sNumber.ToInt();
									if (m_nDumpMsgBufDwords >= 0)
									    m_nOptions |= PROGRAM_TRACE_MSGBUF_DWORDS;
								}
								else
								    Error("The option -ftrace-dump-msgbuf-dwords expects an argument (e.g. -ftrace-dump-msgbuf-dwords=10).\n");
							}
							else
							{
								String sName = sOrig.Right(sOrig.GetLength()-18);
								Verbose("User sets trace function to \"%s\".\n", (const char*)sName);
								m_sTraceMsgBuf = sName;
							}
						}
                    }
					else if (sArg.Left(14) == "TRACE-FUNCTION")
					{
					    if (sArg.GetLength() > 14)
						{
						    String sName = sOrig.Right(sOrig.GetLength()-15);
							Verbose("User sets trace function to \"%s\".\n", (const char*)sName);
							if (m_sTraceServerFunc.IsEmpty())
							    m_sTraceServerFunc = sName;
							if (m_sTraceClientFunc.IsEmpty())
							    m_sTraceClientFunc = sName;
							if (m_sTraceMsgBuf.IsEmpty())
							    m_sTraceMsgBuf = sName;
						}
						else
						    Error("The option -ftrace-function expects an argument (e.g. -ftrace-function=LOGl).\n");
					}
					else if ((sArg == "TEST-NO-SUCCESS") || (sArg == "TEST-NO-SUCCESS-MESSAGE"))
					{
					    Verbose("User does not want to see success messages of testsuite.\n");
						m_nOptions |= PROGRAM_TESTSUITE_NO_SUCCESS_MESSAGE;
					}
					else if (sArg == "TESTSUITE-SHUTDOWN")
					{
					    Verbose("User wants to shutdown Fiasco after running the testsuite.\n");
						m_nOptions |= PROGRAM_TESTSUITE_SHUTDOWN_FIASCO;
					}
					break;
                case 'U':
				    if ((sArg == "USE-SYMBOLS") || (sArg == "USE-DEFINES"))
					{
					    m_nOptions |= PROGRAM_USE_SYMBOLS;
                        Verbose("Use the symbols/defines given as arguments\n");
					}
					break;
				case 'Z':
				    if (sArg == "ZERO-MSGBUF")
					{
					    m_nOptions |= PROGRAM_ZERO_MSGBUF;
						Verbose("Zero message buffer before using.\n");
					}
					break;
                default:
                    // XXX FIXME: might be old-style file-types
                    Warning("unsupported argument \"%s\" for option -f\n", (const char*)sArg);
                    break;
                }
            }
            break;
        case 'B':
            {
                // make upper case
                String sArg(optarg);
                sArg.MakeUpper();

                // test first letter
                // p - platform
                // i - kernel interface
                // m - language mapping
                char sFirst = sArg[0];
                switch (sFirst)
                {
                case 'P':
                    sArg = sArg.Right(sArg.GetLength()-1);
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
                        Warning("ARM back-end not fully supported yet!");
                        m_nBackEnd &= ~PROGRAM_BE_PLATFORM;
                        m_nBackEnd |= PROGRAM_BE_ARM;
                        Verbose("use back-end for ARM platform\n");
                    }
                    else
                    {
                        Error("\"%s\" is an invalid argument for option -B/--back-end\n", optarg);
                    }
                    break;
                case 'I':
                    sArg = sArg.Right(sArg.GetLength()-1);
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
                        Warning("V4 back-end not supported yet!");
                        m_nBackEnd &= ~PROGRAM_BE_INTERFACE;
                        m_nBackEnd |= PROGRAM_BE_V2; // PROGRAM_BE_V4;
                        Verbose("use back-end L4 version V.4\n");
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
                    sArg = sArg.Right(sArg.GetLength()-1);
                    if (sArg == "C")
                    {
                        m_nBackEnd &= ~PROGRAM_BE_LANGUAGE;
                        m_nBackEnd |= PROGRAM_BE_C;
                        Verbose("use back-end C language mapping\n");
                    }
                    else if (sArg == "CPP")
                    {
                        Warning("C++ back-end not supported yet!");
                        m_nBackEnd &= ~PROGRAM_BE_LANGUAGE;
                        m_nBackEnd |= PROGRAM_BE_C; // PROGRAM_BE_CPP;
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
				m_nOptions |= PROGRAM_STOP_AFTER_PRE;
			}
			else
			{
				String sArg(optarg);
				sArg.MakeUpper();
				if (sArg == "XML")
				{
					Verbose("stop after preprocessing and dump XML file.\n");
					m_nOptions |= PROGRAM_STOP_AFTER_PRE_XML;
				}
				else
				{
					Warning("unrecognized argument \"%s\" to option -E.\n", (const char*)sArg);
					m_nOptions |= PROGRAM_STOP_AFTER_PRE;
				}
			}
            break;
	    case 'O':
            {
                if (!optarg)
                {
                     m_nOptimizeLevel = PROGRAM_OPTIMIZE_MAXLEVEL;
                }
                else
                {
                     m_nOptimizeLevel = atoi(optarg);
                     if (m_nOptimizeLevel < PROGRAM_OPTIMIZE_MINLEVEL)
                     {
                         Warning("dice: Optimization levels smaller than %d not supported.", PROGRAM_OPTIMIZE_MINLEVEL);
                         m_nOptimizeLevel = PROGRAM_OPTIMIZE_MINLEVEL;
                     }
                     if (m_nOptimizeLevel > PROGRAM_OPTIMIZE_MAXLEVEL)
                     {
                         Warning("dice: Optimization level %d not supported in this version.", m_nOptimizeLevel);
                         m_nOptimizeLevel = PROGRAM_OPTIMIZE_MAXLEVEL;
                     }
                }
                Verbose("Optimization Level set to %d\n", m_nOptimizeLevel);
            }
            break;
	    case 'T':
            if (optarg)
            {
                String sKind(optarg);
                if (!(sKind.CompareNoCase("thread")))
                    m_nOptions |= PROGRAM_GENERATE_TESTSUITE_THREAD;
                else if (!(sKind.CompareNoCase("task")))
                    m_nOptions |= PROGRAM_GENERATE_TESTSUITE_TASK;
                else
                {
                    Warning("dice: Testsuite option \"%s\" not supported. Using \"thread\".", optarg);
                    m_nOptions |= PROGRAM_GENERATE_TESTSUITE_THREAD;
                }
            }
            else
                m_nOptions |= PROGRAM_GENERATE_TESTSUITE_THREAD;
            Verbose("generate testsuite for IDL.\n");
            // need server stubs for that
            m_nOptions |= PROGRAM_GENERATE_TEMPLATE;
            // need client and server files
            m_nOptions |= PROGRAM_GENERATE_CLIENT | PROGRAM_GENERATE_COMPONENT;
            break;
	    case 'V':
            ShowVersion();
            break;
	    case 'm':
            Verbose("generate message passing functions.\n");
            m_nOptions |= PROGRAM_GENERATE_MESSAGE;
            break;
        case 'M':
            {
                m_nOptions &= ~PROGRAM_DEPEND_MASK;
                // search for depedency options
                if (!optarg)
                {
                    m_nOptions |= PROGRAM_DEPEND_M;
                    Verbose("Create dependencies without argument\n");
                }
                else
                {
                    // make upper case
                    String sArg(optarg);
                    sArg.MakeUpper();
                    if (sArg == "M")
                    {
                        m_nOptions |= PROGRAM_DEPEND_MM;
                    }
                    else if (sArg == "D")
                    {
                        m_nOptions |= PROGRAM_DEPEND_MD;
                    }
                    else if (sArg == "MD")
                    {
                        m_nOptions |= PROGRAM_DEPEND_MMD;
                    }
                    else
                    {
                        Warning("dice: Argument \"%s\" of option -M unrecognized: ignoring.", optarg);
                        m_nOptions |= PROGRAM_DEPEND_M;
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
                String sArg(optarg);
                sArg.MakeUpper();
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
				else if (sArg.Left(2) == "P,")
				{
				    String sTmp(optarg);
					// remove "p,"
					String sCppArg = sTmp.Right(sTmp.GetLength()-2);
					pPreProcess->AddCPPArgument(sCppArg);
					Verbose("preprocessor argument \"%s\" added\n", (const char*)sCppArg);
					// check if CPP argument has special meaning
					if (sCppArg.Left(2) == "-I")
					{
						String sPath = sCppArg.Right(sCppArg.GetLength()-2);
						pPreProcess->AddIncludePath(sPath);
						Verbose("Added %s to include paths\n", (const char*)sPath);
					}
					else if (sCppArg.Left(2) == "-D")
					{
						String sDefine = sCppArg.Right(sCppArg.GetLength()-2);
						AddSymbol(sDefine);
						Verbose("Found symbol \"%s\"\n", (const char*)sDefine);
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
                String sArg("-D");
				sArg += optarg;
                pPreProcess->AddCPPArgument(sArg);
			    AddSymbol(optarg);
			    Verbose("Found symbol \"%s\"\n", optarg);
			}
			break;
        case 'x':
		    {
			    String sArg(optarg);
				pPreProcess->SetCPP((const char*)sArg);
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
            m_sInFileName.Empty();
        Verbose("Input file is: %s\n", (const char*)m_sInFileName);
    }

    if (m_nUseFrontEnd == USE_FE_NONE)
        m_nUseFrontEnd = USE_FE_DCE;

    if ((m_nOptions & PROGRAM_FILE_MASK) == 0)
        m_nOptions |= PROGRAM_FILE_IDLFILE;

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
	    ((m_nBackEnd & PROGRAM_BE_PLATFORM) != PROGRAM_BE_X0))
    {
	    Warning("The Arm Backend currently works with X0 native only!\n");
		Warning("  -> Setting interface to X0 native.\n");
		m_nBackEnd &= ~PROGRAM_BE_INTERFACE;
		m_nBackEnd |= PROGRAM_BE_X0;
    }

    if ((m_nOptions & (PROGRAM_GENERATE_CLIENT | PROGRAM_GENERATE_COMPONENT)) == 0)
        m_nOptions |= PROGRAM_GENERATE_CLIENT | PROGRAM_GENERATE_COMPONENT;

    if ((m_nOptions & (PROGRAM_GENERATE_TESTSUITE_THREAD | PROGRAM_GENERATE_TESTSUITE_TASK)) != 0)
	{
	    m_nOptions &= ~PROGRAM_NO_SERVER_LOOP;
	    m_nOptions |= PROGRAM_FORCE_CORBA_ALLOC;
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
	"\n"
	"    set <string> to 'force-corba-alloc' to force the usage of the CORBA_alloc\n"
	"       function instead of the CORBA_Environment's malloc member\n"
	"    set <string> to 'force-env-malloc' to force the usage of CORBA_Environment's\n"
	"       malloc member instead of CORBA_alloc\n"
	"    depending on their order they may override each other.\n"
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
    "    p - specifies the platform (IA32, IA64, ARM) (default: IA32)\n"
    "    i - specifies the kernel interface (v2, x0, x0adapt, v4, sock)(default:v2)\n"
    "    m - specifies the language mapping (C, CPP) (default: C)\n"
    "    example: -Bpia32 -Biv2 -BmC - which is default\n"
    "    (currently only the default values are supported)\n"
    "    For debugging purposes you can use the \"kernel\" interface 'sockets',\n"
    "    which uses sockets to communicate (-Bisock[ets]).\n"
    " -O <level>                  sets the optimization level\n"
    "    set <level> to \"0\" to turn off optimization\n"
    "    set <level> to \"1\" to optimize array and constructed type marshalling\n"
    "    if <level> is empty the highest is chosen\n"
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
    " -f<string>                   supply flags to compiler\n"
    " -B"
#if defined(HAVE_GETOPT_LONG)
    ", --back-end"
#endif
    " <string>     defines the back-end to use\n"
    " -O <level>                  sets the optimization level\n"
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
    if ((nPos = m_sInFileName.ReverseFind('/')) >= 0)
	{
		pPreProcess->AddIncludePath(m_sInFileName.Left(nPos+1));
		Verbose("Added %s to include paths\n", (const char*)(m_sInFileName.Left(nPos+1)));
	}
    // set namespace to uninitialized
	CParser *pParser = CParser::CreateParser(m_nUseFrontEnd);
	CParser::SetCurrentParser(pParser);
	if (!pParser->Parse(0 /* no exitsing scan buffer*/, m_sInFileName,
	    m_nUseFrontEnd, ((m_nVerboseLevel & PROGRAM_VERBOSE_PARSER) > 0),
		(m_nOptions & PROGRAM_STOP_AFTER_PRE) > 0))
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
            Warning("%s: warning: %d Warning(s) occured while parsing.", (const char *) m_sInFileName, warningcount);
    }
	// if we should stop after preprocessing we write the intermediate output
    if (m_nOptions & PROGRAM_STOP_AFTER_PRE_XML)
    {
        Verbose("Start generating XML output ...\n");
        String sXMLFilename = m_pRootFE->GetFileNameWithoutExtension() + ".xml";
        CFile *pXML = new CFile();
        pXML->Open(sXMLFilename, CFile::Write);
        pXML->Print("<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n");
        m_pRootFE->Serialize(pXML);
        pXML->Close();
        Verbose("... finished generating XML output\n");
    }
}

/**
 *	\brief prepares the write operation
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
    if (m_nOptions & (PROGRAM_STOP_AFTER_PRE | PROGRAM_STOP_AFTER_PRE_XML))
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
    else if (m_nBackEnd & PROGRAM_BE_SOCKETS)
        pCF = new CSockBEClassFactory((m_nVerboseLevel & PROGRAM_VERBOSE_CLASSFACTORY) > 0);
	else if (m_nBackEnd & PROGRAM_BE_CDR)
	    pCF = new CCDRClassFactory((m_nVerboseLevel & PROGRAM_VERBOSE_CLASSFACTORY) > 0);
    else
        pCF = new CBEClassFactory((m_nVerboseLevel & PROGRAM_VERBOSE_CLASSFACTORY) > 0);

    // set name factory depending on arguments
    if ((m_nBackEnd & PROGRAM_BE_V2) || (m_nBackEnd & PROGRAM_BE_X0) || (m_nBackEnd & PROGRAM_BE_X0ADAPT))
        pNF = new CL4BENameFactory((m_nVerboseLevel & PROGRAM_VERBOSE_NAMEFACTORY) > 0);
    else
        pNF = new CBENameFactory((m_nVerboseLevel & PROGRAM_VERBOSE_NAMEFACTORY) > 0);

    // create context
    m_pContext = new CBEContext(pCF, pNF);
    Verbose("...done.\n");

    Verbose("Set context parameters...\n");
    m_pContext->ModifyOptions(m_nOptions);
    m_pContext->ModifyBackEnd(m_nBackEnd);
    m_pContext->SetFilePrefix(m_sFilePrefix);
    m_pContext->SetIncludePrefix(m_sIncludePrefix);
    m_pContext->SetOptimizeLevel(m_nOptimizeLevel);
    m_pContext->ModifyWarningLevel(m_nWarningLevel);
    m_pContext->SetOpcodeSize(m_nOpcodeSize);
    m_pContext->SetInitRcvStringFunc(m_sInitRcvStringFunc);
	m_pContext->SetTraceClientFunc(m_sTraceClientFunc);
	m_pContext->SetTraceServerFunc(m_sTraceServerFunc);
	m_pContext->SetTraceMsgBufFunc(m_sTraceMsgBuf);
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
    if (m_nOptions & PROGRAM_DEPEND_MASK)
		PrintDependencies();
    Verbose("... dependencies done.\n");

    // if we should stop after printing the dependencies, stop here
    if (m_nOptions & (PROGRAM_DEPEND_M | PROGRAM_DEPEND_MM))
    {
         delete m_pContext;
		 delete m_pRootBE;
         delete pNF;
         delete pCF;
         return;
    }

    // optimize
    Verbose("Start optimizing backend...\n");
    Optimize();
    Verbose("... finished optimizing backend.\n");
}

/**
 *	\brief does the optimizing work
 *
 * This method creates an optimizer object and exectutes any optimization tasks.
 */
void CCompiler::Optimize()
{
    // if we should stop after preprocessing skip this function
    if (m_nOptions & (PROGRAM_STOP_AFTER_PRE | PROGRAM_STOP_AFTER_PRE_XML | PROGRAM_DEPEND_M | PROGRAM_DEPEND_MM))
        return;

    // create optimizer
    Verbose("Start optimize...\n");
    if (!m_pRootBE || !m_pContext)
        Error("Internal error: back-end not set when optimizing\n");
    int nRet = 0;
    if ((nRet = m_pRootBE->Optimize(m_nOptimizeLevel, m_pContext)) != 0)
        Error("Could not optimize IDL (return code: %d).\n", nRet);
    Verbose("...done.\n");
}

/**
 *	\brief prints the target files
 *
 * This method calls the write operation of the backend
 */
void CCompiler::Write()
{
    // if we should stop after preprocessing skip this function
    if (m_nOptions & (PROGRAM_STOP_AFTER_PRE | PROGRAM_STOP_AFTER_PRE_XML | PROGRAM_DEPEND_M | PROGRAM_DEPEND_MM))
        return;

    // write backend
    Verbose("Write backend...\n");
    m_pRootBE->Write(m_pContext);
    Verbose("...done.\n");
}

/**
 *	\brief helper function
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
 *	\brief helper function
 *	\param sMsg the error message to print before exiting
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

/**	\brief helper function
 *	\param pFEObject the front-end object where the error occurred
 *	\param nLinenb the linenumber of the error
 *	\param sMsg the error message
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

/**	\brief helper function
 *	\param pFEObject the front-end object where the error occurred
 *	\param nLinenb the linenumber of the error
 *	\param sMsg the error message
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
        CFEBase *pCur = pFEObject->GetFile();
        Vector *pStack = new Vector(RUNTIME_CLASS(CFEFile));
        while (pCur)
        {
            pStack->AddHead(pCur);
            // get file of parent because GetFile start with "this"
            if (pCur->GetParent())
                pCur = ((CFEBase*)(pCur->GetParent()))->GetFile();
            else
                pCur = 0;
        }
        // need at least one file
        if (pStack->GetSize() > 0)
        {
            // walk down
            if (pStack->GetSize() > 1)
                fprintf(stderr, "In file included ");
            VectorElement *pIter;
            for (pIter = pStack->GetFirst(); pIter->GetNext(); pIter = pIter->GetNext())
            {
                if (pIter->GetElement())
                {
                    // we do not use GetFullFileName, because the "normal" filename already includes the whole path
                    // it is the filename generated by Gcc
					CFEFile *pFEFile = (CFEFile *)(pIter->GetElement());
					int nIncludeLine = 1;
					if (pIter->GetNext())
					    nIncludeLine = ((CFEFile*)(pIter->GetNext()->GetElement()))->GetIncludedOnLine();
                    fprintf(stderr, "from %s:%d", (const char*) pFEFile->GetFileName(), nIncludeLine);
                    if (pIter->GetNext()->GetNext())
                        fprintf(stderr, ",\n                 ");
                    else
                        fprintf(stderr, ":\n");
                }
            }
            if (pIter->GetElement())
            {
                // we do not use GetFullFileName, because the "normal" filename already includes the whole path
                // it is the filename generated by Gcc
                fprintf(stderr, "%s:%d: ", (const char*) ((CFEFile *) (pIter->GetElement()))->GetFileName(), nLinenb);
            }
        }
        // cleanup
        delete pStack;
    }
    vfprintf(stderr, sMsg, vl);
    fprintf(stderr, "\n");
}

/**
 *	\brief helper function
 *	\param sMsg the warning message to print
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

/**	\brief print warning messages
 *	\param pFEObject the object where the warning occured
 *	\param nLinenb the line in the file where the object originated
 *	\param sMsg the warning message
 */
void CCompiler::GccWarning(CFEBase * pFEObject, int nLinenb, const char *sMsg, ...)
{
    va_list args;
    va_start(args, sMsg);
    GccWarningVL(pFEObject, nLinenb, sMsg, args);
    va_end(args);
}

/**	\brief print warning messages
 *	\param pFEObject the object where the warning occured
 *	\param nLinenb the line in the file where the object originated
 *	\param sMsg the warning message
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
        CFEBase *pCur = pFEObject->GetFile();
        Vector *pStack = new Vector(RUNTIME_CLASS(CFEFile));
        while (pCur)
        {
            pStack->AddHead(pCur);
            // start with parent of current, because GetFile starts with "this"
            if (pCur->GetParent())
                pCur = ((CFEBase*)(pCur->GetParent()))->GetFile();
            else
                pCur = 0;
        }
        // need at least one file
        if (pStack->GetSize() > 0)
        {
            // walk down
            if (pStack->GetSize() > 1)
                fprintf(stderr, "In file included ");
            VectorElement *pIter;
            for (pIter = pStack->GetFirst(); pIter->GetNext(); pIter = pIter->GetNext())
            {
                if (pIter->GetElement())
                {
                    fprintf(stderr, "from %s:%d", (const char*) ((CFEFile *) (pIter->GetElement()))->GetFullFileName(), 1);
                    if (pIter->GetNext()->GetNext())
                        fprintf(stderr, ",\n                 ");
                    else
                        fprintf(stderr, ":\n");
                }
            }
            if (pIter->GetElement())
            {
                fprintf(stderr, "%s:%d: warning: ", (const char*) ((CFEFile *) (pIter->GetElement()))->GetFullFileName(), nLinenb);
            }
        }
        // cleanup
        delete pStack;
    }
    vfprintf(stderr, sMsg, vl);
    fprintf(stderr, "\n");
}

/**	\brief print the dependency tree
 *
 * The dependency tree contains the files the top IDL file depends on. These are the included files.
 */
void CCompiler::PrintDependencies()
{
    if (!m_pRootBE)
        return;

    // if file, open file
    FILE *output = stdout;
    if ((m_nOptions & PROGRAM_DEPEND_MD) || (m_nOptions & PROGRAM_DEPEND_MMD))
    {
        String sOutName = m_pRootFE->GetFileNameWithoutExtension() + ".d";
        output = fopen((const char *) sOutName, "w");
        if (!output)
        {
			Warning("Could not open %s, use <stdout>\n", (const char*)sOutName);
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
    if ((m_nOptions & PROGRAM_DEPEND_MD) || (m_nOptions & PROGRAM_DEPEND_MMD))
        fclose(output);
}

/**	\brief prints the included files
 *	\param output the output stream
 *	\param pFEFile the current front-end file
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
    VectorElement *pIter = pFEFile->GetFirstChildFile();
    CFEFile *pIncFile;
    while ((pIncFile = pFEFile->GetNextChildFile(pIter)) != 0)
    {
        if (pIncFile->IsStdIncludeFile() && ((m_nOptions & PROGRAM_DEPEND_MM) || (m_nOptions & PROGRAM_DEPEND_MMD)))
            continue;
        PrintDependentFile(output, pIncFile->GetFullFileName());
    }
    // ierate over included files
    pIter = pFEFile->GetFirstChildFile();
    while ((pIncFile = pFEFile->GetNextChildFile(pIter)) != 0)
    {
        if (pIncFile->IsStdIncludeFile() && ((m_nOptions & PROGRAM_DEPEND_MM) || (m_nOptions & PROGRAM_DEPEND_MMD)))
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

    if ((m_nOptions & PROGRAM_FILE_IDLFILE) || (m_nOptions & PROGRAM_FILE_ALL))
    {
        PrintGeneratedFiles4File(output, pFEFile);
    }
    else if (m_nOptions & PROGRAM_FILE_MODULE)
    {
        PrintGeneratedFiles4Library(output, pFEFile);
    }
    else if (m_nOptions & PROGRAM_FILE_INTERFACE)
    {
        PrintGeneratedFiles4Interface(output, pFEFile);
    }
    else if (m_nOptions & PROGRAM_FILE_FUNCTION)
    {
        PrintGeneratedFiles4Operation(output, pFEFile);
    }

    CBENameFactory *pNF = m_pContext->GetNameFactory();
    String sName;
    // create client header file
    if (m_nOptions & PROGRAM_GENERATE_CLIENT)
    {
        m_pContext->SetFileType(FILETYPE_CLIENTHEADER);
        sName = pNF->GetFileName(pFEFile, m_pContext);
        PrintDependentFile(output, sName);
    }
    // create server header file
    if (m_nOptions & PROGRAM_GENERATE_COMPONENT)
    {
        // component file
        m_pContext->SetFileType(FILETYPE_COMPONENTHEADER);
        sName = pNF->GetFileName(pFEFile, m_pContext);
        PrintDependentFile(output, sName);
    }
    // opcodes
    if ((m_nOptions & PROGRAM_NO_OPCODES) == 0)
    {
        m_pContext->SetFileType(FILETYPE_OPCODE);
        sName = pNF->GetFileName(pFEFile, m_pContext);
        PrintDependentFile(output, sName);
    }

    if (m_nOptions & PROGRAM_FILE_ALL)
    {
        VectorElement *pIter = pFEFile->GetFirstChildFile();
        CFEFile *pFile;
        while ((pFile = pFEFile->GetNextChildFile(pIter)) != 0)
            PrintGeneratedFiles(output, pFile);
    }
}

/**	\brief prints the file-name generated for the front-end file
 *	\param output the target stream
 *	\param pFEFile the current front-end file
 */
void CCompiler::PrintGeneratedFiles4File(FILE * output, CFEFile * pFEFile)
{
    CBENameFactory *pNF = m_pContext->GetNameFactory();
    String sName;

    // client
    if (m_nOptions & PROGRAM_GENERATE_CLIENT)
    {
        // client implementation file
        m_pContext->SetFileType(FILETYPE_CLIENTIMPLEMENTATION);
        sName = pNF->GetFileName(pFEFile, m_pContext);
        PrintDependentFile(output, sName);
    }
    // server
    if (m_nOptions & PROGRAM_GENERATE_COMPONENT)
    {
        // component file
        m_pContext->SetFileType(FILETYPE_COMPONENTIMPLEMENTATION);
        sName = pNF->GetFileName(pFEFile, m_pContext);
        PrintDependentFile(output, sName);
    }
    // testsuite
    if (m_nOptions & PROGRAM_GENERATE_TESTSUITE)
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
    VectorElement *pIter = pFEFile->GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = pFEFile->GetNextLibrary(pIter)) != 0)
    {
        PrintGeneratedFiles4Library(output, pLibrary);
    }
    // iterate over interfaces
    pIter = pFEFile->GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = pFEFile->GetNextInterface(pIter)) != 0)
    {
        PrintGeneratedFiles4Library(output, pInterface);
    }

    // if testsuite
    if (m_pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
    {
        m_pContext->SetFileType(FILETYPE_TESTSUITE);
        String sTestName = m_pContext->GetNameFactory()->GetFileName(pFEFile, m_pContext);
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
    String sName;
    // client file
    if (m_nOptions & PROGRAM_GENERATE_CLIENT)
    {
        // implementation
        m_pContext->SetFileType(FILETYPE_CLIENTIMPLEMENTATION);
        sName = pNF->GetFileName(pFELibrary, m_pContext);
        PrintDependentFile(output, sName);
    }
    // component file
    if (m_nOptions & PROGRAM_GENERATE_COMPONENT)
    {
        // implementation
        m_pContext->SetFileType(FILETYPE_COMPONENTIMPLEMENTATION);
        sName = pNF->GetFileName(pFELibrary, m_pContext);
        PrintDependentFile(output, sName);
    }
    // nested libraries
    VectorElement *pIter = pFELibrary->GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = pFELibrary->GetNextLibrary(pIter)) != 0)
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
    String sName;
    // client file
    if (m_nOptions & PROGRAM_GENERATE_CLIENT)
    {
        // implementation
        m_pContext->SetFileType(FILETYPE_CLIENTIMPLEMENTATION);
        sName = pNF->GetFileName(pFEInterface, m_pContext);
        PrintDependentFile(output, sName);
    }
    // component file
    if (m_nOptions & PROGRAM_GENERATE_COMPONENT)
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
     VectorElement *pIter = pFEFile->GetFirstInterface();
     CFEInterface *pInterface;
     while ((pInterface = pFEFile->GetNextInterface(pIter)) != 0)
     {
         PrintGeneratedFiles4Interface(output, pInterface);
     }
     // iterate over libraries
     pIter = pFEFile->GetFirstLibrary();
     CFELibrary *pLibrary;
     while ((pLibrary = pFEFile->GetNextLibrary(pIter)) != 0)
     {
         PrintGeneratedFiles4Interface(output, pLibrary);
     }

     // if testsuite
     if (m_pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
     {
         m_pContext->SetFileType(FILETYPE_TESTSUITE);
         String sTestName = m_pContext->GetNameFactory()->GetFileName(pFEFile, m_pContext);
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
    VectorElement *pIter = pFELibrary->GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = pFELibrary->GetNextInterface(pIter)) != 0)
    {
        PrintGeneratedFiles4Interface(output, pInterface);
    }
    // iterate over nested libraries
    pIter = pFELibrary->GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = pFELibrary->GetNextLibrary(pIter)) != 0)
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
    String sName;
    // client file
    if (m_nOptions & PROGRAM_GENERATE_CLIENT)
    {
        // implementation
        m_pContext->SetFileType(FILETYPE_CLIENTIMPLEMENTATION);
        sName = pNF->GetFileName(pFEInterface, m_pContext);
        PrintDependentFile(output, sName);
    }
    // component file
    if (m_nOptions & PROGRAM_GENERATE_COMPONENT)
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
    VectorElement *pIter = pFEFile->GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = pFEFile->GetNextInterface(pIter)) != 0)
    {
        PrintGeneratedFiles4Operation(output, pInterface);
    }
    // iterate over libraries
    pIter = pFEFile->GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = pFEFile->GetNextLibrary(pIter)) != 0)
    {
        PrintGeneratedFiles4Operation(output, pLibrary);
    }

    // if testsuite
    if (m_pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
    {
        m_pContext->SetFileType(FILETYPE_TESTSUITE);
        String sTestName = m_pContext->GetNameFactory()->GetFileName(pFEFile, m_pContext);
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
    VectorElement *pIter = pFELibrary->GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = pFELibrary->GetNextInterface(pIter)) != 0)
    {
        PrintGeneratedFiles4Operation(output, pInterface);
    }
    // iterate over nested libraries
    pIter = pFELibrary->GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = pFELibrary->GetNextLibrary(pIter)) != 0)
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
    VectorElement *pIter = pFEInterface->GetFirstOperation();
    CFEOperation *pOperation;
    while ((pOperation = pFEInterface->GetNextOperation(pIter)) != 0)
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
     String sName;
     // client file
     if (m_nOptions & PROGRAM_GENERATE_CLIENT)
     {
         // implementation
         m_pContext->SetFileType(FILETYPE_CLIENTIMPLEMENTATION);
         sName = pNF->GetFileName(pFEOperation, m_pContext);
         PrintDependentFile(output, sName);
     }
     // component file
     if (m_nOptions & PROGRAM_GENERATE_COMPONENT)
     {
         // implementation
         m_pContext->SetFileType(FILETYPE_COMPONENTIMPLEMENTATION);
         sName = pNF->GetFileName(pFEOperation, m_pContext);
         PrintDependentFile(output, sName);
     }
}

/**	\brief prints a filename to the dependency tree
 *	\param output the stream to write to
 *	\param sFileName the name to print
 *
 * This implementation adds a spacer after each file name and also checks before writing
 * if the maximum column number would be exceeded by this file. If it would a new line is started
 * The length of the filename is added to the columns.
 */
void CCompiler::PrintDependentFile(FILE * output, String sFileName)
{
     int nLength = sFileName.GetLength();
     if ((m_nCurCol + nLength + 1) >= MAX_SHELL_COLS)
     {
         fprintf(output, "\\\n ");
         m_nCurCol = 1;
     }
     fprintf(output, "%s ", (const char *) sFileName);
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
    m_sSymbols[m_nSymbolCount - 2] = strdup(sNewSymbol);
    m_sSymbols[m_nSymbolCount - 1] = 0;
}

/** \brief adds another symbol to the internal list
 *  \param sNewSymbol the symbol to add
 */
void CCompiler::AddSymbol(String sNewSymbol)
{
    if (sNewSymbol.IsEmpty())
	    return;
    AddSymbol((const char*)sNewSymbol);
}
