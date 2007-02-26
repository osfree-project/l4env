/**
 *    \file    dice/src/be/BEContext.h
 *    \brief   contains the declaration of the class CBEContext
 *
 *    \date    01/10/2002
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
#ifndef __DICE_BE_BECONTEXT_H__
#define __DICE_BE_BECONTEXT_H__

#include "be/BEObject.h"
#include "be/BENameFactory.h"
#include "be/BEClassFactory.h"
#include "be/BEFile.h"
#include "be/BESizes.h"
#include "ProgramOptions.h" // need it for ProgramOptionType here

//@{
/** function types */
#define FUNCTION_SEND               0x01            /**< the send function */
#define FUNCTION_RECV               0x02            /**< the receive function */
#define FUNCTION_WAIT               0x03            /**< the wait function */
#define FUNCTION_UNMARSHAL          0x04            /**< the unmarshal function */
#define FUNCTION_MARSHAL            0x05            /**< the marshal function */
#define FUNCTION_REPLY_RECV         0x06            /**< the reply-and-receive function */
#define FUNCTION_REPLY_WAIT         0x07            /**< the reply-and-wait function */
#define FUNCTION_CALL               0x08            /**< the call function */
#define FUNCTION_TEMPLATE           0x09            /**< the server function template */
#define FUNCTION_WAIT_ANY           0x0a            /**< the wait any function */
#define FUNCTION_RECV_ANY           0x0c            /**< the receive any function */
#define FUNCTION_SRV_LOOP           0x0d            /**< the server loop function */
#define FUNCTION_DISPATCH           0x0e            /**< the server loop'sdispatch function */
#define FUNCTION_SWITCH_CASE        0x0f            /**< the switch case statement */
#define FUNCTION_REPLY_ANY_WAIT_ANY 0x10            /**< the reply and wait function for any message */
#define FUNCTION_REPLY              0x11            /**< the reply only function */
#define FUNCTION_TESTFUNCTION       0x80            /**< additional bit to signal the test function */
#define FUNCTION_NOTESTFUNCTION     0x7f            /**< a bit-mask to find the bits without the test bit */
//@}

//@{
/** file type options */
#define FILETYPE_CLIENTHEADER               0x01
#define FILETYPE_CLIENTIMPLEMENTATION       0x02
#define FILETYPE_COMPONENTHEADER            0x03
#define FILETYPE_COMPONENTIMPLEMENTATION    0x04
#define FILETYPE_TESTSUITE                  0x05
#define FILETYPE_OPCODE                     0x06
#define FILETYPE_TEMPLATE                   0x07
#define FILETYPE_CLIENT                     0x08 /**< includes header and implementation file */
#define FILETYPE_COMPONENT                  0x09 /**< includes header and implementation file */
//@}

/** \class CBEContext
 *    \ingroup backend
 *    \brief The context class of the back-end
 *
 * This class contains information, which makes up the context of the write operation.
 * E.g. the target file, the class and name factory, and some additional options.
 */
class CBEContext:public CBEObject
{
// Constructor
  public:
    /**
     *    \brief constructs a back-end context object
     *    \param pCF a reference to the class factory
     *    \param pNF a reference to the name factory
     *
     * This implementation sets the members m_pClassFactory and m_pNameFactory to the
     * respective parameter values and initializes the other members with standard values.
     */
    CBEContext(CBEClassFactory * pCF, CBENameFactory * pNF);
    virtual ~ CBEContext();

  protected:
    /**    \brief the copy constructor
     *    \param src the source to copy from
     */
    CBEContext(CBEContext & src);

// Operations
  public:
    virtual int SetFunctionType(int nNewType);
    virtual int GetFunctionType();
    virtual string GetIncludePrefix();
    virtual void SetIncludePrefix(string sIncludePrefix);
    virtual string GetFilePrefix();
    virtual void SetFilePrefix(string sFilePrefix);
    virtual bool IsVerbose();
    virtual bool IsOptionSet(ProgramOptionType nOption);
    virtual bool IsOptionSet(unsigned int nRawOption);
    virtual void ModifyOptions(ProgramOptionType nAdd, ProgramOptionType nRemove);
    virtual void ModifyOptions(unsigned int nRawAdd, unsigned int nRawRemove = 0);
    virtual void AddSymbol(const char *sNewSymbol);
    virtual void AddSymbol(string sNewSymbol);
    virtual bool HasSymbol(const char *sSymbol);

