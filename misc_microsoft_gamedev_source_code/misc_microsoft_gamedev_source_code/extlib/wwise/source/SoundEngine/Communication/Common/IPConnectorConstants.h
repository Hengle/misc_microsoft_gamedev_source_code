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


#pragma once

namespace IPConnectorConstants
{
	static const unsigned short kDiscoveryBroadcastPort = 0xF200;		// 61952
	static const unsigned short kDiscoveryResponsePort = 0xF201;		// 61953

	static const unsigned short kCommandChannelPort = 0xF202;			// 61954
	static const unsigned short kNotificationChannelPort = 0xF203;		// 61955
	static const unsigned short kDataChannelPort = 0xF204;				// 61956

	static const unsigned short kControlChannelPort = 0xF205;			// 61957
}
