/**
 *    \file    dice/src/be/BEExpression.cpp
 *  \brief   contains the implementation of the class CBEExpression
 *
 *    \date    01/17/2002
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

#include "be/BEExpression.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BERoot.h"
#include "be/BETypedef.h"
#include "be/BEConstant.h"
#include "Compiler.h"
#include "fe/FEExpression.h"
#include "fe/FEUserDefinedExpression.h"
#include "fe/FEPrimaryExpression.h"
#include "fe/FEUnaryExpression.h"
#include "fe/FEBinaryExpression.h"
#include "fe/FEConditionalExpression.h"
#include "fe/FESizeOfExpression.h"
#include "fe/FETypeSpec.h"
#include "fe/FEInterface.h"
#include "fe/FELibrary.h"
#include "Error.h"
#include <sstream>
#include <cassert>

CBEExpression::CBEExpression()
{
    m_nType = EXPR_NONE;
    m_nIntValue = 0;
    m_fFloatValue = 0.0;
    m_cCharValue = 0;
    m_sStringValue = string();
    m_nOperator = 0;
    m_pCondition = 0;
    m_pOperand1 = 0;
    m_pOperand2 = 0;
    m_pType = 0;
}

CBEExpression::CBEExpression(CBEExpression & src)
: CBEObject(src)
{
    m_nType = src.m_nType;
    m_nIntValue = src.m_nIntValue;
    m_fFloatValue = src.m_fFloatValue;
    m_cCharValue = src.m_cCharValue;
    m_sStringValue = src.m_sStringValue;
    m_nOperator = src.m_nOperator;
    CLONE_MEM(CBEExpression, m_pOperand1);
    CLONE_MEM(CBEExpression, m_pCondition);
    CLONE_MEM(CBEExpression, m_pOperand2);
    CLONE_MEM(CBEType, m_pType);
}

/** \brief destructor of this instance */
CBEExpression::~CBEExpression()
{
    if (m_pOperand1)
        delete m_pOperand1;
    if (m_pOperand2)
        delete m_pOperand2;
    if (m_pCondition)
        delete m_pCondition;
    if (m_pType)
        delete m_pType;
}

/** \brief creates the back-end representation of an expression
 *  \param pFEExpression the corresponding front-end expression
 *  \return true if code generation was successful
 */
void
CBEExpression::CreateBackEnd(CFEExpression * pFEExpression)
{
    // call CBEObject's CreateBackEnd method
    CBEObject::CreateBackEnd(pFEExpression);

    CBENameFactory *pNF = CCompiler::GetNameFactory();
    m_nType = pFEExpression->GetType();
    switch (m_nType)
    {
    case EXPR_NONE:
    case EXPR_NULL:
    case EXPR_TRUE:
    case EXPR_FALSE:
        break;
    case EXPR_CHAR:
        m_cCharValue = pFEExpression->GetChar();
        break;
    case EXPR_STRING:
        m_sStringValue = pFEExpression->GetString();
        break;
    case EXPR_USER_DEFINED:
        {
            m_sStringValue = 
		((CFEUserDefinedExpression *) pFEExpression)->GetExpName();
            // might be constant -> check if we have const with that name
            CFEInterface *pFEInterface = 
		pFEExpression->GetSpecificParent<CFEInterface>();
            CFEConstDeclarator *pFEConstant = 0;
            if (pFEInterface)
                pFEConstant = pFEInterface->m_Constants.Find(m_sStringValue);
            if (pFEConstant)
                m_sStringValue = pNF->GetConstantName(pFEConstant);
            else
            {
                CFELibrary *pFELibrary = 
		    pFEExpression->GetSpecificParent<CFELibrary>();
                while (pFELibrary)
                {
                    if ((pFEConstant = 
			    pFELibrary->FindConstant(m_sStringValue)) != 0)
                    {
                        m_sStringValue = pNF->GetConstantName(pFEConstant);
                        pFELibrary = 0;
                    }
                    else
                        pFELibrary = 
			    pFELibrary->GetSpecificParent<CFELibrary>();
                }
            }
        }
        break;
    case EXPR_INT:
        m_nIntValue = ((CFEPrimaryExpression *) pFEExpression)->GetIntValue();
        break;
    case EXPR_FLOAT:
        m_fFloatValue = 
	    ((CFEPrimaryExpression *) pFEExpression)->GetFloatValue();
        break;
    case EXPR_CONDITIONAL:
        CreateBackEndConditional((CFEConditionalExpression *)pFEExpression);
        break;
    case EXPR_BINARY:
        CreateBackEndBinary((CFEBinaryExpression *) pFEExpression);
        break;
    case EXPR_UNARY:
        CreateBackEndUnary((CFEUnaryExpression *) pFEExpression);
        break;
    case EXPR_PAREN:
        CreateBackEndPrimary((CFEPrimaryExpression *) pFEExpression);
        break;
    case EXPR_SIZEOF:
        CreateBackEndSizeOf((CFESizeOfExpression *) pFEExpression);
        break;
    }
}

