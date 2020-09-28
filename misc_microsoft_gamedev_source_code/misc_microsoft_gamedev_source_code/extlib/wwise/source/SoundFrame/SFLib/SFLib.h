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

#include <crtdbg.h>

#include <AK/IBytes.h>
#include <AK/SoundFrame/SF.h>

#include "SFPrivate.h"
#include "SFLibObjects.h"
#include "SFBytesMem.h"

using namespace AK;
using namespace SoundFrame;

//-------------------------------------------------------------------------------------------------
//  
class CSoundFrame
	: public CRefCountBase<ISoundFrame>
{
public:
	CSoundFrame( IClient * );
	~CSoundFrame();

	bool Init();

	// ISoundFrame
	virtual bool IsConnected() const;
	virtual ULONG GetClientVersion() const;
	virtual ULONG GetServerVersion() const;
	virtual bool PlayEvents( const AkUniqueID * in_pEvents, long in_cEvents, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject );
	virtual bool PlayEvents( LPCWSTR * in_pszEvents, long in_cEvents, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject );

	virtual bool SetPlayBackMode( bool in_bPlayback ) const;

	virtual bool GetCurrentState( AkStateGroupID in_stateGroupID, IState** out_ppCurrentState ) const;
	virtual bool GetCurrentState( LPCWSTR in_szStateGroupName, IState** out_ppCurrentState ) const;

	virtual bool SetCurrentState( AkStateGroupID in_stateGroupID, AkStateID in_currentStateID );
	virtual bool SetCurrentState( LPCWSTR in_szStateGroupName, LPCWSTR in_szCurrentStateName );

	virtual bool GetCurrentSwitch( AkSwitchGroupID in_switchGroupID, ISwitch** out_ppCurrentSwitch, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject ) const;
	virtual bool GetCurrentSwitch( LPCWSTR in_szSwitchGroupName, ISwitch** out_ppCurrentSwitch, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject ) const;

	virtual bool SetCurrentSwitch( AkSwitchGroupID in_switchGroupID, AkSwitchStateID in_currentSwitchID, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject );
	virtual bool SetCurrentSwitch( LPCWSTR in_szSwitchGroupName, LPCWSTR in_szCurrentSwitchName, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject );

	virtual bool RegisterGameObject( AkGameObjectID in_gameObjectID, LPCWSTR in_szGameObjectName = L"" );
	virtual bool UnregisterGameObject( AkGameObjectID in_gameObjectID );

	virtual bool SetRTPCValue( AkRtpcID in_gameParameterID, AkRtpcValue in_value, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject );
	virtual bool SetRTPCValue( LPCWSTR in_szGameParameterName, AkRtpcValue in_value, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject );

	virtual bool PostTrigger( AkTriggerID in_triggerID, AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject );
	virtual bool PostTrigger( LPCWSTR in_szTriggerName,	AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject );

	virtual bool SetActiveListeners( AkGameObjectID in_gameObjectID, AkUInt32 in_uiListenerMask );
	virtual bool SetPosition( AkGameObjectID in_gameObjectID, const AkSoundPosition& in_rPosition, AkUInt32 in_uiListenerIndex = AK_INVALID_LISTENER_INDEX );
	virtual bool SetListenerPosition( const AkListenerPosition& in_rPosition, AkUInt32 in_uiIndex = 0 );
	virtual bool SetListenerSpatialization( AkUInt32 in_uiIndex, bool in_bSpatialized, AkSpeakerVolumes * in_pVolumeOffsets = NULL );

	virtual bool SetGameObjectEnvironmentsValues( AkGameObjectID in_gameObjectID, AkEnvironmentValue* in_aEnvironmentValues, AkUInt32 in_uNumEnvValues );
	virtual bool SetGameObjectDryLevelValue( AkGameObjectID in_gameObjectID, AkReal32 in_fControlValue );
	virtual bool SetEnvironmentVolume( AkEnvID in_FXParameterSetID, AkReal32 in_fVolume );
	virtual bool BypassEnvironment( AkEnvID in_FXParameterSetID, bool in_bIsBypassed );
	virtual bool SetObjectObstructionAndOcclusion( AkGameObjectID in_ObjectID, AkUInt32 in_uListener, AkReal32 in_fObstructionLevel, AkReal32 in_fOcclusionLevel );

	virtual bool PostMsgMonitor( LPCWSTR in_pszMessage );
	virtual bool StopAll( AkGameObjectID in_GameObjID );
	virtual bool StopPlayingID( AkPlayingID in_playingID );

	virtual bool GetEventList( IEventList ** out_ppEventList ) const;
	virtual bool GetEvent( AkUniqueID in_eventID, IEvent ** out_ppEvent ) const;
	virtual bool GetEvent( LPCWSTR in_pszEvent, IEvent ** out_ppEvent ) const;
	virtual bool GetEvents( const AkUniqueID * in_pEvents, long in_cEvents, IEventList ** out_ppEventList ) const;
	virtual bool GetEvents( LPCWSTR * in_pszEvents, long in_cEvents, IEventList ** out_ppEventList ) const;

	virtual bool GetSoundObject( AkUniqueID in_soundObjectID, ISoundObject ** out_ppSoundObject ) const;
	virtual bool GetSoundObjects( const AkUniqueID * in_pSoundObjects, long in_nSoundObjects, ISoundObjectList ** out_ppSoundObjectList ) const;

	virtual bool GetStateGroupList( IStateGroupList ** out_ppStateGroupList	) const;
	virtual bool GetStateGroup( AkUniqueID in_stateGroupID,	IStateGroup ** out_ppStateGroup	) const;
	virtual bool GetStateGroup( LPCWSTR in_pszStateGroup, IStateGroup ** out_ppStateGroup ) const;
	virtual bool GetStateGroups( const AkUniqueID * in_pStateGroups, long in_cStateGroups, IStateGroupList ** out_ppStateGroupList ) const;
	virtual bool GetStateGroups( LPCWSTR * in_pszStateGroups, long in_cStateGroups, IStateGroupList ** out_ppStateGroupList ) const;

	virtual bool GetSwitchGroupList( ISwitchGroupList ** out_ppSwitchGroupList	) const;
	virtual bool GetSwitchGroup( AkUniqueID in_switchGroupID,	ISwitchGroup ** out_ppSwitchGroup	) const;
	virtual bool GetSwitchGroup( LPCWSTR in_pszSwitchGroup, ISwitchGroup ** out_ppSwitchGroup ) const;
	virtual bool GetSwitchGroups( const AkUniqueID * in_pSwitchGroups, long in_cSwitchGroups, ISwitchGroupList ** out_ppSwitchGroupList ) const;
	virtual bool GetSwitchGroups( LPCWSTR * in_pszSwitchGroups, long in_cSwitchGroups, ISwitchGroupList ** out_ppSwitchGroupList ) const;

	virtual bool GetGameObjectList( IGameObjectList ** out_ppGameObjectList	) const;

	virtual bool GetGameParameterList( IGameParameterList ** out_ppGameParameterList ) const;
	virtual bool GetGameParameter( AkUniqueID in_gameParameterID, IGameParameter ** out_ppGameParameter	) const;
	virtual bool GetGameParameter( LPCWSTR in_pszGameParameter, IGameParameter ** out_ppGameParameter ) const;
	virtual bool GetGameParameters( const AkUniqueID * in_pGameParameters, long in_cGameParameters, IGameParameterList ** out_ppGameParameterList ) const;
	virtual bool GetGameParameters( LPCWSTR * in_pszGameParameters, long in_cGameParameters, IGameParameterList ** out_ppGameParameterList ) const;

	virtual bool GetTriggerList( ITriggerList ** out_ppTriggerList ) const;
	virtual bool GetTrigger( AkUniqueID in_triggerID, ITrigger ** out_ppTrigger	) const;
	virtual bool GetTrigger( LPCWSTR in_pszTrigger, ITrigger ** out_ppTrigger ) const;
	virtual bool GetTriggers( const AkUniqueID * in_pTriggers, long in_cTriggers, ITriggerList ** out_ppTriggerList ) const;
	virtual bool GetTriggers( LPCWSTR * in_pszTriggers, long in_cTriggers, ITriggerList ** out_ppTriggerList ) const;

	virtual bool GetEnvironmentList( IEnvironmentList ** out_ppEnvironmentList ) const;
	virtual bool GetEnvironment( AkUniqueID in_environmentID, IEnvironment ** out_ppEnvironment ) const;
	virtual bool GetEnvironment( LPCWSTR in_pszEnvironment, IEnvironment ** out_ppEnvironment ) const;
	virtual bool GetEnvironments( const AkUniqueID * in_pEnvironments, long in_cEnvironments, IEnvironmentList ** out_ppEnvironmentList ) const;
	virtual bool GetEnvironments( LPCWSTR * in_pszEnvironments, long in_cEnvironments, IEnvironmentList ** out_ppEnvironmentList ) const;

	virtual DnDType GetDnDType(	IDataObject * in_pDataObject );
	virtual bool ProcessEventDnD( IDataObject * in_pDataObject, IEventList ** out_ppEventList );
	virtual bool ProcessStateGroupDnD( IDataObject * in_pDataObject, IStateGroupList ** out_ppStateGroupList );
	virtual bool ProcessSwitchGroupDnD( IDataObject * in_pDataObject, ISwitchGroupList ** out_ppSwitchGroupList );
	virtual bool ProcessGameParameterDnD( IDataObject * in_pDataObject,	IGameParameterList ** out_ppGameParameterList );
	virtual bool ProcessTriggerDnD( IDataObject * in_pDataObject,	ITriggerList ** out_ppTriggerList );
	virtual bool ProcessEnvironmentDnD( IDataObject * in_pDataObject,	IEnvironmentList ** out_ppEnvironmentList );

	virtual const WCHAR * GetCurrentProjectName() const;
	virtual GUID GetCurrentProjectID() const;

	virtual bool ProcessDefinitionFiles( LPCWSTR * in_pszPaths, long in_nFiles );
	virtual bool GenerateSoundBanks( LPCWSTR * in_pszBanks, long in_nBanks, 
				                     LPCWSTR * in_pszPlatforms, long in_nPlatforms,
									 LPCWSTR * in_pszLanguages, long in_nLanguages );

	virtual bool ListenAttenuation( const AkUniqueID * in_pSoundObjects, long in_nSoundObjects );

private:
	DnDType ConvertType( long in_lType, long in_lCustomParam );
	bool	GetDnDObjectList( IDataObject * in_pDataObject, AkUniqueID ** out_pIDArray, long& out_lCount );
	void	OnServerStartup( HWND in_hwndServer, ULONG in_ulServerVersion );
	void	OnServerShutdown();

	void	SendCopyData( DWORD in_dwMessageID, SFWriteBytesMem& in_rWriteByte ) const;
	BOOL    ReceiveCopyData( HWND hwndSender, COPYDATASTRUCT * pCopyData );

	template< class TObject, class TList >
	TList* GetCopyDataObjectList( SFReadBytesMem& bytes ) const;

	template<class TList>
	bool ProcessObjectDnD( IDataObject * in_pDataObject, TList ** out_ppObjectList, SFObjectType in_eType );

	template<class TList>
	bool GetObjectList( TList ** out_ppObjectList, UINT in_msg ) const;

	template<class TObject, class TList>
	bool GetObject( AkUniqueID in_id, TObject ** out_pObject, SFObjectType in_eType ) const;

	template<class TObject, class TList>
	bool GetObjectByString( LPCWSTR in_szObject, TObject ** out_pObject, SFObjectType in_eType ) const;

	template<class TList>
	bool GetObjects( const AkUniqueID * in_pObjects, long in_cObjects, TList ** out_ppObjectList, SFObjectType in_eType ) const;

	template<class TList>
	bool GetObjectsByString( LPCWSTR * in_pszObjects, long in_cObjects, TList ** out_ppObjectList, SFObjectType in_eType ) const;

	static LRESULT CALLBACK MsgWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

	HINSTANCE m_hInstance;
	HWND m_hwndMessage; // message window for communication.
	HWND m_hwndServer;
	ULONG m_ulServerVersion;

	IClient * m_pClient;

	mutable void * m_pCopyData; // Holds our data while we go up the call stack when receiving a WM_COPYDATA.

	CLIPFORMAT m_cfWObjectID; // Clipboard format for WObject IDs

	bool  m_bProjectInfoValid;
	WCHAR m_wszProjectName[ kStrSize ];
	GUID  m_guidProject;
};
