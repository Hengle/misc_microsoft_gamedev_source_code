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
#include "SFLib.h"

#include <crtdbg.h>
#include <guiddef.h>
#include <objidl.h>

#include "WObjectType.h"

// serializer.

using namespace SoundFrame;

//-------------------------------------------------------------------------------------------------
//  

CSoundFrame::CSoundFrame( IClient * pClient )
	: m_hwndServer( NULL )
	, m_ulServerVersion( 0 )
	, m_hwndMessage( NULL )
	, m_pClient( pClient )
	, m_pCopyData( NULL )
	, m_bProjectInfoValid( false )
{
	m_wszProjectName[ 0 ] = 0;
	m_guidProject = GUID_NULL;

	m_cfWObjectID = static_cast<CLIPFORMAT>( ::RegisterClipboardFormat( _T("WwiseObjectID") ) ); // FIXME: 
}

CSoundFrame::~CSoundFrame()
{
	_ASSERT( m_pCopyData == NULL );

	if ( m_hwndServer )
		::SendMessage( m_hwndServer, WM_SF_SHUTDOWN, (WPARAM) m_hwndMessage, 0 );

	if ( m_hwndMessage ) 
		::DestroyWindow( m_hwndMessage );	
}

bool CSoundFrame::Init()
{
	if ( !m_pClient )
		return false;

	m_hwndMessage = ::CreateWindow( _T( "STATIC" ), SF_CLIENTWINDOWNAME, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL );
	::SetWindowLongPtr( m_hwndMessage, GWLP_WNDPROC, (LONG) (LONG_PTR) MsgWndProc );
	::SetWindowLongPtr( m_hwndMessage, GWLP_USERDATA, (LONG) (LONG_PTR) this );

	HWND hwndServer = ::FindWindowEx( HWND_MESSAGE, NULL, NULL, SF_SERVERWINDOWNAME ) ;
	if ( hwndServer )
	{
		::PostMessage( hwndServer, WM_SF_STARTUP, (WPARAM) m_hwndMessage, 0 );
	}

	return true;
}

bool CSoundFrame::IsConnected()  const
{
	return m_hwndServer != NULL;
}

ULONG CSoundFrame::GetClientVersion() const
{
	return SF_SOUNDFRAME_VERSION_CURRENT;
}

ULONG CSoundFrame::GetServerVersion() const
{
	return m_ulServerVersion;
}

bool CSoundFrame::PlayEvents( const AkUniqueID * in_pEvents, long in_cEvents, AkGameObjectID in_gameObjectID )
{
	if ( !in_pEvents || !in_cEvents || !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<AkGameObjectID>( in_gameObjectID );
	bytes.Write<int>( in_cEvents );

	for ( int i = 0; i < in_cEvents; ++i )
		bytes.Write<AkUniqueID>( in_pEvents[ i ] );

	SendCopyData( SF_COPYDATA_PLAYEVENTS, bytes );

	return true;
}

bool CSoundFrame::PlayEvents( LPCWSTR * in_pszEvents, long in_cEvents, AkGameObjectID in_gameObjectID )
{
	if ( !in_pszEvents || !in_cEvents || !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<AkGameObjectID>( in_gameObjectID );
	bytes.Write<int>( in_cEvents );

	for ( int i = 0; i < in_cEvents; ++i )
		bytes.WriteString( in_pszEvents[ i ] );

	SendCopyData( SF_COPYDATA_PLAYEVENTSBYSTRING, bytes );

	return true;
}

bool CSoundFrame::SetPlayBackMode( bool in_bPlayback ) const
{
	if ( !m_hwndServer )
		return false;

	::SendMessage( m_hwndServer, in_bPlayback? WM_SF_PLAY : WM_SF_STOP, (WPARAM) m_hwndMessage, 0 );

	return true;
}

bool CSoundFrame::GetCurrentState( AkStateGroupID in_stateGroupID, IState** out_ppCurrentState ) const
{
	if ( !m_hwndServer || !out_ppCurrentState )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( false );	// We are not using string
	bytes.Write<AkStateGroupID>( in_stateGroupID );

	SendCopyData( SF_COPYDATA_REQCURRENTSTATE, bytes );

	if ( m_pCopyData )
	{
		*out_ppCurrentState = (IState *) m_pCopyData;

		m_pCopyData = NULL;
		return true;
	}
	else
	{
		*out_ppCurrentState = NULL;
		return false;
	}
}

bool CSoundFrame::GetCurrentState( LPCWSTR in_szStateGroupName, IState** out_ppCurrentState ) const
{
	if ( !m_hwndServer || !in_szStateGroupName || !out_ppCurrentState )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( true );	// We are using string
	bytes.WriteString( in_szStateGroupName );

	SendCopyData( SF_COPYDATA_REQCURRENTSTATE, bytes );

	if ( m_pCopyData )
	{
		*out_ppCurrentState = (IState *) m_pCopyData;

		m_pCopyData = NULL;
		return true;
	}
	else
	{
		*out_ppCurrentState = NULL;
		return false;
	}
}

bool CSoundFrame::SetCurrentState( AkStateGroupID in_stateGroupID, AkStateID in_currentStateID )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( false );	// Not using string
	bytes.Write<AkStateGroupID>( in_stateGroupID );
	bytes.Write<AkStateGroupID>( in_currentStateID );

	SendCopyData( SF_COPYDATA_SETCURRENTSTATE, bytes );

	return true;
}

bool CSoundFrame::SetCurrentState( LPCWSTR in_szStateGroupName, LPCWSTR in_szCurrentStateName )
{
	if ( !m_hwndServer || !in_szStateGroupName || !in_szCurrentStateName )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( true );	// Using string
	bytes.WriteString( in_szStateGroupName );
	bytes.WriteString( in_szCurrentStateName );

	SendCopyData( SF_COPYDATA_SETCURRENTSTATE, bytes );

	return true;
}

bool CSoundFrame::GetCurrentSwitch( AkSwitchGroupID in_switchGroupID, ISwitch** out_ppCurrentSwitch, AkGameObjectID in_gameObjectID ) const
{
	if ( !m_hwndServer || !out_ppCurrentSwitch )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( false ); // Not using string
	bytes.Write<AkSwitchGroupID>( in_switchGroupID );
	bytes.Write<AkGameObjectID>( in_gameObjectID );

	SendCopyData( SF_COPYDATA_REQCURRENTSWITCH, bytes );

	if ( m_pCopyData )
	{
		*out_ppCurrentSwitch = (ISwitch *) m_pCopyData;

		m_pCopyData = NULL;
		return true;
	}
	else
	{
		*out_ppCurrentSwitch = NULL;
		return false;
	}
}

bool CSoundFrame::GetCurrentSwitch( LPCWSTR in_szSwitchGroupName, ISwitch** out_ppCurrentSwitch, AkGameObjectID in_gameObjectID ) const
{
	if ( !m_hwndServer || !out_ppCurrentSwitch )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( true ); // Not using string
	bytes.WriteString( in_szSwitchGroupName );
	bytes.Write<AkGameObjectID>( in_gameObjectID );

	SendCopyData( SF_COPYDATA_REQCURRENTSWITCH, bytes );

	if ( m_pCopyData )
	{
		*out_ppCurrentSwitch = (ISwitch *) m_pCopyData;

		m_pCopyData = NULL;
		return true;
	}
	else
	{
		*out_ppCurrentSwitch = NULL;
		return false;
	}
}

bool CSoundFrame::SetCurrentSwitch( AkSwitchGroupID in_switchGroupID, AkSwitchStateID in_currentSwitchID, AkGameObjectID in_gameObjectID )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( false ); // Not using String
	bytes.Write<AkSwitchGroupID>( in_switchGroupID );
	bytes.Write<AkSwitchStateID>( in_currentSwitchID );
	bytes.Write<AkGameObjectID>( in_gameObjectID );

	SendCopyData( SF_COPYDATA_SETCURRENTSWITCH, bytes );

	return true;
}

