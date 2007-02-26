/**
 *	\file	dice/src/be/BEContext.h 
 *	\brief	contains the declaration of the class CBEContext
 *
 *	\date	01/10/2002
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

/** preprocessing symbol to check header file */
#ifndef __DICE_BE_BECONTEXT_H__
#define __DICE_BE_BECONTEXT_H__

#include "be/BEObject.h"
#include "be/BENameFactory.h"
#include "be/BEClassFactory.h"
#include "be/BEFile.h"
#include "be/BESizes.h"

//@{
/** program argument options */
#define PROGRAM_NONE				0x00000000		/**< no arguments */
#define PROGRAM_VERBOSE				0x00000001		/**< print verbose status info */
#define PROGRAM_GENERATE_SKELETON	0x00000002		/**< generate server skeleton files */
#define PROGRAM_NO_OPCODES			0x00000004		/**< generate NO opcode files */
#define PROGRAM_GENERATE_INLINE		0x00000008		/**< generate client stub as inline */
#define PROGRAM_GENERATE_INLINE_STATIC	0x0010		/**< generate client stub as static inline */
#define PROGRAM_GENERATE_INLINE_EXTERN	0x0020		/**< generate client stub as extern inline */
#define PROGRAM_SERVER_PARAMETER	0x00000040		/**< generate server with wait timeout */
#define PROGRAM_SERVER_RETURN_CODE	0x00000080		/**< do generate a return code from server to client */
#define PROGRAM_FILE_IDLFILE		0x00000100		/**< target client C files are one per IDL file */
#define PROGRAM_FILE_MODULE			0x00000200		/**< target client C files are one per module */
#define PROGRAM_FILE_INTERFACE		0x00000400		/**< target client C files are one per interface */
#define PROGRAM_FILE_FUNCTION		0x00000800		/**< target client C files are one per function */
#define PROGRAM_FILE_ALL			0x00001000		/**< target client C files is one for all */
#define PROGRAM_STOP_AFTER_PRE		0x00002000		/**< the compiler stops processing after the pre-processing */
#define PROGRAM_GENERATE_TESTSUITE_THREAD	0x00004000		/**< generate testsuite for stubs with server in thread */
#define PROGRAM_GENERATE_TESTSUITE_TASK     0x00008000      /**< generate testsuite for stubs with server in task */
#define PROGRAM_GENERATE_TESTSUITE  0x0000c000      /**< general test for testsuite */
#define PROGRAM_GENERATE_MESSAGE	0x00010000		/**< generate message passing functions as well */
#define PROGRAM_GENERATE_CLIENT		0x00020000		/**< generate client side code */
#define PROGRAM_GENERATE_COMPONENT	0x00040000		/**< generate component side code */
#define PROGRAM_INIT_RCVSTRING      0x00080000      /**< use a user-provided function to init recv-ref-strings */

#define PROGRAM_DEPEND_M			0x01000000		/**< print includes to stdout and stop */
#define PROGRAM_DEPEND_MM			0x02000000		/**< print includes with "" to stdout and stop */
#define PROGRAM_DEPEND_MD			0x04000000		/**< print includes to .d file and compile */
#define PROGRAM_DEPEND_MMD			0x08000000		/**< print includes with "" to .d file and compile */

#define PROGRAM_DYNAMIC_INLINE		0x10000000		/**< set and unset dynamically during runtime */
#define PROGRAM_USE_CTYPES          0x20000000      /**< use C-style type names, instead of CORBA-style type names */
#define PROGRAM_USE_L4TYPES         0x40000000      /**< use L4-style type names where appropriate, otherwise use C style */
#define PROGRAM_FORCE_CORBA_ALLOC   0x80000000      /**< Force use of CORBA_alloc function (instead of Env->malloc) */

// for use with the second word ...
typedef long long ProgramOptionType;

#define PROGRAM_FORCE_C_BINDINGS    (((ProgramOptionType)0x00000001)<<32)  /**< Force use or L4 C-bindings (instead of inline assembler) */
#define PROGRAM_TRACE_SERVER        (((ProgramOptionType)0x00000002)<<32)  /**< Trace all messages received by the server loop */

#define PROGRAM_MASK				0xffffffff		/**< the argument mask */
#define PROGRAM_FILE_MASK			0x00000f00		/**< the target client C file arguments */
#define PROGRAM_DEPEND_MASK			0x0f000000		/**< the dependency mask */

