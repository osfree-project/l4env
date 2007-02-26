/**
 *    \file    dice/src/be/BECPPClass.cpp
 *    \brief   contains the implementation of the class CBECPPClass
 *
 *    \date    Tue Jul 08 2003
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004
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

#include "be/BECPPClass.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"

CBECPPClass::CBECPPClass()
 : CBEClass()
{
}

/** destroys this object */
CBECPPClass::~CBECPPClass()
{
}

/** \brief writes the declaration of the class
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBECPPClass::Write(CBEHeaderFile *pFile, CBEContext *pContext)
{
    const char* sName = GetName().c_str();
    // preamble
    pFile->PrintIndent("class %s;\n", sName);
    pFile->PrintIndent("typedef %s *%s_ptr;\n", sName, sName);
    // pFile->PrintIndent("class %s_var;\n", sName);

    // class header
    pFile->PrintIndent("class %s /*: public virtual Object*/", sName);
    pFile->PrintIndent("{\n");
    // common CORBA functions
    pFile->PrintIndent("public:\n");
    pFile->IncIndent();
    pFile->PrintIndent("typedef %s_ptr _ptr_type;\n", sName);
    //pFile->PrintIndent("typedef %s_var _var_type;\n", sName);
    pFile->PrintIndent("static %s_ptr _duplicate(%s_ptr obj);\n", sName, sName);
    //pFile->PrintIndent("static %s_ptr _narrow(Object_ptr obj);\n", sName);
    pFile->PrintIndent("static %s_ptr _nil();\n", sName);
    // write all constants, typedefs, and functions
    CBEClass::Write(pFile, pContext);

    // protected
    pFile->DecIndent();
    pFile->Print("\n");
    pFile->PrintIndent("protected:\n");
    pFile->IncIndent();
    pFile->PrintIndent("%s();\n", sName);
    pFile->PrintIndent("virtual ~%s();\n", sName);

    // private
    pFile->DecIndent();
    pFile->Print("\n");
    pFile->PrintIndent("private:\n");
    pFile->IncIndent();
    pFile->PrintIndent("%s(const %s&);\n", sName, sName);
    pFile->PrintIndent("void operator=(const %s&);\n", sName);
    pFile->DecIndent();
    pFile->Print("}\n");

    // for CORBA compliance we need the _var and _out class as well
    // WriteClass_var(pFile, pContext);
    // WriteClass_out(pFile, pContext);
}

