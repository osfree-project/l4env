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

#ifndef L4X0aBERCVFUNCTION_H
#define L4X0aBERCVFUNCTION_H


#include <be/l4/L4BERcvFunction.h>

/** \class CL4X0aBERcvFunction
 *  \ingroup backend
 *  \brief writes L4 X0 specific implementations of the IPC code
 */
class CL4X0aBERcvFunction : public CL4BERcvFunction
{
DECLARE_DYNAMIC(CL4X0aBERcvFunction);

public:
  CL4X0aBERcvFunction();
  ~CL4X0aBERcvFunction();

protected:
    virtual void WriteIPC(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteAsmLongIPC(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteAsmShortIPC(CBEFile *pFile, CBEContext *pContext);
    virtual bool UseAsmLongIPC(CBEContext *pContext);
    virtual bool UseAsmShortIPC(CBEContext *pContext);
    virtual void WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext);
};

#endif
