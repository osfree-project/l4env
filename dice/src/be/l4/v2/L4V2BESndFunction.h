/**
 *    \file    dice/src/be/l4/v2/L4V2BESndFunction.h
 *    \brief   contains the decalration of the class CL4V2BESndFunction
 *
 *    \date    06/01/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004
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

#ifndef L4V2BESNDFUNCTION_H
#define L4V2BESNDFUNCTION_H


#include <be/l4/L4BESndFunction.h>

/** \class CL4V2BESndFunction
 *  \brief implements L4 V2 specifc elements
 */
class CL4V2BESndFunction : public CL4BESndFunction
{
public:
    /** constructs a new send function */
    CL4V2BESndFunction();
    virtual ~CL4V2BESndFunction();

protected:
    virtual void WriteMarshalling(CBEFile* pFile,  int nStartOffset,  bool& bUseConstOffset,  CBEContext* pContext);
    virtual void WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext);
};

#endif
