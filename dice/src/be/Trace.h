/**
 *    \file    dice/src/be/Trace.h
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

/** \class CTrace
 *  \brief interface to tracing libraries
 */
class CTrace
{
public:
    /** contructor of trace object */
    CTrace() {}
    virtual ~CTrace() {}

	/** \brief write tracing code when wrinting default include files
	 *  \param pFile the file to write to
	 */
    virtual void DefaultIncludes(CBEFile& pFile) = 0;
	/** \brief add local variables to function for tracing
	 *  \param pFunction the function calling
	 */
    virtual void AddLocalVariable(CBEFunction *pFunction) = 0;
	/** \brief write tracing code when wrinting variable declaration
	 *  \param pFile the file to write to
	 *  \param pFunction the function writing for
	 */
    virtual void VariableDeclaration(CBEFile& pFile, CBEFunction *pFunction) = 0;
	/** \brief write tracing code before writing the call
	 *  \param pFile the file to write to
	 *  \param pFunction the function writing for
	 */
    virtual void BeforeCall(CBEFile& pFile, CBEFunction *pFunction) = 0;
	/** \brief write tracing code after writing the call
	 *  \param pFile the file to write to
	 *  \param pFunction the function writing for
	 */
    virtual void AfterCall(CBEFile& pFile, CBEFunction *pFunction) = 0;
	/** \brief write tracing code when initializing the server
	 *  \param pFile the file to write to
	 *  \param pFunction the function writing for
	 */
    virtual void InitServer(CBEFile& pFile, CBEFunction *pFunction) = 0;
	/** \brief write tracing code before writing the loop in the server
	 *  \param pFile the file to write to
	 *  \param pFunction the function writing for
	 */
    virtual void BeforeLoop(CBEFile& pFile, CBEFunction *pFunction) = 0;
	/** \brief write tracing code before writing the dipatch function
	 *  \param pFile the file to write to
	 *  \param pFunction the function writing for
	 */
    virtual void BeforeDispatch(CBEFile& pFile, CBEFunction *pFunction) = 0;
	/** \brief write tracing code after writing the dispatch function
	 *  \param pFile the file to write to
	 *  \param pFunction the function writing for
	 */
    virtual void AfterDispatch(CBEFile& pFile, CBEFunction *pFunction) = 0;
	/** \brief write tracing code before writing reply only code
	 *  \param pFile the file to write to
	 *  \param pFunction the function writing for
	 */
    virtual void BeforeReplyOnly(CBEFile& pFile, CBEFunction *pFunction) = 0;
	/** \brief write tracing code after writing reply only code
	 *  \param pFile the file to write to
	 *  \param pFunction the function writing for
	 */
    virtual void AfterReplyOnly(CBEFile& pFile, CBEFunction *pFunction) = 0;
	/** \brief write tracing code before reply and wait function
	 *  \param pFile the file to write to
	 *  \param pFunction the function writing for
	 */
    virtual void BeforeReplyWait(CBEFile& pFile, CBEFunction *pFunction) = 0;
	/** \brief write tracing code after reply and wait
	 *  \param pFile the file to write to
	 *  \param pFunction the function writing for
	 */
    virtual void AfterReplyWait(CBEFile& pFile, CBEFunction *pFunction) = 0;
	/** \brief write tracing code before writing invocation of component function
	 *  \param pFile the file to write to
	 *  \param pFunction the function writing for
	 */
    virtual void BeforeComponent(CBEFile& pFile, CBEFunction *pFunction) = 0;
	/** \brief write tracing code after writing invocation of component function
	 *  \param pFile the file to write to
	 *  \param pFunction the function writing for
	 */
    virtual void AfterComponent(CBEFile& pFile, CBEFunction *pFunction) = 0;
	/** \brief write tracing code before marshalling code
	 *  \param pFile the file to write to
	 *  \param pFunction the function writing for
	 */
    virtual void BeforeMarshalling(CBEFile& pFile, CBEFunction *pFunction) = 0;
	/** \brief write tracing code after marshalling code
	 *  \param pFile the file to write to
	 *  \param pFunction the function writing for
	 */
    virtual void AfterMarshalling(CBEFile& pFile, CBEFunction *pFunction) = 0;
	/** \brief write tracing code before unmarshalling code
	 *  \param pFile the file to write to
	 *  \param pFunction the function writing for
	 */
    virtual void BeforeUnmarshalling(CBEFile& pFile, CBEFunction *pFunction) = 0;
	/** \brief write tracing code after unmarshalling code
	 *  \param pFile the file to write to
	 *  \param pFunction the function writing for
	 */
    virtual void AfterUnmarshalling(CBEFile& pFile, CBEFunction *pFunction) = 0;
	/** \brief write tracing code in communication error code
	 *  \param pFile the file to write to
	 *  \param pFunction the function writing for
	 */
    virtual void WaitCommError(CBEFile& pFile, CBEFunction *pFunction) = 0;
};

#endif /* __DICE_TRACE_H__ */
