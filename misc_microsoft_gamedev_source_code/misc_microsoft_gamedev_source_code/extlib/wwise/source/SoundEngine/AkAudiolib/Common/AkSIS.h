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
// AkSIS.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _SIS_H_
#define _SIS_H_

#include <AK/Tools/Common/AkObject.h>
#include "ITransitionable.h"

#define _MAX_PITCH_FOR_NODE		1000000000
#define _MIN_PITCH_FOR_NODE	   -1000000000

class CAkParameterNodeBase;
class CAkTransition;
class CAkRegisteredObj;

// This class contains the Specific Information
class CAkSIS : public CAkObject,
			   public ITransitionable
               
{

public:
	//Constructor
	CAkSIS(CAkParameterNodeBase* in_Parent, AkUInt8 in_bitsFXBypass );
	CAkSIS(CAkParameterNodeBase* in_Parent, CAkRegisteredObj * in_pGameObj, AkUInt8 in_bitsFXBypass );

	//destructor
	virtual ~CAkSIS();

	CAkParameterNodeBase*	m_pParamNode;
	CAkRegisteredObj*		m_pGameObj;

	CAkTransition*	m_pvVolumeTransition;
	CAkTransition*	m_pvLFETransition;
	CAkTransition*	m_pvLPFTransition;
	CAkTransition*	m_pvPitchTransition;
	CAkTransition*	m_pvMuteTransition;
	CAkTransition*	m_pvFeedbackVolumeTransition;

	AkVolumeValue	m_EffectiveVolumeOffset;
	AkVolumeValue	m_EffectiveLFEOffset;
	AkLPFType		m_EffectiveLPFOffset;
	AkPitchValue	m_EffectivePitchOffset;
	AkVolumeValue	m_EffectiveFeedbackVolumeOffset;
	AkUInt8			m_cMuteLevel;
	AkUInt8         m_bitsFXBypass; // 0-3 is effect-specific, 4 is bypass all

	virtual void TransUpdateValue(
		TransitionTargetTypes in_eTargetType,
		TransitionTarget in_unionValue,
		bool in_bIsTerminated
		);
};
#endif
