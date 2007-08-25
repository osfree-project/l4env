/**
 *    \file    dice/src/ProgramOptions.h
 *  \brief   contains the declaration of all program options
 *
 *    \date    04/18/2003
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

#ifndef __DICE_PROGRAMOPTIONS_H__
#define __DICE_PROGRAMOPTIONS_H__

/**
 * \defgroup ProgramOptions Constants to be used for Program Options 
 */
//@{

/** \enum ProgramOption_Type
 *  \brief defines the valid program options
 */
enum ProgramOption_Type
{
    PROGRAM_NONE,               /**< no arguments */
    PROGRAM_GENERATE_TEMPLATE,  /**< generate the server function templates */
    PROGRAM_NO_OPCODES,         /**< generate NO opcode files */
    PROGRAM_GENERATE_INLINE,    /**< generate client stub as inline */
    PROGRAM_GENERATE_INLINE_STATIC, /**< generate client stub as static inline */
    PROGRAM_GENERATE_INLINE_EXTERN, /**< generate client stub as extern inline */
    PROGRAM_INIT_RCVSTRING,     /**< use a user-provided function to init recv-ref-strings */
    PROGRAM_STOP_AFTER_PRE,     /**< the compiler stops processing after the pre-processing */
    PROGRAM_GENERATE_MESSAGE,   /**< generate message passing functions as well */
    PROGRAM_GENERATE_CLIENT,    /**< generate client side code */
    PROGRAM_GENERATE_COMPONENT, /**< generate component side code */
    PROGRAM_USE_CORBA_TYPES,    /**< use CORBA-style type names */
    PROGRAM_FORCE_CORBA_ALLOC,  /**< Force use of CORBA_alloc function (instead of Env->malloc) */
    PROGRAM_FORCE_C_BINDINGS,   /**< Force use or L4 C-bindings (instead of inline assembler) */
    PROGRAM_TRACE_SERVER,       /**< Trace all messages received by the server loop */
    PROGRAM_TRACE_CLIENT,       /**< Trace all messages send to the server and answers received from it */
    PROGRAM_TRACE_MSGBUF,       /**< Trace usage of message buffer */
    PROGRAM_ZERO_MSGBUF,        /**< Zero the message buffer before using it */
    PROGRAM_TRACE_MSGBUF_DWORDS, /**< If this is set, the number of dumped dwords is restricted */
    PROGRAM_NO_SEND_CANCELED_CHECK, /**< If this is set, we do not retry sending an IPC call */
    PROGRAM_CONST_AS_DEFINE,    /**< print const declarators as define statements */
    PROGRAM_KEEP_TMP_FILES,     /**< keep temp files generated during preprocessing */
    PROGRAM_FORCE_ENV_MALLOC,   /**< force usage of env.malloc (overrides -fforce-corba-alloc) */
    PROGRAM_NO_SERVER_LOOP,     /**< do not generate a server loop function */
    PROGRAM_NO_DISPATCHER,      /**< do not generate the dispatcher function */
    PROGRAM_FREE_MEM_AFTER_REPLY, /**< always free memory after the reply */
    PROGRAM_ALIGN_TO_TYPE,      /**< align parameters in message buffer to size of type (or mword) */
    PROGRAM_GENERATE_LINE_DIRECTIVE, /**< generate line diretives from source file */
    PROGRAM_OPTIONS_MAX         /**< the maximum value of program options */
};

/** \enum ProgramFile_Type
 *  \brief defines the valid file options 
 */
enum ProgramFile_Type
{
    PROGRAM_FILE_IDLFILE,      /**< target client C files are one per IDL file */
    PROGRAM_FILE_MODULE,       /**< target client C files are one per module */
    PROGRAM_FILE_INTERFACE,    /**< target client C files are one per interface */
    PROGRAM_FILE_FUNCTION,     /**< target client C files are one per function */
    PROGRAM_FILE_ALL           /**< target client C files is one for all */
};

