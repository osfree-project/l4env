/**
 *    \file    dice/src/be/l4/x0/L4X0BEReplyFunction.h
 *    \brief   contains the declaration of the class CL4X0BEReplyFunction
 *
 *    \date    08/15/2003
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

 /** preprocessor symbol */
#ifndef L4X0BEREPLYFUNCTION_H
#define L4X0BEREPLYFUNCTION_H

#include <be/l4/L4BEReplyFunction.h>

/** \class CL4X0BEReplyFunction
 *  \ingroup backend
 *  \brief encloses the initialisation for X0 backend's reply function
 */
class CL4X0BEReplyFunction : public CL4BEReplyFunction
{

public:
    /** creates a reply function object */
    CL4X0BEReplyFunction();
    virtual ~CL4X0BEReplyFunction();

protected:
    virtual void WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext);
    virtual void WriteUnmarshalling(CBEFile* pFile,  int nStartOffset,  bool& bUseConstOffset,  CBEContext* pContext);
};

#endif
