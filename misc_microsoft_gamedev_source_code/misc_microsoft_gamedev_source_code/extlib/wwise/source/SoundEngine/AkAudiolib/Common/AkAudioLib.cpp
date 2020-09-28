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

// AkAudioLib.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"

#include "AkMath.h"
#include "AkAudioLib.h"
#include "AkAudioMgr.h"
#include "AkEffectsMgr.h"
#include "AkLEngine.h"
#include "AkStateMgr.h"
#include "AkAudioLibIndex.h"
#include "AkRegistryMgr.h"
#include "AkRegisteredObj.h"
#include "AkEvent.h"
#include "AkDynamicSequence.h"
#include "AkDialogueEvent.h"
#include "AkParameterNode.h"
#include "AkTransitionManager.h"
#include "AkPlayingMgr.h"
#include "AkPathManager.h"
#include "AkBankMgr.h"
#include "AkMonitor.h"
#include "AkCritical.h"

#include "AkURenderer.h"
#include "AkRTPCMgr.h"
#include "AkAudiolibTimer.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/Tools/Common/AkMonitorError.h>
#include "AkActionStop.h"
#include "AkProfile.h"
#include "AkEnvironmentsMgr.h"
#include "AkAttenuationMgr.h"
#include "AkFXMemAlloc.h"
#include "AkPositionRepository.h"
#include "AkLayer.h"
#include "AkSink.h"
#include "AkFeedbackMgr.h"
#include <AK/SoundEngine/Common/AkDynamicDialogue.h>

#include "AkPoolSizes.h"
#include <AK/Tools/Common/AkFNVHash.h>

#ifdef WIN32
#include "AkCheckSSESupport.h"
#endif

#ifdef RVL_OS
#include "AkWiimoteMgr.h"
#endif

//-----------------------------------------------------------------------------
// Behavioral engine singletons.
//-----------------------------------------------------------------------------
CAkAudioLibIndex*		g_pIndex			 = NULL;
CAkAudioMgr*			g_pAudioMgr			 = NULL;
CAkStateMgr*			g_pStateMgr			 = NULL;
CAkRegistryMgr*			g_pRegistryMgr		 = NULL;
CAkBankMgr*				g_pBankManager		 = NULL;
CAkTransitionManager*	g_pTransitionManager = NULL;
CAkPathManager*			g_pPathManager		 = NULL;
CAkRTPCMgr*				g_pRTPCMgr			 = NULL;
CAkEnvironmentsMgr*		g_pEnvironmentMgr	 = NULL;
CAkPlayingMgr*			g_pPlayingMgr		 = NULL;
CAkPositionRepository*  g_pPositionRepository = NULL;

AkMemPoolId				g_DefaultPoolId = AK_INVALID_POOL_ID;

// Behavioral settings.
AkInitSettings			g_settings     		 = { 0, };

CAkLock g_csMain;

CAkInterpolation g_Interpol;

AkBehavioralExtensionCallback g_pBehavioralExtensionCallback = NULL;
AkExternalStateHandlerCallback g_pExternalStateHandler = NULL;
AkExternalBankHandlerCallback g_pExternalBankHandlerCallback = NULL;
AkExternalProfileHandlerCallback g_pExternalProfileHandlerCallback = NULL;

extern AkReal32 g_fVolumeThreshold;
extern AkReal32 g_fVolumeThresholdDB;

namespace AK
{

namespace SoundEngine
{

// Privates

static bool s_bInitialized = false;
static AkPlayingID g_PlayingID = 0;

// Forward declarations

AKRESULT CreateDefaultMemPools();
void DestroyDefaultMemPools();
AKRESULT PreInit( AkInitSettings * io_pSettings );
AKRESULT InitRenderer();

//////////////////////////////////////////////////////////////////////////////////
// MAIN PUBLIC INTERFACE IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////////////

void _MakeLower( AkChar* in_pString );// propotype to avoid no prototype warning with some compilers.
void _MakeLower( AkChar* in_pString )
{
	size_t strsize = strlen( in_pString );
	for( size_t i = 0; i < strsize; ++i )
	{
		if( in_pString[i] >= 'A' && in_pString[i] <= 'Z' )
		{
			in_pString[i] += 0x20;  
		}
	}
}

AkUInt32 HashName( AkChar * in_pString )
{
	static FNVHash<32> MainHash;
	return MainHash.Compute( (const unsigned char *) in_pString, (AkUInt32)strlen( in_pString ) );
}

AkUInt32 GetIDFromString( AkLpCtstr in_pszString )
{
	if( !in_pszString )
		return AK_INVALID_UNIQUE_ID;

	// 1- Make Ansi string.( temporary step as the input should soon become a normal char type ).

	AkUInt32 stringSize = (AkUInt32)wcslen( in_pszString );

	AkChar* pCharString = (AkChar*) alloca( (stringSize+1) * sizeof( AkChar ) );

	AkWideCharToAnsi( in_pszString, stringSize, pCharString );
	pCharString[ stringSize ] = 0;

	// 2- Make lower case.
	_MakeLower( pCharString );

	// 3- Hash the resulting string.
	return HashName( pCharString );
}

bool IsInitialized()
{
	return s_bInitialized;
}

AKRESULT Init(
    AkInitSettings *			in_pSettings,   		///< Sound engine settings. Can be NULL (use default values).
    AkPlatformInitSettings *	in_pPlatformSettings  	///< Platform specific settings. Can be NULL (use default values).
	)
{
#ifdef WIN32
	if ( !AkCheckSSESupport() )
	{
		AKASSERT( !"SSE instruction set not supported." );
        return AK_SSEInstructionsNotSupported;
	}
#endif

    // Check Memory manager.
	if ( !MemoryMgr::IsInitialized() )
    {
        AKASSERT( !"Memory manager is not initialized" );
        return AK_MemManagerNotInitialized;
    }
    // Check Stream manager.
    if ( IAkStreamMgr::Get( ) == NULL )
    {
        AKASSERT( !"Stream manager does not exist" );
        return AK_StreamMgrNotInitialized;
    }

    // Store upper engine global settings.
    if ( in_pSettings == NULL )
    {
		GetDefaultInitSettings( g_settings );
    }
    else
    {
    	// TODO. Check values, clamp to min or max.
        g_settings = *in_pSettings;
    }

	// Store lower engine global settings.
	CAkURenderer::ApplyGlobalSettings( in_pPlatformSettings );
    
    // Instantiate, initialize and assign global pointer.

	AKRESULT eResult = AK_Fail;

	AKASSERT( !s_bInitialized || !"SoundEngine::Init should be called only once" );
    if ( !s_bInitialized )
    {
		eResult = CreateDefaultMemPools();
        if ( eResult == AK_Success )
        {
            AKASSERT( g_DefaultPoolId != AK_INVALID_POOL_ID );

			eResult = PreInit( in_pSettings );
			if( eResult == AK_Success )
				eResult = InitRenderer();
			
			if ( eResult != AK_Success )
			{
                Term();
			}
			else
			{
				s_bInitialized = true;
			}
        }

        // If instantiation failed, need to destroy pools. 
        if ( !s_bInitialized )
        {
            DestroyDefaultMemPools();
        }
    }

	// WG-6434
	{
		static const char s_szRandomSeed[] = { "779AD1D9-3419-4cbf-933B-B038DF5A2818" };
		char szRandomSeed[36] = { 0 };
		::strncpy( szRandomSeed, s_szRandomSeed, sizeof szRandomSeed );
	}

    return eResult;
}

void GetDefaultInitSettings(
    AkInitSettings & out_settings   		///< Default sound engine settings returned
	)
{
	out_settings.pfnAssertHook = NULL;
	out_settings.uMaxNumPaths = DEFAULT_MAX_NUM_PATHS;
    out_settings.uMaxNumTransitions = DEFAULT_MAX_NUM_TRANSITIONS;
    out_settings.uDefaultPoolSize = DEFAULT_POOL_SIZE;
	out_settings.uCommandQueueSize = COMMAND_QUEUE_SIZE;
	out_settings.uPrepareEventMemoryPoolID = AK_INVALID_POOL_ID;
	out_settings.bEnableGameSyncPreparation = false;

#ifndef AK_OPTIMIZED
    out_settings.uMonitorPoolSize = MONITOR_POOL_SIZE;		
    out_settings.uMonitorQueuePoolSize = MONITOR_QUEUE_POOL_SIZE;
#endif
}

void GetDefaultPlatformInitSettings(
    AkPlatformInitSettings &	out_platformSettings  	///< Default platform specific settings returned
	)
{
	CAkLEngine::GetDefaultPlatformInitSettings( out_platformSettings );
}

void Term()
{
	{//Audiomanager must be stopped, then the renderer must be destroyed, then the audiomanager gets destroyed
		CAkFeedbackDeviceMgr* pMgr = CAkFeedbackDeviceMgr::Get();
		if (pMgr != NULL)
			pMgr->Stop();
			
		if (g_pAudioMgr)
		{
			g_pAudioMgr->Stop();
		}

		CAkURenderer::Term();

		if (g_pAudioMgr)
		{
			g_pAudioMgr->Term();
			AkDelete(g_DefaultPoolId,g_pAudioMgr);
			g_pAudioMgr = NULL;
		}
	}

	if(g_pBankManager)
	{
		AKVERIFY(g_pBankManager->Term() == AK_Success);
		AkDelete(g_DefaultPoolId,g_pBankManager);
		g_pBankManager = NULL;
	}

#ifdef RVL_OS
	CAkWiimoteMgr::Term();
#endif

	if (g_pStateMgr)
	{
		g_pStateMgr->Term();
		AkDelete(g_DefaultPoolId,g_pStateMgr);
		g_pStateMgr = NULL;
	}

	if(g_pPathManager)
	{
		g_pPathManager->Term();
		AkDelete(g_DefaultPoolId,g_pPathManager);
		g_pPathManager = NULL;
	}

	if(g_pTransitionManager)
	{
		g_pTransitionManager->Term();
		AkDelete(g_DefaultPoolId,g_pTransitionManager);
		g_pTransitionManager = NULL;
	}

	if (g_pRegistryMgr)
	{
		g_pRegistryMgr->Term();
		AkDelete(g_DefaultPoolId,g_pRegistryMgr);
		g_pRegistryMgr = NULL;
	}

	if(g_pPlayingMgr)
	{
		g_pPlayingMgr->Term();
		AkDelete(g_DefaultPoolId,g_pPlayingMgr);
		g_pPlayingMgr = NULL;
	}

	if(g_pPositionRepository)
	{
		g_pPositionRepository->Term();
		AkDelete(g_DefaultPoolId,g_pPositionRepository);
		g_pPositionRepository = NULL;
	}

	if(g_pEnvironmentMgr)
	{
		g_pEnvironmentMgr->Term();
		AkDelete(g_DefaultPoolId,g_pEnvironmentMgr);
		g_pEnvironmentMgr = NULL;
	}

	if(g_pRTPCMgr)
	{
		AKVERIFY(g_pRTPCMgr->Term() == AK_Success);
		AkDelete(g_DefaultPoolId,g_pRTPCMgr);
		g_pRTPCMgr = NULL;
	}

	g_Interpol.Term();

	if (g_pIndex)//IMPORTANT : g_pIndex MUST STAY ANTE-PENULTIEME DELETION OF AKINIT()!!!!!!!!!!!!!!!!!!
	{
		g_pIndex->Term();
		CAkAudioLibIndex::Destroy();
		g_pIndex = NULL;
	}

#ifndef AK_OPTIMIZED
	AkMonitor * pMonitor = AkMonitor::Get();
	if ( pMonitor )
	{
		pMonitor->StopMonitoring(); 
		AkMonitor::Destroy();
	}
#endif

	AK_PERF_TERM();
	AK_TERM_TIMERS();

    DestroyDefaultMemPools();

	s_bInitialized = false;
}

// Get the output speaker configuration.
AkChannelMask GetSpeakerConfiguration()
{
	return CAkLEngine::GetChannelMask();
}

//////////////////////////////////////////////////////////////////////////////////
// Tell the Audiolib it may now process all the events in the queue
//////////////////////////////////////////////////////////////////////////////////
AKRESULT RenderAudio()
{
	AKASSERT(g_pAudioMgr);
	return g_pAudioMgr->RenderAudio();
}

AKRESULT RegisterPlugin( 
	AkPluginType in_eType,
	AkUInt32 in_ulCompanyID,						// Company identifier (as declared in plugin description XML)
	AkUInt32 in_ulPluginID,							// Plugin identifier (as declared in plugin description XML)
    AkCreatePluginCallback	in_pCreateFunc,			// Pointer to the effect's Create() function.
    AkCreateParamCallback	in_pCreateParamFunc		// Pointer to the effect param's Create() function.
    )
{
	return CAkEffectsMgr::RegisterPlugin( in_eType, in_ulCompanyID, in_ulPluginID, in_pCreateFunc, in_pCreateParamFunc);
}

AKRESULT RegisterCodec( 
	AkUInt32 in_ulCompanyID,						// Company identifier (as declared in plugin description XML)
	AkUInt32 in_ulPluginID,							// Plugin identifier (as declared in plugin description XML)
	AkCreateFileSourceCallback in_pFileCreateFunc,	// File source creation function
	AkCreateBankSourceCallback in_pBankCreateFunc	// Bank source creation function
    )
{
	return CAkEffectsMgr::RegisterCodec( in_ulCompanyID, in_ulPluginID, in_pFileCreateFunc, in_pBankCreateFunc);
}

///////////////////////////////////////////////////////////////////////////
// RTPC
///////////////////////////////////////////////////////////////////////////
AKRESULT SetPosition( 
	AkGameObjectID in_GameObj, 
	const AkSoundPosition & in_Position,
	AkUInt32 in_ulListenerIndex
	)
{
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_GameObjPosition;

	Item.gameobjpos.GameObjID = in_GameObj;
	Item.gameobjpos.Position = in_Position;
	Item.gameobjpos.uListenerIndex = in_ulListenerIndex;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_GameObjPosition() );
}

AKRESULT SetActiveListeners(
	AkGameObjectID in_GameObj,					///< Game object.
	AkUInt32 in_uListenerMask					///< Bitmask representing active listeners. LSB = Listener 0, set to 1 means active.
	)
{
	AKASSERT(g_pAudioMgr);

	AKASSERT( !( in_uListenerMask & ~AK_ALL_LISTENERS_MASK ) || !"Invalid listener mask" );

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_GameObjActiveListeners;

	Item.gameobjactlist.GameObjID = in_GameObj;
	Item.gameobjactlist.uListenerMask = in_uListenerMask;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_GameObjActiveListeners() );
}

