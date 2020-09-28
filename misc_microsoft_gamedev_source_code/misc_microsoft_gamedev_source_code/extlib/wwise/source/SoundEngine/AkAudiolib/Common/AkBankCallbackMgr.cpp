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

//////////////////////////////////////////////////////////////////////
//
// AkBankCallbackMgr.cpp
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"

#include "AkBankCallbackMgr.h"
#include <AK/Tools/Common/AkAutoLock.h>

CAkBankCallbackMgr::CAkBankCallbackMgr()
{
}

CAkBankCallbackMgr::~CAkBankCallbackMgr()
{
	m_ListCookies.Term();
}

AKRESULT CAkBankCallbackMgr::AddCookie( void* in_cookie )
{
	AkAutoLock<CAkLock> gate(m_csLock);
	BankCallbackItem* pItem = m_ListCookies.Exists( in_cookie );
	if( pItem )
	{
		++(pItem->m_cookieCount);
	}
	else
	{
		if( !m_ListCookies.Set( in_cookie ) )
			return AK_InsufficientMemory;
	}
	return AK_Success;
}

void CAkBankCallbackMgr::RemoveOneCookie( void* in_cookie )
{
	AkAutoLock<CAkLock> gate(m_csLock);
	BankCallbackItem* pItem = m_ListCookies.Exists( in_cookie );
	if( pItem )
	{
		if( pItem->m_cookieCount <=1 )
		{
			m_ListCookies.Unset( in_cookie );
		}
		else
		{
			--(pItem->m_cookieCount);
		}
	}
}

void CAkBankCallbackMgr::DoCallback( 
		AkBankCallbackFunc	in_pfnBankCallback,
		AkBankID			in_bankID,
		AKRESULT			in_eLoadResult,
		AkMemPoolId			in_memPoolId,
		void *				in_pCookie
		)
{
	if( in_pfnBankCallback )
	{
		AkAutoLock<CAkLock> gate(m_csLock);
		BankCallbackItem* pItem = m_ListCookies.Exists( in_pCookie );
		if( pItem )
		{
			bool bNeedToSkip = pItem->m_toSkipCount != 0;

			if( pItem->m_cookieCount <=1 )
				m_ListCookies.Unset( in_pCookie );
			else
			{
				--(pItem->m_cookieCount);
				if( bNeedToSkip )
					--(pItem->m_toSkipCount);
			}

			if( !bNeedToSkip )
			{
				in_pfnBankCallback( in_bankID, in_eLoadResult, in_memPoolId, in_pCookie );
			}
		}
	}
}

void CAkBankCallbackMgr::CancelCookie( void* in_cookie )
{
	AkAutoLock<CAkLock> gate(m_csLock);
	BankCallbackItem* pItem = m_ListCookies.Exists( in_cookie );
	if( pItem )
	{
		// set the count over the number of callback that will have to be skipped.
		pItem->m_toSkipCount = pItem->m_cookieCount;
	}
}
