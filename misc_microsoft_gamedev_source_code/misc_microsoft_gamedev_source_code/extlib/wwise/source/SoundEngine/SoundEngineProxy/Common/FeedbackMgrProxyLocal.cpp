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
#include "FeedbackMgrProxyLocal.h"
#include <AK/MotionEngine/Common/AkMotionEngine.h>

#ifdef WIN32
	#ifdef _DEBUG
		#define new DEBUG_NEW
	#endif
#endif

FeedbackMgrProxyLocal::FeedbackMgrProxyLocal()
{
}

FeedbackMgrProxyLocal::~FeedbackMgrProxyLocal()
{
}

void FeedbackMgrProxyLocal::Enable( AkInt16 in_usCompany, AkInt16 in_usPlugin, AkCreatePluginCallback in_fnInit, bool in_bEnable )
{
	//Use "player" 0 to represent the Wwise application.
	if (in_bEnable)
	{
		AK::MotionEngine::RegisterMotionDevice(in_usCompany, in_usPlugin, in_fnInit);
		AK::MotionEngine::AddPlayerMotionDevice(0, in_usCompany, in_usPlugin);
	}
	else
		AK::MotionEngine::RemovePlayerMotionDevice(0, in_usCompany, in_usPlugin);
}
