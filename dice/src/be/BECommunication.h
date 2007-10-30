/**
 *  \file    dice/src/be/BECommunication.h
 *  \brief   contains the declaration of the class CBECommunication
 *
 *  \date    08/13/2003
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

#ifndef BECOMMUNICATION_H
#define BECOMMUNICATION_H

#include <be/BEObject.h>

class CBEFunction;

/** \class CBECommunication
 *  \ingroup backend
 *  \brief is the base class for the communication classes
 */
class CBECommunication : public CBEObject
{


public:
    /** creates a new object of this class */
    CBECommunication();
    ~CBECommunication();

public:
    /** \brief write the call implementation
     *  \param pFile the file to write to
     *  \param pFunction the function to write for
     */
    virtual void WriteCall(CBEFile& pFile, CBEFunction* pFunction) = 0;
    /** \brief write the receive implementation
     *  \param pFile the file to write to
     *  \param pFunction the function to write for
     */
    virtual void WriteReceive(CBEFile& pFile, CBEFunction* pFunction) = 0;
    /** \brief write the reply and receive implementation
     *  \param pFile the file to write to
     *  \param pFunction the function to write for
     */
    virtual void WriteReplyAndWait(CBEFile& pFile, CBEFunction* pFunction) = 0;
    /** \brief write the wait implementation
     *  \param pFile the file to write to
     *  \param pFunction the function to write for
     */
    virtual void WriteWait(CBEFile& pFile, CBEFunction *pFunction) = 0;
    /** \brief write the send implementation
     *  \param pFile the file to write to
     *  \param pFunction the function to write for
     */
    virtual void WriteSend(CBEFile& pFile, CBEFunction* pFunction) = 0;
    /** \brief write the reply implementation
     *  \param pFile the file to write to
     *  \param pFunction the function to write for
     */
    virtual void WriteReply(CBEFile& pFile, CBEFunction* pFunction) = 0;
    /** \brief write the initialization code for the communication
     *  \param pFile the file to write to
     *  \param pFunction the function to write for
     */
    virtual void WriteInitialization(CBEFile& pFile,
	CBEFunction *pFunction) = 0;
    /** \brief write the binding code of the application to the communication
     *         socket
     *  \param pFile the file to write to
     *  \param pFunction the function to write for
     */
    virtual void WriteBind(CBEFile& pFile, CBEFunction *pFunction) = 0;
    /** \brief write the clean up code for the communication
     *  \param pFile the file to write to
     *  \param pFunction the function to write for
     */
    virtual void WriteCleanup(CBEFile& pFile, CBEFunction *pFunction) = 0;

    virtual void AddLocalVariable(CBEFunction *pFunction);
};

#endif
