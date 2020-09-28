//==============================================================================
// Ensemble Studios 
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

#include "xsystem.h"
#include "SAXContentHandlerImpl.h"


//==============================================================================
// Construction/Destruction  
//==============================================================================


//==============================================================================
// BSAXContentHandlerImpl
//==============================================================================
BSAXContentHandlerImpl::BSAXContentHandlerImpl()
{
}


//==============================================================================
// BSAXContentHandlerImpl::~BSAXContentHandlerImpl
//==============================================================================
BSAXContentHandlerImpl::~BSAXContentHandlerImpl()
{
}


//==============================================================================
// BSAXContentHandlerImpl::putDocumentLocator
//==============================================================================
HRESULT STDMETHODCALLTYPE BSAXContentHandlerImpl::putDocumentLocator(ISAXLocator __RPC_FAR * /*pLocator*/)
{
    return S_OK;
}
  
//==============================================================================
// BSAXContentHandlerImpl::startDocument
//==============================================================================      
HRESULT STDMETHODCALLTYPE BSAXContentHandlerImpl::startDocument()
{
    return S_OK;
}
        

//==============================================================================
// BSAXContentHandlerImpl::endDocument
//==============================================================================       
HRESULT STDMETHODCALLTYPE BSAXContentHandlerImpl::endDocument( void)
{
    return S_OK;
}
        
//==============================================================================
// BSAXContentHandlerImpl::startPrefixMapping
//==============================================================================        
HRESULT STDMETHODCALLTYPE BSAXContentHandlerImpl::startPrefixMapping( unsigned short __RPC_FAR * /*pwchPrefix*/, int /*cchPrefix*/, unsigned short __RPC_FAR * /*pwchUri*/, int /*cchUri*/)
{
    return S_OK;
}
        
//==============================================================================
// BSAXContentHandlerImpl::endPrefixMapping
//==============================================================================        
HRESULT STDMETHODCALLTYPE BSAXContentHandlerImpl::endPrefixMapping(unsigned short __RPC_FAR * /*pwchPrefix*/, int /*cchPrefix*/)
{
    return S_OK;
}
        

//==============================================================================
// BSAXContentHandlerImpl::startElement
//==============================================================================       
HRESULT STDMETHODCALLTYPE BSAXContentHandlerImpl::startElement( 
	unsigned short __RPC_FAR * /*pwchNamespaceUri*/,
	int /*cchNamespaceUri*/,
	unsigned short __RPC_FAR * /*pwchLocalName*/,
	int /*cchLocalName*/,
	unsigned short __RPC_FAR * /*pwchRawName*/,
	int /*cchRawName*/,
	ISAXAttributes __RPC_FAR * /*pAttributes*/)
{
    return S_OK;
}
        

//==============================================================================
// BSAXContentHandlerImpl::endElement
//==============================================================================       
HRESULT STDMETHODCALLTYPE BSAXContentHandlerImpl::endElement( 
	unsigned short __RPC_FAR * /*pwchNamespaceUri*/,
	int /*cchNamespaceUri*/,
	unsigned short __RPC_FAR * /*pwchLocalName*/,
	int /*cchLocalName*/,
	unsigned short __RPC_FAR * /*pwchRawName*/,
	int /*cchRawName*/)
{
    return S_OK;
}
 
//==============================================================================
// BSAXContentHandlerImpl::characters
//==============================================================================       
HRESULT STDMETHODCALLTYPE BSAXContentHandlerImpl::characters(unsigned short __RPC_FAR * /*pwchChars*/, int /*cchChars*/)
{
    return S_OK;
}
        
//==============================================================================
// BSAXContentHandlerImpl::ignorableWhitespace
//==============================================================================
HRESULT STDMETHODCALLTYPE BSAXContentHandlerImpl::ignorableWhitespace(unsigned short __RPC_FAR * /*pwchChars*/, int /*cchChars*/)
{
    return S_OK;
}
        
//==============================================================================
// BSAXContentHandlerImpl::processingInstruction
//==============================================================================
HRESULT STDMETHODCALLTYPE BSAXContentHandlerImpl::processingInstruction( 
unsigned short __RPC_FAR * /*pwchTarget*/,
int /*cchTarget*/,
unsigned short __RPC_FAR * /*pwchData*/,
int /*cchData*/)
{
    return S_OK;
}
        
//==============================================================================
// BSAXContentHandlerImpl::skippedEntity
//==============================================================================        
HRESULT STDMETHODCALLTYPE BSAXContentHandlerImpl::skippedEntity(unsigned short __RPC_FAR * /*pwchVal*/, int /*cchVal*/)
{
    return S_OK;
}

//==============================================================================
// BSAXContentHandlerImpl::QueryInterface
//==============================================================================
long __stdcall BSAXContentHandlerImpl::QueryInterface(const struct _GUID & /*riid*/,void ** /*ppvObject*/)
{
    //-- hack-hack-hack!
    return 0;
}

//==============================================================================
// BSAXContentHandlerImpl::AddRef
//==============================================================================
unsigned long __stdcall BSAXContentHandlerImpl::AddRef()
{
    //-- hack-hack-hack!
    return 0;
}

//==============================================================================
// BSAXContentHandlerImpl::Release
//==============================================================================
unsigned long __stdcall BSAXContentHandlerImpl::Release()
{
    //-- hack-hack-hack!
    return 0;
}