#ifdef RVL_OS
namespace Wii
{
AKRESULT SetActiveControllers(
	AkGameObjectID in_GameObj,
	AkUInt32 in_uActiveControllersMask
	)
{
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_GameObjActiveControllers;

	Item.gameobjactcontroller.GameObjID = in_GameObj;
	Item.gameobjactcontroller.uActiveControllerMask = in_uActiveControllersMask;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_GameObjActiveControllers() );
}

AKRESULT SetControllerVolume(
			AkUInt32 in_uControllerID,			///< Controller ID (0 to 3)
			AkReal32 in_fControllerVolume		///< Controller volume ( 0.0 = Muted, 1.0 = Volume max )
			)
{
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_ControllerVolume;

	Item.controllervolume.uControllerID = in_uControllerID;
	Item.controllervolume.fControllerVolume = in_fControllerVolume;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_ControllerVolume() );
}

} // namespace Wii
#endif

AKRESULT SetListenerPosition( 
		    const AkListenerPosition & in_Position,
		    AkUInt32 in_ulIndex /*= 0*/ //actually unused, only there in the situation we would start using miltiple listeners
		    )
{
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_ListenerPosition;

	Item.listpos.uListenerIndex = in_ulIndex;
	Item.listpos.Position = in_Position;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_ListenerPosition() );
}

AKRESULT SetListenerSpatialization(
	AkUInt32 in_ulIndex,						///< Listener index. 
	bool in_bSpatialized,
	AkSpeakerVolumes * in_pVolumeOffsets
	)
{
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_ListenerSpatialization;

	Item.listspat.uListenerIndex = in_ulIndex;
	Item.listspat.bSpatialized = in_bSpatialized;

	if ( in_pVolumeOffsets )
	{
		Item.listspat.bSetVolumes = true;
		Item.listspat.Volumes = *in_pVolumeOffsets;
	}
	else
	{
		Item.listspat.bSetVolumes = false;
	}

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_ListenerSpatialization() );
}

AKRESULT SetListenerPipeline(
	AkUInt32 in_uIndex,						///< Listener index (0: first listener, 7: 8th listener)
	bool in_bAudio,							///< True=Listens to audio events (by default it is true)
	bool in_bFeedback						///< True=Listens to feedback events (by default it is false)
	)
{
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_ListenerPipeline;

	Item.listpipe.uListenerIndex = in_uIndex;
	Item.listpipe.bAudio = in_bAudio;
	Item.listpipe.bFeedback = in_bFeedback;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_ListenerPipeline() );
}

AKRESULT SetRTPCValue( 
		    AkRtpcID in_RTPCid, 
		    AkReal32 in_Value, 
		    AkGameObjectID in_GameObj /*= AK_INVALID_GAME_OBJECT*/
		    )
{
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_RTPC;

	Item.rtpc.ID = in_RTPCid;
	Item.rtpc.Value = in_Value;
	Item.rtpc.GameObjID = in_GameObj;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_Rtpc() );
}

AKRESULT SetRTPCValue( 
		        AkLpCtstr in_pszRTPCName, 
		        AkReal32 in_Value, 
		        AkGameObjectID in_GameObj /*= AK_INVALID_GAME_OBJECT */
		        )
{
	AkRtpcID id = GetIDFromString( in_pszRTPCName );
	if ( id == AK_INVALID_RTPC_ID )
		return AK_IDNotFound;

	return SetRTPCValue( id, in_Value, in_GameObj );
}

AKRESULT SetSwitch( 
		    AkSwitchGroupID in_SwitchGroup, 
		    AkSwitchStateID in_SwitchState, 
		    AkGameObjectID in_GameObj /*= AK_INVALID_GAME_OBJECT*/
		    )
{
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_Switch;

	Item.setswitch.GameObjID = in_GameObj;
	Item.setswitch.SwitchGroupID = in_SwitchGroup;
	Item.setswitch.SwitchStateID = in_SwitchState;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_Switch() );
}

AKRESULT SetSwitch( 
		                AkLpCtstr in_pszSwitchGroup, 
		                AkLpCtstr in_pszSwitchState, 
		                AkGameObjectID in_GameObj /*= AK_INVALID_GAME_OBJECT*/ 
		                )
{
	AkSwitchGroupID l_SwitchGroup = GetIDFromString( in_pszSwitchGroup );
	AkSwitchStateID l_SwitchState = GetIDFromString( in_pszSwitchState );

	if( l_SwitchGroup != AK_INVALID_RTPC_ID && l_SwitchState != AK_INVALID_RTPC_ID )
	{
		return SetSwitch( l_SwitchGroup, l_SwitchState, in_GameObj );
	}
	else
	{
		return AK_IDNotFound;
	}
}

AKRESULT PostTrigger( 
			AkTriggerID in_Trigger, 
		    AkGameObjectID in_GameObj /*= AK_INVALID_GAME_OBJECT*/
		    )
{
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_Trigger;

	Item.trigger.GameObjID = in_GameObj;
	Item.trigger.TriggerID = in_Trigger;

	return g_pAudioMgr->Enqueue( Item, AkQueuedMsg::Sizeof_Trigger() );
}

AKRESULT PostTrigger( 
			AkLpCtstr in_pszTrigger, 
			AkGameObjectID in_GameObj /*= AK_INVALID_GAME_OBJECT*/ 
		                )
{
	AkTriggerID l_TriggerID = GetIDFromString( in_pszTrigger );

	if( l_TriggerID != AK_INVALID_UNIQUE_ID  )
	{
		return PostTrigger( l_TriggerID, in_GameObj );
	}
	else
	{
		return AK_IDNotFound;
	}
}

AKRESULT SetState( AkStateGroupID in_StateGroup, AkStateID in_State )
{
	return SetState( in_StateGroup, in_State, false, false );
}

AKRESULT SetState( AkStateGroupID in_StateGroup, AkStateID in_State, bool in_bSkipTransitionTime, bool in_bSkipExtension )
{
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item;

	// none is a reserved name in Wwise and no name can be named none, except the none state.
	#define AK_HASH_STATE_NONE 748895195 // This is the hash of "none" by GetIDFromString( "none" )
	// We still accept state = 0, for backward compatibility.
	// Letting this assert to catch errors if the hash algorithm changes
	AKASSERT( HashName( "none" ) == AK_HASH_STATE_NONE );

	if( in_State == AK_HASH_STATE_NONE )
		in_State = 0;

	Item.type = QueuedMsgType_State;

	Item.setstate.StateGroupID = in_StateGroup;
	Item.setstate.StateID = in_State;
	Item.setstate.bSkipTransition = in_bSkipTransitionTime;
    Item.setstate.bSkipExtension = in_bSkipExtension;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_State() );
}

AKRESULT SetState( AkLpCtstr in_pszStateGroup, AkLpCtstr in_pszState )
{
	AkStateGroupID	l_StateGroup	= GetIDFromString( in_pszStateGroup );
	AkStateID		l_State			= GetIDFromString( in_pszState );

	if( l_StateGroup != AK_INVALID_UNIQUE_ID && l_State != AK_INVALID_UNIQUE_ID )
	{
		return SetState( l_StateGroup, l_State );
	}
	else
	{
		return AK_IDNotFound;
	}
}

AKRESULT ResetSwitches( AkGameObjectID in_GameObjID )
{
	AKASSERT( g_pAudioMgr );

	AkQueuedMsg Item;
	Item.type = QueuedMsgType_ResetSwitches;
	Item.resetswitches.GameObjID = in_GameObjID;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_ResetSwitches() );
}

