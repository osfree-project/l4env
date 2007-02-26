/**
 *    \file    dice/src/be/BEFunction.h
 *    \brief   contains the declaration of the class CBEFunction
 *
 *    \date    01/11/2002
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

/** preprocessing symbol to check header file */
#ifndef __DICE_BEFUNCTION_H__
#define __DICE_BEFUNCTION_H__

#include "be/BEObject.h"
#include <vector>
using namespace std;

class CBEAttribute;
class CBETypedDeclarator;
class CBEMsgBufferType;
class CBEType;
class CBEException;
class CBEFile;
class CBEHeaderFile;
class CBEImplementationFile;
class CBEContext;
class CBEDeclarator;
class CBEOpcodeType;
class CBEReplyCodeType;
class CBEClass;
class CBETarget;
class CBECommunication;

class CFEInterface;
class CFEOperation;
class CFETypeSpec;

//@{
#define DIRECTION_IN    1    /**< alias for ATTR_IN */
#define DIRECTION_OUT    2    /**< alias for ATTR_OUT */
#define DIRECTION_INOUT    3    /**< alias for neither */
#define DIRECTION_OTHER(x) (3-x) /**< macro to determine alternative direction */
//@}

/**    \class CBEFunction
 *    \ingroup backend
 *    \brief the function class for the back-end
 *
 * This class contains the relevant informations about the target functions. such as name, return type, attributes, parameters,
 * exceptions, etc.
 */