/** \brief creates the back-end expression
 *  \param pFEExpression the corresponding front-end expression
 *  \return true if the code generation was successful
 */
void
CBEExpression::CreateBackEndConditional(
    CFEConditionalExpression *pFEExpression)
{
    string exc = string(__func__);
    
    if (!pFEExpression->GetCondition())
    {
	exc += " failed because no condition.";
	throw new CBECreateException(exc);
    }
    m_pCondition = CCompiler::GetClassFactory()->GetNewExpression();
    m_pCondition->SetParent(this);
    try
    {
	m_pCondition->CreateBackEnd(pFEExpression->GetCondition());
    }
    catch (CBECreateException *e)
    {
        delete m_pCondition;
        m_pCondition = 0;
        throw;
    }
    CreateBackEndBinary(pFEExpression);
}

/** \brief creates the back-end expression
 *  \param pFEExpression the corresponding front-end expression
 *  \return true if the code generation was successful
 */
void
CBEExpression::CreateBackEndBinary(CFEBinaryExpression * pFEExpression)
{
    string exc = string(__func__);
    
    if (!pFEExpression->GetOperand2())
    {
	exc += " failed because no second operand.";
	throw new CBECreateException(exc);
    }
    m_pOperand2 = CCompiler::GetClassFactory()->GetNewExpression();
    m_pOperand2->SetParent(this);
    try
    {
	m_pOperand2->CreateBackEnd(pFEExpression->GetOperand2());
    }
    catch (CBECreateException *e)
    {
        delete m_pOperand2;
        m_pOperand2 = 0;
        throw;
    }
    CreateBackEndUnary(pFEExpression);
}

/** \brief creates the back-end expression
 *  \param pFEExpression the corresponding front-end expression
 *  \return true if the code generation was successful
 */
void
CBEExpression::CreateBackEndUnary(CFEUnaryExpression * pFEExpression)
{
    m_nOperator = pFEExpression->GetOperator();
    CreateBackEndPrimary(pFEExpression);
}

/** \brief creates back end unary expression
 *  \param nOperator specifies the operator fo the unary expression
 *  \param pOperand the operand
 *  \return true if successful
 */
void
CBEExpression::CreateBackEndUnary(int nOperator, 
    CBEExpression *pOperand)
{
    m_nOperator = nOperator;
    m_pOperand1 = pOperand;
    m_pOperand1->SetParent(this);
}

/** \brief creates the back-end expression
 *  \param pFEExpression the corresponding front-end expression
 *  \return true if the code generation was successful
 *
 * This function is only called to create an expression for m_pOperand1:
 */
void
CBEExpression::CreateBackEndPrimary(CFEPrimaryExpression * pFEExpression)
{
    CFEExpression *pFEOperand = pFEExpression->GetOperand();
    if (!pFEOperand)
    {
	string exc = string(__func__);
	exc += " failed because no expression.";
	throw new CBECreateException(exc);
    }
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBEExpression *pExpression = pCF->GetNewExpression();
    pExpression->SetParent(this);
    try
    {
	pExpression->CreateBackEnd(pFEOperand);
    }
    catch (CBECreateException *e)
    {
        delete pExpression;
        throw;
    }
    m_pOperand1 = pExpression;
}

/** \brief creates a primary expression
 *  \param nType the type of the new expression
 *  \param pExpression a source of information
 *  \return true if successful
 */
