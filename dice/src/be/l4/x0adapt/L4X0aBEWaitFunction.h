/**
 *    \file    dice/src/be/l4/x0adapt/L4X0aBEWaitFunction.
 *    \brief   contains the declaration of the class CL4X0aBEWaitFunction
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

#ifndef CL4X0aBEWAITFUNCTION_H
#define CL4X0aBEWAITFUNCTION_H


#include <be/l4/L4BEWaitFunction.h>

/** \class CL4X0aBEWaitFunction
 *  \ingroup backend
 *  \brief implements X0 specific ipc code
 **/
class CL4X0aBEWaitFunction : public CL4BEWaitFunction
{

public:
    /** creates a wait function object */
    CL4X0aBEWaitFunction(bool bOpenWait);
    virtual ~CL4X0aBEWaitFunction();

protected:
    virtual void WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext);
};

#endif
