/**
 *	\file	dice/src/be/BEClient.h
 *	\brief	contains the declaration of the class CBEClient
 *
 *	\date	01/11/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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
#ifndef __DICE_BECLIENT_H__
#define __DICE_BECLIENT_H__

#include "be/BETarget.h"

class CBEContext;
class CFEFile;
class CBEImplementationFile;

/** \class CPredefinedFunctionID
 *  \ingroup backend
 *  \brief contains a reference to the function id and the name of the function
 */
class CPredefinedFunctionID : public CObject
{
DECLARE_DYNAMIC(CPredefinedFunctionID);
public:
    /** \brief constructs a new object
     *  \param sName the function's name
     *  \param nNumber the function ID
     */
    CPredefinedFunctionID(String sName, int nNumber)
    {
        m_sName = sName;
        m_nNumber = nNumber;
    }

    /** \brief destroys this object */
    virtual ~CPredefinedFunctionID() { IMPLEMENT_DYNAMIC_BASE(CPredefinedFunctionID, CObject); }

    /** \var String m_sName
     *  \brief the name of the function
     */
    String m_sName;
    /** \var int m_nNumber
     *  \brief the function ID
     */
    int m_nNumber;
};

/**	\class CBEClient
 *	\ingroup backend
 *	\brief the client - a collection of files
 */
class CBEClient : public CBETarget  
{
DECLARE_DYNAMIC(CBEClient);
// Constructor
public:
	/**	\brief constructor
	 */
	CBEClient();
	virtual ~CBEClient();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CBEClient(CBEClient &src);

public:
    virtual void Write(CBEContext *pContext);

protected:
    virtual void SetFileType(CBEContext *pContext, int nHeaderOrImplementation);
    virtual bool CreateBackEndHeader(CFEFile * pFEFile, CBEContext * pContext);
    virtual bool CreateBackEndImplementation(CFEFile * pFEFile, CBEContext * pContext);
    virtual bool CreateBackEndFile(CFEFile *pFEFile, CBEContext *pContext);
    virtual bool CreateBackEndFile(CFEFile * pFEFile, CBEContext * pContext, CBEImplementationFile *pImpl);
    virtual bool CreateBackEndModule(CFELibrary *pFELibrary, CBEContext *pContext);
    virtual bool CreateBackEndModule(CFEFile *pFEFile, CBEContext *pContext);
    virtual bool CreateBackEndInterface(CFEFile *pFEFile, CBEContext *pContext);
    virtual bool CreateBackEndInterface(CFELibrary *pFELibrary, CBEContext *pContext);
    virtual bool CreateBackEndInterface(CFEInterface *pFEInterface, CBEContext *pContext);
    virtual bool CreateBackEndFunction(CFEFile *pFEFile, CBEContext *pContext);
    virtual bool CreateBackEndFunction(CFELibrary *pFELibrary, CBEContext *pContext);
    virtual bool CreateBackEndFunction(CFEInterface *pFEInterface, CBEContext *pContext);
    virtual bool CreateBackEndFunction(CFEOperation *pFEOperation, CBEContext *pContext);
};

#endif // !__DICE_BECLIENT_H__