void
CBEExpression::CreateBackEndPrimary(int nType, CBEExpression *pExpression)
{
    m_nType = nType;
    switch (m_nType)
    {
    case EXPR_NONE:
    case EXPR_NULL:
    case EXPR_TRUE:
    case EXPR_FALSE:
        break;
    case EXPR_CHAR:
        m_cCharValue = pExpression->m_cCharValue;
        break;
    case EXPR_STRING:
    case EXPR_USER_DEFINED:
        m_sStringValue = pExpression->m_sStringValue;
        break;
    case EXPR_INT:
        m_nIntValue = pExpression->m_nIntValue;
        break;
    case EXPR_FLOAT:
        m_fFloatValue = pExpression->m_fFloatValue;
        break;
    case EXPR_CONDITIONAL:
        m_pCondition = pExpression->m_pCondition;
    case EXPR_BINARY:
        m_pOperand2 = pExpression->m_pOperand2;
    case EXPR_UNARY:
        m_pOperand1 = pExpression->m_pOperand1;
        m_nOperator = pExpression->m_nOperator;
        break;
    case EXPR_PAREN:
        m_pOperand1 = pExpression;
        break;
    }
}

/** \brief create the size-of expression
 *  \param pFEExpression the size of expression
 *  \return true if successful
 */
void
CBEExpression::CreateBackEndSizeOf(CFESizeOfExpression *pFEExpression)
{
    // can be type
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CFETypeSpec *pFEType = pFEExpression->GetSizeOfType();
    if (pFEType)
    {
        m_pType = pCF->GetNewType(pFEType->GetType());
        m_pType->SetParent(this);
	try
	{
	    m_pType->CreateBackEnd(pFEType);
	}
	catch (CBECreateException *e)
        {
            delete m_pType;
	    m_pType = 0;
            throw;
        }
        return;
    }
    // can be expression
    CFEExpression *pFESizeOfExpression = pFEExpression->GetSizeOfExpression();
    if (pFESizeOfExpression)
    {
        m_pOperand1 = pCF->GetNewExpression();
        m_pOperand1->SetParent(this);
	try
	{
	    m_pOperand1->CreateBackEnd(pFESizeOfExpression);
	}
	catch (CBECreateException *e)
        {
            delete m_pOperand1;
	    m_pOperand1 = 0;
	    throw;
        }
        return;
    }
    // then it is a string
    m_sStringValue = pFEExpression->GetString();
}

/** \brief write the content of the expression to the target file
 *  \param pFile the target file to write to
 */
void CBEExpression::Write(CBEFile *pFile)
{
    if (!pFile->IsOpen())
        return;

    string sOut;
    WriteToStr(sOut);
    *pFile << sOut;
}

/** \brief write the content of the expression to the string
 *  \param sStr the string to write to
 */
void CBEExpression::WriteToStr(string& sStr)
{
    std::ostringstream os;
    switch (m_nType)
    {
    case EXPR_NONE:
        break;
    case EXPR_NULL:
	sStr += "0";
        break;
    case EXPR_TRUE:
	sStr += "true";
        break;
    case EXPR_FALSE:
	sStr += "false";
        break;
    case EXPR_CHAR:
	os << m_cCharValue;
	sStr += os.str();
        break;
    case EXPR_STRING:
        if (m_sStringValue.empty())
	    sStr += "0";
        else
	    sStr += "\"" + m_sStringValue + "\"";
        break;
    case EXPR_USER_DEFINED:
        if (m_sStringValue.empty())
	    sStr += "0";
        else
	    sStr += m_sStringValue;
        break;
    case EXPR_INT:
	os << m_nIntValue;
	sStr += os.str();
        break;
    case EXPR_FLOAT:
	os << m_fFloatValue;
	sStr += os.str();
        break;
    case EXPR_CONDITIONAL:
        WriteConditionalToStr(sStr);
        break;
    case EXPR_BINARY:
        WriteBinaryToStr(sStr);
        break;
    case EXPR_UNARY:
        WriteUnaryToStr(sStr);
        break;
    case EXPR_PAREN:
	sStr += "(";
        m_pOperand1->WriteToStr(sStr);
	sStr += ")";
        break;
    }
}

/** \brief writes a conditional expression to the string
 *  \param sStr the string to write to
 */
void CBEExpression::WriteConditionalToStr(string &sStr)
{
    sStr += "(";
    m_pCondition->WriteToStr(sStr);
    sStr += ") ? (";
    m_pOperand1->WriteToStr(sStr);
    sStr += ") : (";
    m_pOperand2->WriteToStr(sStr);
    sStr += ")";
}

/** \brief writes a binary expression to the string
 *  \param sStr the string to write to
 */
