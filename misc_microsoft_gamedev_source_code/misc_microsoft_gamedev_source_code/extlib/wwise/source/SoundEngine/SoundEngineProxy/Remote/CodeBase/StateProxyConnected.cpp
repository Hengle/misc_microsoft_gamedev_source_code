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

#include "StateProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"


StateProxyConnected::StateProxyConnected( AkUniqueID in_id, bool in_bIsCustomState, AkStateGroupID in_StateGroupID )
	: m_proxyLocal( in_id, in_bIsCustomState, in_StateGroupID )
{
}

StateProxyConnected::~StateProxyConnected()
{
}

void StateProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData objectData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( objectData );
	}

	switch( objectData.m_methodID )
	{
	case IStateProxy::MethodVolume:
		{
			StateProxyCommandData::Volume volume;
			in_rSerializer.Get( volume );

			m_proxyLocal.Volume( volume.m_volume );
			break;
		}

	case IStateProxy::MethodVolumeMeaning:
		{
			StateProxyCommandData::VolumeMeaning volumeMeaning;
			in_rSerializer.Get( volumeMeaning );

			m_proxyLocal.VolumeMeaning( volumeMeaning.m_eMeaning );
			break;
		}

	case IStateProxy::MethodPitch:
		{
			StateProxyCommandData::Pitch pitch;
			in_rSerializer.Get( pitch );

			m_proxyLocal.Pitch( pitch.m_pitchType );
			break;
		}

	case IStateProxy::MethodPitchMeaning:
		{
			StateProxyCommandData::PitchMeaning pitchMeaning;
			in_rSerializer.Get( pitchMeaning );

			m_proxyLocal.PitchMeaning( pitchMeaning.m_eMeaning );
			break;
		}

	case IStateProxy::MethodLPF:
		{
			StateProxyCommandData::LPF lpf;
			in_rSerializer.Get( lpf );

			m_proxyLocal.LPF( lpf.m_LPF );
			break;
		}

	case IStateProxy::MethodLPFMeaning:
		{
			StateProxyCommandData::LPFMeaning lpfMeaning;
			in_rSerializer.Get( lpfMeaning );

			m_proxyLocal.LPFMeaning( lpfMeaning.m_eMeaning );
			break;
		}

	case IStateProxy::MethodLFEVolume:
		{
			StateProxyCommandData::LFEVolume lfeVolume;
			in_rSerializer.Get( lfeVolume );

			m_proxyLocal.LFEVolume( lfeVolume.m_LFEVolume );
			break;
		}

	case IStateProxy::MethodLFEVolumeMeaning:
		{
			StateProxyCommandData::LFEVolumeMeaning lfeVolumeMeaning;
			in_rSerializer.Get( lfeVolumeMeaning );

			m_proxyLocal.LFEVolumeMeaning( lfeVolumeMeaning.m_eMeaning );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}