bool CSoundFrame::SetCurrentSwitch( LPCWSTR in_szSwitchGroupName, LPCWSTR in_szCurrentSwitchName, AkGameObjectID in_gameObjectID )
{
	if ( !m_hwndServer || !in_szSwitchGroupName || !in_szCurrentSwitchName )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( true ); // Using String
	bytes.WriteString( in_szSwitchGroupName );
	bytes.WriteString( in_szCurrentSwitchName );
	bytes.Write<AkGameObjectID>( in_gameObjectID );

	SendCopyData( SF_COPYDATA_SETCURRENTSWITCH, bytes );

	return true;
}

bool CSoundFrame::RegisterGameObject( AkGameObjectID in_gameObjectID, LPCWSTR in_szGameObjectName )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<UINT32>( in_gameObjectID );
	bytes.WriteString( in_szGameObjectName );

	SendCopyData( SF_COPYDATA_REGISTERGAMEOBJECT, bytes );

	return true;
}

bool CSoundFrame::UnregisterGameObject( AkGameObjectID in_gameObjectID )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<UINT32>( in_gameObjectID );

	SendCopyData( SF_COPYDATA_UNREGISTERGAMEOBJECT, bytes );

	return true;
}

bool CSoundFrame::SetRTPCValue( AkRtpcID in_gameParameterID, AkRtpcValue in_value, AkGameObjectID in_gameObjectID )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( false ); // Not Using string
	bytes.Write<AkRtpcID>( in_gameParameterID );
	bytes.Write<AkRtpcValue>( in_value );
	bytes.Write<AkGameObjectID>( in_gameObjectID );

	SendCopyData( SF_COPYDATA_SETRTPCVALUE, bytes );

	return true;
}

bool CSoundFrame::SetRTPCValue( LPCWSTR in_szGameParameterName, AkRtpcValue in_value, AkGameObjectID in_gameObjectID )
{
	if ( !m_hwndServer || !in_szGameParameterName )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( true ); // Using string
	bytes.WriteString( in_szGameParameterName );
	bytes.Write<AkRtpcValue>( in_value );
	bytes.Write<AkGameObjectID>( in_gameObjectID );

	SendCopyData( SF_COPYDATA_SETRTPCVALUE, bytes );

	return true;
}

bool CSoundFrame::PostTrigger( AkTriggerID in_triggerID, AkGameObjectID in_gameObjectID )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( false ); // Not Using String
	bytes.Write<AkTriggerID>( in_triggerID );
	bytes.Write<AkGameObjectID>( in_gameObjectID );

	SendCopyData( SF_COPYDATA_TRIGGER, bytes );

	return true;
}

bool CSoundFrame::PostTrigger( LPCWSTR in_szTriggerName, AkGameObjectID in_gameObjectID )
{
	if ( !m_hwndServer || !in_szTriggerName )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( true ); // Using String
	bytes.WriteString( in_szTriggerName );
	bytes.Write<AkGameObjectID>( in_gameObjectID );

	SendCopyData( SF_COPYDATA_TRIGGER, bytes );

	return true;
}

