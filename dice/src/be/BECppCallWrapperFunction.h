/**
 *  \file    dice/src/be/BECppCallWrapperFunction.h
 *  \brief   contains the declaration of the class CBECppCallWrapperFunction
 *
 *  \date    04/23/2007
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
#ifndef __DICE_BECPPCALLWRAPPERFUNCTION_H__
#define __DICE_BECPPCALLWRAPPERFUNCTION_H__

#include "be/BECallFunction.h"

/** \class CBECppCallWrapperFunction
 *  \ingroup backend
 *  \brief the call function wrapper for C++ classes
 */
class CBECppCallWrapperFunction : public CBECallFunction
{
// Constructor
public:
    /** \brief constructor
     */
    CBECppCallWrapperFunction();
    virtual ~CBECppCallWrapperFunction();

protected:
    /** \brief copy constructor */
    CBECppCallWrapperFunction(CBECppCallWrapperFunction &src);

    virtual void WriteBody(CBEFile * pFile);

public:
    virtual void CreateBackEnd(CFEOperation *pFEOperation, int nSkipParameter);
    virtual bool DoWriteParameter(CBETypedDeclarator *pParam);

protected:
    /** \var int m_nSkipParameter
     *  \brief bitmap indication whether to skip CORBA Object or Env
     */
    int m_nSkipParameter;
};

#endif // !__DICE_BECPPCALLWRAPPERFUNCTION_H__
