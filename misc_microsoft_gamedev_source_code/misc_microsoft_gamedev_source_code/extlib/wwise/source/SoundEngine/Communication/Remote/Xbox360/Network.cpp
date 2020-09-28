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


/*	----------------------------------------------------------------------------------------
	Xbox360 implementation.
	Location: \Wwise\Communication\Remote\Xbox360
	Header location: \Wwise\Communication\Remote\CodeBase
	-----------------------------------------------------------------------------------------
*/

#include "stdafx.h"

#include "Network.h"

#include <Xbdm.h>

namespace Network
{
	AkInt32 Init( AkMemPoolId in_pool )
	{
		XNetStartupParams xnsp; 
		memset( &xnsp, 0, sizeof xnsp ); 
		xnsp.cfgSizeOfStruct = sizeof xnsp;
		xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
		XNetStartup( &xnsp );

		WSAData wsaData = { 0 };
		return ::WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
	}

	AkInt32 Term()
	{
		return ::WSACleanup();
	}

	AkInt32 GetLastError()
	{
		return ::WSAGetLastError();
	}

	void GetMachineName( char* out_pMachineName, AkInt32* io_pStringSize )
	{
		DmGetXboxName( out_pMachineName, (DWORD*)io_pStringSize );
	}

	bool SameEndianAsNetwork()
	{
		return htons( 12345 ) == 12345;
	}
}