bool CSoundFrame::SetActiveListeners( AkGameObjectID in_gameObjectID, AkUInt32 in_uiListenerMask )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<AkGameObjectID>( in_gameObjectID );
	bytes.Write<AkUInt32>( in_uiListenerMask );

	SendCopyData( SF_COPYDATA_SETACTIVELISTENER, bytes );
	
	return true;
}
	
bool CSoundFrame::SetPosition( AkGameObjectID in_gameObjectID, const AkSoundPosition& in_rPosition, AkUInt32 in_uiListenerIndex )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<AkGameObjectID>( in_gameObjectID );
	bytes.Write<AkSoundPosition>( in_rPosition );
	bytes.Write<AkUInt32>( in_uiListenerIndex );

	SendCopyData( SF_COPYDATA_SETPOSITION, bytes );

	return true;
}

bool CSoundFrame::SetListenerPosition( const AkListenerPosition& in_rPosition, AkUInt32 in_uiIndex )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<AkListenerPosition>( in_rPosition );
	bytes.Write<AkUInt32>( in_uiIndex );

	SendCopyData( SF_COPYDATA_SETLISTENERPOSITION, bytes );

	return true;
}

bool CSoundFrame::SetListenerSpatialization( AkUInt32 in_uiIndex, bool in_bSpatialized, AkSpeakerVolumes * in_pVolumeOffsets )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<AkUInt32>( in_uiIndex );
	bytes.Write<bool>( in_bSpatialized );
	bytes.Write<bool>( in_pVolumeOffsets? true : false );	// Do we have a Speaker Volumes struct

	if( in_pVolumeOffsets )
		bytes.Write<AkSpeakerVolumes>( *in_pVolumeOffsets );

	SendCopyData( SF_COPYDATA_SETLISTENERSPATIALIZATION, bytes );

	return true;
}

bool CSoundFrame::SetGameObjectEnvironmentsValues( AkGameObjectID in_gameObjectID, AkEnvironmentValue* in_aEnvironmentValues, AkUInt32 in_uNumEnvValues )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<AkGameObjectID>( in_gameObjectID );
	bytes.Write<AkUInt32>( in_aEnvironmentValues? in_uNumEnvValues : 0 );

	if( in_aEnvironmentValues && in_uNumEnvValues > 0 )
	{
		for(unsigned int i = 0; i < in_uNumEnvValues; ++i)
			bytes.Write<AkEnvironmentValue>( in_aEnvironmentValues[i] );
	}

	SendCopyData( SF_COPYDATA_SETGAMEOBJECTENVIRONMENTSVALUES, bytes );

	return true;
}
	
bool CSoundFrame::SetGameObjectDryLevelValue( AkGameObjectID in_gameObjectID, AkReal32 in_fControlValue )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<AkGameObjectID>( in_gameObjectID );
	bytes.Write<AkReal32>( in_fControlValue );

	SendCopyData( SF_COPYDATA_SETGAMEOBJECTDRYLEVELVALUE, bytes );

	return true;
}
	
bool CSoundFrame::SetEnvironmentVolume( AkEnvID in_FXParameterSetID, AkReal32 in_fVolume )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<AkEnvID>( in_FXParameterSetID );
	bytes.Write<AkReal32>( in_fVolume );

	SendCopyData( SF_COPYDATA_SETENVIRONMENTVOLUME, bytes );

	return true;
}
	
bool CSoundFrame::BypassEnvironment( AkEnvID in_FXParameterSetID, bool in_bIsBypassed )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<AkEnvID>( in_FXParameterSetID );
	bytes.Write<bool>( in_bIsBypassed );

	SendCopyData( SF_COPYDATA_BYPASSENVIRONMENT, bytes );

	return true;
}
	
bool CSoundFrame::SetObjectObstructionAndOcclusion( AkGameObjectID in_ObjectID, AkUInt32 in_uListener, AkReal32 in_fObstructionLevel, AkReal32 in_fOcclusionLevel )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<AkGameObjectID>( in_ObjectID );
	bytes.Write<AkUInt32>( in_uListener );
	bytes.Write<AkReal32>( in_fObstructionLevel );
	bytes.Write<AkReal32>( in_fOcclusionLevel );

	SendCopyData( SF_COPYDATA_SETOBJECTOBSTRUCTIONANDOCCLUSION, bytes );

	return true;
}

bool CSoundFrame::PostMsgMonitor( LPCWSTR in_pszMessage )
{
	if ( !m_hwndServer )
		return false;
	
	SFWriteBytesMem bytes;
	bytes.WriteString( in_pszMessage );

	SendCopyData( SF_COPYDATA_POSTMONITORINGMESSAGE, bytes );

	return true;
}
bool CSoundFrame::StopAll( AkGameObjectID in_GameObjID )
{
	if ( !m_hwndServer )
		return false;
	
	SFWriteBytesMem bytes;
	bytes.Write<AkGameObjectID>( in_GameObjID );

	SendCopyData( SF_COPYDATA_STOPALL, bytes );

	return true;
}

bool CSoundFrame::StopPlayingID( AkPlayingID in_playingID )
{
	if ( !m_hwndServer )
		return false;
	
	SFWriteBytesMem bytes;
	bytes.Write<AkPlayingID>( in_playingID );

	SendCopyData( SF_COPYDATA_STOPPLAYINGID, bytes );

	return true;
}

bool CSoundFrame::GetEventList( IEventList ** out_ppEventList ) const
{
	return GetObjectList( out_ppEventList, WM_SF_REQEVENTS );
}

bool CSoundFrame::GetEvent( AkUniqueID in_eventID, IEvent ** out_ppEvent ) const
{
	return GetObject<IEvent, IEventList>( in_eventID, out_ppEvent, SFType_Event );
}

