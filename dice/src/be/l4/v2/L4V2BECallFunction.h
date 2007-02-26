/* Copyright (C) 2001-2003 by
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
#ifndef L4V2BECALLFUNCTION_H
#define L4V2BECALLFUNCTION_H


#include "be/l4/L4BECallFunction.h"

/** \class CL4V2BECallFunction
 *  \ingroup backend
 *  \brief implements L4 version 2 specific aspects of the call function
 **/
class CL4V2BECallFunction : public CL4BECallFunction
{
DECLARE_DYNAMIC(CL4V2BECallFunction);

public:
  CL4V2BECallFunction();
  ~CL4V2BECallFunction();

protected:
    virtual void WriteUnmarshalling(CBEFile * pFile,  int nStartOffset,  bool & bUseConstOffset,  CBEContext * pContext);
    virtual void WriteMarshalling(CBEFile * pFile,  int nStartOffset,  bool & bUseConstOffset,  CBEContext * pContext);
    virtual void WriteInvocation(CBEFile * pFile,  CBEContext * pContext);
    virtual bool UseAsmShortIPC(CBEContext* pContext);
    virtual void WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext);
    virtual void WriteIPC(CBEFile * pFile,  CBEContext * pContext);
    virtual void WriteVariableInitialization(CBEFile * pFile,  CBEContext * pContext);
    virtual void WriteAsmShortIPC(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteAsmLongIPC(CBEFile *pFile, CBEContext *pContext);
    virtual bool UseAsmLongIPC(CBEContext *pContext);
};

#endif
