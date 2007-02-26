/**
 *    \file    dice/src/ProgramOptions.h
 *    \brief   contains the declaration of all program options
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

/** defines the number of option groups */
#define PROGRAM_OPTION_GROUPS        4
/** calculates the number of bits necessary to enumberate the option groups */
#define PROGRAM_OPTION_GROUP_BITS    (PROGRAM_OPTION_GROUPS / 2)
/** calculates the number of bits remaining for the options themselves */
#define PROGRAM_OPTION_OPTION_BITS    (sizeof(unsigned int)*8 - PROGRAM_OPTION_GROUP_BITS)

/** defines a bit mask for the option group number */
#define PROGRAM_OPTION_GROUP_MASK  0xc0000000
/** defines a bit mask for the option */
#define PROGRAM_OPTION_OPTION_MASK 0x3fffffff

/** calculates the option group number from a raw option */
#define PROGRAM_OPTION_GROUP_INDEX(raw) \
    ((raw & PROGRAM_OPTION_GROUP_MASK) >> PROGRAM_OPTION_OPTION_BITS)
/** calculates the option number from a raw option */
#define PROGRAM_OPTION_OPTION(raw) \
    (raw & PROGRAM_OPTION_OPTION_MASK)

/** \union ProgramOptionType
 *  \ingroup frontend
 *  \brief represents the options set for the program
 *
 *  To be able to use lot's of options using flags (e.g. bits) without
 *  a 64bit type, such as long long, I came up with this grouped option
 *  type. An option is split into a group identifier and a bit-mask within
 *  the group. That way I can support PROGRAM_OPTION_GROUPS * .._OPTION_BITS
 *  options: currently 112 (with 4 groups). If they are not sufficient,
 *  increasing the groups to 8 (1 more bit), results in 216 options.
 */
typedef union
{
    /** \var unsigned int raw
     *  \brief the raw value of the option
     */
    unsigned int raw;
    /** \struct ProgramOptionBitmask
     *  \brief bitmask member of the option type
     */
    /** \var struct ProgramOptionBitmask _s
     *  \brief bitmask member of the option type
     */
    struct ProgramOptionBitmask {
    /** \var unsigned int group
     *  \brief the option group
     */
        unsigned int group  : PROGRAM_OPTION_GROUP_BITS;
    /** \var unsigned int option
     *  \brief the option number
     */
        unsigned int option : PROGRAM_OPTION_OPTION_BITS;
    } _s;
} ProgramOptionType;

/** helper macro to declare a program option */
#define DECLARE_PROGRAM_OPTION(name, group, option) \
    name ((group << PROGRAM_OPTION_OPTION_BITS) | (option & PROGRAM_OPTION_OPTION_MASK))

//@{
/** program argument options */
#define PROGRAM_NONE                      0x00000000        /**< no arguments */

/** the following options are in 'Group 0' */
#define PROGRAM_VERBOSE                (0x00000001)    /**< print verbose status info */
#define PROGRAM_GENERATE_TEMPLATE      (0x00000002)    /**< generate the server function templates */
#define PROGRAM_NO_OPCODES             (0x00000004)  /**< generate NO opcode files */
#define PROGRAM_GENERATE_INLINE        (0x00000008)  /**< generate client stub as inline */
#define PROGRAM_GENERATE_INLINE_STATIC (0x00000010)  /**< generate client stub as static inline */
#define PROGRAM_GENERATE_INLINE_EXTERN (0x00000020)    /**< generate client stub as extern inline */
#define PROGRAM_SERVER_PARAMETER       (0x00000040)  /**< generate server with wait timeout */
#define PROGRAM_INIT_RCVSTRING         (0x00000080)  /**< use a user-provided function to init recv-ref-strings */

#define PROGRAM_FILE_IDLFILE           (0x00000100)  /**< target client C files are one per IDL file */
#define PROGRAM_FILE_MODULE            (0x00000200)    /**< target client C files are one per module */
#define PROGRAM_FILE_INTERFACE         (0x00000400)  /**< target client C files are one per interface */
#define PROGRAM_FILE_FUNCTION          (0x00000800)  /**< target client C files are one per function */
#define PROGRAM_FILE_ALL               (0x00001000)  /**< target client C files is one for all */
#define PROGRAM_FILE_MASK              (0x00001f00)     /**< the target client C file arguments */

#define PROGRAM_STOP_AFTER_PRE         (0x00002000)  /**< the compiler stops processing after the pre-processing */

