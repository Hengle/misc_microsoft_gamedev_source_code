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

#ifndef _PARAMS_H_
#define _PARAMS_H_

enum ParamType
{
	PT_All				= 0xffffffff,
    PT_Volume			= 0x0001,
    PT_Pitch			= 0x0002,
    PT_LPF				= 0x0004,
	PT_LFE				= 0x0008
};

#endif
