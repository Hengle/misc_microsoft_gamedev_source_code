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
	Common file used by the communication for general network related functionalities.
	Location: \Wwise\Communication\Remote\CodeBase
	Implementation location: In the platform folders: Wwise\Communication\Remote\{Platform}
	
	See more information in the implementation file.
	-----------------------------------------------------------------------------------------
*/

#pragma once

#include "AkPrivateTypes.h"

namespace Network
{
	AkInt32 Init( AkMemPoolId in_pool );
	AkInt32 Term();
	AkInt32 GetLastError();

	void GetMachineName( char* out_pMachineName, AkInt32* io_pStringSize );

	bool SameEndianAsNetwork();
}