#define PROGRAM_GENERATE_TESTSUITE_THREAD (0x00004000) /**< generate testsuite for stubs with server in thread */
#define PROGRAM_GENERATE_TESTSUITE_TASK (0x00008000)  /**< generate testsuite for stubs with server in task */
#define PROGRAM_GENERATE_TESTSUITE     (0x0000c000)  /**< general test for testsuite */
#define PROGRAM_GENERATE_MESSAGE       (0x00010000)  /**< generate message passing functions as well */
#define PROGRAM_GENERATE_CLIENT        (0x00020000)  /**< generate client side code */
#define PROGRAM_GENERATE_COMPONENT     (0x00040000)  /**< generate component side code */

#define PROGRAM_DEPEND_M               (0x00100000)  /**< print includes to stdout and stop */
#define PROGRAM_DEPEND_MM              (0x00200000)  /**< print includes with "" to stdout and stop */
#define PROGRAM_DEPEND_MD              (0x00400000)  /**< print includes to .d file and compile */
#define PROGRAM_DEPEND_MMD             (0x00800000)  /**< print includes with "" to .d file and compile */
#define PROGRAM_DEPEND_MASK            (0x00f00000)  /**< the dependency mask */

#define PROGRAM_DYNAMIC_INLINE         (0x01000000)  /**< set and unset dynamically during runtime */
#define PROGRAM_USE_CTYPES             (0x02000000)  /**< use C-style type names, instead of CORBA-style type names */
#define PROGRAM_USE_L4TYPES            (0x04000000)  /**< use L4-style type names where appropriate, otherwise use C style */
#define PROGRAM_USE_ALLTYPES           (PROGRAM_USE_CTYPES | PROGRAM_USE_L4TYPES) /**< test for C or L4 types */
#define PROGRAM_FORCE_CORBA_ALLOC      (0x08000000)  /**< Force use of CORBA_alloc function (instead of Env->malloc) */

/** the following options are in 'Group 1' */
#define PROGRAM_FORCE_C_BINDINGS       (0x40000001)     /**< Force use or L4 C-bindings (instead of inline assembler) */
#define PROGRAM_TRACE_SERVER           (0x40000002)    /**< Trace all messages received by the server loop */
#define PROGRAM_TRACE_CLIENT           (0x40000004)    /**< Trace all messages send to the server and answers received from it */
#define PROGRAM_TRACE_MSGBUF           (0x40000008)    /**< Trace usage of message buffer */

#define PROGRAM_USE_SYMBOLS            (0x40000010)    /**< If this option is set, then -D options are regarded */
#define PROGRAM_ZERO_MSGBUF            (0x40000020)    /**< Zero the message buffer before using it */
#define PROGRAM_TRACE_MSGBUF_DWORDS    (0x40000040)    /**< If this is set, the number of dumped dwords is restricted */
#define PROGRAM_NO_SEND_CANCELED_CHECK (0x40000080)    /**< If this is set, we do not retry sending an IPC call */

#define PROGRAM_TESTSUITE_NO_SUCCESS_MESSAGE (0x40000100) /**< The testsuite will only print error messages */
#define PROGRAM_TESTSUITE_SHUTDOWN_FIASCO (0x40000200) /**< The testsuite will shut down fiasco after running the testsuite */
#define PROGRAM_CONST_AS_DEFINE        (0x40000400)    /**< print const declarators as define statements */
#define PROGRAM_STOP_AFTER_PRE_XML     (0x40000800)    /**< stop after preprocessing and dump XML file */

#define PROGRAM_KEEP_TMP_FILES         (0x40001000)    /**< keep temporary files generated during preprocessing */
#define PROGRAM_FORCE_ENV_MALLOC       (0x40002000)    /**< force usage of env.malloc (overrides -fforce-corba-alloc) */
#define PROGRAM_NO_SERVER_LOOP         (0x40004000)    /**< do not generate a server loop function */
#define PROGRAM_NO_DISPATCHER          (0x40008000)    /**< do not generate the dispatcher function */

#define PROGRAM_FREE_MEM_AFTER_REPLY   (0x40010000)    /**< always free memory after the reply */
#define PROGRAM_ALIGN_TO_TYPE          (0x40020000)    /**< align parameters in message buffer to size of type (or mword) */

#define PROGRAM_GENERATE_LINE_DIRECTIVE (0x40040000)   /**< generate line diretives from source file */
//@}

//@{
/** Optimization Options */
#define PROGRAM_OPTIMIZE_MINLEVEL    0    /**< the minimum supported optimization level */
#define PROGRAM_OPTIMIZE_MAXLEVEL    2    /**< the maximum supported optimization level */
//@}