void CBEExpression::WriteBinaryToStr(string &sStr)
{
    // write first operand
    m_pOperand1->WriteToStr(sStr);
    // write operator
    switch (m_nOperator)
    {
    case EXPR_MUL:
	sStr += " * ";
	break;
    case EXPR_DIV:
	sStr += " / ";
	break;
    case EXPR_MOD:
	sStr += " % ";
	break;
    case EXPR_PLUS:
	sStr += " + ";
	break;
    case EXPR_MINUS:
	sStr += " - ";
	break;
    case EXPR_LSHIFT:
	sStr += " << ";
	break;
    case EXPR_RSHIFT:
	sStr += " >> ";
	break;
    case EXPR_LT:
	sStr += " < ";
	break;
    case EXPR_GT:
	sStr += " > ";
	break;
    case EXPR_LTEQU:
	sStr += " <= ";
	break;
    case EXPR_GTEQU:
	sStr += " >= ";
	break;
    case EXPR_EQUALS:
	sStr += " == ";
	break;
    case EXPR_NOTEQUAL:
	sStr += " != ";
	break;
    case EXPR_BITAND:
	sStr += " & ";
	break;
    case EXPR_BITXOR:
	sStr += " ^ ";
	break;
    case EXPR_BITOR:
	sStr += " | ";
	break;
    case EXPR_LOGAND:
	sStr += " && ";
	break;
    case EXPR_LOGOR:
	sStr += " || ";
	break;
    default:
	throw new error::invalid_operator();
	break;
    }
    // write second operand
    m_pOperand2->WriteToStr(sStr);
}

/** \brief writes a unary expression to the string
 *  \param sStr the string to write to
 */
void CBEExpression::WriteUnaryToStr(string &sStr)
{
    switch (m_nOperator)
    {
    case EXPR_SPLUS:
	sStr += "+";
	break;
    case EXPR_SMINUS:
	sStr += "-";
	break;
    case EXPR_TILDE:
	sStr += "~";
	break;
    case EXPR_EXCLAM:
	sStr += "!";
	break;
    default:
	throw new error::invalid_operator();
	break;
    }
    // write operand
    m_pOperand1->WriteToStr(sStr);
}

/** \brief creates expression with integer value
 *  \param nValue the value of the expression
 *  \return true if successful
 */
void
CBEExpression::CreateBackEnd(int nValue)
{
    m_nType = EXPR_INT;
    m_nIntValue = nValue;
}

/** \brief creates expression with string value
 *  \param sValue the string value
 *  \return true if successful
 */
void
CBEExpression::CreateBackEnd(string sValue)
{
    m_nType = EXPR_USER_DEFINED;
    m_sStringValue = sValue;
}

/** \brief creates binary expression from existing expressions
 *  \param pOperand1 the first operand
 *  \param nOperator the operator
 *  \param pOperand2 the second operand
 *  \return true if successful
 */
void
CBEExpression::CreateBackEndBinary(CBEExpression * pOperand1, 
    int nOperator, 
    CBEExpression * pOperand2)
{
    m_pOperand1 = pOperand1;
    m_nOperator = nOperator;
    m_pOperand2 = pOperand2;
    m_nType = EXPR_BINARY;
}

/** \brief calculates the integer value of this expression
 *  \return the integer value of this expression (or 0 if not evaluable)
 */
