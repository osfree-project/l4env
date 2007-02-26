/**
 *	\file	dice/src/be/BEImplementationFile.h
 *	\brief	contains the declaration of the class CBEImplementationFile
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
#ifndef __DICE_BEIMPLEMENTATIONFILE_H__
#define __DICE_BEIMPLEMENTATIONFILE_H__

#include "be/BEFile.h"

class CBEHeaderFile;

class CFEFile;
class CFELibrary;
class CFEInterface;
class CFEOperation;

/**	\class CBEImplementationFile
 *	\ingroup backend
 *	\brief the header file class
 */
class CBEImplementationFile : public CBEFile  
{
DECLARE_DYNAMIC(CBEImplementationFile);
// Constructor
public:
	/**	\brief constructor
	 */
	CBEImplementationFile();
	virtual ~CBEImplementationFile();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CBEImplementationFile(CBEImplementationFile &src);

public:
    virtual void Write(CBEContext *pContext);
    virtual bool CreateBackEnd(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEInterface *pFEInterface, CBEContext *pContext);
    virtual bool CreateBackEnd(CFELibrary *pFELibrary, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEFile *pFEFile, CBEContext *pContext);
    virtual CBEHeaderFile* GetHeaderFile();
    virtual void SetHeaderFile(CBEHeaderFile *pHeaderFile);

protected:  // Protected methods
    virtual void WriteNameSpaces(CBEContext * pContext);
    virtual void WriteClasses(CBEContext * pContext);
    virtual void WriteFunctions(CBEContext * pContext);

protected: // Protected members
	/**	\var CBEHeaderFile *m_pHeaderFile
	 *	\brief reference to the corresponding header file
	 */
	CBEHeaderFile *m_pHeaderFile;
};

#endif // !__DICE_BEIMPLEMENTATIONFILE_H__
