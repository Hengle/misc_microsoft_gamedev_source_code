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

namespace CommunicationDefines
{
	enum ConsoleType
	{
		ConsoleUnknown,
		ConsoleWindows,
        ConsoleXBox360,
		ConsolePS3,
		ConsoleWii
	};

#ifdef WIN32
	static ConsoleType g_eConsoleType = ConsoleWindows;
#endif
#ifdef XBOX360
	static ConsoleType g_eConsoleType = ConsoleXBox360;
#endif
#ifdef __PPU__
	static ConsoleType g_eConsoleType = ConsolePS3;
#endif
#ifdef RVL_OS
	static ConsoleType g_eConsoleType = ConsoleWii;
#endif
}