//@{
/** verbose output options */
#define PROGRAM_VERBOSE_PARSER             0x00000001        /**< makes the parser generate verbose output */
#define PROGRAM_VERBOSE_CLASSFACTORY       0x00000002        /**< makes the class-factory generate verbose output */
#define PROGRAM_VERBOSE_NAMEFACTORY        0x00000004        /**< makes the name-factory generate verbose output */
#define PROGRAM_VERBOSE_FACTORIES          0x00000006        /**< makes all factories generate verbose output */
#define PROGRAM_VERBOSE_FRONTEND           0x00000008        /**< makes the front-end g.v.o. */
#define PROGRAM_VERBOSE_DATAREP            0x00000010        /**< makes the data-representation g.v.o. */
#define PROGRAM_VERBOSE_BACKEND            0x00000020        /**< makes the back-end g.v.o. */

#define PROGRAM_VERBOSE_ALL                0x0000003f        /**< generates all verbose output */
#define PROGRAM_VERBOSE_MODULES            0x00000038        /**< generates verbose output for all "ends" */
#define PROGRAM_VERBOSE_LEVEL_1            PROGRAM_VERBOSE_BACKEND    /**< level 1 */
#define PROGRAM_VERBOSE_LEVEL_2            PROGRAM_VERBOSE_BACKEND | PROGRAM_VERBOSE_DATAREP    /** level 2 */
#define PROGRAM_VERBOSE_LEVEL_3            PROGRAM_VERBOSE_MODULES    /**< level 3 */
#define PROGRAM_VERBOSE_LEVEL_4            PROGRAM_VERBOSE_FACTORIES /**< level 4 */
#define PROGRAM_VERBOSE_LEVEL_5            PROGRAM_VERBOSE_MODULES | PROGRAM_VERBOSE_FACTORIES    /**< level 5 */
#define PROGRAM_VERBOSE_LEVEL_6            PROGRAM_VERBOSE_MODULES | PROGRAM_VERBOSE_FACTORIES | PROGRAM_VERBOSE_PARSER /**< level 6 */
#define PROGRAM_VERBOSE_LEVEL_7            PROGRAM_VERBOSE_PARSER    /**< level 7 */

#define PROGRAM_VERBOSE_MAXLEVEL        7            /**< the maximum verbose level */
//@}

//@{
/** warning level options */
#define PROGRAM_WARNING_IGNORE_DUPLICATE_FID   0x00000001 /**< ignore duplicated function IDs */
#define PROGRAM_WARNING_PREALLOC               0x00000002 /**< warn on allocating memory for unbound strings */
#define PROGRAM_WARNING_NO_MAXSIZE             0x00000004 /**< warn on missing max-size attributes */
#define PROGRAM_WARNING_ALL                    0xffffffff /**< warn on everything */
//@}

//@{
/** back-end options */
#define PROGRAM_BE_V2               0x0001        /**< defines the back-end for L4 version 2 */
#define PROGRAM_BE_X0               0x0002        /**< defines the back-end for L4 version X.0 */
#define PROGRAM_BE_X0ADAPT          0x0004      /**< defines the back-end for adaption version of V2 user land to X0 kernel */
#define PROGRAM_BE_FLICK            0x0008        /**< defines the back-end for Flick compatibility */
#define PROGRAM_BE_V4               0x0010        /**< defines the back-end for L4 version X.2 */
#define PROGRAM_BE_SOCKETS          0x0020      /**< defines the back-end for the Linux Sockets */
#define PROGRAM_BE_CDR              0x0040      /**< defines the back-end for the Common Data Representation (CORBA) */
#define PROGRAM_BE_INTERFACE        0x007f        /**< bit-mask for interface */

#define PROGRAM_BE_IA32             0x0100        /**< defines the back-end for IA32 platform */
#define PROGRAM_BE_IA64             0x0200        /**< defines the back-end for IA64 platform */
#define PROGRAM_BE_ARM              0x0400        /**< defines the back-end for ARM platform */
#define PROGRAM_BE_AMD64            0x0800        /**< defines the back-end for AMD64 platform */
#define PROGRAM_BE_PLATFORM         0x0f00        /**< bit-mask for platform */

#define PROGRAM_BE_C                0x1000        /**< defines the back-end for C mapping */
#define PROGRAM_BE_CPP              0x2000        /**< defines the back-end for C++ mapping */
#define PROGRAM_BE_LANGUAGE         0xf000        /**< bit-mask for language mapping */
//@}

#endif // __DICE_PROGRAMOPTIONS_H__
