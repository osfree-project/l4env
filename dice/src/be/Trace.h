/**
 *    \file    dice/src/Trace.h
 *    \brief   contains the interface for tracing generated code
 *
 *    \date    05/30/2005
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef __DICE_TRACE_H__
#define __DICE_TRACE_H__

class CBEFile;
class CBEFunction;

class CTrace
{
public:
    /** contructor of trace object */
    CTrace() {}
    virtual ~CTrace() {}
    
    virtual void DefaultIncludes(CBEFile *pFile) = 0;
    virtual void AddLocalVariable(CBEFunction *pFunction) = 0;
    virtual void VariableDeclaration(CBEFile *pFile, CBEFunction *pFunction) = 0;
    virtual void BeforeCall(CBEFile *pFile, CBEFunction *pFunction) = 0;
    virtual void AfterCall(CBEFile *pFile, CBEFunction *pFunction) = 0;
    virtual void InitServer(CBEFile *pFile, CBEFunction *pFunction) = 0;
    virtual void BeforeLoop(CBEFile *pFile, CBEFunction *pFunction) = 0;
    virtual void BeforeDispatch(CBEFile *pFile, CBEFunction *pFunction) = 0;
    virtual void AfterDispatch(CBEFile *pFile, CBEFunction *pFunction) = 0;
    virtual void BeforeReplyOnly(CBEFile *pFile, CBEFunction *pFunction) = 0;
    virtual void AfterReplyOnly(CBEFile *pFile, CBEFunction *pFunction) = 0;
    virtual void BeforeReplyWait(CBEFile *pFile, CBEFunction *pFunction) = 0;
    virtual void AfterReplyWait(CBEFile *pFile, CBEFunction *pFunction) = 0;
    virtual void BeforeComponent(CBEFile *pFile, CBEFunction *pFunction) = 0;
    virtual void AfterComponent(CBEFile *pFile, CBEFunction *pFunction) = 0;
    virtual void BeforeMarshalling(CBEFile *pFile, CBEFunction *pFunction) = 0;
    virtual void AfterMarshalling(CBEFile *pFile, CBEFunction *pFunction) = 0;
    virtual void BeforeUnmarshalling(CBEFile *pFile, CBEFunction *pFunction) = 0;
    virtual void AfterUnmarshalling(CBEFile *pFile, CBEFunction *pFunction) = 0;
    virtual void WaitCommError(CBEFile*, CBEFunction*) = 0;
};

#endif /* __DICE_TRACE_H__ */