AKRESULT ResetRTPC( AkGameObjectID in_GameObjID )
{
	AKASSERT( g_pAudioMgr );

	AkQueuedMsg Item;
	Item.type = QueuedMsgType_ResetRTPC;
	Item.resetrtpc.GameObjID = in_GameObjID;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_ResetRTPC() );
}

AKRESULT SetGameObjectEnvironmentsValues( 
		AkGameObjectID		in_GameObj,				///< the unique object ID
		AkEnvironmentValue*	in_aEnvironmentValues,	///< variable-size array of AkEnvironmentValue(s)
		AkUInt32			in_uNumEnvValues		///< number of elements in struct
		)
{
	AKASSERT(g_pAudioMgr);

	if( in_uNumEnvValues > AK_MAX_ENVIRONMENTS_PER_OBJ )
		return AK_InvalidParameter;

	if( in_uNumEnvValues )
	{
		// It would cause unhandled situations if an environment value was duplicated in the 
		// array, making the system unable to transit from one environment state to another.
		for( AkUInt32 i = 0; i < in_uNumEnvValues - 1; ++i )
		{
			for( AkUInt32 j = (i + 1); j < in_uNumEnvValues; ++j )
			{
				if( in_aEnvironmentValues[i].EnvID == in_aEnvironmentValues[j].EnvID )
					return AK_InvalidParameter;
			}
		}
	}

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_GameObjEnvValues;

	Item.gameobjenvvalues.GameObjID = in_GameObj;
	Item.gameobjenvvalues.uNumValues = in_uNumEnvValues;
	AKPLATFORM::AkMemCpy( Item.gameobjenvvalues.EnvValues, in_aEnvironmentValues, sizeof( AkEnvironmentValue ) * in_uNumEnvValues );

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_GameObjEnvValues() );
}

AKRESULT SetGameObjectDryLevelValue( 
		AkGameObjectID		in_GameObj,			///< the unique object ID
		AkReal32			in_fControlValue	///< control value for dry level
		)
{
	AKASSERT( g_pAudioMgr );

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_GameObjDryLevel;

	Item.gameobjdrylevel.GameObjID = in_GameObj;
	Item.gameobjdrylevel.fValue = in_fControlValue;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_GameObjDryLevel() );
}

AKRESULT SetEnvironmentVolume( 
		AkEnvID				in_FXParameterSetID,	///< Environment ID.
		AkReal32			in_fVolume				///< Volume control value.
		)
{
	AKASSERT( g_pAudioMgr );

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_EnvVolume;

	Item.envvolume.EnvID = in_FXParameterSetID;
	Item.envvolume.fVolume = in_fVolume;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_EnvVolume() );
}

AKRESULT BypassEnvironment(
		AkEnvID	in_FXParameterSetID, 
		bool	in_bIsBypassed
		)
{
	AKASSERT( g_pAudioMgr );

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_EnvBypass;

	Item.envbypass.EnvID = in_FXParameterSetID;
	Item.envbypass.bBypass = in_bIsBypassed;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_EnvBypass() );
}

AKRESULT SetObjectObstructionAndOcclusion(  
	AkGameObjectID in_ObjectID,           ///< Game object ID.
	AkUInt32 in_uListener,             ///< Listener ID.
	AkReal32 in_fObstructionLevel,		///< ObstructionLevel : [0.0f..1.0f]
	AkReal32 in_fOcclusionLevel			///< OcclusionLevel   : [0.0f..1.0f]
	)
{
	AKASSERT( g_pAudioMgr );

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_GameObjObstruction;

	Item.gameobjobstr.GameObjID = in_ObjectID;
	Item.gameobjobstr.uListenerIndex = in_uListener;
	Item.gameobjobstr.fObstructionLevel = in_fObstructionLevel;
	Item.gameobjobstr.fOcclusionLevel = in_fOcclusionLevel;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_GameObjObstruction() );
}

extern void SetVolumeThreshold( AkReal32 in_fVolumeThresholdDB )
{
	// To protect against numerical errors, use our three methods of doing the dB-to-Lin conversion
	// and pick the highest mark as our linear threshold.

	AkReal32 fThresholdLinA = powf( 10.0f, in_fVolumeThresholdDB * 0.05f );
	AkReal32 fThresholdLinB = AkMath::dBToLin( in_fVolumeThresholdDB );

	AkSIMDSpeakerVolumes vLin;
	vLin.Set( in_fVolumeThresholdDB );
	vLin.dBToLin();
	AkReal32 fThresholdLinC = vLin.volumes.fFrontLeft;

	g_fVolumeThreshold = AkMath::Max( AkMath::Max( fThresholdLinA, fThresholdLinB ), fThresholdLinC );
	g_fVolumeThresholdDB = in_fVolumeThresholdDB;
	AKASSERT( g_fVolumeThreshold <= 1.0f && g_fVolumeThreshold >=0.0f );
}

//////////////////////////////////////////////////////////////////////////////////
//Monitoring
//////////////////////////////////////////////////////////////////////////////////
#ifndef AK_OPTIMIZED
IALMonitor* GetMonitor()
{
	return AkMonitor::Get();
}
#else
IALMonitor* GetMonitor()
{
	// No monitoring in optimized mode
	// Get Monitor should be removed from the SDK...
	return NULL;
}
#endif

//////////////////////////////////////////////////////////////////////////////////
// Event Management
//////////////////////////////////////////////////////////////////////////////////

AkPlayingID PostEvent(
	AkUniqueID in_ulEventID,
	AkGameObjectID in_GameObjID,
	AkUInt32 in_uiFlags,// = 0
	AkCallbackFunc in_pfnCallback, // = NULL
	void* in_pCookie // = NULL
	)
{
	return PostEvent( in_ulEventID, in_GameObjID, in_uiFlags, in_pfnCallback, in_pCookie, NULL );
}

AkPlayingID PostEvent(
	AkUniqueID in_ulEventID,
	AkGameObjectID in_GameObjID,
	AkUInt32 in_uiFlags,
	AkCallbackFunc in_pfnCallback,
	void* in_pCookie,
	AkCustomParamType * in_pCustomParam
	)
{
	AKASSERT(g_pIndex);
	AKASSERT(g_pAudioMgr);
	AkQueuedMsg Item;
	Item.type = QueuedMsgType_Event;
    if ( in_pCustomParam != NULL )
    {
	    Item.event.CustomParam = *in_pCustomParam;
    }
    else
    {
        Item.event.CustomParam.customParam = 0;
	    Item.event.CustomParam.ui32Reserved = 0;
    }

	// This function call does get the pointer and addref it in an atomic call. 
	// Item.event.Event will be filled with NULL if it was not possible. 
	Item.event.Event = g_pIndex->m_idxEvents.GetPtrAndAddRef( in_ulEventID ); 

	if( Item.event.Event )
	{
		Item.event.PlayingID = ++g_PlayingID;
		Item.event.GameObjID = in_GameObjID;
		AKRESULT eResult = g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_Event() );
		if(eResult != AK_Success)
		{
			return AK_INVALID_PLAYING_ID;
		}
#ifdef AK_OPTIMIZED
		if( in_pfnCallback || ( in_uiFlags & AK_EnableGetSourcePlayPosition ) )
#endif
		{
			if( g_pPlayingMgr )
			{
				eResult = g_pPlayingMgr->AddPlayingID( Item.event, in_pfnCallback, in_pCookie, in_uiFlags, Item.event.Event->ID() );
				if( eResult != AK_Success )
				{
					// Here we had a "chicken or the egg" situation.
					// The event has been successfully sent, but we cannot track its end.
					// We then return an error code, so the user will know the event failed.
					// The playback will fail since the PBI will refuse to instantiate if the playing ID is required and not existing
					return AK_INVALID_PLAYING_ID;
				}
			}
		}

		return Item.event.PlayingID;
	}
	else
	{
		MONITOR_ERROR( AK::Monitor::ErrorCode_IDNotFound );
		return AK_INVALID_PLAYING_ID;
	}
}

AkPlayingID PostEvent(
	AkLpCtstr in_pszEventName,
	AkGameObjectID in_GameObjID,
	AkUInt32 in_uiFlags,
	AkCallbackFunc in_pfnCallback,
	void* in_pCookie
	)
{
	AkUniqueID EventID = GetIDFromString( in_pszEventName );
    AkPlayingID returnedPlayingID = PostEvent( EventID, in_GameObjID, in_uiFlags, in_pfnCallback, in_pCookie, NULL );
	if( returnedPlayingID == AK_INVALID_PLAYING_ID )
	{
		MONITOR_ERRORMSG2( L"Failed posting event: ", in_pszEventName );
	}
	return returnedPlayingID;
}

void CancelEventCallbackCookie( void* in_pCookie )
{
	if( g_pPlayingMgr )
		g_pPlayingMgr->CancelCallbackCookie( in_pCookie );
}


void CancelEventCallback( AkPlayingID in_playingID )
{
	if( g_pPlayingMgr )
		g_pPlayingMgr->CancelCallback( in_playingID );
}

AKRESULT GetSourcePlayPosition( 
	AkPlayingID		in_PlayingID,				///< PlayingID returned by PostEvent
	AkTimeMs*		out_puPosition				///< Position (in ms) of the source associated with that PlayingID
	)
{
	if( out_puPosition == NULL )
		return AK_InvalidParameter;

	return g_pPositionRepository->GetCurrPosition( in_PlayingID, (AkUInt32*)out_puPosition );
}

//////////////////////////////////////////////////////////////////////////////////
// Dynamic Dialog
//////////////////////////////////////////////////////////////////////////////////
namespace DynamicDialogue
{

AkUniqueID ResolveDialogueEvent(
		AkUniqueID			in_eventID,					///< Unique ID of the dialog event
		AkArgumentValueID*	in_aArgumentValues,			///< Array of argument value IDs
		AkUInt32			in_uNumArguments			///< Number of argument value IDs in in_aArguments
	)
{
	CAkDialogueEvent * pDialogueEvent = g_pIndex->m_idxDialogueEvents.GetPtrAndAddRef( in_eventID );
	if ( !pDialogueEvent )
		return AK_INVALID_UNIQUE_ID;

	AkUniqueID audioNodeID = pDialogueEvent->ResolvePath( in_aArgumentValues, in_uNumArguments );

	MONITOR_RESOLVEDIALOGUE( in_eventID, audioNodeID, in_uNumArguments, in_aArgumentValues );

	pDialogueEvent->Release();

	return audioNodeID;
}

AkUniqueID ResolveDialogueEvent(
		AkLpCtstr			in_pszEventName,			///< Name of the dialog event
		AkLpCtstr*			in_aArgumentValueNames,		///< Array of argument value names
		AkUInt32			in_uNumArguments			///< Number of argument value names in in_aArguments
	)
{
	AkUniqueID eventID = GetIDFromString( in_pszEventName );

	CAkDialogueEvent * pDialogueEvent = g_pIndex->m_idxDialogueEvents.GetPtrAndAddRef( eventID );
	if ( !pDialogueEvent )
	{
		MONITOR_ERRORMSG2( L"Unknown Dialogue Event: ", in_pszEventName );
		return AK_INVALID_UNIQUE_ID;
	}

	AkArgumentValueID * pArgumentValues = (AkArgumentValueID *) alloca( in_uNumArguments * sizeof( AkArgumentValueID ) );

	AkUniqueID audioNodeID = AK_INVALID_UNIQUE_ID;

	AKRESULT eResult = pDialogueEvent->ResolveArgumentValueNames( in_aArgumentValueNames, pArgumentValues, in_uNumArguments );
	if ( eResult == AK_Success )
	{
		audioNodeID = pDialogueEvent->ResolvePath( pArgumentValues, in_uNumArguments );

		MONITOR_RESOLVEDIALOGUE( eventID, audioNodeID, in_uNumArguments, pArgumentValues );
	}

	pDialogueEvent->Release();

	return audioNodeID;
}

} // namespace DynamicDialogue

