/**
 *  \file    dice/src/be/BEHeaderFile.h
 *  \brief   contains the declaration of the class CBEHeaderFile
 *
 *  \date    01/11/2002
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

/** preprocessing symbol to check header file */
#ifndef __DICE_BEHEADERFILE_H__
#define __DICE_BEHEADERFILE_H__

#include "be/BEFile.h"
#include <vector>

class CFEFile;
class CFELibrary;
class CFEInterface;
class CFEOperation;

class CBETypedef;
class CBEType;
class CBEConstant;

class CObject;

/** \class CBEHeaderFile
 *  \ingroup backend
 *  \brief the header file class
 */
class CBEHeaderFile : public CBEFile
{
// Constructor
public:
    /** \brief constructor
     */
    CBEHeaderFile();
    ~CBEHeaderFile();

public:
    virtual void Write();
    virtual void CreateBackEnd(CFEOperation *pFEOperation, FILE_TYPE nFileType);
    virtual void CreateBackEnd(CFEInterface *pFEInterface, FILE_TYPE nFileType);
    virtual void CreateBackEnd(CFELibrary *pFELibrary, FILE_TYPE nFileType);
    virtual void CreateBackEnd(CFEFile *pFEFile, FILE_TYPE nFileType);

    /** \brief tries to match file names
     *  \param sName the name to match
     *  \return true if name matches file name
     */
    bool Match(std::string sName)
    { return GetFileName() == sName; }
    /** \brief returns the file name used in include statements
     *  \return the file name used in include statements
     */
    std::string GetIncludeFileName()
    { return m_sIncludeName; }

protected:
    virtual void WriteTaggedType(CBEType *pType);
    virtual void WriteTypedef(CBETypedef* pTypedef);
    virtual void WriteConstant(CBEConstant *pConstant);
    virtual void WriteNameSpace(CBENameSpace *pNameSpace);
    virtual void WriteClass(CBEClass *pClass);
    virtual void WriteFunction(CBEFunction *pFunction);
    virtual void WriteDefaultIncludes();

    void CreateOrderedElementList();

protected:
    /** \var std::string m_sIncludeName
     *  \brief the file name used in include statements
     */
    std::string m_sIncludeName;

public:
    /** \var CCollection<CBEConstant> m_Constants
     *  \brief contains the constant declarators of the header file
     */
    CCollection<CBEConstant> m_Constants;
    /** \var CSearchableCollection<CBETypedef, std::string> m_Typedefs
     *  \brief contains the type definitions of the header file
     */
    CSearchableCollection<CBETypedef, std::string> m_Typedefs;
    /** \var CSearchableCollection<CBEType, std::string> m_TaggedTypes
     *  \brief contains the tagged types of the header files (types without typedef)
     */
    CSearchableCollection<CBEType, std::string> m_TaggedTypes;
};

#endif // !__DICE_BEHEADERFILE_H__