bool CSoundFrame::GetEvent( LPCWSTR in_pszEvent, IEvent ** out_ppEvent ) const
{
	return GetObjectByString<IEvent, IEventList>( in_pszEvent, out_ppEvent, SFType_Event );
}

bool CSoundFrame::GetEvents( const AkUniqueID * in_pEvents, long in_cEvents, IEventList ** out_ppEventList ) const
{
	return GetObjects( in_pEvents, in_cEvents, out_ppEventList, SFType_Event );
}

bool CSoundFrame::GetEvents( LPCWSTR * in_pszEvents, long in_cEvents, IEventList ** out_ppEventList ) const
{
	return GetObjectsByString( in_pszEvents, in_cEvents, out_ppEventList, SFType_Event );
}

CSoundFrame::DnDType 
	CSoundFrame::GetDnDType( IDataObject * in_pDataObject )
{
	FORMATETC fmt;
	fmt.cfFormat = m_cfWObjectID;
	fmt.dwAspect = DVASPECT_CONTENT;
	fmt.lindex = -1;
	fmt.ptd = NULL;
	fmt.tymed = TYMED_HGLOBAL;

	DnDType result = TypeUnknown;

	HRESULT hr = in_pDataObject->QueryGetData( &fmt );
	if ( hr == S_OK )
	{
		STGMEDIUM stg;

		hr = in_pDataObject->GetData( &fmt, &stg );
		if ( hr == S_OK )
		{
			SFReadBytesMem bytes( ::GlobalLock( stg.hGlobal ), (long) ::GlobalSize( stg.hGlobal ) );

			long count = bytes.Read<long>();
			if ( count )
			{
				for ( long i = 0; i < count; i++ )
				{
					long objectType = bytes.Read<long>();
					long customParam = bytes.Read<long>();
					DnDType type = ConvertType( objectType, customParam );
					UINT32 uiId = bytes.Read<UINT32>();

					if( type == TypeUnknown )
					{
						result = TypeUnknown;
						break;
					}
					
					if( result == TypeUnknown )
					{
						result = type;
					}
					else if( type != result )
					{
						result = TypeUnknown;
						break;
					}
				}
			}

			::GlobalUnlock( stg.hGlobal );
            ReleaseStgMedium( &stg );
		}
	}

	return result;
}

bool CSoundFrame::ProcessEventDnD( IDataObject * in_pDataObject, IEventList ** out_ppEventList )
{
	return ProcessObjectDnD( in_pDataObject, out_ppEventList, SFType_Event );
}

bool CSoundFrame::ProcessStateGroupDnD( IDataObject * in_pDataObject, IStateGroupList ** out_ppStateGroupList	)
{
	return ProcessObjectDnD( in_pDataObject, out_ppStateGroupList, SFType_StateGroup );
}

bool CSoundFrame::ProcessSwitchGroupDnD( IDataObject * in_pDataObject, ISwitchGroupList ** out_ppSwitchGroupList )
{
	return ProcessObjectDnD( in_pDataObject, out_ppSwitchGroupList, SFType_SwitchGroup );
}

bool CSoundFrame::ProcessGameParameterDnD( IDataObject * in_pDataObject, IGameParameterList ** out_ppGameParameterList )
{
	return ProcessObjectDnD( in_pDataObject, out_ppGameParameterList, SFType_GameParameter );
}

bool CSoundFrame::ProcessTriggerDnD( IDataObject * in_pDataObject,	ITriggerList ** out_ppTriggerList )
{
	return ProcessObjectDnD( in_pDataObject, out_ppTriggerList, SFType_Trigger );
}

bool CSoundFrame::ProcessEnvironmentDnD( IDataObject * in_pDataObject,	IEnvironmentList ** out_ppEnvironmentList )
{
	return ProcessObjectDnD( in_pDataObject, out_ppEnvironmentList, SFType_Environment );
}

CSoundFrame::DnDType CSoundFrame::ConvertType( long in_lType, long in_lCustomParam )
{
	switch( in_lType )
	{
	case WEventType:
		return TypeEvent;
	case WStateGroupType:
		return TypeStates;
	case WSwitchGroupType:
		return TypeSwitches;
	case WGameParameterType:
		return TypeGameParameters;
	case WTriggerType:
		return TypeTriggers;
	case WEffectPluginType:
		// for TypeEnvironments custom Data must be 1 ...
		// If its 0 it means that it is an effect that is not environmental.
		return ( in_lCustomParam == 0 )? TypeUnknown : TypeEnvironments;
	}

	return TypeUnknown;
}

bool CSoundFrame::GetDnDObjectList( IDataObject * in_pDataObject, AkUniqueID ** out_pIDArray, long& out_lCount )
{
	FORMATETC fmt;
	fmt.cfFormat = m_cfWObjectID;
	fmt.dwAspect = DVASPECT_CONTENT;
	fmt.lindex = -1;
	fmt.ptd = NULL;
	fmt.tymed = TYMED_HGLOBAL;

	bool result = false;

	HRESULT hr = in_pDataObject->QueryGetData( &fmt );
	if ( hr == S_OK )
	{
		STGMEDIUM stg;

		hr = in_pDataObject->GetData( &fmt, &stg );
		if ( hr == S_OK )
		{
			SFReadBytesMem bytes( ::GlobalLock( stg.hGlobal ), (long) ::GlobalSize( stg.hGlobal ) );

			out_lCount = bytes.Read<long>();
			if ( out_lCount )
			{
				(*out_pIDArray) = new AkUniqueID[ out_lCount ];

				for ( long i = 0; i < out_lCount; ++i )
				{
					long type = bytes.Read<long>();
					long customParam = bytes.Read<long>();
					(*out_pIDArray)[ i ] = bytes.Read<UINT32>();
				}

				result = true;
			}

			::GlobalUnlock( stg.hGlobal );
            ReleaseStgMedium( &stg );
		}
	}

	return result;
}