//////////////////////////////////////////////////////////////////////////////////
// Dynamic Sequence
//////////////////////////////////////////////////////////////////////////////////
namespace DynamicSequence
{
AkPlayingID Open(
	AkGameObjectID	in_gameObjectID,
	AkUInt32		in_uiFlags /* = 0 */,
	AkCallbackFunc	in_pfnCallback /* = NULL*/,
	void* 			in_pCookie	   /* = NULL */,
	DynamicSequenceType in_eDynamicSequenceType /*= DynamicSequenceType_SampleAccurate*/
	)
{
	AKASSERT(g_pIndex);
	AKASSERT(g_pAudioMgr);
	AkQueuedMsg Item;

	Item.type = QueuedMsgType_OpenDynamicSequence;

	Item.opendynamicsequence.PlayingID = ++g_PlayingID;
	Item.opendynamicsequence.pDynamicSequence = CAkDynamicSequence::Create( Item.opendynamicsequence.PlayingID, in_eDynamicSequenceType );

	if( Item.opendynamicsequence.pDynamicSequence )
	{
		Item.opendynamicsequence.GameObjID = in_gameObjectID;
		Item.opendynamicsequence.CustomParam.customParam = 0;
		Item.opendynamicsequence.CustomParam.ui32Reserved = 0;

		AKRESULT eResult = g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_OpenDynamicSequence() );
		if(eResult != AK_Success)
		{
			return AK_INVALID_PLAYING_ID;
		}
#ifdef AK_OPTIMIZED
		if( in_pfnCallback || ( in_uiFlags & AK_EnableGetSourcePlayPosition ) )
#endif
		{
			if( g_pPlayingMgr )
			{
				eResult = g_pPlayingMgr->AddPlayingID( Item.opendynamicsequence, in_pfnCallback, in_pCookie, in_uiFlags, Item.opendynamicsequence.PlayingID );
				if( eResult != AK_Success )
				{
					// Here we had a "chicken or the egg" situation.
					// The event has been successfully sent, but we cannot track its end.
					// We then return an error code, so the user will know the event failed.
					// The playback will fail since the PBI will refuse to instantiate if the playing ID is required and not existing
					return AK_INVALID_PLAYING_ID;
				}
			}
		}

		return Item.opendynamicsequence.PlayingID;
	}
	else
	{
		return AK_INVALID_PLAYING_ID;
	}
}

AKRESULT _DynamicSequenceCommand( AkPlayingID in_playingID, AkQueuedMsg_DynamicSequenceCmd::Command in_eCommand )
{
	AKASSERT(g_pIndex);
	AKASSERT(g_pAudioMgr);
	AkQueuedMsg Item;
	Item.type = QueuedMsgType_DynamicSequenceCmd;

	Item.dynamicsequencecmd.pDynamicSequence = g_pIndex->m_idxDynamicSequences.GetPtrAndAddRef( in_playingID ); 

	if( Item.dynamicsequencecmd.pDynamicSequence && !Item.dynamicsequencecmd.pDynamicSequence->WasClosed() )
	{
		if( in_eCommand == AkQueuedMsg_DynamicSequenceCmd::Close )
		{
			// Must do it here. to prevent another call to succeed.
			// this operation can be processed from here without locking
			Item.dynamicsequencecmd.pDynamicSequence->Close();
			
			// Then we still have to push the close command so that the release is done to free the ressources.
		}

		Item.dynamicsequencecmd.eCommand = in_eCommand;
		return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_DynamicSequenceCmd() );
	}
	else
	{
		MONITOR_ERROR( AK::Monitor::ErrorCode_IDNotFound );
		return AK_Fail;
	}
}

AKRESULT Play( AkPlayingID in_playingID )
{
	return _DynamicSequenceCommand( in_playingID, AkQueuedMsg_DynamicSequenceCmd::Play );
}

AKRESULT Pause( AkPlayingID in_playingID )
{
	return _DynamicSequenceCommand( in_playingID, AkQueuedMsg_DynamicSequenceCmd::Pause );
}

AKRESULT Resume( AkPlayingID in_playingID )
{
	return _DynamicSequenceCommand( in_playingID, AkQueuedMsg_DynamicSequenceCmd::Resume );
}

AKRESULT Stop( AkPlayingID	in_playingID )
{
	return _DynamicSequenceCommand( in_playingID, AkQueuedMsg_DynamicSequenceCmd::Stop );
}

AKRESULT Break( AkPlayingID	in_playingID )
{
	return _DynamicSequenceCommand( in_playingID, AkQueuedMsg_DynamicSequenceCmd::Break );
}

AKRESULT Close( AkPlayingID in_playingID )
{
	return _DynamicSequenceCommand( in_playingID, AkQueuedMsg_DynamicSequenceCmd::Close );
}

Playlist * LockPlaylist(
	AkPlayingID in_playingID						///< AkPlayingID returned by DynamicSequence::Open
	)
{
	CAkDynamicSequence * pDynaSeq = g_pIndex->m_idxDynamicSequences.GetPtrAndAddRef( in_playingID ); 
	if ( !pDynaSeq )
	{
		MONITOR_ERROR( AK::Monitor::ErrorCode_IDNotFound );
		return NULL;
	}

	Playlist * pPlaylist = pDynaSeq->LockPlaylist();

	pDynaSeq->Release();

	return pPlaylist;
}

AKRESULT UnlockPlaylist(
	AkPlayingID in_playingID						///< AkPlayingID returned by DynamicSequence::Open
	)
{
	CAkDynamicSequence * pDynaSeq = g_pIndex->m_idxDynamicSequences.GetPtrAndAddRef( in_playingID ); 
	if ( !pDynaSeq )
	{
		MONITOR_ERROR( AK::Monitor::ErrorCode_IDNotFound );
		return AK_Fail;
	}

	pDynaSeq->UnlockPlaylist();

	pDynaSeq->Release();

	return AK_Success;
}

} // namespace DynamicSequence
//////////////////////////////////////////////////////////////////////////////////
// Game Objects
//////////////////////////////////////////////////////////////////////////////////
AKRESULT RegisterGameObj( AkGameObjectID in_GameObj )
{
	if ( in_GameObj == 0 || in_GameObj == AK_INVALID_GAME_OBJECT ) // omni
		return AK_Fail;

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_RegisterGameObj;
	Item.reggameobj.GameObjID = in_GameObj;
	Item.reggameobj.pMonitorData = NULL;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_RegisterGameObj() );
}

AKRESULT RegisterGameObj( AkGameObjectID in_GameObj, const AkChar* in_pszObjName )
{
	if ( in_GameObj == 0 || in_GameObj == AK_INVALID_GAME_OBJECT ) // omni
		return AK_Fail;

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_RegisterGameObj;
	Item.reggameobj.GameObjID = in_GameObj;

#ifndef AK_OPTIMIZED
	if ( in_pszObjName )
		Item.reggameobj.pMonitorData = AkMonitor::Monitor_AllocateGameObjNameString( in_GameObj, in_pszObjName );
	else
		Item.reggameobj.pMonitorData = NULL;
#else
	Item.reggameobj.pMonitorData = NULL;
#endif

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_RegisterGameObj() );
}

AKRESULT UnregisterGameObj( AkGameObjectID in_GameObj )
{
	if ( in_GameObj == 0 ) // omni
		return AK_Fail;

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_UnregisterGameObj;
	Item.unreggameobj.GameObjID = in_GameObj;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_UnregisterGameObj() );
}

AKRESULT UnregisterAllGameObj()
{
	AkQueuedMsg Item;

	Item.type = QueuedMsgType_UnregisterGameObj;
	Item.unreggameobj.GameObjID = AK_INVALID_GAME_OBJECT;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_UnregisterGameObj() );
}

//////////////////////////////////////////////////////////////////////////////////
// Bank Management
//////////////////////////////////////////////////////////////////////////////////
AKRESULT ClearBanks()
{
	if( !g_pBankManager )
		return AK_Fail;

	// We must first clear prepared events prior clearing all banks.
	AKRESULT eResult = ClearPreparedEvents();

	if( eResult == AK_Success )
	{
		AkSyncLoader syncLoader;
		AKRESULT eResult = syncLoader.Init();
		if( eResult != AK_Success )
			return eResult;

		CAkBankMgr::AkBankQueueItem item;
		item.eType						 = CAkBankMgr::QueueItemClearBanks;
		item.callbackInfo.pfnBankCallback = DefaultBankCallbackFunc;
		item.callbackInfo.pCookie		 = syncLoader.GetCookie();

		eResult = g_pBankManager->QueueBankCommand( item );

		return syncLoader.Wait( eResult );
	}

	return eResult;
}

// Bank loading I/O settings.
AKRESULT SetBankLoadIOSettings(
    AkReal32            in_fThroughput,         // Average throughput of bank data streaming. Default is AK_DEFAULT_BANKLOAD_THROUGHPUT.
    AkPriority          in_priority             // Priority of bank streaming. Default is AK_DEFAULT_PRIORITY.
    )
{
    AKRESULT eResult = AK_Fail;

	AKASSERT( g_pBankManager );

	if( g_pBankManager )
	{
		eResult = g_pBankManager->SetBankLoadIOSettings(
            in_fThroughput,
            in_priority );
	}

	return eResult;
}

////////////////////////////////////////////////////////////////////////////
// Banks
////////////////////////////////////////////////////////////////////////////

AKRESULT AkSyncLoader::Init()
{
	using namespace AKPLATFORM;

	if ( AkCreateEvent( m_hEvent ) != AK_Success )
	{
		AKASSERT( !"Could not create synchronisation event" );
		return AK_Fail;
	}

	m_SyncLoadInfo.phEvent = &m_hEvent;
	return AK_Success;
}

AKRESULT AkSyncLoader::Wait( AKRESULT in_eResult )
{
	using namespace AKPLATFORM;

	if ( in_eResult != AK_Success )
	{
		AkDestroyEvent( m_hEvent );
		return in_eResult;
	}

	// Task queueing successful. Block until completion.
	AkWaitForEvent( m_hEvent );
	AkDestroyEvent( m_hEvent );

	return m_SyncLoadInfo.eLoadResult;
}