int CBEExpression::GetIntValue()
{
    int nValue = 0;

    switch (m_nType)
    {
    case EXPR_NONE:
    case EXPR_NULL:
    case EXPR_STRING:
        break;
    case EXPR_USER_DEFINED:
        // might be constant
        {
            CBERoot *pRoot = GetSpecificParent<CBERoot>();
            assert(pRoot);
            CBEConstant *pConst;
            if ((pConst = pRoot->FindConstant(m_sStringValue)) != 0)
            {
                CBEExpression *pValue = pConst->GetValue();
                nValue = pValue ? pValue->GetIntValue() : 0;
            }
        }
        break;
    case EXPR_TRUE:
        nValue = (int) true;
        break;
    case EXPR_FALSE:
        nValue = (int) false;
        break;
    case EXPR_CHAR:
        nValue = (int) m_cCharValue;
        break;
    case EXPR_INT:
        nValue = m_nIntValue;
        break;
    case EXPR_FLOAT:
        nValue = (int) m_fFloatValue;
        break;
    case EXPR_CONDITIONAL:
        if (m_pCondition->GetBoolValue() == true)
            nValue = m_pOperand1->GetIntValue();
        else
            nValue = m_pOperand2->GetIntValue();
        break;
    case EXPR_BINARY:
        nValue = GetIntValueBinary();
        break;
    case EXPR_UNARY:
        nValue = GetIntValueUnary();
        break;
    case EXPR_PAREN:
        nValue = m_pOperand1->GetIntValue();
        break;
    case EXPR_SIZEOF:
        if (m_pOperand1)
            nValue = m_pOperand1->GetIntValue(); // cause that's what sizeof does
        else if (m_pType)
            nValue = m_pType->GetSize(); // sizeof(type)
        else
        {
            // check for user defined type
            CBERoot *pRoot = GetSpecificParent<CBERoot>();
            assert(pRoot);
            CBETypedef *pUserDefined = pRoot->FindTypedef(m_sStringValue);
            assert(pUserDefined);
            nValue = pUserDefined->GetSize();
        }
        break;
    }

    return nValue;
}

/** \brief calculates the integer value for a binary expression
 *  \return the caculated integer expression
 */
int CBEExpression::GetIntValueBinary()
{
    int nValue1 = m_pOperand1->GetIntValue();
    int nValue2 = m_pOperand2->GetIntValue();
    int nValue = 0;

    switch (m_nOperator)
    {
    case EXPR_MUL:
        nValue = nValue1 * nValue2;
        break;
    case EXPR_DIV:
        if (nValue2)
            nValue = nValue1 / nValue2;
        break;
    case EXPR_MOD:
        if (nValue2)
            nValue = nValue1 % nValue2;
        break;
    case EXPR_PLUS:
        nValue = nValue1 + nValue2;
        break;
    case EXPR_MINUS:
        nValue = nValue1 - nValue2;
        break;
    case EXPR_LSHIFT:
        nValue = nValue1 << nValue2;
        break;
    case EXPR_RSHIFT:
        nValue = nValue1 >> nValue2;
        break;
    case EXPR_LT:
        nValue = (int) (nValue1 < nValue2);
        break;
    case EXPR_GT:
        nValue = (int) (nValue1 > nValue2);
        break;
    case EXPR_LTEQU:
        nValue = (int) (nValue1 <= nValue2);
        break;
    case EXPR_GTEQU:
        nValue = (int) (nValue1 >= nValue2);
        break;
    case EXPR_EQUALS:
        nValue = (int) (nValue1 == nValue2);
        break;
    case EXPR_NOTEQUAL:
        nValue = (int) (nValue1 != nValue2);
        break;
    case EXPR_BITAND:
        nValue = nValue1 & nValue2;
        break;
    case EXPR_BITXOR:
        nValue = nValue1 ^ nValue2;
        break;
    case EXPR_BITOR:
        nValue = nValue1 | nValue2;
        break;
    case EXPR_LOGAND:
        nValue = (int) (nValue1 && nValue2);
        break;
    case EXPR_LOGOR:
        nValue = (int) (nValue1 || nValue2);
        break;
    default:
	throw new error::invalid_operator();
        break;
    }

    return nValue;
}

/** \brief calculates the unary expression
 *  \return the calculated value
 */
int CBEExpression::GetIntValueUnary()
{
    int nValue = m_pOperand1->GetIntValue();

    switch (m_nOperator)
    {
    case EXPR_SPLUS:
        nValue = +nValue;
        break;
    case EXPR_SMINUS:
        nValue = -nValue;
        break;
    case EXPR_TILDE:
        nValue = ~nValue;
        break;
    case EXPR_EXCLAM:
        nValue = !nValue;
        break;
    default:
	throw new error::invalid_operator();
        break;
    }

    return nValue;
}


/** \brief generates the string value of this expression
 *  \return the string value of this expression (empty if not evaluable)
 */
