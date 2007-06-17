/**
 *  \file    dice/src/be/BEContext.h
 *  \brief   contains the declaration of the class CBEContext
 *
 *  \date    01/10/2002
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
#ifndef __DICE_BE_BECONTEXT_H__
#define __DICE_BE_BECONTEXT_H__

#include "be/BEObject.h"

/** \enum FUNCTION_TYPE
 *  \brief defines the valid function types
 */
enum FUNCTION_TYPE {
    FUNCTION_NONE,
    FUNCTION_SEND,        /**< the send function */
    FUNCTION_RECV,        /**< the receive function */
    FUNCTION_WAIT,        /**< the wait function */
    FUNCTION_UNMARSHAL,   /**< the unmarshal function */
    FUNCTION_MARSHAL,     /**< the marshal function */
    FUNCTION_MARSHAL_EXCEPTION, /**< the marshal function for exceptions */
    FUNCTION_REPLY_RECV,  /**< the reply-and-receive function */
    FUNCTION_REPLY_WAIT,  /**< the reply-and-wait function */
    FUNCTION_CALL,        /**< the call function */
    FUNCTION_TEMPLATE,    /**< the server function template */
    FUNCTION_WAIT_ANY,    /**< the wait any function */
    FUNCTION_RECV_ANY,    /**< the receive any function */
    FUNCTION_SRV_LOOP,    /**< the server loop function */
    FUNCTION_DISPATCH,    /**< the dispatch function */
    FUNCTION_SWITCH_CASE, /**< the switch case statement */
    FUNCTION_REPLY        /**< the reply only function */
};

/** \enum FILE_TYPE
 *  \brief defines the valid file types
 */
enum FILE_TYPE {
    FILETYPE_NONE,                    /**< empty */
    FILETYPE_CLIENTHEADER,            /**< client header file */
    FILETYPE_CLIENTIMPLEMENTATION,    /**< client implementation file */
    FILETYPE_COMPONENTHEADER,         /**< component header file */
    FILETYPE_COMPONENTIMPLEMENTATION, /**< component implementation file */
    FILETYPE_OPCODE,                  /**< opcode file */
    FILETYPE_TEMPLATE,                /**< template/server skeleton file */
    FILETYPE_CLIENT,                  /**< includes header and implementation */
    FILETYPE_COMPONENT,               /**< includes header and implementation */
    FILETYPE_HEADER,                  /**< includes client, server and opcode */
    FILETYPE_IMPLEMENTATION           /**< includes client, server and template */
};

class CBEFile;

/** \class CBEContext
 *  \ingroup backend
 *  \brief The context class of the back-end
 *
 * This class contains information, which makes up the context of the write
 * operation.  E.g. the target file, the class and name factory, and some
 * additional options.
 */
class CBEContext : public CBEObject
{
// Constructor
public:
    /** \brief constructs a back-end context object  */
    CBEContext();
    ~CBEContext();

// Operations
public:
    static void WriteMalloc(CBEFile& pFile, CBEFunction* pFunction);
    static void WriteFree(CBEFile& pFile, CBEFunction* pFunction);
    static void WriteMemory(CBEFile& pFile, CBEFunction *pFunction,
	string sEnv, string sCorba);
};

#endif                // __DICE_BE_BECONTEXT_H__