void DefaultBankCallbackFunc(
                    AkBankID    in_bankID, 
                    AKRESULT	in_eLoadResult,
                    AkMemPoolId in_memPoolId,
					void *		in_pCookie )
{
    AKASSERT( in_pCookie != NULL );
    if ( in_pCookie != NULL )
    {
        AkSyncLoadInfo * pSyncLoadInfo = (AkSyncLoadInfo*)in_pCookie;

        // Fill status info.
        pSyncLoadInfo->eLoadResult  = in_eLoadResult;
        pSyncLoadInfo->memPoolId    = in_memPoolId;

        // Signal event.
		AkSignalEvent( *pSyncLoadInfo->phEvent );
    }
}

// Load/Unload Bank Internal.
AKRESULT LoadBankInternal(
	AkBankID            in_bankID,              // ID of the bank to load.
	AkBankLoadFlag		in_flag,
	CAkBankMgr::AkBankQueueItemType	in_eType,	// load or unload
    AkBankCallbackFunc  in_pfnBankCallback,	    // Callback function that will be called.
	void *              in_pCookie,				// Callback cookie.
	AkMemPoolId         in_memPoolId,           // Memory pool ID. Pool is created inside if AK_INVALID_POOL_ID.
	void *				in_pInMemoryBank = NULL,	// Pointer to an in-memory bank
	AkUInt32			in_ui32InMemoryBankSize = 0 // Size of the specified in-memory bank
	)
{
    CAkBankMgr::AkBankQueueItem item;
	item.eType					= in_eType;
	item.load.BankID			= in_bankID;
    item.load.memPoolId			= in_memPoolId;
    item.callbackInfo.pfnBankCallback	= in_pfnBankCallback;
    item.callbackInfo.pCookie			= in_pCookie;
	item.load.pInMemoryBank		= in_pInMemoryBank;
	item.load.ui32InMemoryBankSize = in_ui32InMemoryBankSize;
	item.load.bankLoadFlag		= in_flag;

	return g_pBankManager->QueueBankCommand( item );
}

AKRESULT PrepareEventInternal(
	PreparationType		in_PreparationType,
    AkBankCallbackFunc  in_pfnBankCallback,	// Callback function that will be called.
	void *              in_pCookie,			// Callback cookie.
	AkUniqueID*			in_pEventID,
	AkUInt32			in_uNumEvents,
	bool				in_bDoAllocAndCopy = true // When set to false, the provided array can be used as is and must be freed.
	)
{
	if( in_uNumEvents == 0 )
		return AK_InvalidParameter;

    CAkBankMgr::AkBankQueueItem item;
	item.eType						= ( in_PreparationType == Preparation_Load ) ? CAkBankMgr::QueueItemPrepareEvent : CAkBankMgr::QueueItemUnprepareEvent;
	item.callbackInfo.pCookie			= in_pCookie;
	item.callbackInfo.pfnBankCallback	= in_pfnBankCallback;

	item.prepare.numEvents = in_uNumEvents;

	if( in_uNumEvents == 1 )
	{
		item.prepare.eventID = *in_pEventID;
	}
	else if( in_bDoAllocAndCopy )
	{
		item.prepare.pEventID = (AkUniqueID*)AkAlloc( g_DefaultPoolId, in_uNumEvents * sizeof( AkUniqueID ) );
		if( !item.prepare.pEventID )
		{
			return AK_InsufficientMemory;
		}
		memcpy( item.prepare.pEventID, in_pEventID, in_uNumEvents * sizeof( AkUniqueID ) );
	}
	else
	{
		item.prepare.pEventID = in_pEventID;
	}

	AKRESULT eResult = g_pBankManager->QueueBankCommand( item );

	if( eResult != AK_Success && in_uNumEvents != 1 )
	{
		AKASSERT( item.prepare.pEventID );
		AkFree( g_DefaultPoolId, item.prepare.pEventID );
	}

	return eResult;
}

AKRESULT PrepareGameSyncsInternal(
    AkBankCallbackFunc  in_pfnBankCallback,	    // Callback function that will be called.
	void *              in_pCookie,				// Callback cookie.
	bool				in_bSupported,
	AkGroupType			in_eGroupType,
	AkUInt32			in_GroupID,
	AkUInt32*			in_pGameSyncID,
	AkUInt32			in_uNumGameSync,
	bool				in_bDoAllocAndCopy = true // When set to false, the provided array can be used as is and must be freed.
	)
{
	if( in_uNumGameSync == 0 || in_pGameSyncID == NULL )
		return AK_InvalidParameter;

	CAkBankMgr::AkBankQueueItem item;
	item.eType						= CAkBankMgr::QueueItemSupportedGameSync;
    item.callbackInfo.pfnBankCallback	= in_pfnBankCallback;
    item.callbackInfo.pCookie			= in_pCookie;
	item.gameSync.bSupported		= in_bSupported;
	item.gameSync.eGroupType		= in_eGroupType;
	item.gameSync.uGroupID			= in_GroupID;

	item.gameSync.uNumGameSync = in_uNumGameSync;

	if( in_uNumGameSync == 1 )
	{
		item.gameSync.uGameSyncID = *in_pGameSyncID;
	}
	else if( in_bDoAllocAndCopy )
	{
		item.gameSync.pGameSyncID = (AkUInt32*)AkAlloc( g_DefaultPoolId, in_uNumGameSync * sizeof( AkUInt32 ) );
		if( !item.gameSync.pGameSyncID )
		{
			return AK_InsufficientMemory;
		}
		memcpy( item.gameSync.pGameSyncID, in_pGameSyncID, in_uNumGameSync * sizeof( AkUInt32 ) );
	}
	else
	{
		item.gameSync.pGameSyncID = in_pGameSyncID;
	}

	AKRESULT eResult = g_pBankManager->QueueBankCommand( item );

	if( eResult != AK_Success && in_uNumGameSync != 1 )
	{
		AKASSERT( item.gameSync.pGameSyncID );
		AkFree( g_DefaultPoolId, item.gameSync.pGameSyncID );
	}

	return eResult;
}

void RemoveFileExtension( AkTChar* in_pstring );// propotype to avoid no prototype warning with some compilers.
void RemoveFileExtension( AkTChar* in_pstring )
{
	while( *in_pstring != 0 )
	{
		if( *in_pstring == L'.' )
		{
			*in_pstring = 0;
			return;
		}
		++in_pstring;
	}
}

AkBankID GetBankIDFromString( AkLpCtstr in_pszString );// propotype to avoid no prototype warning with some compilers.
AkBankID GetBankIDFromString( AkLpCtstr in_pszString )
{
	// Remove the file extension of it was provided.
	AkUInt32 stringSize = (AkUInt32)wcslen( in_pszString );
	AkTChar* pStringWithoutExtension = (AkTChar *) alloca( (stringSize+1) * sizeof( AkTChar ) );
	wcscpy( pStringWithoutExtension, in_pszString );
	RemoveFileExtension( pStringWithoutExtension );

	// Get the ID from the resulting string
	AkBankID bankID = GetIDFromString( pStringWithoutExtension );

	// Convert to char* so it is conform with UpdateBankName(...)
	AkChar* pCharString = (AkChar*) alloca( (stringSize+1) * sizeof( AkChar ) );
	AkWideCharToAnsi( pStringWithoutExtension, stringSize, pCharString );
	pCharString[ stringSize ] = 0;

	// Call UpdateBankName(...)
	g_pBankManager->UpdateBankName( bankID, pCharString );

	return bankID;
}

// Synchronous bank load (by string).
AKRESULT LoadBank(
	    AkLpCtstr           in_pszString,		    // Name/path of the bank to load
        AkMemPoolId         in_memPoolId, 			// Memory pool ID. Pool is created inside if AK_INVALID_POOL_ID.
        AkBankID &          out_bankID				// Returned bank ID.
	    )
{
	out_bankID = GetBankIDFromString( in_pszString );

	AkSyncLoader syncLoader;
	AKRESULT eResult = syncLoader.Init();
	if( eResult != AK_Success )
		return eResult;

    eResult = LoadBankInternal(
	                        out_bankID,             // ID of the bank to load.
							AkBankLoadFlag_None,
                            CAkBankMgr::QueueItemLoad,
                            DefaultBankCallbackFunc,// Callback function that will be called.
							syncLoader.GetCookie(), // Callback cookie.
	                        in_memPoolId            // Pool ID.
	                        );

	return syncLoader.Wait( eResult );
}


// Synchronous bank load (by id).
AKRESULT LoadBank(
	    AkBankID			in_bankID,              // Bank ID of the bank to load
        AkMemPoolId         in_memPoolId			// Memory pool ID. Pool is created inside if AK_INVALID_POOL_ID.
	    )
{
    AkSyncLoader syncLoader;
	AKRESULT eResult = syncLoader.Init();
	if( eResult != AK_Success )
		return eResult;

    eResult = LoadBankInternal(
	                        in_bankID,             // ID of the bank to load.
							AkBankLoadFlag_UsingFileID,
                            CAkBankMgr::QueueItemLoad,
                            DefaultBankCallbackFunc,// Callback function that will be called.
							syncLoader.GetCookie(),		    // Callback cookie.
	                        in_memPoolId,           // Pool ID.
							NULL,
							0 ); 

	return syncLoader.Wait( eResult );
}

AKRESULT CheckBankAlignment( void* in_pMem, AkUInt32 in_uMemSize );
AKRESULT CheckBankAlignment( void* in_pMem, AkUInt32 in_uMemSize )
{
	if( ((uintptr_t)in_pMem) % AK_BANK_PLATFORM_DATA_ALIGNMENT != 0 )
	{
		return AK_InvalidParameter;
	}

	if( in_pMem == NULL || in_uMemSize < AK_MINIMUM_BANK_SIZE )
	{
		return AK_InvalidParameter;
	}

	return AK_Success;
}

// Synchronous bank load (by in-memory Bank).
AKRESULT LoadBank(
	    void *				in_pInMemoryBankPtr,	// Pointer to the in-memory bank to load
		AkUInt32			in_ui32InMemoryBankSize,// Size of the in-memory bank to load
        AkBankID &          out_bankID				// Returned bank ID.
	    )
{
	AKRESULT eResult = CheckBankAlignment( in_pInMemoryBankPtr, in_ui32InMemoryBankSize );
	if( eResult != AK_Success )
		return eResult;

	out_bankID = g_pBankManager->GetBankIDFromInMemorySpace( in_pInMemoryBankPtr, in_ui32InMemoryBankSize );

    AkSyncLoader syncLoader;
	eResult = syncLoader.Init();
	if( eResult != AK_Success )
		return eResult;

    eResult = LoadBankInternal(
	                        out_bankID,					 // ID of the bank to load.
							AkBankLoadFlag_InMemory,
                            CAkBankMgr::QueueItemLoad,
                            DefaultBankCallbackFunc,	// Callback function that will be called.
							syncLoader.GetCookie(),		// Callback cookie.
	                        AK_INVALID_POOL_ID,			// Pool ID unused when from memory.
							in_pInMemoryBankPtr,
							in_ui32InMemoryBankSize
	                        ); 

	return syncLoader.Wait( eResult );
}

