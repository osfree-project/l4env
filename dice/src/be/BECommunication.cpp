/**
 *    \file    dice/src/be/BECommunication.cpp
 * \brief   contains the implementation of the class CBECommunication
 *
 *    \date    08/13/2003
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

#include "BECommunication.h"

CBECommunication::CBECommunication()
 : CBEObject()
{
}

/** clean up this object */
CBECommunication::~CBECommunication()
{
}

/** \brief check if the property is fulfilled for this communication
 *  \param pFunction the function using the communication
 *  \param nProperty the property to check
 *  \param pContext the omnipresent context
 *  \return true if the property if fulfilled
 */
bool CBECommunication::CheckProperty(CBEFunction *pFunction,
    int nProperty,
    CBEContext *pContext)
{
    return false;
}

/** \brief writes a send with an immediate wait
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 *  \param pContext the context of the writing
 */
void CBECommunication::WriteCall(CBEFile *pFile,
    CBEFunction* pFunction,
    CBEContext *pContext)
{
}

/** \brief writes a closed wait
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 *  \param pContext the context of the writing
 */
void CBECommunication::WriteReceive(CBEFile *pFile,
    CBEFunction* pFunction,
    CBEContext *pContext)
{
}

/** \brief writes a send with an immediate wait
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 *  \param pContext the context of the writing
 */
void CBECommunication::WriteReplyAndWait(CBEFile* pFile,
    CBEFunction* pFunction,
    CBEContext* pContext)
{
}

/** \brief writes a wait
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 *  \param pContext the context of the writing
 */
void CBECommunication::WriteWait(CBEFile* pFile,
    CBEFunction *pFunction,
    CBEContext* pContext)
{
}

/** \brief writes a send
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 *  \param pContext the context of the writing
 */
void CBECommunication::WriteSend(CBEFile* pFile,
    CBEFunction* pFunction,
    CBEContext* pContext)
{
}

/** \brief writes a reply
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 *  \param pContext the context of the writing
 */
void CBECommunication::WriteReply(CBEFile* pFile,
    CBEFunction* pFunction,
    CBEContext* pContext)
{
}

/** \brief writes the initialization
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 *  \param pContext the context of the writing
 */
void CBECommunication::WriteInitialization(CBEFile *pFile,
    CBEFunction *pFunction,
    CBEContext *pContext)
{
}

/** \brief writes the assigning of a local name to a communication port
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 *  \param pContext the context of the writing
 */
void CBECommunication::WriteBind(CBEFile *pFile,
    CBEFunction *pFunction,
    CBEContext *pContext)
{
}

/** \brief writes the initialization
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 *  \param pContext the context of the writing
 */
void CBECommunication::WriteCleanup(CBEFile *pFile,
    CBEFunction *pFunction,
    CBEContext *pContext)
{
}