bool CSoundFrame::GetSoundObject( AkUniqueID in_soundObjectID, ISoundObject ** out_ppSoundObject ) const
{
	return GetObject<ISoundObject, ISoundObjectList>( in_soundObjectID, out_ppSoundObject, SFType_SoundObject );
}

bool CSoundFrame::GetSoundObjects( const AkUniqueID * in_pSoundObjects, long in_nSoundObjects, ISoundObjectList ** out_ppSoundObjectList ) const
{
	return GetObjects( in_pSoundObjects, in_nSoundObjects, out_ppSoundObjectList, SFType_SoundObject );
}

bool CSoundFrame::GetStateGroupList( IStateGroupList ** out_ppStateGroupList ) const
{
	return GetObjectList( out_ppStateGroupList, WM_SF_REQSTATES );
}

bool CSoundFrame::GetStateGroup( AkUniqueID in_stateGroupID, IStateGroup ** out_ppStateGroup ) const
{
	return GetObject<IStateGroup, IStateGroupList>( in_stateGroupID, out_ppStateGroup, SFType_StateGroup );
}

bool CSoundFrame::GetStateGroup( LPCWSTR in_pszStateGroup, IStateGroup ** out_ppStateGroup ) const
{
	return GetObjectByString<IStateGroup, IStateGroupList>( in_pszStateGroup, out_ppStateGroup, SFType_StateGroup );
}

bool CSoundFrame::GetStateGroups( const AkUniqueID * in_pStateGroups, long in_cStateGroups, IStateGroupList ** out_ppStateGroupList ) const
{
	return GetObjects( in_pStateGroups, in_cStateGroups, out_ppStateGroupList, SFType_StateGroup );
}

bool CSoundFrame::GetStateGroups( LPCWSTR * in_pszStateGroups, long in_cStateGroups, IStateGroupList ** out_ppStateGroupList ) const
{
	return GetObjectsByString( in_pszStateGroups, in_cStateGroups, out_ppStateGroupList, SFType_StateGroup );
}

bool CSoundFrame::GetSwitchGroupList( ISwitchGroupList ** out_ppSwitchGroupList	) const
{
	return GetObjectList( out_ppSwitchGroupList, WM_SF_REQSWITCHES );
}
	
bool CSoundFrame::GetSwitchGroup( AkUniqueID in_switchGroupID,	ISwitchGroup ** out_ppSwitchGroup	) const
{
	return GetObject<ISwitchGroup, ISwitchGroupList>( in_switchGroupID, out_ppSwitchGroup, SFType_SwitchGroup );
}
	
bool CSoundFrame::GetSwitchGroup( LPCWSTR in_pszSwitchGroup, ISwitchGroup ** out_ppSwitchGroup ) const
{
	return GetObjectByString<ISwitchGroup, ISwitchGroupList>( in_pszSwitchGroup, out_ppSwitchGroup, SFType_SwitchGroup );
}
	
bool CSoundFrame::GetSwitchGroups( const AkUniqueID * in_pSwitchGroups, long in_cSwitchGroups, ISwitchGroupList ** out_ppSwitchGroupList ) const
{
	return GetObjects( in_pSwitchGroups, in_cSwitchGroups, out_ppSwitchGroupList, SFType_SwitchGroup );
}
	
bool CSoundFrame::GetSwitchGroups( LPCWSTR * in_pszSwitchGroups, long in_cSwitchGroups, ISwitchGroupList ** out_ppSwitchGroupList ) const
{
	return GetObjectsByString( in_pszSwitchGroups, in_cSwitchGroups, out_ppSwitchGroupList, SFType_SwitchGroup );
}

bool CSoundFrame::GetGameObjectList( IGameObjectList ** out_ppGameObjectList ) const
{
	return GetObjectList( out_ppGameObjectList, WM_SF_REQGAMEOBJECTS );
}

bool CSoundFrame::GetGameParameterList( IGameParameterList ** out_ppGameParameterList ) const
{
	return GetObjectList( out_ppGameParameterList, WM_SF_REQGAMEPARAMETERS );
}
	
bool CSoundFrame::GetGameParameter( AkUniqueID in_gameParameterID, IGameParameter ** out_ppGameParameter ) const
{
	return GetObject<IGameParameter, IGameParameterList>( in_gameParameterID, out_ppGameParameter, SFType_GameParameter );
}
	
bool CSoundFrame::GetGameParameter( LPCWSTR in_pszGameParameter, IGameParameter ** out_ppGameParameter ) const
{
	return GetObjectByString<IGameParameter, IGameParameterList>( in_pszGameParameter, out_ppGameParameter, SFType_GameParameter );
}
	
bool CSoundFrame::GetGameParameters( const AkUniqueID * in_pGameParameters, long in_cGameParameters, IGameParameterList ** out_ppGameParameterList ) const
{
	return GetObjects( in_pGameParameters, in_cGameParameters, out_ppGameParameterList, SFType_GameParameter );
}
	
bool CSoundFrame::GetGameParameters( LPCWSTR * in_pszGameParameters, long in_cGameParameters, IGameParameterList ** out_ppGameParameterList ) const
{
	return GetObjectsByString( in_pszGameParameters, in_cGameParameters, out_ppGameParameterList, SFType_GameParameter );
}

bool CSoundFrame::GetTriggerList( ITriggerList ** out_ppTriggerList ) const
{
	return GetObjectList( out_ppTriggerList, WM_SF_REQTRIGGERS );
}
	