// Asynchronous bank load (by string).
AKRESULT LoadBank(
	    AkLpCtstr           in_pszString,           // Name/path of the bank to load.
		AkBankCallbackFunc  in_pfnBankCallback,	    // Callback function.
		void *              in_pCookie,				// Callback cookie.
        AkMemPoolId         in_memPoolId,			// Memory pool ID. Pool is created inside if AK_INVALID_POOL_ID.
		AkBankID &          out_bankID				// Returned bank ID.
	    )
{
    out_bankID = GetBankIDFromString( in_pszString );
    return LoadBankInternal(		out_bankID,                 // ID of the bank to load.
									AkBankLoadFlag_None,
                                    CAkBankMgr::QueueItemLoad,	// true = load, false = unload
                                    in_pfnBankCallback,			// Callback function that will be called.
	                                in_pCookie,					// Callback cookie.
                                    in_memPoolId );				// Custom parameter. Currently not used.
}

// Asynchronous bank load (by id).
AKRESULT LoadBank(
	    AkBankID			in_bankID,              // Bank ID of the bank to load.
		AkBankCallbackFunc  in_pfnBankCallback,	    // Callback function.
		void *              in_pCookie,				// Callback cookie.
	    AkMemPoolId         in_memPoolId			// Memory pool ID. Pool is created inside if AK_INVALID_POOL_ID.
	    )
{
    return LoadBankInternal( in_bankID,             // ID of the bank to load.
							 AkBankLoadFlag_UsingFileID,
                             CAkBankMgr::QueueItemLoad,
                             in_pfnBankCallback,	 // Callback function that will be called.
	                         in_pCookie,             // Callback cookie.
                             in_memPoolId,           // Custom parameter. Currently not used.
							 NULL,
							 0
							 );
}

// Asynchronous bank load (by in-memory bank).
AKRESULT LoadBank(
	    void *			in_pInMemoryBankPtr,	// Pointer to the in-memory bank to load
		AkUInt32			in_ui32InMemoryBankSize,// Size of the in-memory bank to load
		AkBankCallbackFunc  in_pfnBankCallback,	    // Callback function.
		void *              in_pCookie,				// Callback cookie.
		AkBankID &          out_bankID				// Returned bank ID.
	    )
{
	AKRESULT eResult = CheckBankAlignment( in_pInMemoryBankPtr, in_ui32InMemoryBankSize );
	if( eResult != AK_Success )
		return eResult;

	out_bankID = g_pBankManager->GetBankIDFromInMemorySpace( in_pInMemoryBankPtr, in_ui32InMemoryBankSize );

    return LoadBankInternal( out_bankID,			// ID of the bank to load.
							 AkBankLoadFlag_InMemory,
                             CAkBankMgr::QueueItemLoad,
                             in_pfnBankCallback,	// Callback function that will be called.
	                         in_pCookie,			// Callback cookie.
                             AK_INVALID_POOL_ID,	// Custom parameter. Currently not used.
							 in_pInMemoryBankPtr,
							 in_ui32InMemoryBankSize
							 );
}

// Synchronous bank unload (by string).
AKRESULT UnloadBank(
	    AkLpCtstr           in_pszString,				// Name/path of the bank to unload.
	    AkMemPoolId *       out_pMemPoolId /*= NULL*/	// Returned memory pool ID used with LoadBank(). Can pass NULL.
	    )
{
    AkBankID bankID = GetBankIDFromString( in_pszString );
    return UnloadBank( bankID, out_pMemPoolId );
}

// Synchronous bank unload (by id).
AKRESULT UnloadBank(
	    AkBankID            in_bankID,              // ID of the bank to unload.
        AkMemPoolId *       out_pMemPoolId /*= NULL*/   // Returned memory pool ID used with LoadBank(). Can pass NULL.
	    )
{
	AkSyncLoader syncLoader;
	AKRESULT eResult = syncLoader.Init();
	if( eResult != AK_Success )
		return eResult;

    eResult = LoadBankInternal(
	                        in_bankID,              // ID of the bank to load.
							AkBankLoadFlag_None,
                            CAkBankMgr::QueueItemUnload,
                            DefaultBankCallbackFunc,// Callback function that will be called.
							syncLoader.GetCookie(),		    // Callback cookie.
	                        AK_INVALID_POOL_ID      // Pool: ignored on unload.
	                        );

	eResult = syncLoader.Wait( eResult );

    if ( out_pMemPoolId != NULL )
    {
		*out_pMemPoolId = syncLoader.GetCookie()->memPoolId;
    }

	return eResult;
}


// Asynchronous bank unload (by string).
AKRESULT UnloadBank(
	    AkLpCtstr           in_pszString,           // Name/path of the bank to load.
		AkBankCallbackFunc  in_pfnBankCallback,	    // Callback function.
		void *              in_pCookie 				// Callback cookie.
	    )
{
    AkBankID bankID = GetBankIDFromString( in_pszString );

    return LoadBankInternal(		bankID,                 // ID of the bank to load.
									AkBankLoadFlag_None,
                                    CAkBankMgr::QueueItemUnload,
                                    in_pfnBankCallback,	    // Callback function that will be called.
	                                in_pCookie,             // Callback cookie.
                                    AK_INVALID_POOL_ID      // Pool: ignored on unload.
                                    );
}


// Asynchronous bank unload (by id).
AKRESULT UnloadBank(
	    AkBankID            in_bankID,              // ID of the bank to unload.
		AkBankCallbackFunc  in_pfnBankCallback,	    // Callback function.
		void *              in_pCookie 				// Callback cookie.
	    )
{
    return LoadBankInternal( in_bankID,              // ID of the bank to load.
							 AkBankLoadFlag_None,
							 CAkBankMgr::QueueItemUnload,
                             in_pfnBankCallback,	 // Callback function that will be called.
	                         in_pCookie,             // Callback cookie.
                             AK_INVALID_POOL_ID      // Pool: ignored on unload.
                             );
}

void CancelBankCallbackCookie( 
		void * in_pCookie
		)
{
	if( g_pBankManager )
	{
		g_pBankManager->CancelCookie( in_pCookie );
	}
}

// Synchronous PrepareEvent (by id).
AKRESULT PrepareEvent( 
	PreparationType		in_PreparationType,		///< Preparation type ( Preparation_Load or Preparation_Unload )
	AkUniqueID*			in_pEventID,			///< Array of Event ID
	AkUInt32			in_uNumEvent			///< number of Event ID in the array
	)
{
	AkSyncLoader syncLoader;
	AKRESULT eResult = syncLoader.Init();
	if( eResult != AK_Success )
		return eResult;

	eResult = PrepareEventInternal( in_PreparationType, DefaultBankCallbackFunc, syncLoader.GetCookie(), in_pEventID, in_uNumEvent );

	return syncLoader.Wait( eResult );
}

// Synchronous PrepareEvent (by string).
AKRESULT PrepareEvent( 
	PreparationType		in_PreparationType,		///< Preparation type ( Preparation_Load or Preparation_Unload )
	AkLpCtstr*			in_ppszString,			///< Array of EventName
	AkUInt32			in_uNumEvent			///< Number of Events in the array
	)
{
	AKRESULT eResult;
	switch ( in_uNumEvent )
	{
	case 0:
		eResult = AK_InvalidParameter;
		break;

	case 1:
		{
			AkUniqueID eventID = GetIDFromString( in_ppszString[0] );
			eResult = PrepareEvent( in_PreparationType, &eventID, 1 );
		}
		break;

	default:
		{
			AkUInt32* pEventIDArray = (AkUInt32*)AkAlloc( g_DefaultPoolId, in_uNumEvent * sizeof( AkUInt32 ) );
			if( pEventIDArray )
			{
				for( AkUInt32 i = 0; i < in_uNumEvent; ++i )
				{
					pEventIDArray[i] = 	GetIDFromString( in_ppszString[i] );
				}
				AkSyncLoader syncLoader;
				eResult = syncLoader.Init();
				if( eResult == AK_Success )
				{
					eResult = PrepareEventInternal( in_PreparationType, DefaultBankCallbackFunc, syncLoader.GetCookie(), pEventIDArray, in_uNumEvent, false );
					eResult = syncLoader.Wait( eResult );
				}
			}
			else
			{
				eResult = AK_InsufficientMemory;
			}
		}
		break;
	}

	return eResult;
}

// Asynchronous PrepareEvent (by id).
AKRESULT PrepareEvent( 
	PreparationType		in_PreparationType,		///< Preparation type ( Preparation_Load or Preparation_Unload )
	AkUniqueID*			in_pEventID,			///< Array of Event ID
	AkUInt32			in_uNumEvent,			///< number of Event ID in the array
	AkBankCallbackFunc	in_pfnBankCallback,		///< Callback function
	void *              in_pCookie				///< Callback cookie
	)
{
	return PrepareEventInternal( in_PreparationType, in_pfnBankCallback, in_pCookie, in_pEventID, in_uNumEvent );
}

// Asynchronous PrepareEvent (by string).
AKRESULT PrepareEvent( 
	PreparationType		in_PreparationType,		///< Preparation type ( Preparation_Load or Preparation_Unload )
	AkLpCtstr*			in_ppszString,			///< Array of EventName
	AkUInt32			in_uNumEvent,			///< Number of Events in the array
	AkBankCallbackFunc	in_pfnBankCallback,		///< Callback function
	void *              in_pCookie				///< Callback cookie
	)
{
	AKRESULT eResult;
	switch ( in_uNumEvent )
	{
	case 0:
		eResult = AK_InvalidParameter;
		break;

	case 1:
		{
			AkUniqueID eventID = GetIDFromString( in_ppszString[0] );
			eResult = PrepareEventInternal( in_PreparationType, in_pfnBankCallback, in_pCookie, &eventID, 1 );
		}
		break;

	default:
		{
			AkUInt32* pEventIDArray = (AkUInt32*)AkAlloc( g_DefaultPoolId, in_uNumEvent * sizeof( AkUInt32 ) );
			if( pEventIDArray )
			{
				for( AkUInt32 i = 0; i < in_uNumEvent; ++i )
				{
					pEventIDArray[i] = 	GetIDFromString( in_ppszString[i] );
				}
				eResult = PrepareEventInternal( in_PreparationType, in_pfnBankCallback, in_pCookie, pEventIDArray, in_uNumEvent, false);
			}
			else
			{
				eResult = AK_InsufficientMemory;
			}
		}
		break;
	}

	return eResult;
}

AKRESULT ClearPreparedEvents()
{
	AkSyncLoader syncLoader;
	AKRESULT eResult = syncLoader.Init();
	if( eResult != AK_Success )
		return eResult;

	CAkBankMgr::AkBankQueueItem item;
	item.eType						 = CAkBankMgr::QueueItemUnprepareAllEvents;
    item.callbackInfo.pfnBankCallback = DefaultBankCallbackFunc;
    item.callbackInfo.pCookie		 = syncLoader.GetCookie();

	eResult = g_pBankManager->QueueBankCommand( item );

	return syncLoader.Wait( eResult );
}

