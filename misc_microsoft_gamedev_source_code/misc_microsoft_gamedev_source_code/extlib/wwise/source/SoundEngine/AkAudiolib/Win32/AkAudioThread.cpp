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
// AkAudioThread.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkAudioThread.h"
#include "AkAudioMgr.h"
#include "AkRandom.h"
#include "AkLEngine.h"

using namespace AKPLATFORM;

extern AkPlatformInitSettings			g_PDSettings;

AkThread		CAkAudioThread::m_hEventMgrThread;

CAkAudioThread::CAkAudioThread()
	:m_eventProcess( NULL )
	,m_bStopThread(false)
{
}

CAkAudioThread::~CAkAudioThread()
{
}

void CAkAudioThread::WakeupEventsConsumer()
{
	::SetEvent( m_eventProcess );
}

//-----------------------------------------------------------------------------
// Name: EventMgrThreadFunc
// Desc: Âudio loop
//-----------------------------------------------------------------------------
AK_DECLARE_THREAD_ROUTINE(CAkAudioThread::EventMgrThreadFunc)
{
	AKRANDOM::AkRandomInit();

	// get our info from the parameter
    CAkAudioThread* pAudioThread = reinterpret_cast<CAkAudioThread*>(AK_THREAD_ROUTINE_PARAMETER);

	CAkLEngine::StartVoice();

	AKASSERT(g_pAudioMgr);

	DWORD dwWaitRes = WAIT_TIMEOUT;
	do
    {
		switch( dwWaitRes )
		{
		case WAIT_OBJECT_0:			//ThreadProcessEvent
		case WAIT_TIMEOUT:			// Default Time Out
			g_pAudioMgr->Perform();
			break;
		default:
			AKASSERT( !"Unexpected event received on main thread" );
		}

		DWORD dwWaitRes = ::WaitForSingleObject( pAudioThread->m_eventProcess, AK_PC_WAIT_TIME ); 
    }
	while ( !pAudioThread->m_bStopThread );

	AkExitThread( AK_RETURN_THREAD_OK );
}


AKRESULT CAkAudioThread::Start()
{
	m_eventProcess	= ::CreateEvent( NULL, false, false, NULL );
	m_bStopThread	= false;

	if ( !m_eventProcess )
		return AK_Fail;

	//Create the EventManagerThread
	AkCreateThread(	EventMgrThreadFunc,					// Start address
					this,								// Parameter
					&g_PDSettings.threadLEngine,		// Properties 
					&m_hEventMgrThread,					// AkHandle
					"AK::EventManager" );				// Debug name
	// is the thread ok ?
	if ( !AkIsValidThread(&m_hEventMgrThread) )
		return AK_Fail;
	return AK_Success;
}

void CAkAudioThread::Stop()
{
	m_bStopThread = true;
	if ( AkIsValidThread( &m_hEventMgrThread ) )
	{
		WakeupEventsConsumer();
		AkWaitForSingleThread( &m_hEventMgrThread );
		AkCloseThread(&m_hEventMgrThread);
	}
	if ( m_eventProcess )
		::CloseHandle( m_eventProcess );
	m_eventProcess = NULL;
}
