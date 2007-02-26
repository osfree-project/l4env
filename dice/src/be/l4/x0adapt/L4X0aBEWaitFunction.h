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

#ifndef CL4X0aBEWAITFUNCTION_H
#define CL4X0aBEWAITFUNCTION_H


#include <be/l4/L4BEWaitFunction.h>

/** \class CL4X0aBEWaitFunction
 *  \ingroup backend
 *  \brief implements X0 specific ipc code
 **/
class CL4X0aBEWaitFunction : public CL4BEWaitFunction
{
DECLARE_DYNAMIC(CL4X0aBEWaitFunction);

public:
  CL4X0aBEWaitFunction();
  ~CL4X0aBEWaitFunction();

protected:
    virtual void WriteAsmLongIPC(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteAsmShortIPC(CBEFile *pFile, CBEContext *pContext);
    virtual bool UseAsmLongIPC(CBEContext *pContext);
    virtual bool UseAsmShortIPC(CBEContext *pContext);
    virtual void WriteIPC(CBEFile * pFile,  CBEContext * pContext);
    virtual void WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext);
};

#endif