/// Async IDs
AKRESULT PrepareGameSyncs(
	PreparationType	in_PreparationType,			///< Preparation type ( Preparation_Load or Preparation_Unload )
	AkGroupType			in_eGameSyncType,		///< The type of game sync.
	AkUInt32			in_GroupID,				///< The state group ID or the Switch Group ID.
	AkUInt32*			in_paGameSyncID,		///< Array of ID of the gamesyncs to either support or not support.
	AkUInt32			in_uNumGameSyncs,		///< The number of game sync ID in the array.
	AkBankCallbackFunc	in_pfnBankCallback,		///< Callback function
	void *				in_pCookie				///< Callback cookie
	)
{
	return PrepareGameSyncsInternal( in_pfnBankCallback, in_pCookie, (in_PreparationType == Preparation_Load), in_eGameSyncType, in_GroupID, in_paGameSyncID, in_uNumGameSyncs );
}

/// Async strings
AKRESULT PrepareGameSyncs(
	PreparationType	in_PreparationType,			///< Preparation type ( Preparation_Load or Preparation_Unload )
	AkGroupType			in_eGameSyncType,		///< The type of game sync.
	AkLpCtstr			in_pszGroupName,		///< The state group Name or the Switch Group Name.
	AkLpCtstr*			in_ppszGameSyncName,	///< The specific ID of the state to either support or not support.
	AkUInt32			in_uNumGameSyncNames,	///< The number of game sync in the string array.
	AkBankCallbackFunc	in_pfnBankCallback,		///< Callback function
	void *				in_pCookie				///< Callback cookie
	)
{
	AKRESULT eResult;
	
	if( !in_ppszGameSyncName || !in_uNumGameSyncNames )
		return AK_InvalidParameter;

	AkUInt32 groupID = GetIDFromString( in_pszGroupName );

	if ( in_uNumGameSyncNames == 1 )
	{
		AkUInt32 gameSyncID = GetIDFromString( in_ppszGameSyncName[0] );

		eResult = PrepareGameSyncsInternal( in_pfnBankCallback, in_pCookie, (in_PreparationType == Preparation_Load), in_eGameSyncType, groupID, &gameSyncID, 1 );
	}
	else
	{
		AkUInt32* pGameSyncIDArray = (AkUInt32*)AkAlloc( g_DefaultPoolId, in_uNumGameSyncNames * sizeof( AkUInt32 ) );
		if( pGameSyncIDArray )
		{
			for( AkUInt32 i = 0; i < in_uNumGameSyncNames; ++i )
			{
				pGameSyncIDArray[i] = 	GetIDFromString( in_ppszGameSyncName[i] );
			}
			eResult = PrepareGameSyncsInternal( in_pfnBankCallback, in_pCookie, (in_PreparationType == Preparation_Load), in_eGameSyncType, groupID, pGameSyncIDArray, in_uNumGameSyncNames, false );
		}
		else
		{
			eResult = AK_InsufficientMemory;
		}
	}

	return eResult;
}

/// Sync IDs
AKRESULT PrepareGameSyncs(
	PreparationType	in_PreparationType,			///< Preparation type ( Preparation_Load or Preparation_Unload )
	AkGroupType		in_eGameSyncType,			///< The type of game sync.
	AkUInt32		in_GroupID,					///< The state group ID or the Switch Group ID.
	AkUInt32*		in_paGameSyncID,			///< Array of ID of the gamesyncs to either support or not support.
	AkUInt32		in_uNumGameSyncs			///< The number of game sync ID in the array.
	)
{
	AkSyncLoader syncLoader;
	AKRESULT eResult = syncLoader.Init();
	if( eResult != AK_Success )
		return eResult;

	eResult = PrepareGameSyncsInternal( DefaultBankCallbackFunc, syncLoader.GetCookie(), (in_PreparationType == Preparation_Load), in_eGameSyncType, in_GroupID, in_paGameSyncID, in_uNumGameSyncs );

	return syncLoader.Wait( eResult );
}

/// Sync strings
AKRESULT PrepareGameSyncs(
	PreparationType	in_PreparationType,			///< Preparation type ( Preparation_Load or Preparation_Unload )
	AkGroupType		in_eGameSyncType,			///< The type of game sync.
	AkLpCtstr		in_pszGroupName,			///< The state group Name or the Switch Group Name.
	AkLpCtstr*		in_ppszGameSyncName,		///< The specific ID of the state to either support or not support.
	AkUInt32		in_uNumGameSyncNames		///< The number of game sync in the string array.
	)
{
	AKRESULT eResult;
	
	if( !in_ppszGameSyncName || !in_uNumGameSyncNames )
		return AK_InvalidParameter;

	AkUInt32 groupID = GetIDFromString( in_pszGroupName );

	if ( in_uNumGameSyncNames == 1 )
	{
		AkUInt32 gameSyncID = GetIDFromString( in_ppszGameSyncName[0] );

		eResult = PrepareGameSyncs( in_PreparationType, in_eGameSyncType, groupID, &gameSyncID, 1 );
	}
	else
	{
		AkUInt32* pGameSyncIDArray = (AkUInt32*)AkAlloc( g_DefaultPoolId, in_uNumGameSyncNames * sizeof( AkUInt32 ) );
		if( pGameSyncIDArray )
		{
			for( AkUInt32 i = 0; i < in_uNumGameSyncNames; ++i )
			{
				pGameSyncIDArray[i] = 	GetIDFromString( in_ppszGameSyncName[i] );
			}

			AkSyncLoader syncLoader;
			eResult = syncLoader.Init();
			if( eResult == AK_Success )
			{
				eResult = PrepareGameSyncsInternal( DefaultBankCallbackFunc, syncLoader.GetCookie(), (in_PreparationType == Preparation_Load), in_eGameSyncType, groupID, pGameSyncIDArray, in_uNumGameSyncNames, false );
				eResult = syncLoader.Wait( eResult );
			}
		}
		else
		{
			eResult = AK_InsufficientMemory;
		}
	}

	return eResult;
}

////////////////////////////////////////////////////////////////////////////
// Banks new API END.
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// Behavioral extensions registration
////////////////////////////////////////////////////////////////////////////
void AddBehavioralExtension( 
    AkBehavioralExtensionCallback in_pCallback
    )
{
    g_pBehavioralExtensionCallback = in_pCallback;
}
void AddExternalStateHandler( 
    AkExternalStateHandlerCallback in_pCallback
    )
{
    g_pExternalStateHandler = in_pCallback;
}
void AddExternalBankHandler(
	AkExternalBankHandlerCallback in_pCallback
	)
{
	g_pExternalBankHandlerCallback = in_pCallback;
}

void AddExternalProfileHandler(
	AkExternalProfileHandlerCallback in_pCallback
	)
{
	g_pExternalProfileHandlerCallback = in_pCallback;
}

////////////////////////////////////////////////////////////////////////////
// FXParameterSet
////////////////////////////////////////////////////////////////////////////

AKRESULT AddFXParameterSet(
		AkUniqueID		in_FXParameterSetID,	// FXParameterSet unique ID
		AkPluginID		in_EffectTypeID,		// Effect unique type ID. 
		void*			in_pvInitParamsBlock,	// FXParameterSet.
		AkUInt32			in_ulParamBlockSize		// FXParameterSet size.
		)
{
	if ( NULL == g_pEnvironmentMgr )
    {
        AKASSERT( !"AudioLib not properly initialized. Requested action not possible" );
        return AK_Fail;
    }
	return g_pEnvironmentMgr->AddFXParameterSet( in_FXParameterSetID, in_EffectTypeID, in_pvInitParamsBlock, in_ulParamBlockSize );
}

AKRESULT SetFXParameterSetParam( 
		AkPluginID      in_FXParameterSetID,		// FXParameterSet unique ID
		AkPluginParamID in_ulParamID,				// ID of the parameter to modify.
		void*     	in_pvParamsBlock,			// FXParameter bloc.
		AkUInt32     	in_ulParamBlockSize			// FXParameter bloc size.
		)
{
	if ( NULL == g_pEnvironmentMgr )
    {
        AKASSERT( !"AudioLib not properly initialized. Requested action not possible" );
        return AK_Fail;
    }
	return g_pEnvironmentMgr->SetFXParameterSetParam( in_FXParameterSetID, in_ulParamID, in_pvParamsBlock, in_ulParamBlockSize );
}

AKRESULT RemoveFXParameterSet(
		AkUniqueID	in_FXParameterSetID			// FXParameterSet unique ID.
		)
{
	if ( NULL == g_pEnvironmentMgr )
    {
        AKASSERT( !"AudioLib not properly initialized. Requested action not possible" );
        return AK_Fail;
    }
	return g_pEnvironmentMgr->RemoveFXParameterSet( in_FXParameterSetID );
}

///////////////////////////////////////////////////////////////////////////
// Output Capture
///////////////////////////////////////////////////////////////////////////
#if !defined(AK_OPTIMIZED) && !defined(RVL_OS)
AKRESULT StartOutputCapture( AkLpCtstr in_CaptureFileName )
{
	AKASSERT( g_pAkSink != NULL );
	g_pAkSink->StartOutputCapture(in_CaptureFileName);
	return AK_Success;
}
#else
AKRESULT StartOutputCapture( AkLpCtstr in_CaptureFileName )
{
	//This function is disabled from the SDK in optimized mode.
	return AK_NotCompatible;
}
#endif

