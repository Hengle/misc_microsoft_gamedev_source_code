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
// AkFXContext.cpp
//
// Implementation of FX context interface for source, insert and bus FX.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkFXContext.h"

//-----------------------------------------------------------------------------
// CAkInsertFXContext class.
//-----------------------------------------------------------------------------
CAkInsertFXContext::CAkInsertFXContext( CAkPBI * in_pCtx, AkUInt32 in_uFXIndex ) 
	: m_pContext( in_pCtx )
	, m_uFXIndex( in_uFXIndex )
{
}

CAkInsertFXContext::~CAkInsertFXContext( )
{
	m_pContext = NULL;
}

bool CAkInsertFXContext::IsSendModeEffect( )
{
	return false;	// Always insert FX mode
}

//-----------------------------------------------------------------------------
// CAkBusFXContext class.
//-----------------------------------------------------------------------------
CAkBusFXContext::CAkBusFXContext(  ) 
: m_bSendMode( false )
{
}

CAkBusFXContext::~CAkBusFXContext( )
{
}

void CAkBusFXContext::SetIsSendModeEffect( bool in_bSendMode )
{
	m_bSendMode = in_bSendMode;	// SendMode FX mode for environmentals
}

bool CAkBusFXContext::IsSendModeEffect( )
{
	return m_bSendMode;	// SendMode FX mode for environmentals
}

//-----------------------------------------------------------------------------
// CAkSourceFXContext class.
//-----------------------------------------------------------------------------
CAkSourceFXContext::CAkSourceFXContext( CAkPBI * in_pCtx, AK::IAkStreamMgr * in_pStreamMgr ) 
	: m_pContext( in_pCtx ),
	m_pStreamMgr( in_pStreamMgr )
{
}

CAkSourceFXContext::~CAkSourceFXContext( )
{
	m_pContext = NULL;
}

// Number of loops set through context

AkUInt16 CAkSourceFXContext::GetNumLoops( )
{
	AKASSERT( m_pContext != NULL );
	return m_pContext->GetLooping( );
}

AK::IAkStreamMgr * CAkSourceFXContext::GetStreamMgr( )
{
	AKASSERT( m_pStreamMgr != NULL );
	return m_pStreamMgr;
}