string CBEExpression::GetStringValue()
{
    string value;

    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "type : %d\n", m_nType);
    switch (m_nType)
    {
    case EXPR_NONE:
    case EXPR_NULL:
	break;
    case EXPR_STRING:
	value = m_sStringValue;
        break;
    case EXPR_USER_DEFINED:
        // might be constant
        {
            CBERoot *pRoot = GetSpecificParent<CBERoot>();
            assert(pRoot);
            CBEConstant *pConst;
            if ((pConst = pRoot->FindConstant(m_sStringValue)) != 0)
            {
                CBEExpression *pValue = pConst->GetValue();
		if (pValue)
		    value = pValue->GetStringValue();
            }
        }
        break;
    case EXPR_TRUE:
        value = string("true");
        break;
    case EXPR_FALSE:
        value = string("false");
        break;
    case EXPR_CHAR:
        value = m_cCharValue;
        break;
    case EXPR_INT:
	{
	    std::ostringstream os;
	    os << m_nIntValue;
	    value = os.str();
	}
        break;
    case EXPR_FLOAT:
	{
	    std::ostringstream os;
	    os << m_fFloatValue;
	    value = os.str();
	}
        break;
    case EXPR_CONDITIONAL:
        if (m_pCondition->GetBoolValue() == true)
            value = m_pOperand1->GetStringValue();
        else
            value = m_pOperand2->GetStringValue();
        break;
    case EXPR_BINARY:
        value = GetStringValueBinary();
        break;
    case EXPR_UNARY:
        value = GetStringValueUnary();
        break;
    case EXPR_PAREN:
        value = m_pOperand1->GetStringValue();
        break;
    case EXPR_SIZEOF:
        if (m_pOperand1)
            value = m_pOperand1->GetStringValue(); // cause that's what sizeof does
        break;
    }

    return value;
}

/** \brief calculates the integer value for a binary expression
 *  \return the caculated integer expression
 */
string CBEExpression::GetStringValueBinary()
{
    string value1 = m_pOperand1->GetStringValue();
    string value2 = m_pOperand2->GetStringValue();
    string value;

    switch (m_nOperator)
    {
    case EXPR_MUL:
        value = value1 + string(" * ") + value2;
        break;
    case EXPR_DIV:
        value = value1 + string(" / ") + value2;
        break;
    case EXPR_MOD:
	value = value1 + string(" % ") + value2;
        break;
    case EXPR_PLUS:
        value = value1 + string(" + ") + value2;
        break;
    case EXPR_MINUS:
        value = value1 + string(" - ") + value2;
        break;
    case EXPR_LSHIFT:
        value = value1 + string(" << ") + value2;
        break;
    case EXPR_RSHIFT:
        value = value1 + string(" >> ") + value2;
        break;
    case EXPR_LT:
        value = value1 + string(" < ") + value2;
        break;
    case EXPR_GT:
        value = value1 + string(" > ") + value2;
        break;
    case EXPR_LTEQU:
        value = value1 + string(" <= ") + value2;
        break;
    case EXPR_GTEQU:
        value = value1 + string(" >= ") + value2;
        break;
    case EXPR_EQUALS:
        value = value1 + string(" == ") + value2;
        break;
    case EXPR_NOTEQUAL:
        value = value1 + string(" != ") + value2;
        break;
    case EXPR_BITAND:
        value = value1 + string(" & ") + value2;
        break;
    case EXPR_BITXOR:
        value = value1 + string(" ^ ") + value2;
        break;
    case EXPR_BITOR:
        value = value1 + string(" | ") + value2;
        break;
    case EXPR_LOGAND:
        value = value1 + string(" && ") + value2;
        break;
    case EXPR_LOGOR:
        value = value1 + string(" || ") + value2;
        break;
    default:
	throw new error::invalid_operator();
        break;
    }

    return value;
}

/** \brief calculates the unary expression
 *  \return the calculated value
 */
string CBEExpression::GetStringValueUnary()
{
    string value = m_pOperand1->GetStringValue();

    switch (m_nOperator)
    {
    case EXPR_SPLUS:
        value = string("+") + value;
        break;
    case EXPR_SMINUS:
        value = string("-") + value;
        break;
    case EXPR_TILDE:
        value = string("~") + value;
        break;
    case EXPR_EXCLAM:
        value = string("!") + value;
        break;
    default:
	throw new error::invalid_operator();
        break;
    }

    return value;
}

/** \brief evaluates the expression to a boolean value
 *  \return true or false
 */