#if !defined(AK_OPTIMIZED) && !defined(RVL_OS)
AKRESULT StopOutputCapture()
{
	AKASSERT( g_pAkSink != NULL );
	g_pAkSink->StopOutputCapture();
	return AK_Success;
}
#else
AKRESULT StopOutputCapture()
{
	//This function is disabled from the SDK in optimized mode.
	return AK_NotCompatible;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// BELOW ARE GENERAL, BUT ****INTERNAL**** FUNCTIONS
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AKRESULT CreateDefaultMemPools()
{
    // The Memory Pool(s) should, of course, be created before
	// any other that will need memory.
    if ( g_DefaultPoolId == AK_INVALID_POOL_ID )
    {
	    // default pool
		g_DefaultPoolId = AK::MemoryMgr::CreatePool( 
			NULL, 
			g_settings.uDefaultPoolSize > DEFAULT_POOL_BLOCK_SIZE ? g_settings.uDefaultPoolSize : DEFAULT_POOL_SIZE,
			DEFAULT_POOL_BLOCK_SIZE,
#ifndef XBOX360
			AkMalloc
#else
			// must use physical alloc as the PrepareEvent will load sounds in this pool by default.
			AkPhysicalAlloc
#endif
			);

		AkFXMemAlloc::GetUpper()->SetPoolId(g_DefaultPoolId);
		AK_SETPOOLNAME(g_DefaultPoolId,L"Default");
    }

    if ( g_DefaultPoolId == AK_INVALID_POOL_ID )
    {
        return AK_InsufficientMemory;
    }

	return AK_Success;
}

void DestroyDefaultMemPools()
{
	if(g_DefaultPoolId != AK_INVALID_POOL_ID)
    {
	    AKVERIFY( AK::MemoryMgr::DestroyPool(g_DefaultPoolId) == AK_Success );
	    g_DefaultPoolId = AK_INVALID_POOL_ID;
    }
}

//====================================================================================================
// Internal INITS
//====================================================================================================
AKRESULT PreInit( AkInitSettings * io_pSettings )
{
	// make sure you get rid of things in the reverse order
	// you created them

	AKRESULT eResult = AK_Success;

	#define CHECK_PREINIT_FAILURE {if( eResult != AK_Success ) goto preinit_failure;}

	#define CHECK_ALLOCATION_PREINIT_FAILURE( _IN_PTR_ ) {if( _IN_PTR_ == NULL ) {eResult = AK_InsufficientMemory; goto preinit_failure;}}

	//Initialise the timers for performance measurement.
	AK_INIT_TIMERS();
	AK_PERF_INIT();

#ifndef AK_OPTIMIZED
	AkMonitor * pMonitor = AkMonitor::Instance(); 
	if ( pMonitor ) 
		pMonitor->StartMonitoring();
#endif

	//IMPORTANT : g_pIndex MUST STAY SECOND CREATION OF AKINIT()!!!!!!!!!!!!!!!!!!
	if(!g_pIndex)
	{
		g_pIndex = CAkAudioLibIndex::Instance();
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pIndex );

		eResult = g_pIndex->Init();
		CHECK_PREINIT_FAILURE;
	}

	// Following table sizes must be ODD numbers ( 333 is pretty much arbitrary ).
	eResult = g_Interpol.Init( 333, 333, AkITable_All );
	CHECK_PREINIT_FAILURE;

    if(!g_pRTPCMgr)
	{
		g_pRTPCMgr = AkNew( g_DefaultPoolId, CAkRTPCMgr() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pRTPCMgr );

		eResult = g_pRTPCMgr->Init();
		CHECK_PREINIT_FAILURE;
	}

	if(!g_pEnvironmentMgr)
	{
		g_pEnvironmentMgr = AkNew( g_DefaultPoolId, CAkEnvironmentsMgr() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pEnvironmentMgr );

		eResult = g_pEnvironmentMgr->Init();
		CHECK_PREINIT_FAILURE;
	}

	if(!g_pBankManager)
	{
		g_pBankManager = AkNew( g_DefaultPoolId, CAkBankMgr() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pBankManager );

		eResult = g_pBankManager->Init();
		CHECK_PREINIT_FAILURE;
	}

	if(!g_pPlayingMgr)
	{
		g_pPlayingMgr = AkNew( g_DefaultPoolId, CAkPlayingMgr() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pPlayingMgr );

		eResult = g_pPlayingMgr->Init();
		CHECK_PREINIT_FAILURE;
	}

	if(!g_pPositionRepository)
	{
		g_pPositionRepository = AkNew( g_DefaultPoolId, CAkPositionRepository() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pPositionRepository );

		eResult = g_pPositionRepository->Init();
		CHECK_PREINIT_FAILURE;
	}

	if(!g_pRegistryMgr)
	{
		g_pRegistryMgr = AkNew( g_DefaultPoolId, CAkRegistryMgr() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pRegistryMgr );

		eResult = g_pRegistryMgr->Init();
		CHECK_PREINIT_FAILURE;
	}

	// this one has to be ready before StateMgr is
	if(!g_pTransitionManager)
	{
		g_pTransitionManager = AkNew( g_DefaultPoolId, CAkTransitionManager() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pTransitionManager );

		eResult = g_pTransitionManager->Init( g_settings.uMaxNumTransitions );
		CHECK_PREINIT_FAILURE;
	}

	// this one needs math and transitions to be ready
	if(!g_pPathManager)
	{
		g_pPathManager = AkNew( g_DefaultPoolId, CAkPathManager() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pPathManager );

		eResult = g_pPathManager->Init( g_settings.uMaxNumPaths );
		CHECK_PREINIT_FAILURE;
	}

	if(!g_pStateMgr)
	{
		g_pStateMgr = AkNew( g_DefaultPoolId, CAkStateMgr() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pStateMgr );

		eResult = g_pStateMgr->Init();
		CHECK_PREINIT_FAILURE;
	}

#ifdef RVL_OS
	CAkWiimoteMgr::Init();
#endif

preinit_failure:

    // Update client settings with (possibly) modified values.
    if ( io_pSettings )
        *io_pSettings = g_settings;

    return eResult;
}

AKRESULT InitRenderer()
{
	AKRESULT eResult = CAkURenderer::Init();

	if (!g_pAudioMgr && eResult == AK_Success )
	{
		g_pAudioMgr = AkNew(g_DefaultPoolId,CAkAudioMgr());
		if ( g_pAudioMgr )
		{
			eResult = g_pAudioMgr->Init();
			if( eResult == AK_Success )
			{
				eResult = g_pAudioMgr->Start();
			}
		}
		else
		{
			eResult = AK_InsufficientMemory;
		}
	}
	return eResult;
}

void StopAll( AkGameObjectID in_gameObjectID /*= AK_INVALID_GAME_OBJECT*/ )
{
	if( in_gameObjectID == AK_INVALID_GAME_OBJECT )
	{
		MONITOR_MSG( L"Stop all sounds manually requested" );
	}

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_StopAll;
	Item.stopAll.GameObjID = in_gameObjectID;

	g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_StopAll() );

	if( in_gameObjectID == AK_INVALID_GAME_OBJECT )
	{
		// PhM : B2635 temp fix
		RenderAudio();
	}
}

void StopPlayingID( AkPlayingID in_playingID )
{
	AKASSERT(g_pIndex);
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item;
	Item.type = QueuedMsgType_StopPlayingID;

	Item.stopEvent.playingID = in_playingID;
	g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_StopPlayingID() );
}

#ifndef AK_OPTIMIZED

CAkIndexable* GetIndexable( AkUniqueID in_IndexableID, AkIndexType in_eIndexType )
{
	CAkIndexable* pReturnedIndexable = NULL;

	//Ensure the Index Was initialized
	AKASSERT(g_pIndex);

	switch(in_eIndexType)
	{
	case AkIdxType_AudioNode:
		pReturnedIndexable = g_pIndex->m_idxAudioNode.GetPtrAndAddRef(in_IndexableID);
		break;
	case AkIdxType_Action:
		pReturnedIndexable = g_pIndex->m_idxActions.GetPtrAndAddRef(in_IndexableID);
		break;
	case AkIdxType_Event:
		pReturnedIndexable = g_pIndex->m_idxEvents.GetPtrAndAddRef(in_IndexableID);
		break;
	case AkIdxType_Layer:
		pReturnedIndexable = g_pIndex->m_idxLayers.GetPtrAndAddRef(in_IndexableID);
		break;
	case AkIdxType_Attenuation:
		pReturnedIndexable = g_pIndex->m_idxAttenuations.GetPtrAndAddRef(in_IndexableID);
		break;
	case AkIdxType_DynamicSequence:
		pReturnedIndexable = g_pIndex->m_idxDynamicSequences.GetPtrAndAddRef(in_IndexableID);
		break;
	default:
		AKASSERT(!"Invalid Index Type");
		break;
	}

	return pReturnedIndexable;
}

CAkIndexable* GetStateIndexable( AkUniqueID in_IndexableID, AkIndexType in_eIndexType, AkStateGroupID in_StateGroupID )
{
	CAkIndexable* pReturnedIndexable = NULL;

	switch(in_eIndexType)
	{
	case AkIdxType_State:
		pReturnedIndexable = g_pIndex->m_idxStates.GetPtrAndAddRef( in_StateGroupID, in_IndexableID );
		break;
	case AkIdxType_CustomState:
		pReturnedIndexable = g_pIndex->m_idxCustomStates.GetPtrAndAddRef( in_IndexableID );
		break;
	default:
		AKASSERT(!"Invalid Index Type");
		break;
	}

	return pReturnedIndexable;
}

#endif // AK_OPTIMIZED

} // namespace SoundEngine

namespace MotionEngine
{

AKRESULT AddPlayerMotionDevice(AkUInt8 in_iPlayerID, AkUInt32 in_iCompanyID, AkUInt32 in_iDevicePluginID)
{
	AKASSERT(g_pAudioMgr);
	AkQueuedMsg Item;

	Item.type = QueuedMsgType_AddRemovePlayerDevice;

	Item.playerdevice.iPlayer = in_iPlayerID;
	Item.playerdevice.idCompany = (AkUInt16)in_iCompanyID;
	Item.playerdevice.idDevice = (AkUInt16)in_iDevicePluginID;
	Item.playerdevice.bAdd = true;
	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_AddRemovePlayerDevice() );
}

void RemovePlayerMotionDevice(AkUInt8 in_iPlayerID, AkUInt32 in_iCompanyID, AkUInt32 in_iDevicePluginID)
{
	AKASSERT(g_pAudioMgr);
	AkQueuedMsg Item;

	Item.type = QueuedMsgType_AddRemovePlayerDevice;

	Item.playerdevice.iPlayer = in_iPlayerID;
	Item.playerdevice.idCompany = (AkUInt16)in_iCompanyID;
	Item.playerdevice.idDevice = (AkUInt16)in_iDevicePluginID;
	Item.playerdevice.bAdd = false;
	g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_AddRemovePlayerDevice() );
}

void RegisterMotionDevice(AkUInt32 in_ulCompanyID, AkUInt32 in_ulPluginID, AkCreatePluginCallback	in_pCreateFunc)
{
	CAkEffectsMgr::RegisterFeedbackBus(in_ulCompanyID, in_ulPluginID, in_pCreateFunc);
}

void SetPlayerListener(AkUInt8 in_iPlayerID, AkUInt8 in_iListener)
{
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item;

	Item.type = QueuedMsgType_SetPlayerListener;

	Item.playerlistener.iPlayer = in_iPlayerID;
	Item.playerlistener.iListener = in_iListener;

	g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_SetPlayerListener() );
}

void SetPlayerVolume(AkUInt8 in_iPlayerID, AkReal32 in_fVolume)
{
	AkQueuedMsg Item;

	Item.type = QueuedMsgType_SetPlayerVolume;

	Item.playervolume.iPlayer = in_iPlayerID;
	Item.playervolume.fVolume = in_fVolume;

	g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_SetPlayerVolume() );
}

} // namespace MotionEngine

namespace Monitor
{

AKRESULT PostCode( 
	ErrorCode in_eErrorCode,
	ErrorLevel in_eErrorLevel )
{
#ifndef AK_OPTIMIZED
	if ( AkMonitor::Get() )
		AkMonitor::Monitor_PostCode( in_eErrorCode, in_eErrorLevel );

	return AK_Success;
#else
	return AK_NotCompatible;
#endif
}

AKRESULT PostString( 
	AkLpCtstr in_pszError,
	ErrorLevel in_eErrorLevel
	)
{
#ifndef AK_OPTIMIZED
	if ( AkMonitor::Get() )
		AkMonitor::Monitor_PostString( in_pszError, in_eErrorLevel );

	return AK_Success;
#else
	return AK_NotCompatible;
#endif
}

AKRESULT SetLocalOutput(
	AkUInt32 in_uErrorLevel,
	LocalOutputFunc in_pMonitorFunc
	)
{
#ifndef AK_OPTIMIZED
	AkMonitor::SetLocalOutput( in_uErrorLevel, in_pMonitorFunc );
	return AK_Success;
#else
	return AK_NotCompatible;
#endif
}

AkTimeMs GetTimeStamp()
{
#ifndef AK_OPTIMIZED
	return AkMonitor::GetThreadTime();
#else
	return 0;
#endif
}

} // namespace Monitor

} // namespace AK
