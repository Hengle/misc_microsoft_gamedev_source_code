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
// AkContinuationList.cpp
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "AkContinuationList.h"
#include "AkRanSeqCntr.h"
#include "AkSwitchCntr.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

CAkContinueListItem::CAkContinueListItem()
:m_pContainer( NULL )
,m_pContainerInfo( NULL )
,m_pMultiPlayNode( NULL )
,m_pAlternateContList( NULL )
{
}

CAkContinueListItem::~CAkContinueListItem()
{
	if(m_pContainerInfo)
	{
		m_pContainerInfo->Destroy();
		m_pContainerInfo = NULL;
	}
}

void CAkContinuationList::Term()
{
	for ( AkContinueListItem::Iterator iter = m_listItems.Begin(); iter != m_listItems.End(); ++iter )
	{
		CAkContinueListItem & item = *iter;

		if( item.m_pMultiPlayNode && item.m_pAlternateContList )
		{
			//Unref removes the ref and destroy the list if comes to zero
			item.m_pMultiPlayNode->ContUnrefList( item.m_pAlternateContList );
		}
	}

	m_listItems.Term();
}

CAkContinuationList::~CAkContinuationList()
{
}

CAkContinuationList* CAkContinuationList::Create()
{
	return AkNew( g_DefaultPoolId, CAkContinuationList );
}

void CAkContinuationList::AddRef()
{ 
	AKPLATFORM::AkInterlockedIncrement( &m_iRefCount ); 
}
void CAkContinuationList::Release()
{
	if( AKPLATFORM::AkInterlockedDecrement( &m_iRefCount ) == 0 )
	{
		Term();
		AkDelete( g_DefaultPoolId, this );
	}
}
