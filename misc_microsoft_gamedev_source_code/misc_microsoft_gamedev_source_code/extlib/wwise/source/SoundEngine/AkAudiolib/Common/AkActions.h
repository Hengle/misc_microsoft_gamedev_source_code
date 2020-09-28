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
// AkActions.h
//
// The only goal of this file is to centralise inclusion of the 
// actions headers, avoiding it to be repeatedlt included everywhere
// 
// At everytime an action type is added, add it here too!!!
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTIONS_H_
#define _ACTIONS_H_

#include "AkActionPlay.h"
#include "AkActionStop.h"
#include "AkActionPause.h"
#include "AkActionResume.h"
#include "AkActionBreak.h"
#include "AkActionMute.h"
#include "AkActionSetVolume.h"
#include "AkActionSetLFE.h"
#include "AkActionSetLPF.h"
#include "AkActionSetPitch.h"
#include "AkActionSetState.h"
#include "AkActionSetSwitch.h"
#include "AkActionSetRTPC.h"
#include "AkActionPlayAndContinue.h"
#include "AkActionUseState.h"
#include "AkActionBypassFX.h"
#include "AkActionTrigger.h"

#endif
