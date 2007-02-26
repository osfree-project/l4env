/**
 *  \file    dice/src/be/BEDispatchFunction.h
 *  \brief   contains the declaration of the class CBEDispatchFunction
 *
 *  \date    10/10/2003
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef BEDISPATCHFUNCTION_H
#define BEDISPATCHFUNCTION_H

#include <be/BEInterfaceFunction.h>
#include "template.h"
#include <vector>

class CBESwitchCase;

/** \class CBEDispatchFunction
 *  \ingroup backend
 *  \brief the server loop's dispatch function class for the back-end
 *
 * This class contains the code to write a dispatch function
 */
class CBEDispatchFunction : public CBEInterfaceFunction
{
// Constructor
public:
    /** \brief constructor
     */
    CBEDispatchFunction();
    virtual ~CBEDispatchFunction();

protected:
    /** \brief copy constructor */
    CBEDispatchFunction(CBEDispatchFunction &src);

public:
    virtual bool DoWriteFunction(CBEHeaderFile* pFile);
    virtual bool DoWriteFunction(CBEImplementationFile* pFile);
    virtual void CreateBackEnd(CFEInterface *pFEInterface);

protected:
    virtual void WriteVariableInitialization(CBEFile *pFile);
    virtual void WriteSwitch(CBEFile *pFile);
    virtual void WriteSetWrongOpcodeException(CBEFile* pFile);
    virtual void WriteDefaultCase(CBEFile *pFile);
    virtual void WriteDefaultCaseWithoutDefaultFunc(CBEFile *pFile);
    virtual void WriteDefaultCaseWithDefaultFunc(CBEFile *pFile);
    virtual void WriteFunctionDeclaration(CBEFile * pFile);
    virtual void WriteInvocation(CBEFile *pFile);
    virtual void WriteBody(CBEFile *pFile);
    virtual bool AddSwitchCases(CFEInterface *pFEInterface);
    virtual void AddBeforeParameters();

protected:
    /** \var string m_sDefaultFunction
     *  \brief contains the name of the default function
     */
    string m_sDefaultFunction;

public:
    /** \var CCollection<CBESwitchCase> m_SwitchCases
     *  \brief contains references to the interface's functions
     */
    CCollection<CBESwitchCase> m_SwitchCases;
};

#endif
