/**
 *  \file    dice/src/be/BESwitchCase.h
 *  \brief   contains the declaration of the class CBESwitchCase
 *
 *  \date    01/29/2002
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
#ifndef __DICE_BESWITCHCASE_H__
#define __DICE_BESWITCHCASE_H__

#include "be/BEOperationFunction.h"

class CBEUnmarshalFunction;
class CBEComponentFunction;
class CBEMarshalFunction;

/** \class CBESwitchCase
 *  \ingroup backend
 *  \brief the function class for the back-end
 *
 * This class contains resembles a back-end function which belongs to a front-end operation
 */
class CBESwitchCase : public CBEOperationFunction
{
// Constructor
public:
    /** \brief constructor
     */
    CBESwitchCase();
    virtual ~CBESwitchCase();

protected:
    /** \brief copy constructor */
    CBESwitchCase(CBESwitchCase &src);

public:
    virtual void Write(CBEFile *pFile);
    virtual void CreateBackEnd(CFEOperation *pFEOperation);

    virtual void SetMessageBufferType();
    virtual void SetCallVariable(string sOriginalName, int nStars, 
	string sCallName);
    
    /** \brief tests if this function should be written
     *  \return true if successful
     */
    virtual bool DoWriteFunction(CBEHeaderFile * /*pFile*/) 
    { return true; }
    /** \brief tests if this function should be written
     *  \return true if successful
     */
    virtual bool DoWriteFunction(CBEImplementationFile * /*pFile*/)
    { return true; }

protected:
    virtual void WriteVariableInitialization(CBEFile *pFile, DIRECTION_TYPE nDirection);
    virtual void WriteVariableDeclaration(CBEFile *pFile);
    virtual void WriteCleanup(CBEFile *pFile);
    virtual bool DoWriteVariable(CBETypedDeclarator *pParameter);

    /** \brief writes the initialization of the variables
     */
    virtual void WriteVariableInitialization(CBEFile * /*pFile*/)
    { }
    /** \brief writes the invocation of the message transfer
     */
    virtual void WriteInvocation(CBEFile * /*pFile*/)
    { }

protected:
    /** \var bool m_bSameClass
     *  \brief true if switch case is from the same class as the dispatcher
     *  function
     */
    bool m_bSameClass;
    /** \var string m_sOpcode
     *  \brief the opcode constant
     */
    string m_sOpcode;
    /** \var CBEUnmarshalFunction *m_pUnmarshalFunction
     *  \brief a reference to the corresponding unmarshal function
     */
    CBEUnmarshalFunction *m_pUnmarshalFunction;
    /** \var CBEMarshalFunction *m_pMarshalFunction
     *  \brief a reference to the corresponding marshal function
     */
    CBEMarshalFunction *m_pMarshalFunction;
    /** \var CBEComponentFunction *m_pComponentFunction
     *  \brief a reference to the corresponding component function
     */
    CBEComponentFunction *m_pComponentFunction;
};

#endif // !__DICE_BESWITCHCASE_H__