/** \brief writes the definition of the class
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBECPPClass::Write(CBEImplementationFile *pFile, CBEContext *pContext)
{
    // write implementation for additional functions (constructor, destructor)
    // write implementation for functions
    CBEClass::Write(pFile, pContext);
    // write implementation for _var and _out classes
    //WriteClass_var(pFile, pContext);
    //WriteClass_out(pFile, pContext);
}

/** \brief writes the declaration for the _var class
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBECPPClass::WriteClass_var(CBEHeaderFile *pFile, CBEContext *pContext)
{
    const char* sName = GetName().c_str();
    pFile->Print("class %s_var : public _var\n", sName);
    pFile->Print("{\n");
    pFile->IncIndent();
    pFile->PrintIndent("public:\n");
    pFile->IncIndent();
    pFile->PrintIndent("%s_var() : ptr_(%s::_nil()) {}\n", sName, sName);
    pFile->PrintIndent("%s_var(%s_ptr p) : ptr_(p) {}\n", sName, sName);
    pFile->PrintIndent("%s_var(const %s_var &a) : ptr_(%s::_duplicate(%s_ptr(a))) {}\n", sName, sName, sName, sName);
    pFile->PrintIndent("~%s_var() { free(); }\n", sName);
    pFile->Print("\n");
    pFile->PrintIndent("%s_var &operator=(%s_ptr p) {\n", sName, sName);
    pFile->IncIndent();
    pFile->PrintIndent("reset(p); return *this;\n");
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
    pFile->PrintIndent("%s_var &operator=(const %s_var& a) {\n", sName, sName);
    pFile->IncIndent();
    pFile->PrintIndent("if (this != &a) {\n");
    pFile->IncIndent();
    pFile->PrintIndent("free();\n");
    pFile->PrintIndent("ptr_ = %s::_duplicate(%s_ptr(a));\n", sName, sName);
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
    pFile->PrintIndent("return *this;\n");
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
    pFile->PrintIndent("%s_ptr in() const { return ptr_; }\n", sName);
    pFile->PrintIndent("%s_ptr& inout() { return ptr_; }\n", sName);
    pFile->PrintIndent("%s_ptr& out() {\n", sName);
    pFile->IncIndent();
    pFile->PrintIndent("reset(%s::_nil());\n", sName);
    pFile->PrintIndent("return ptr_;\n");
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
    pFile->PrintIndent("%s_ptr _retn() {\n", sName);
    pFile->IncIndent();
    pFile->PrintIndent("// yield ownership of managed object reference\n");
    pFile->PrintIndent("%s_ptr val = ptr_;\n", sName);
    pFile->PrintIndent("ptr_ = %s::_nil();\n", sName);
    pFile->PrintIndent("return val;\n");
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
    pFile->PrintIndent("operator const %s_ptr&() const { return ptr_; }\n", sName);
    pFile->PrintIndent("operator %a_ptr&() { return ptr_; }\n", sName);
    pFile->PrintIndent("%s_ptr operator->() const { return ptr_; }\n", sName);
    pFile->Print("\n");
    pFile->DecIndent();
    pFile->PrintIndent("protected:\n");
    pFile->IncIndent();
    pFile->PrintIndent("%s_ptr ptr_;\n", sName);
    pFile->PrintIndent("void free() { release(ptr_); }\n");
    pFile->PrintIndent("void reset(%s_ptr p) { free(); ptr_ = p; }\n", sName);
    pFile->Print("\n");
    pFile->DecIndent();
    pFile->PrintIndent("private:\n");
    pFile->IncIndent();
    pFile->PrintIndent("void operator=(const _var&);\n");
    pFile->DecIndent();
    pFile->DecIndent();
    pFile->Print("};\n");
}

/** \brief writes the implementation for the _var class
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBECPPClass::WriteClass_var(CBEImplementationFile *pFile, CBEContext *pContext)
{
}

/** \brief write the declaration for the _out class
 *  \param pFile the file to write to
 *  \param pContext the context of the write
 */
void CBECPPClass::WriteClass_out(CBEHeaderFile *pFile, CBEContext *pContext)
{
    const char* sName = GetName().c_str();
    pFile->PrintIndent("class %s_out\n", sName);
    pFile->PrintIndent("{\n");
    pFile->IncIndent();
    pFile->PrintIndent("public:\n");
    pFile->IncIndent();
    pFile->PrintIndent("%s_out(%s_ptr& p) : ptr_(p) { ptr_ = %s::_nil(); }\n", sName, sName, sName);
    pFile->PrintIndent("%s_out(%s_var& p) : ptr_(p.ptr_) {\n", sName, sName);
    pFile->IncIndent();
    pFile->PrintIndent("release(ptr_); ptr_ = %s::nil();\n", sName);
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
    pFile->PrintIndent("%s_out(%s_out& a) : ptr_(a.ptr_) {}\n", sName, sName);
    pFile->PrintIndent("%s_out& operator=(%s_out& a) {\n", sName, sName);
    pFile->IncIndent();
    pFile->PrintIndent("ptr_ = a.ptr_; return *this;\n");
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
    pFile->PrintIndent("%s_out& operator=(const %s_var& a) {\n", sName, sName);
    pFile->IncIndent();
    pFile->PrintIndent("ptr_ = %s::_duplicate(%s_ptr(a)); return *this; \n", sName, sName);
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
    pFile->PrintIndent("%s_out& operator=(%s_ptr p) { ptr_ = p; return *this; }\n", sName, sName);
    pFile->PrintIndent("operator %s_ptr&() { return ptr_; } \n", sName);
    pFile->PrintIndent("%s_ptr& ptr() { return ptr_; }\n", sName);
    pFile->PrintIndent("%s_ptr operator->() { return ptr_; }\n", sName);
    pFile->Print("\n");
    pFile->DecIndent();
    pFile->PrintIndent("private:\n");
    pFile->IncIndent();
    pFile->PrintIndent("%s_ptr& ptr_;\n", sName);
    pFile->DecIndent();
    pFile->DecIndent();
    pFile->PrintIndent("};\n");
}

/** \brief write the implementation for the _out class
 *  \param pFile the file to write to
 *  \param pContext the context of the write
 */
void CBECPPClass::WriteClass_out(CBEImplementationFile *pFile, CBEContext *pContext)
{
}