#define FUNCTION_SEND				0x01			/**< the send function */
#define FUNCTION_RECV				0x02			/**< the receive function */
#define FUNCTION_WAIT				0x03			/**< the wait function */
#define FUNCTION_UNMARSHAL			0x04			/**< the unmarshal function */
#define FUNCTION_REPLY_RECV			0x05			/**< the reply-and-receive function */
#define FUNCTION_REPLY_WAIT			0x06			/**< the reply-and-wait function */
#define FUNCTION_CALL				0x07			/**< the call function */
#define FUNCTION_SKELETON			0x08			/**< the server skeleton function */
#define FUNCTION_WAIT_ANY			0x09			/**< the wait any function */
#define FUNCTION_RECV_ANY			0x0a			/**< the receive any function */
#define FUNCTION_SRV_LOOP			0x0b			/**< the server loop function */
#define FUNCTION_SWITCH_CASE		0x0c			/**< the switch case statement */
#define FUNCTION_REPLY_ANY_WAIT_ANY 0x0d            /**< the reply and wait function for any message */
#define FUNCTION_TESTFUNCTION		0x10			/**< additional bit to signal the test function */
#define FUNCTION_NOTESTFUNCTION		0x0f			/**< a bit-mask to find the bits without the test bit */
//@}

#define PROGRAM_OPTIMIZE_MINLEVEL   0   /**< the minimum supported optimization level */
#define PROGRAM_OPTIMIZE_MAXLEVEL	2	/**< the maximum supported optimization level */

//@{
/** verbose output options */
#define PROGRAM_VERBOSE_PARSER			0x00000001		/**< makes the parser generate verbose output */
#define PROGRAM_VERBOSE_CLASSFACTORY	0x00000002		/**< makes the class-factory generate verbose output */
#define PROGRAM_VERBOSE_NAMEFACTORY		0x00000004		/**< makes the name-factory generate verbose output */
#define PROGRAM_VERBOSE_FACTORIES		0x00000006		/**< makes all factories generate verbose output */
#define PROGRAM_VERBOSE_FRONTEND		0x00000008		/**< makes the front-end g.v.o. */
#define PROGRAM_VERBOSE_DATAREP			0x00000010		/**< makes the data-representation g.v.o. */
#define PROGRAM_VERBOSE_BACKEND			0x00000020		/**< makes the back-end g.v.o. */

#define PROGRAM_VERBOSE_ALL				0x0000003f		/**< generates all verbose output */
#define PROGRAM_VERBOSE_MODULES			0x00000038		/**< generates verbose output for all "ends" */
#define PROGRAM_VERBOSE_LEVEL_1			PROGRAM_VERBOSE_BACKEND	/**< level 1 */
#define PROGRAM_VERBOSE_LEVEL_2			PROGRAM_VERBOSE_BACKEND | PROGRAM_VERBOSE_DATAREP	/** level 2 */
#define PROGRAM_VERBOSE_LEVEL_3			PROGRAM_VERBOSE_MODULES	/**< level 3 */
#define PROGRAM_VERBOSE_LEVEL_4			PROGRAM_VERBOSE_FACTORIES /**< level 4 */
#define PROGRAM_VERBOSE_LEVEL_5			PROGRAM_VERBOSE_MODULES | PROGRAM_VERBOSE_FACTORIES	/**< level 5 */
#define PROGRAM_VERBOSE_LEVEL_6			PROGRAM_VERBOSE_MODULES | PROGRAM_VERBOSE_FACTORIES | PROGRAM_VERBOSE_PARSER /**< level 6 */
#define PROGRAM_VERBOSE_LEVEL_7			PROGRAM_VERBOSE_PARSER	/**< level 7 */

#define PROGRAM_VERBOSE_MAXLEVEL		7			/**< the maximum verbose level */
//@}

//@{
/** warning level options */
#define PROGRAM_WARNING_IGNORE_DUPLICATE_FID   0x00000001 /**< ignore duplicated function IDs */
#define PROGRAM_WARNING_PREALLOC               0x00000002 /**< warn on allocating memory for unbound strings */
#define PROGRAM_WARNING_NO_MAXSIZE             0x00000004 /**< warn on missing max-size attributes */
#define PROGRAM_WARNING_ALL                    0xffffffff /**< warn on everything */
//@}

//@{
/** file type options */
#define FILETYPE_CLIENTHEADER				0x01
#define FILETYPE_CLIENTIMPLEMENTATION		0x02
#define FILETYPE_COMPONENTHEADER			0x03
#define FILETYPE_COMPONENTIMPLEMENTATION	0x04
#define FILETYPE_TESTSUITE					0x05
#define FILETYPE_OPCODE						0x06
#define FILETYPE_SKELETON                   0x07
#define FILETYPE_CLIENT                     0x08 /**< includes header and implementation file */
#define FILETYPE_COMPONENT                  0x09 /**< includes header and implementation file */
//@}

//@{
/** back-end options */
#define PROGRAM_BE_V2				0x0001		/**< defines the back-end for L4 version 2 */
#define PROGRAM_BE_X0				0x0002		/**< defines the back-end for L4 version X.0 */
#define PROGRAM_BE_X0ADAPT          0x0004      /**< defines the back-end for adaption version of V2 user land to X0 kernel */
#define PROGRAM_BE_FLICK			0x0008		/**< defines the back-end for Flick compatibility */
#define PROGRAM_BE_V4				0x0010		/**< defines the back-end for L4 version X.2 */
#define PROGRAM_BE_SOCKETS          0x0020      /**< defines the back-end for the Linux Sockets */
#define PROGRAM_BE_INTERFACE		0x003f		/**< bit-mask for interface */