    virtual CBENameFactory *GetNameFactory();
    virtual CBEClassFactory *GetClassFactory();
    virtual int GetFileType();
    virtual int SetFileType(int nFileType);
    virtual void ModifyWarningLevel(unsigned long nAdd, unsigned long nRemove = 0);
    virtual unsigned long GetWarningLevel();
    virtual bool IsWarningSet(unsigned long nLevel);
    virtual void ModifyBackEnd(unsigned long nAdd, unsigned long nRemove = 0);
    virtual CBESizes* GetSizes();
    virtual bool IsBackEndSet(unsigned long nOption);
    void SetOpcodeSize(int nSize);
    virtual string GetInitRcvStringFunc();
    virtual void SetInitRcvStringFunc(string sName);
    virtual string GetTraceClientFunc();
    virtual void SetTraceClientFunc(string sName);
    virtual string GetTraceServerFunc();
    virtual void SetTraceServerFunc(string sName);
    virtual string GetTraceMsgBufFunc();
    virtual void SetTraceMsgBufFunc(string sName);
    virtual int GetTraceMsgBufDwords();
    virtual void SetTraceMsgBufDwords(int nDwords);
    virtual string GetOutputDir();
    virtual void SetOutputDir(string sOutputDir);

    virtual void WriteMalloc(CBEFile* pFile, CBEFunction* pFunction);
    virtual void WriteFree(CBEFile* pFile, CBEFunction* pFunction);

  protected:
    virtual void SetOption(ProgramOptionType nOption);
    virtual void SetOption(unsigned int nRawOption);
    virtual void UnsetOption(unsigned int nRawOption);

// Attributes
  protected:
    /** \var int m_nFunctionType
     *    \brief contains the function type value
     */
    int m_nFunctionType;
    /**    \var int m_nFileType
     *    \brief contains the file type
     *
     * This file type is used to distinguish client-header or client-implementation from component-header or
     * component-implementation files.
     */
    int m_nFileType;
    /**    \var string m_sIncludePrefix
     *    \brief contains the include prefix string
     */
    string m_sIncludePrefix;
    /**    \var string m_sFilePrefix
     *    \brief contains the file prefix string
     */
    string m_sFilePrefix;
    /**    \var unsigned int m_nOptions[PROGRAM_OPTION_GROUPS]
     *    \brief the options which are specified with the compiler call
     */
    unsigned int m_nOptions[PROGRAM_OPTION_GROUPS];
    /** \var unsigned long m_nBackEnd
     *  \brief determines the back-end
     */
    unsigned long m_nBackEnd;
    /**    \var CBEClassFactory *m_pClassFactory
     *    \brief a reference to the class factory
     */
    CBEClassFactory *m_pClassFactory;
    /**    \var CBENameFactory *m_pNameFactory
     *    \brief a reference to the name factory
     */
    CBENameFactory *m_pNameFactory;
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
    /** \var string m_sInitRcvStringFunc
     *  \brief contains the name of the function to init the receive strings
     */
    string m_sInitRcvStringFunc;
    /** \var string m_sTraceClientFunc
     *  \brief contains the name used for tracing
     */
    string m_sTraceClientFunc;
    /** \var string m_sTraceServerFunc
     *  \brief contains the name used for tracing
     */
    string m_sTraceServerFunc;
    /** \var string m_sTraceMsgBufFunc
     *  \brief contains the name used for tracing
     */
    string m_sTraceMsgBufFunc;
    /** \var string m_sOutputDir
     *  \brief contains the output directory for the files
     */
    string m_sOutputDir;
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

#endif                // __DICE_BE_BECONTEXT_H__