bool CSoundFrame::GetTrigger( AkUniqueID in_triggerID, ITrigger ** out_ppTrigger	) const
{
	return GetObject<ITrigger, ITriggerList>( in_triggerID, out_ppTrigger, SFType_Trigger );
}
	
bool CSoundFrame::GetTrigger( LPCWSTR in_pszTrigger, ITrigger ** out_ppTrigger ) const
{
	return GetObjectByString<ITrigger, ITriggerList>( in_pszTrigger, out_ppTrigger, SFType_Trigger );
}
	
bool CSoundFrame::GetTriggers( const AkUniqueID * in_pTriggers, long in_cTriggers, ITriggerList ** out_ppTriggerList ) const
{
	return GetObjects( in_pTriggers, in_cTriggers, out_ppTriggerList, SFType_Trigger );
}
	
bool CSoundFrame::GetTriggers( LPCWSTR * in_pszTriggers, long in_cTriggers, ITriggerList ** out_ppTriggerList ) const
{
	return GetObjectsByString( in_pszTriggers, in_cTriggers, out_ppTriggerList, SFType_Trigger );
}

bool CSoundFrame::GetEnvironmentList( IEnvironmentList ** out_ppEnvironmentList ) const
{
	return GetObjectList( out_ppEnvironmentList, WM_SF_REQENVIRONMENTS );
}
	
bool CSoundFrame::GetEnvironment( AkUniqueID in_environmentID, IEnvironment ** out_ppEnvironment ) const
{
	return GetObject<IEnvironment, IEnvironmentList>( in_environmentID, out_ppEnvironment, SFType_Environment );
}
	
bool CSoundFrame::GetEnvironment( LPCWSTR in_pszEnvironment, IEnvironment ** out_ppEnvironment ) const
{
	return GetObjectByString<IEnvironment, IEnvironmentList>( in_pszEnvironment, out_ppEnvironment, SFType_Environment );
}
	
bool CSoundFrame::GetEnvironments( const AkUniqueID * in_pEnvironments, long in_cEnvironments, IEnvironmentList ** out_ppEnvironmentList ) const
{
	return GetObjects( in_pEnvironments, in_cEnvironments, out_ppEnvironmentList, SFType_Environment );
}
	
bool CSoundFrame::GetEnvironments( LPCWSTR * in_pszEnvironments, long in_cEnvironments, IEnvironmentList ** out_ppEnvironmentList ) const
{
	return GetObjectsByString( in_pszEnvironments, in_cEnvironments, out_ppEnvironmentList, SFType_Environment );
}

const WCHAR * CSoundFrame::GetCurrentProjectName() const
{
	if ( m_hwndServer && !m_bProjectInfoValid )
	{
		::SendMessage( m_hwndServer, WM_SF_REQPROJ, (WPARAM) m_hwndMessage, 0 );
	}

	return m_wszProjectName;
}

GUID CSoundFrame::GetCurrentProjectID() const
{
	if ( m_hwndServer && !m_bProjectInfoValid )
	{
		::SendMessage( m_hwndServer, WM_SF_REQPROJ, (WPARAM) m_hwndMessage, 0 );
	}

	return m_guidProject;
}

bool CSoundFrame::ProcessDefinitionFiles( LPCWSTR * in_pszPaths, long in_nFiles )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;

	bytes.Write<int>( in_nFiles );

	for ( int i = 0; i < in_nFiles; ++i )
		bytes.WriteString( in_pszPaths[ i ] );

	SendCopyData( SF_COPYDATA_PROCESSDEFINITIONFILES, bytes );

	return true;
}

bool CSoundFrame::GenerateSoundBanks( LPCWSTR * in_pszBanks, long in_nBanks, 
									  LPCWSTR * in_pszPlatforms, long in_nPlatforms,
									  LPCWSTR * in_pszLanguages, long in_nLanguages )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;

	bytes.Write<int>( in_nBanks );
	for ( int i = 0; i < in_nBanks; ++i )
		bytes.WriteString( in_pszBanks[ i ] );

	bytes.Write<int>( in_nPlatforms );
	for ( int i = 0; i < in_nPlatforms; ++i )
		bytes.WriteString( in_pszPlatforms[ i ] );

	bytes.Write<int>( in_nLanguages );
	for ( int i = 0; i < in_nLanguages; ++i )
		bytes.WriteString( in_pszLanguages[ i ] );

	SendCopyData( SF_COPYDATA_GENERATESOUNDBANKS, bytes );

	return true;
}

// -----------------------------------------------------------------------------

bool CSoundFrame::ListenAttenuation( const AkUniqueID * in_pSoundObjects, long in_nSoundObjects )
{
	if ( !m_hwndServer )
		return false;
	
	COPYDATASTRUCT sCopyData;

	sCopyData.dwData = SF_COPYDATA_LISTENRADIUSFORSOUNDS;
	sCopyData.cbData = in_nSoundObjects * sizeof( AkUniqueID );
	sCopyData.lpData = (void *) in_pSoundObjects;

	::SendMessage( m_hwndServer, WM_COPYDATA, (WPARAM) m_hwndMessage, (LPARAM) &sCopyData );

	return true;
}

// -----------------------------------------------------------------------------

void CSoundFrame::OnServerStartup( HWND in_hwndServer, ULONG in_ulServerVersion )
{
	m_bProjectInfoValid = false;
	m_wszProjectName[ 0 ] = 0;
	m_guidProject = GUID_NULL;

	m_hwndServer = in_hwndServer;
	m_ulServerVersion = in_ulServerVersion;

	::SendMessage( m_hwndServer, WM_SF_CLIENTVERSION, (WPARAM) m_hwndMessage, (LPARAM) SF_SOUNDFRAME_VERSION_CURRENT );

	if ( m_pClient )
	{
		m_pClient->OnConnect( true );
	}
}

