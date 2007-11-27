/**
 *  \file    dice/src/be/FunctionType.h
 *  \brief   contains the declaration of the enum FUNCTION_TYPE
 *
 *  \date    11/07/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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
#ifndef __DICE_BE_FUNCTIONTYPE_H__
#define __DICE_BE_FUNCTIONTYPE_H__

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

#endif /* __DICE_BE_FUNCTIONTYPE_H__ */
