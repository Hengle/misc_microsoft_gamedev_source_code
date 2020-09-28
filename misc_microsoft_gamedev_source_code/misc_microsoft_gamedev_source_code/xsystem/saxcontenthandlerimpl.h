//==============================================================================
// SAXContentHandlerImpl.h
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

#ifndef _SAXContentHandlerImpl_H_
#define _SAXContentHandlerImpl_H_

#pragma once

#include "xmltypes.h"

class BSAXContentHandlerImpl : public ISAXContentHandler  
{
public:
    BSAXContentHandlerImpl();
    virtual ~BSAXContentHandlerImpl();

   // This must be correctly implemented, if your handler must be a COM Object 
   // For our purposes, these are not implemented
   long __stdcall QueryInterface(const struct _GUID &,void ** );
   unsigned long __stdcall AddRef(void);
   unsigned long __stdcall Release(void);

   virtual HRESULT STDMETHODCALLTYPE putDocumentLocator( 
      /* [in] */ ISAXLocator __RPC_FAR *pLocator);

   virtual HRESULT STDMETHODCALLTYPE startDocument( void);

   virtual HRESULT STDMETHODCALLTYPE endDocument( void);

   virtual HRESULT STDMETHODCALLTYPE startPrefixMapping( 
      /* [in] */ unsigned short __RPC_FAR *pwchPrefix,
      /* [in] */ int cchPrefix,
      /* [in] */ unsigned short __RPC_FAR *pwchUri,
      /* [in] */ int cchUri);

   virtual HRESULT STDMETHODCALLTYPE endPrefixMapping( 
      /* [in] */ unsigned short __RPC_FAR *pwchPrefix,
      /* [in] */ int cchPrefix);

   virtual HRESULT STDMETHODCALLTYPE startElement( 
      /* [in] */ unsigned short __RPC_FAR *pwchNamespaceUri,
      /* [in] */ int cchNamespaceUri,
      /* [in] */ unsigned short __RPC_FAR *pwchLocalName,
      /* [in] */ int cchLocalName,
      /* [in] */ unsigned short __RPC_FAR *pwchRawName,
      /* [in] */ int cchRawName,
      /* [in] */ ISAXAttributes __RPC_FAR *pAttributes);

   virtual HRESULT STDMETHODCALLTYPE endElement( 
      /* [in] */ unsigned short __RPC_FAR *pwchNamespaceUri,
      /* [in] */ int cchNamespaceUri,
      /* [in] */ unsigned short __RPC_FAR *pwchLocalName,
      /* [in] */ int cchLocalName,
      /* [in] */ unsigned short __RPC_FAR *pwchRawName,
      /* [in] */ int cchRawName);

   virtual HRESULT STDMETHODCALLTYPE characters( 
      /* [in] */ unsigned short __RPC_FAR *pwchChars,
      /* [in] */ int cchChars);

   virtual HRESULT STDMETHODCALLTYPE ignorableWhitespace( 
      /* [in] */ unsigned short __RPC_FAR *pwchChars,
      /* [in] */ int cchChars);

   virtual HRESULT STDMETHODCALLTYPE processingInstruction( 
      /* [in] */ unsigned short __RPC_FAR *pwchTarget,
      /* [in] */ int cchTarget,
      /* [in] */ unsigned short __RPC_FAR *pwchData,
      /* [in] */ int cchData);

   virtual HRESULT STDMETHODCALLTYPE skippedEntity( 
      /* [in] */ unsigned short __RPC_FAR *pwchName,
      /* [in] */ int cchName);

};

#endif // !defined _BSAXContentHandlerImpl_H_