void CSoundFrame::OnServerShutdown()
{
	m_hwndServer = NULL;
	m_ulServerVersion = 0;

	m_bProjectInfoValid = false;
	m_wszProjectName[ 0 ] = 0;
	m_guidProject = GUID_NULL;

	if ( m_pClient )
	{
		m_pClient->OnConnect( false );
	}
}

template< class TObject, class TList >
TList* CSoundFrame::GetCopyDataObjectList( SFReadBytesMem& bytes ) const
{
	long cObjects = bytes.Read<long>();

	TList * pList = new TList();
	pList->SetSize( cObjects );

	for ( int i = 0; i < cObjects; ++i )
	{
		pList->SetAt( i, TObject::From( &bytes ) );
	}

	return pList;
}

template<class TList>
bool CSoundFrame::ProcessObjectDnD( IDataObject * in_pDataObject, TList ** out_ppObjectList, SFObjectType in_eType )
{
	AkUniqueID* pIdArray = NULL;
	long lGuidCount = 0;

	bool result = GetDnDObjectList( in_pDataObject, &pIdArray, lGuidCount );

	if( result )
	{
		result = GetObjects( pIdArray, lGuidCount, out_ppObjectList, in_eType );

		delete [] pIdArray;
	}

	return result;
}

template<class TList>
bool CSoundFrame::GetObjectList( TList ** out_ppObjectList, UINT in_msg ) const
{
	if ( !out_ppObjectList ) 
		return false;

	::SendMessage( m_hwndServer, in_msg, (WPARAM) m_hwndMessage, 0 );

	if ( m_pCopyData )
	{
		*out_ppObjectList = (TList *) m_pCopyData;
		m_pCopyData = NULL;
		return true;
	}
	else
	{
		*out_ppObjectList = NULL;
		return false;
	}
}

template<class TObject, class TList>
bool CSoundFrame::GetObject( AkUniqueID in_id, TObject ** out_pObject, SFObjectType in_eType ) const
{
	if ( !out_pObject ) 
		return false;

	TList * pObjectList = NULL;
	bool result = GetObjects( &in_id, 1, &pObjectList, in_eType );
	if ( result )
	{
		*out_pObject = pObjectList->Next();
		if ( *out_pObject )
			(*out_pObject)->AddRef();
		pObjectList->Release();
	}
	else
	{
		*out_pObject = NULL;
	}

	return result;
}

template<class TObject, class TList>
bool CSoundFrame::GetObjectByString( LPCWSTR in_szObject, TObject ** out_pObject, SFObjectType in_eType ) const
{
	if ( !out_pObject ) 
		return false;

	TList * pObjectList = NULL;
	bool result = GetObjectsByString( &in_szObject, 1, &pObjectList, in_eType );
	if ( result )
	{
		*out_pObject = pObjectList->Next();
		if ( *out_pObject )
			(*out_pObject)->AddRef();
		pObjectList->Release();
	}
	else
	{
		*out_pObject = NULL;
	}

	return result;
}

template<class TList>
bool CSoundFrame::GetObjects( const AkUniqueID * in_pObjects, long in_cObjects, TList ** out_ppObjectList, SFObjectType in_eType ) const
{
	if ( !out_ppObjectList || !in_cObjects ) 
		return false;

	SFWriteBytesMem bytes;

	bytes.Write<long>( in_eType );
	bytes.Write<long>( in_cObjects );

	for( int i = 0; i < in_cObjects; ++i )
		bytes.Write<AkUniqueID>( in_pObjects[i] );

	SendCopyData( SF_COPYDATA_REQOBJECTS, bytes );

	if ( m_pCopyData )
	{
		*out_ppObjectList = (TList *) m_pCopyData;
		m_pCopyData = NULL;
		return true;
	}
	else
	{
		*out_ppObjectList = NULL;
		return false;
	}
}

template<class TList>
bool CSoundFrame::GetObjectsByString( LPCWSTR * in_pszObjects, long in_cObjects, TList ** out_ppObjectList, SFObjectType in_eType ) const
{
	if ( !out_ppObjectList || !in_cObjects ) 
		return false;

	SFWriteBytesMem bytes;

	bytes.Write<long>( in_eType );
	bytes.Write<long>( in_cObjects );

	for ( int i = 0; i < in_cObjects; ++i )
		bytes.WriteString( in_pszObjects[ i ] );

	SendCopyData( SF_COPYDATA_REQOBJECTSBYSTRING, bytes );

	if ( m_pCopyData )
	{
		*out_ppObjectList = (TList *) m_pCopyData;
		m_pCopyData = NULL;
		return true;
	}
	else
	{
		*out_ppObjectList = NULL;
		return false;
	}
}

void CSoundFrame::SendCopyData( DWORD in_dwMessageID, SFWriteBytesMem& in_rWriteByte ) const
{
	COPYDATASTRUCT sCopyData = {0};

	sCopyData.dwData = in_dwMessageID;
	sCopyData.cbData = in_rWriteByte.Count();
	sCopyData.lpData = in_rWriteByte.Bytes();

	::SendMessage( m_hwndServer, WM_COPYDATA, (WPARAM) m_hwndMessage, (LPARAM) &sCopyData );
}

