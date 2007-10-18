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
class CBEMarshalExceptionFunction;

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

public:
    virtual void Write(CBEFile& pFile);
    virtual void CreateBackEnd(CFEOperation *pFEOperation, bool bComponentSide);

    virtual void SetMessageBufferType();
    virtual void SetCallVariable(std::string sOriginalName, int nStars,
	std::string sCallName);

    virtual bool DoWriteFunction(CBEFile* pFile);

protected:
    virtual void WriteVariableInitialization(CBEFile& pFile, DIRECTION_TYPE nDirection);
    virtual void WriteVariableDeclaration(CBEFile& pFile);
    virtual void WriteCleanup(CBEFile& pFile);
    virtual bool DoWriteVariable(CBETypedDeclarator *pParameter);

    virtual void WriteVariableInitialization(CBEFile& pFile);
    virtual void WriteInvocation(CBEFile& pFile);

	virtual void WriteCaseStart(CBEFile& pFile);
	virtual void WriteCaseEnd(CBEFile& pFile);

	/** \class SetCallVariableCall
	 *  \brief used as functor to set call variables
	 */
	class SetCallVariableCall
	{
		CBEFunction *f;
	public:
		SetCallVariableCall(CBEFunction *ff) : f(ff)
		{ }

		void operator() (CBETypedDeclarator *pParameter);
	};

protected:
    /** \var bool m_bSameClass
     *  \brief true if switch case is from the same class as the dispatcher function
     */
    bool m_bSameClass;
    /** \var std::string m_sOpcode
     *  \brief the opcode constant
     */
    std::string m_sOpcode;
	/** \var std::string m_sUpper;
	 *  \brief if this is a switch-case for a uuid-range, then store upper bound here
	 */
	std::string m_sUpper;
    /** \var CBEUnmarshalFunction *m_pUnmarshalFunction
     *  \brief a reference to the corresponding unmarshal function
     */
    CBEUnmarshalFunction *m_pUnmarshalFunction;
    /** \var CBEMarshalFunction *m_pMarshalFunction
     *  \brief a reference to the corresponding marshal function
     */
    CBEMarshalFunction *m_pMarshalFunction;
    /** \var CBEMarshalExceptionFunction *m_pMarshalExceptionFunction
     *  \brief a reference to the corresponding marshal function for exceptions
     */
    CBEMarshalExceptionFunction *m_pMarshalExceptionFunction;
    /** \var CBEComponentFunction *m_pComponentFunction
     *  \brief a reference to the corresponding component function
     */
    CBEComponentFunction *m_pComponentFunction;
};

#endif // !__DICE_BESWITCHCASE_H__
