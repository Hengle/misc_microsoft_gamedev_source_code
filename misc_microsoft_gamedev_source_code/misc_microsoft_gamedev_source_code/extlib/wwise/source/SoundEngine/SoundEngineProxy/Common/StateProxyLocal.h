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

#include "ObjectProxyLocal.h"
#include "IStateProxy.h"

class CAkState;

class StateProxyLocal : public ObjectProxyLocal
						, virtual public IStateProxy
{
public:
	StateProxyLocal( AkUniqueID in_id, bool in_bIsCustomState, AkStateGroupID in_StateGroupID );
	virtual ~StateProxyLocal();

    virtual void Volume( AkReal32 in_volume );
    virtual void VolumeMeaning( AkValueMeaning in_eMeaning );
    virtual void Pitch( AkPitchValue in_pitch );
    virtual void PitchMeaning( AkValueMeaning in_eMeaning );
	virtual void LPF( AkLPFType in_LPF );
    virtual void LPFMeaning( AkValueMeaning in_eMeaning );
    virtual void LFEVolume( AkReal32 in_LFEVolume );
    virtual void LFEVolumeMeaning( AkValueMeaning in_eMeaning );
};