BOOL CSoundFrame::ReceiveCopyData( HWND hwndSender, COPYDATASTRUCT * pCopyData )
{
	// Reject WM_COPYDATA from unknown source.
	if ( ( m_hwndServer == NULL ) || ( m_hwndServer != hwndSender ) )
		return FALSE;

	_ASSERT( m_pCopyData == NULL );

	switch( pCopyData->dwData )
	{
	case SF_COPYDATA_OBJECTLIST:
		{
			SFReadBytesMem bytes( pCopyData->lpData, pCopyData->cbData );		

			SFObjectType eType = (SFObjectType) bytes.Read<long>();

			switch( eType )
			{
			case SFType_Event:
				m_pCopyData = GetCopyDataObjectList<SFEvent, EventList>( bytes );
				break;
			case SFType_SoundObject:
				m_pCopyData = GetCopyDataObjectList<SFSoundObject, SoundObjectList>( bytes );
				break;
			case SFType_StateGroup:
				m_pCopyData = GetCopyDataObjectList<SFStateGroup, StateGroupList>( bytes );
				break;
			case SFType_SwitchGroup:
				m_pCopyData = GetCopyDataObjectList<SFSwitchGroup, SwitchGroupList>( bytes );
				break;
			case SFType_GameObject:
				m_pCopyData = GetCopyDataObjectList<SFGameObject, GameObjectList>( bytes );
				break;
			case SFType_GameParameter:
				m_pCopyData = GetCopyDataObjectList<SFGameParameter, GameParameterList>( bytes );
				break;
			case SFType_Trigger:
				m_pCopyData = GetCopyDataObjectList<SFTrigger, TriggerList>( bytes );
				break;
			case SFType_Environment:
				m_pCopyData = GetCopyDataObjectList<SFEnvironment, EnvironmentList>( bytes );
				break;
			}
		}
		break;

	case SF_COPYDATA_NOTIF:
		{
			SFReadBytesMem bytes( pCopyData->lpData, pCopyData->cbData );

			SFNotifData notifData = bytes.Read<SFNotifData>();
			
			switch ( notifData.eType )
			{
			case SFType_Event:
				m_pClient->OnEventNotif( (IClient::Notif) notifData.eNotif, notifData.objectID );
				break;

			case SFType_SoundObject:
				m_pClient->OnSoundObjectNotif( (IClient::Notif) notifData.eNotif, notifData.objectID );
				break;

			case SFType_StateGroup:
				m_pClient->OnStatesNotif( (IClient::Notif) notifData.eNotif, notifData.objectID );
				break;

			case SFType_SwitchGroup:
				m_pClient->OnSwitchesNotif( (IClient::Notif) notifData.eNotif, notifData.objectID );
				break;

			case SFType_GameObject:
				m_pClient->OnGameObjectsNotif( (IClient::Notif) notifData.eNotif, notifData.objectID );
				break;

			case SFType_GameParameter:
				m_pClient->OnGameParametersNotif( (IClient::Notif) notifData.eNotif, notifData.objectID );
				break;

			case SFType_Trigger:
				m_pClient->OnTriggersNotif( (IClient::Notif) notifData.eNotif, notifData.objectID );
				break;

			case SFType_Environment:
				m_pClient->OnEnvironmentsNotif( (IClient::Notif) notifData.eNotif, notifData.objectID );
				break;
			}
		}
		break;

	case SF_COPYDATA_PROJ:
		{
			SFReadBytesMem bytes( pCopyData->lpData, pCopyData->cbData );

			m_guidProject = bytes.Read<GUID>();
			bytes.ReadString( m_wszProjectName, kStrSize );

			m_bProjectInfoValid = true;
		}
		break;

	case SF_COPYDATA_CURRENTSTATE:
		{
			SFReadBytesMem bytes( pCopyData->lpData, pCopyData->cbData );

			m_pCopyData = (VOID*) SFState::From( &bytes, SF_PERSIST_STATEGROUP_VERSION_CURRENT );
		}
		break;

	case SF_COPYDATA_CURRENTSWITCH:
		{
			SFReadBytesMem bytes( pCopyData->lpData, pCopyData->cbData );

			m_pCopyData = (VOID*) SFSwitch::From( &bytes, SF_PERSIST_SWITCHGROUP_VERSION_CURRENT );
		}
		break;

	default: // Unknown data ! 
		return FALSE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------

LRESULT CALLBACK CSoundFrame::MsgWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	CSoundFrame * pSndFrame = (CSoundFrame *) (LONG_PTR) GetWindowLongPtr( hwnd, GWL_USERDATA ) ;
	if ( pSndFrame )
	{
		switch ( msg ) 
		{
		case WM_SF_STARTUP:
			pSndFrame->OnServerStartup( (HWND) wParam, (ULONG) lParam );
			return TRUE;

		case WM_SF_SHUTDOWN:
			pSndFrame->OnServerShutdown();
			return TRUE;

		case WM_COPYDATA:
			return pSndFrame->ReceiveCopyData( (HWND) wParam, (COPYDATASTRUCT *) lParam );
		}
	}

	return DefWindowProc( hwnd, msg, wParam, lParam );
}

// -----------------------------------------------------------------------------

bool AK::SoundFrame::Create( IClient * in_pClient, ISoundFrame ** out_ppSoundFrame )
{
	if ( !out_ppSoundFrame )
		return false;

	*out_ppSoundFrame = NULL;

	if ( !in_pClient )
		return false;

	CSoundFrame * pSoundFrame = new CSoundFrame( in_pClient );

	if ( pSoundFrame->Init() )
	{
		*out_ppSoundFrame = static_cast<ISoundFrame *>( pSoundFrame );
		return true;
	}
	else
	{
		pSoundFrame->Release();
		return false;
	}
}
