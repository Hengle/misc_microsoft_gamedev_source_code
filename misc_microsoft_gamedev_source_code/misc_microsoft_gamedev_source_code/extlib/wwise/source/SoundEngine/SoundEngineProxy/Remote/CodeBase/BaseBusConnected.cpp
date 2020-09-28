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
#include "BaseBusConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"
#include "IBusProxy.h"
#include "BusProxyLocal.h"

BaseBusConnected::BaseBusConnected()
{
}

BaseBusConnected::~BaseBusConnected()
{
}

void BaseBusConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case IBusProxy::MethodVolume:
		{
			BusProxyCommandData::Volume volume;
			in_rSerializer.Get( volume );

			((BusProxyLocal&)GetLocalProxy()).Volume( volume.m_volume );
			break;
		}

	case IBusProxy::MethodLFEVolume:
		{
			BusProxyCommandData::LFEVolume lfeVolume;
			in_rSerializer.Get( lfeVolume );

			((BusProxyLocal&)GetLocalProxy()).LFEVolume( lfeVolume.m_LFEVolume );
			break;
		}

	case IBusProxy::MethodPitch:
		{
			BusProxyCommandData::Pitch pitch;
			in_rSerializer.Get( pitch );

			((BusProxyLocal&)GetLocalProxy()).Pitch( pitch.m_pitch );
			break;
		}

	case IBusProxy::MethodLPF:
		{
			BusProxyCommandData::LPF lpf;
			in_rSerializer.Get( lpf );

			((BusProxyLocal&)GetLocalProxy()).LPF( lpf.m_LPF );
			break;
		}

	case IBusProxy::MethodSetMaxNumInstancesOverrideParent:
		{
			BusProxyCommandData::SetMaxNumInstancesOverrideParent setMaxNumInstancesOverrideParent;
			in_rSerializer.Get( setMaxNumInstancesOverrideParent );

			((BusProxyLocal&)GetLocalProxy()).SetMaxNumInstancesOverrideParent( setMaxNumInstancesOverrideParent.m_bOverride );
			break;
		}

	case IBusProxy::MethodSetMaxNumInstances:
		{
			BusProxyCommandData::SetMaxNumInstances setMaxNumInstances;
			in_rSerializer.Get( setMaxNumInstances );

			((BusProxyLocal&)GetLocalProxy()).SetMaxNumInstances( setMaxNumInstances.m_ulMaxNumInstance );
			break;
		}
		
	case IBusProxy::MethodSetMaxReachedBehavior:
		{
			BusProxyCommandData::SetMaxReachedBehavior setMaxReachedBehavior;
			in_rSerializer.Get( setMaxReachedBehavior );

			((BusProxyLocal&)GetLocalProxy()).SetMaxReachedBehavior( setMaxReachedBehavior.m_bKillNewest );
			break;
		}

	case IBusProxy::MethodAddChild:
		{
			BusProxyCommandData::AddChild addChild;
			in_rSerializer.Get( addChild );

			((BusProxyLocal&)GetLocalProxy()).AddChild( addChild.m_id );

			break;
		}

	case IBusProxy::MethodRemoveChild:
		{
			BusProxyCommandData::RemoveChild removeChild;
			in_rSerializer.Get( removeChild );

			((BusProxyLocal&)GetLocalProxy()).RemoveChild( removeChild.m_id );

			break;
		}

	case IBusProxy::MethodRemoveAllChildren:
		{
			BusProxyCommandData::RemoveAllChildren removeAllChildren;
			in_rSerializer.Get( removeAllChildren );

			((BusProxyLocal&)GetLocalProxy()).RemoveAllChildren();

			break;
		}

	case IBusProxy::MethodSetRecoveryTime:
		{
			BusProxyCommandData::SetRecoveryTime setRecoveryTime;
			in_rSerializer.Get( setRecoveryTime );

			((BusProxyLocal&)GetLocalProxy()).SetRecoveryTime( setRecoveryTime.m_recoveryTime );
			break;
		}

	case IBusProxy::MethodAddDuck:
		{
			BusProxyCommandData::AddDuck addDuck;
			in_rSerializer.Get( addDuck );
			
			((BusProxyLocal&)GetLocalProxy()).AddDuck(	addDuck.m_busID, 
									addDuck.m_duckVolume, 
									addDuck.m_fadeOutTime, 
									addDuck.m_fadeInTime, 
									addDuck.m_eFadeCurve );

			break;
		}

	case IBusProxy::MethodRemoveDuck:
		{
			BusProxyCommandData::RemoveDuck removeDuck;
			in_rSerializer.Get( removeDuck );
			
			((BusProxyLocal&)GetLocalProxy()).RemoveDuck( removeDuck.m_busID );

			break;
		}

	case IBusProxy::MethodRemoveAllDuck:
		{
			BusProxyCommandData::RemoveAllDuck removeAllDuck;
			in_rSerializer.Get( removeAllDuck );
			
			((BusProxyLocal&)GetLocalProxy()).RemoveAllDuck();

			break;
		}

	case IBusProxy::MethodSetAsBackgroundMusic:
		{
			BusProxyCommandData::SetAsBackgroundMusic setAsBackgroundMusic;
			in_rSerializer.Get( setAsBackgroundMusic );
			
			((BusProxyLocal&)GetLocalProxy()).SetAsBackgroundMusic();

			break;
		}
		
	case IBusProxy::MethodUnsetAsBackgroundMusic:
		{
			BusProxyCommandData::UnsetAsBackgroundMusic unsetAsBackgroundMusic;
			in_rSerializer.Get( unsetAsBackgroundMusic );
			
			((BusProxyLocal&)GetLocalProxy()).UnsetAsBackgroundMusic();

			break;
		}

	case IBusProxy::MethodEnableWiiCompressor:
		{
			BusProxyCommandData::EnableWiiCompressor enableWiiCompressor;
			in_rSerializer.Get( enableWiiCompressor );

			((BusProxyLocal&)GetLocalProxy()).EnableWiiCompressor( enableWiiCompressor.m_bEnable );

			break;
		}
	case IBusProxy::MethodFeedbackVolume:
		{
			BusProxyCommandData::FeedbackVolume volume;
			in_rSerializer.Get( volume );
			((BusProxyLocal&)GetLocalProxy()).FeedbackVolume(volume.m_FeedbackVolume);
			break;
		}
	case IBusProxy::MethodFeedbackLPF:
		{
			BusProxyCommandData::FeedbackLPF feedbackLPF;
			in_rSerializer.Get( feedbackLPF );
			((BusProxyLocal&)GetLocalProxy()).FeedbackLPF( feedbackLPF.m_FeedbackLPF );
			break;
		}
	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}