#define PROGRAM_BE_IA32				0x0100		/**< defines the back-end for IA32 platform */
#define PROGRAM_BE_IA64				0x0200		/**< defines the back-end for IA64 platform */
#define PROGRAM_BE_ARM				0x0400		/**< defines the back-end for ARM platform */
#define PROGRAM_BE_PLATFORM			0x0f00		/**< bit-mask for platform */

#define PROGRAM_BE_C				0x1000		/**< defines the back-end for C mapping */
#define PROGRAM_BE_CPP				0x2000		/**< defines the back-end for C++ mapping */
#define PROGRAM_BE_LANGUAGE			0xf000		/**< bit-mask for language mapping */
//@}

/** \class CBEContext
 *	\ingroup backend
 *	\brief The context class of the back-end
 *
 * This class contains information, which makes up the context of the write operation.
 * E.g. the target file, the class and name factory, and some additional options.
 */
class CBEContext:public CBEObject
{
    DECLARE_DYNAMIC(CBEContext);
// Constructor
  public:
	/**
	 *	\brief constructs a back-end context object
	 *	\param pCF a reference to the class factory
	 *	\param pNF a reference to the name factory
	 *
	 * This implementation sets the members m_pClassFactory and m_pNameFactory to the
	 * respective parameter values and initializes the other members with standard values.
	 */
    CBEContext(CBEClassFactory * pCF, CBENameFactory * pNF);
    virtual ~ CBEContext();

  protected:
	/**	\brief the copy constructor
	 *	\param src the source to copy from
	 */
    CBEContext(CBEContext & src);

// Operations
  public:
    virtual int SetFunctionType(int nNewType);
    virtual int GetFunctionType();
    virtual String GetIncludePrefix();
    virtual void SetIncludePrefix(String sIncludePrefix);
    virtual String GetFilePrefix();
    virtual void SetFilePrefix(String sFilePrefix);
    virtual bool IsVerbose();
    virtual bool IsOptionSet(ProgramOptionType nOption);
    virtual void ModifyOptions(ProgramOptionType nAdd, ProgramOptionType nRemove = 0);


    virtual CBENameFactory *GetNameFactory();
    virtual CBEClassFactory *GetClassFactory();
    virtual int GetFileType();
    virtual int SetFileType(int nFileType);
    virtual void SetOptimizeLevel(int nNewLevel);
    virtual int GetOptimizeLevel();
    virtual void ModifyWarningLevel(unsigned long nAdd, unsigned long nRemove = 0);
    virtual unsigned long GetWarningLevel();
    virtual bool IsWarningSet(unsigned long nLevel);
    virtual void ModifyBackEnd(unsigned long nAdd, unsigned long nRemove = 0);
    virtual CBESizes* GetSizes();
    virtual bool IsBackEndSet(unsigned long nOption);
    void SetOpcodeSize(int nSize);
    virtual String GetInitRcvStringFunc();
    virtual void SetInitRcvStringFunc(String sName);

// Attributes
  protected:
	/** \var int m_nFunctionType
	 *	\brief contains the function type value
	 */
    int m_nFunctionType;
	/**	\var int m_nFileType
	 *	\brief contains the file type
	 *
	 * This file type is used to distinguish client-header or client-implementation from component-header or
	 * component-implementation files.
	 */
    int m_nFileType;
	/**	\var String m_sIncludePrefix
	 *	\brief contains the include prefix string
	 */
    String m_sIncludePrefix;
	/**	\var String m_sFilePrefix
	 *	\brief contains the file prefix string
	 */
    String m_sFilePrefix;
	/**	\var ProgramOptionType m_nOptions
	 *	\brief contains the compiler's parameters
	 */
    ProgramOptionType m_nOptions;
    /** \var unsigned long m_nBackEnd
     *  \brief determines the back-end
     */
    unsigned long m_nBackEnd;
	/**	\var CBEClassFactory *m_pClassFactory
	 *	\brief a reference to the class factory
	 */
    CBEClassFactory *m_pClassFactory;
	/**	\var CBENameFactory *m_pNameFactory
	 *	\brief a reference to the name factory
	 */
    CBENameFactory *m_pNameFactory;
    /** \var int m_nOptimizeLevel
     *  \brief contains the optimization level selected by the user
     */
    int m_nOptimizeLevel;
    /** \var int m_nWarningLevel
     *  \brief contains the warning levels
     */
    unsigned long m_nWarningLevel;
    /** \var CBESizes *m_pSizes
     *  \brief contains a reference to the sizes class of a traget architecture
     */
    CBESizes *m_pSizes;
    /** \var int m_nOpcodeSize
     *  \brief contains the size of the opcode type in bytes
     */
    int m_nOpcodeSize;
    /** \var String m_sInitRcvStringFunc
     *  \brief contains the name of the function to init the receive strings
     */
    String m_sInitRcvStringFunc;
};

#endif				// __DICE_BE_BECONTEXT_H__