class CBEFunction : public CBEObject
{
// Constructor
public:
    /**    \brief constructor
     */
    CBEFunction();
    virtual ~ CBEFunction();

protected:
    /**    \brief copy constructor */
    CBEFunction(CBEFunction & src);

public:
    virtual CBETypedDeclarator * FindParameter(string sName, bool bCall = false);
    virtual void RemoveParameter(CBETypedDeclarator * pParameter);
    virtual void RemoveException(CBEException * pException);
    virtual void RemoveAttribute(CBEAttribute * pAttribute);
    virtual int GetSize(int nDirection, CBEContext *pContext);
    virtual int GetMaxSize(int nDirection, CBEContext *pContext);
    virtual int GetVariableSizedParameterCount(int nDirection);
    virtual int GetFixedSize(int nDirection, CBEContext *pContext);
    virtual int GetStringParameterCount(int nDirection, int nMustAttrs = 0, int nMustNotAttrs = 0);
    virtual CBEAttribute *FindAttribute(int nAttrType);
    virtual void WriteCall(CBEFile * pFile, string sReturnVar, CBEContext * pContext);
    virtual void Write(CBEImplementationFile * pFile, CBEContext * pContext);
    virtual void Write(CBEHeaderFile * pFile, CBEContext * pContext);
    virtual CBEException *GetNextException(vector<CBEException*>::iterator &iter);
    virtual vector<CBEException*>::iterator GetFirstException();
    virtual void AddException(CBEException * pException);
    virtual CBETypedDeclarator *GetNextParameter(vector<CBETypedDeclarator*>::iterator &iter);
    virtual vector<CBETypedDeclarator*>::iterator GetFirstParameter();
    virtual void AddParameter(CBETypedDeclarator * pParameter);
    virtual CBEAttribute *GetNextAttribute(vector<CBEAttribute*>::iterator &iter);
    virtual vector<CBEAttribute*>::iterator GetFirstAttribute();
    virtual void AddAttribute(CBEAttribute * pAttribute);
    virtual void SetReturnType(CBEType * pReturnType);
    virtual CBEType *GetReturnType();
    virtual string GetName();
    virtual bool DoUnmarshalParameter(CBETypedDeclarator *pParameter, CBEContext *pContext);
    virtual bool DoMarshalParameter(CBETypedDeclarator *pParameter, CBEContext *pContext);
    virtual bool HasAdditionalReference(CBEDeclarator *pDeclarator, CBEContext *pContext, bool bCall = false);
    virtual CBETypedDeclarator* GetNextSortedParameter(vector<CBETypedDeclarator*>::iterator &iter);
    virtual vector<CBETypedDeclarator*>::iterator GetFirstSortedParameter();
    virtual bool IsLastSortedParameter(vector<CBETypedDeclarator*>::iterator iter);
    virtual void RemoveSortedParameter(CBETypedDeclarator * pParameter);
    virtual void AddSortedParameter(CBETypedDeclarator * pParameter);
    virtual CBEClass* GetClass();
    virtual int GetParameterCount(int nFEType, int nDirection);
    virtual int GetParameterCount(int nMustAttrs, int nMustNotAttrs, int nDirection);
    virtual void SetMsgBufferCastOnCall(bool bCastMsgBufferOnCall);
    virtual bool AddToFile(CBEHeaderFile *pHeader, CBEContext *pContext);
    virtual bool AddToFile(CBEImplementationFile *pImpl, CBEContext *pContext);
    virtual bool IsComponentSide();
    virtual void SetComponentSide(bool bComponentSide);
    virtual bool HasVariableSizedParameters(int nDirection = DIRECTION_IN | DIRECTION_OUT);
    virtual bool HasArrayParameters(int nDirection = DIRECTION_IN | DIRECTION_OUT);
    virtual bool DoWriteFunction(CBEHeaderFile *pFile, CBEContext *pContext);
    virtual bool DoWriteFunction(CBEImplementationFile *pFile, CBEContext *pContext);
    virtual CBETypedDeclarator* FindParameterType(string sTypeName);
    virtual CBETypedDeclarator* FindParameterAttribute(int nAttributeType);
    virtual CBETypedDeclarator* FindParameterIsAttribute(int nAttributeType, string sAttributeParameter);
    virtual int GetReceiveDirection();
    virtual int GetSendDirection();
    virtual CBEMsgBufferType* GetMessageBuffer();
    virtual void SetCallVariable(string sOriginalName, int nStars, string sCallName, CBEContext *pContext);
    virtual CBETypedDeclarator* GetReturnVariable();
    virtual CBETypedDeclarator* GetEnvironment();
    virtual CBETypedDeclarator* GetObject();
    virtual CBETypedDeclarator* GetExceptionWord();
    virtual string GetOpcodeConstName();
    virtual void WriteReturn(CBEFile * pFile, CBEContext * pContext);
    virtual int GetParameterAlignment(int nCurrentOffset, int nParamSize, CBEContext *pContext);
    virtual bool SortParameters(int nMode, CBEContext *pContext);

protected:
    virtual void WriteReturnType(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteFunctionDefinition(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteFunctionDeclaration(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteCleanup(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteInvocation(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteBody(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteInlinePrefix(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext);
    virtual void WriteMarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext);
    virtual int WriteUnmarshalReturn(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext);
    virtual int WriteMarshalReturn(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext);
    virtual int WriteMarshalException(CBEFile* pFile, int nStartOffset, bool& bUseConstOffset, CBEContext* pContext);
    virtual int WriteUnmarshalException(CBEFile* pFile,  int nStartOffset,  bool& bUseConstOffset,  CBEContext* pContext);
    virtual void WriteParameterName(CBEFile * pFile, CBEDeclarator * pDeclarator, CBEContext * pContext);
    virtual void WriteParameter(CBEFile * pFile, CBETypedDeclarator * pParameter, CBEContext * pContext, bool bUseConst = true);
    virtual void WriteAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma);
    virtual bool WriteBeforeParameters(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteParameterList(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteCallParameterName(CBEFile * pFile, CBEDeclarator * pInternalDecl, CBEDeclarator * pExternalDecl, CBEContext * pContext);
    virtual void WriteCallParameter(CBEFile * pFile, CBETypedDeclarator * pParameter, CBEContext * pContext);
    virtual void WriteCallAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma);
    virtual bool WriteCallBeforeParameters(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteCallParameterList(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteFunctionAttributes(CBEFile* pFile,  CBEContext* pContext);
    virtual bool AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext);
    virtual bool AddMessageBuffer(CFEOperation *pFEOperation, CBEContext * pContext);
    virtual bool SetReturnVar(bool bUnsigned, int nSize, int nFEType, string sName, CBEContext * pContext);
    virtual bool SetReturnVar(CBEType * pType, string sName, CBEContext * pContext);
    virtual bool SetReturnVar(CFETypeSpec * pFEType, string sName, CBEContext * pContext);
    virtual bool CreateBackEnd(CFEBase* pFEObject, CBEContext *pContext);

    virtual CBETypedDeclarator* GetNextCallParameter(vector<CBETypedDeclarator*>::iterator &iter);
    virtual vector<CBETypedDeclarator*>::iterator GetFirstCallParameter();
    virtual void SetCallVariable(CBETypedDeclarator *pTypedDecl, string sNewDeclName, int nStars, CBEContext *pContext);

    virtual int GetFixedReturnSize(int nDirection, CBEContext *pContext);
    virtual int GetReturnSize(int nDirection, CBEContext *pContext);
    virtual int GetMaxReturnSize(int nDirection, CBEContext *pContext);
    virtual bool DoExchangeParameters(CBETypedDeclarator *pPrecessor, CBETypedDeclarator *pSuccessor, CBEContext *pContext);
    virtual CBETypedDeclarator* GetParameter(CBEDeclarator *pDeclarator, bool bCall);
    virtual void WriteExceptionWordDeclaration(CBEFile* pFile, bool bInit, CBEContext* pContext);
    virtual void WriteExceptionWordInitialization(CBEFile* pFile, CBEContext* pContext);
    virtual void WriteExceptionCheck(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteEnvExceptionFromWord(CBEFile* pFile, CBEContext* pContext);
    virtual string GetExceptionWordInitString();

protected:
    /**  \var bool m_bComponentSide
    *   \brief true if this function is at the client side
    */
    bool m_bComponentSide;
    /**   \var bool m_bCastMsgBufferOnCall
    *    \brief true if the message buffer should be cast to local type on call
    */
    bool m_bCastMsgBufferOnCall;
    /**    \var int m_nParameterIndent
    *    \brief the indent of the function's parameters
    */
    int m_nParameterIndent;
    /**    \var string m_sName
    *    \brief the name of the function
    *
    * This is the name of the function as it appears in the target code.
    */
    string m_sName;
    /**    \var string m_sOpcodeConstName
    *    \brief the name of the functions opcode
    */
    string m_sOpcodeConstName;
    /**    \var CBETypedDeclarator *m_pReturnVar
    *    \brief a reference to the return type.
    */
    CBETypedDeclarator *m_pReturnVar;
    /** \var vector<CBEAttribute*> m_vAttributes
     *  \brief contains the attributes of the function (CBEAttribute)
     */
    vector<CBEAttribute*> m_vAttributes;
    /** \var vector<CBETypedDeclarator*> m_vParameters
     *  \brief contains the parameters of the functuin (CBETypedDeclarator)
     */
    vector<CBETypedDeclarator*> m_vParameters;
    /** \var vector<CBETypedDeclarator*> m_vSortedParameters
     *  \brief a backup copy of the parameters of the function, which contains the sorted parameters
     */
    vector<CBETypedDeclarator*> m_vSortedParameters;
    /** \var vector<CBETypedDeclarator*> m_vCallParameters
    *  \brief a copy of the parameters of the function, which contains the names of the \
    * variables used to call this function
    */
    vector<CBETypedDeclarator*> m_vCallParameters;
    /**    \var vector<CBEException*> m_vExceptions
    *    \brief contains the exceptions the function may throw
    */
    vector<CBEException*> m_vExceptions;
    /** \var CBEClass *m_pClass
    *  \brief a reference to the class of this function
    */
    CBEClass* m_pClass;
    /** \var CBETarget *m_pTarget
    *  \brief a reference to the corresponding target class
    */
    CBETarget *m_pTarget;
    /** \var CBEMsgBufferType *m_pMsgBuffer
     *  \brief reference to function local message buffer
     */
    CBEMsgBufferType *m_pMsgBuffer;
    /** \var CBETypedDeclarator *m_pCorbaObject
    *  \brief contains a reference to the CORBA Object parameter
    */
    CBETypedDeclarator *m_pCorbaObject;
    /** \var CBETypedDeclarator *m_pCorbaEnv
    *  \brief contains a reference to the CORBA Environment parameter
    */
    CBETypedDeclarator *m_pCorbaEnv;
    /** \var CBETypedDeclarator *m_pExceptionWord
     *  \brief contains a reference to the local exception word
     */
    CBETypedDeclarator *m_pExceptionWord;
    /** \var string m_sErrorFunction
    *  \brief contains the name of the error function
    */
    string m_sErrorFunction;
    /** \var CBECommunication *m_pComm
     *  \brief a reference to the communication class of this function
     *
     * Even though there should be one communication class for a backend
     * and thus be stored in the context,
     * we have one for each function.
     */
    CBECommunication *m_pComm;
    /** \var vector<CBETypedDeclarator*> m_vVariables
     *  \brief contains the variables used in this function
     */
    vector<CBETypedDeclarator*> m_vVariables;
};

#endif                // !__DICE_BEFUNCTION_H__
