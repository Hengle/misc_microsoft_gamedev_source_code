/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

#include "stdafx.h"

#include "ObjectProxyLocal.h"
#include "AkIndexable.h"


ObjectProxyLocal::ObjectProxyLocal()
	: m_pIndexable( NULL )
#ifndef PROXYCENTRAL_CONNECTED
	, m_refCount( 1 )
#endif
{
}

ObjectProxyLocal::~ObjectProxyLocal()
{
#ifndef PROXYCENTRAL_CONNECTED
	AKASSERT( m_refCount == 0 );
#else
	if ( m_pIndexable )
		m_pIndexable->Release();
#endif
}

void ObjectProxyLocal::AddRef()
{
#ifndef PROXYCENTRAL_CONNECTED
	++m_refCount;
#endif
}

bool ObjectProxyLocal::Release()
{
#ifndef PROXYCENTRAL_CONNECTED
	AKASSERT( m_refCount > 0 );

	bool bRet = false;
	
	if( --m_refCount == 0 )
	{
		if( m_pIndexable != NULL )
			m_pIndexable->Release();
		delete this;

		bRet = true;
	}

	return bRet;
#else
	return false;
#endif
}

AkUniqueID ObjectProxyLocal::GetID() const
{
	return m_pIndexable->ID();
}

void ObjectProxyLocal::SetIndexable( CAkIndexable* in_pIndexable )
{
	AKASSERT( m_pIndexable == NULL );
	m_pIndexable = in_pIndexable;
}

