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
// AkDuckItem.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _DUCK_ITEM_H_
#define _DUCK_ITEM_H_

#include "AkTransition.h"

class CAkBus;
class CAkTransition;

// This class contains the Specific Information
class CAkDuckItem : public ITransitionable
{

public:
	//destructor
	AKRESULT Term();

	AKRESULT Init(CAkBus* in_pBusNode);

	CAkBus*	m_pBusNode;

	CAkTransition*	m_pvVolumeTransition;

	AkVolumeValue	m_EffectiveVolumeOffset;

	virtual void TransUpdateValue(
		TransitionTargetTypes in_eTargetType,
		TransitionTarget in_unionValue,
		bool in_bIsTerminated
		);
};

#endif //_DUCK_ITEM_H_