bool CBEExpression::GetBoolValue()
{
    bool bValue = false;

    switch (m_nType)
    {
    case EXPR_NONE:
    case EXPR_NULL:
    case EXPR_STRING:
    case EXPR_USER_DEFINED:
    case EXPR_SIZEOF:
        break;
    case EXPR_TRUE:
        bValue = true;
        break;
    case EXPR_FALSE:
        bValue = false;
        break;
    case EXPR_CHAR:
        bValue = (bool) m_cCharValue;
        break;
    case EXPR_INT:
        bValue = (bool) m_nIntValue;
        break;
    case EXPR_FLOAT:
        bValue = (bool) m_fFloatValue;
        break;
    case EXPR_CONDITIONAL:
        if (m_pCondition->GetBoolValue() == true)
            bValue = m_pOperand1->GetBoolValue();
        else
            bValue = m_pOperand2->GetBoolValue();
        break;
    case EXPR_BINARY:
        bValue = GetBoolValueBinary();
        break;
    case EXPR_UNARY:
        bValue = GetBoolValueUnary();
        break;
    case EXPR_PAREN:
        bValue = m_pOperand1->GetBoolValue();
        break;
    }

    return bValue;
}

/** \brief calculates the boolean value for the binary expression
 *  \return true or false
 */
bool CBEExpression::GetBoolValueBinary()
{
    bool bValue1 = m_pOperand1->GetBoolValue();
    bool bValue2 = m_pOperand2->GetBoolValue();
    bool bValue = false;
    
    switch (m_nOperator)
    {
    case EXPR_MUL:
  	bValue = (bool) ((int) bValue1 * (int) bValue2);
    	break;
    case EXPR_DIV:
    	if ((int) bValue2)
      	    bValue = (bool) ((int) bValue1 / (int) bValue2);
	break;
    case EXPR_MOD:
	if ((int) bValue2)
	    bValue = (bool) ((int) bValue1 % (int) bValue2);
	break;
    case EXPR_PLUS:
	bValue = (bool) ((int) bValue1 + (int) bValue2);
	break;
    case EXPR_MINUS:
	bValue = (bool) ((int) bValue1 - (int) bValue2);
	break;
    case EXPR_LSHIFT:
	bValue = (bool) ((int) bValue1 << (int) bValue2);
	break;
    case EXPR_RSHIFT:
	bValue = (bool) ((int) bValue1 >> (int) bValue2);
	break;
    case EXPR_LT:
	bValue = bValue1 < bValue2;
	break;
    case EXPR_GT:
	bValue = bValue1 > bValue2;
	break;
    case EXPR_LTEQU:
	bValue = bValue1 <= bValue2;
	break;
    case EXPR_GTEQU:
	bValue = bValue1 >= bValue2;
	break;
    case EXPR_EQUALS:
	bValue = bValue1 == bValue2;
	break;
    case EXPR_NOTEQUAL:
	bValue = bValue1 != bValue2;
	break;
    case EXPR_BITAND:
	bValue = bValue1 & bValue2;
	break;
    case EXPR_BITXOR:
	bValue = bValue1 ^ bValue2;
	break;
    case EXPR_BITOR:
	bValue = bValue1 | bValue2;
	break;
    case EXPR_LOGAND:
	bValue = bValue1 && bValue2;
	break;
    case EXPR_LOGOR:
	bValue = bValue1 || bValue2;
	break;
    default:
	throw new error::invalid_operator();
	break;
    }
    
    return bValue;
}

/** \brief calculates the boolean value for the unary expression
 *  \return true or false
 */
bool CBEExpression::GetBoolValueUnary()
{
    bool bValue = m_pOperand1->GetBoolValue();
    
    switch (m_nOperator)
    {
    case EXPR_SPLUS:
  	bValue = (bool) (+((int) bValue));
    	break;
    case EXPR_SMINUS:
    	bValue = (bool) (-((int) bValue));
      	break;
    case EXPR_TILDE:
      	{
	    int iValue = (int) bValue;
	    iValue = ~iValue;
	    bValue = (bool) iValue;
	}
	break;
    case EXPR_EXCLAM:
	bValue = !bValue;
	break;
    default:
	throw new error::invalid_operator();
	break;
    }
    
    return bValue;
}

/** \brief tests for a specific front-end type
 *  \param nFEType the front-end type to test for
 *  \return true if this is of the given FE type
 */
bool CBEExpression::IsOfType(int nFEType)
{
    return (m_nType == nFEType);
}

/** \brief creates a new instance of the expression object
 *  \return a reference to the new instance
 */
CObject* CBEExpression::Clone()
{
    return new CBEExpression(*this);
}