/** \enum ProgramDepend_Type
 *  \brief defines the different dependency options
 *
 * MAX has to be 1 more than last element to reflect number of possible bits.
 */
enum ProgramDepend_Type
{
    PROGRAM_DEPEND_M,    /**< print includes to stdout and stop */
    PROGRAM_DEPEND_MM,   /**< print includes with "" to stdout and stop */
    PROGRAM_DEPEND_MD,   /**< print includes to .d file and compile */
    PROGRAM_DEPEND_MMD,  /**< print includes with "" to .d file and compile */
    PROGRAM_DEPEND_MF,   /**< specifies the file to generate the dependency output in */
    PROGRAM_DEPEND_MP,   /**< generated phony dependency rules */
    PROGRAM_DEPEND_MAX   /**< max-value for dependency option */
};

/** \enum ProgramVerbose_Type
 *  \brief defines valid verboseness levels
 */
enum ProgramVerbose_Type
{
    PROGRAM_VERBOSE_NONE,    /**< no veboseness */
    PROGRAM_VERBOSE_OPTIONS, /**< only print options */
    PROGRAM_VERBOSE_NORMAL,  /**< normal verboseness level (-v) */
    PROGRAM_VERBOSE_PARSER,  /**< the verboseness level of the parser */
    PROGRAM_VERBOSE_SCANNER, /**< scanner is more verbose than parser */
    PROGRAM_VERBOSE_DEBUG,   /**< debug output */
    PROGRAM_VERBOSE_MAXLEVEL /**< the maximum verbose level */
};

/** \enum ProgramWarning_Type
 *  \brief contains the different possible warning options
 *
 * The Warning options are actually implemented using a bitset template. Thus
 * these items index one specific bit in the bitset.
 */
enum ProgramWarning_Type
{
    PROGRAM_WARNING_IGNORE_DUPLICATE_FID,	/**< ignore duplicated function IDs */
    PROGRAM_WARNING_PREALLOC,			/**< allocating memory for unbound strings */
    PROGRAM_WARNING_NO_MAXSIZE,			/**< warn on missing max-size attributes */
    PROGRAM_WARNING_ALL,			/**< warn on everything */
    PROGRAM_WARNING_MAX				/**< maximum index */
};

/** \enum BackEnd_Interface_Type
 *  \brief defines the options for the backend interface to use
 */
enum BackEnd_Interface_Type
{
    PROGRAM_BE_NONE_I,		/**< nothing set */
    PROGRAM_BE_V2,		/**< defines the back-end for L4 version 2 */
    PROGRAM_BE_V4,		/**< defines the back-end for L4 version X.2 */
    PROGRAM_BE_SOCKETS,		/**< defines the back-end for the Linux Sockets */
    PROGRAM_BE_FIASCO,		/**< defines the back-end for the Fiasco */
    PROGRAM_BE_INTERFACE	/**< max-value for interface values */
};

/** \enum BackEnd_Platform_Type
 *  \brief defines the options for the backend platform to use
 */
enum BackEnd_Platform_Type
{
    PROGRAM_BE_NONE_P,    /**< nothing set */
    PROGRAM_BE_IA32,      /**< defines the back-end for IA32 platform */
    PROGRAM_BE_IA64,      /**< defines the back-end for IA64 platform */
    PROGRAM_BE_ARM,       /**< defines the back-end for ARM platform */
    PROGRAM_BE_AMD64,     /**< defines the back-end for AMD64 platform */
    PROGRAM_BE_PLATFORM   /**< max-value for platform values */
};

/** \enum BackEnd_Language_Type
 *  \brief defines the options for the backend language to use
 */
enum BackEnd_Language_Type
{
    PROGRAM_BE_NONE_L,    /**< nothing set */
    PROGRAM_BE_C,         /**< defines the back-end for C mapping */
    PROGRAM_BE_CPP,       /**< defines the back-end for C++ mapping */
    PROGRAM_BE_LANGUAGE   /**< max-value for language mapping values */
};

//@}

#endif // __DICE_PROGRAMOPTIONS_H__
