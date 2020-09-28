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
// AkLEngine.h
//
// Implementation of the Lower Audio Engine.
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_LENGINE_H_
#define _AK_LENGINE_H_

#include "AkSrcBase.h"
#include "AkCommon.h"
#include <AK/Tools/Common/AkArray.h>
#include "AkKeyList.h"
#include "AkLEngineStructs.h"
#include "AkList2.h"
#include "AkListBare.h"
#include <AK/Tools/Common/AkLock.h>
#include "AkStaticArray.h"
#include "AkVPLSrcCbxNode.h"
#include "AkVPLMixBusNode.h"
#include "AkVPLFinalMixNode.h"
#include "AkFeedbackMgr.h"

#define CACHED_BUFFER_SIZE_DIVISOR		1024
#define NUM_CACHED_BUFFER_SIZES			24 // 1024 (8-bit mono) to 24576 (32-bit 5.1) in CACHED_BUFFER_SIZE_DIVISOR increments
#define NUM_CACHED_BUFFERS				2 // should be somewhat like the max number of buffers allocated at once during a single voice execution

#define MIN_NUM_ENV_MIX_BUSSES			0
#define MAX_NUM_ENV_MIX_BUSSES			AK_NO_MAX_LIST_SIZE

#define LENGINE_DEFAULT_POOL_SIZE		(16 * 1024 * 1024)
#define LENGINE_DEFAULT_POOL_BLOCK_SIZE	(64)
#define LENGINE_DEFAULT_POOL_ALIGN		(16)

#define LENGINE_LIST_POOL_BLOCK_SIZE	(32)

#define EPSILON_CONTROL_VALUE			0.001f

//-----------------------------------------------------------------------------
// Enums.
//-----------------------------------------------------------------------------
enum LEState
{
	LEStatePlay			= 0,
	LEStatePlayPause	= 1,
	LEStateStop			= 2,
	LEStatePause		= 3,
	LEStateResume		= 4,
	LEStateStopLooping	= 5
};

//-----------------------------------------------------------------------------
// Structs.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Locals.
//-----------------------------------------------------------------------------
static void MergeLastAndCurrentValues(const AkEnvironmentValue *in_pLastValues, 
									  const AkEnvironmentValue *in_pNewValues,
									  AkMergedEnvironmentValue* io_paMergedValues);

//-----------------------------------------------------------------------------
// Classes.
//-----------------------------------------------------------------------------

class AkVPLSrc
{
public:
	AkVPLSrc *			pNextItem;				// For AkListVPLSrcs

	AkUInt32			m_ulPlayID;				// Playing id.
	CAkVPLSrcCbxNode	m_Src;					// Source node.

#ifndef AK_OPTIMIZED
	AkUniqueID			m_ID;					// Profiling id.
#endif
};

typedef AkListBare<AkVPLSrc> AkListVPLSrcs;

class AkVPL
{
public:
	AkListVPLSrcs			m_listVPLSrcs;		// List of sounds that are to be played or are playing.	 
	AkUniqueID				m_BusID;			// Bus id.

	CAkVPLMixBusNode		m_MixBus;			// Mix bus node.	 
	
	AkUInt8					m_bFeedback :1;		// This goes in the feedback pipeline
};

struct AkLECmd
{
	CAkPBI *		m_pCtx;				// Pointer to a sound context.
	LEState			m_eState;			// Running state of the source.	
    AkUInt32		m_ulSequenceNumber;
	AkVPL *			m_pVPL;
	AkVPLSrc *		m_pVPLSrc;
};

//-----------------------------------------------------------------------------
// CAkLEngine class.
//-----------------------------------------------------------------------------
class CAkLEngine
{
public:
    // Initialize/terminate.
	static void ApplyGlobalSettings( AkPlatformInitSettings *   io_pPDSettings );
    static AKRESULT Init();
    static AKRESULT Term();
	static void GetDefaultPlatformInitSettings( 
		AkPlatformInitSettings & out_pPlatformSettings // Platform specific settings. Can be changed depending on hardware.
		);

    // Perform processing.
	static AkUInt32 GetNumBufferNeededAndSubmit();
	static AkUInt32 Perform( AkUInt32 in_uNumBufferToFill ); // return the number of buffers that still need to be refilled
	static void     IncrementSyncCount(){ ++m_ulPlayEventID; };
	static void StartVoice();
	static AkEvent & GetVoiceEvent();

	static void DequeuePBI( CAkPBI* in_pPBI );

	// Playback methods.
	static AKRESULT	EnqueueAction( LEState in_eState, CAkPBI * in_pContext );
	static AKRESULT	EnqueueActionStop( CAkPBI * in_pContext );

	// Bus management.
	static void ForceBusVolume( AkUniqueID in_MixBusID, AkVolumeValue in_Volume );
	static void ForceMasterBusVolume( AkVolumeValue in_Volume );
	static void SetBusVolume( AkUniqueID in_MixBusID, AkVolumeValue in_VolumeOffset );
	static void SetMasterBusVolume( AkVolumeValue in_VolumeOffset );
	static void ForceBusLFE( AkUniqueID in_MixBusID, AkVolumeValue in_LFE );
	static void ForceMasterBusLFE( AkVolumeValue in_LFE );
	static void SetBusLFE( AkUniqueID in_MixBusID, AkVolumeValue in_LFEOffset );
	static void SetMasterBusLFE( AkVolumeValue in_LFEOffset );
	static void BypassBusFx( AkUniqueID in_MixBusID, AkUInt32 in_bitsFXBypass, AkUInt32 in_uTargetMask );
	static void BypassMasterBusFx( AkUInt32 in_bitsFXBypass, AkUInt32 in_uTargetMask );
	static void BypassEnvFx( AkEnvID in_EnvID, bool in_bIsBypassed );

	static void LockFxParams();
	static void UnlockFxParams();
	static IAkRTPCSubscriber* GetMixBusFXParams( AkUniqueID in_MixBusID, AkPluginID in_FXID );
	static IAkRTPCSubscriber* GetMasterBusFXParams( AkPluginID in_FXID );
	static IAkRTPCSubscriber* GetEnvFXParams( AkEnvID in_envID );

	static void StopMixBus( AkUniqueID in_MixBusID );

	static void StopAllMixBus( );

	// Cached buffer
	static void * GetCachedAudioBuffer( AkUInt32 in_uSize );
	static void ReleaseCachedAudioBuffer( AkUInt32 in_uSize, void * in_pvBuffer );

	//Feedback related function
	static void EnableFeedbackPipeline();
	static bool IsFeedbackEnabled();

#ifndef AK_OPTIMIZED
    // Lower engine profiling.
	static void GetNumPipelines(AkUInt16 &out_rAudio, AkUInt16& out_rFeedback);
	static void GetPipelineData( AkMonitorData::PipelineData * out_pPipelineData, bool in_bGetFeedback );
#endif

	typedef AkArray<AkVPL*, AkVPL*, ArrayPoolLEngineDefault, LENGINE_LIST_POOL_BLOCK_SIZE / sizeof( AkVPL * ) > AkArrayVPL;
	typedef CAkKeyList<AkEnvID, CAkVPLMixBusNode*, AkAllocAndFree> AkEnvBusList;

	static inline AkChannelMask GetChannelMask() {return m_uOutputChannelMask;}

private:

    static AKRESULT CreateLEnginePools();
    static void DestroyLEnginePools( void );

	// Execution.
	static AKRESULT				SequencerVoiceFilling( AkUInt32& io_NumBufferToRefill );
	static void					RunVPL( struct AkRunningVPL & io_runningVPL );
	static void					GetBuffer( AkPipelineBufferBase * io_pBuffer );
	static void					HandleStarvation();

	// VPL management.
	static AKRESULT				AddSound( AkLECmd & io_cmd );

	static void					RemoveVPLMixBussesSources();
	static void					RemoveVPLMixBusSources( AkVPL * in_pMixBus );

	static AkVPLSrc *			FindExistingVPLSrc( CAkPBI * in_pCtx );
	// Source management.
    static void                 ProcessCommands( AkUInt32 in_ulTag );
    static AKRESULT             ResolveCommandVPL( AkLECmd & io_cmd );
	static void    				ExecuteCmds( void );
	static AKRESULT				VPLCreateSource( CAkPBI * in_pCtx,
											 AkVPLSrc **	   out_ppVPLSrc );
    static AKRESULT             VPLTryConnectSource( CAkPBI * in_pContext,
                                                 AkVPLSrc * in_pVPLSrc,                                       
                                                 AkVPL *& out_pMixBus );
	static void					VPLDestroySource( AkVPLSrc * in_pVPLSrc );

	// Voice management.
	static AKRESULT				AllocVoice();

	// Bus management.
	static AkVPL *				CreateVPLMixBus( CAkBusCtx in_BusCtx, AkChannelMask in_uChannelMask, AkUInt16 in_usMaxFrames );
	static CAkVPLMixBusNode *	GetEnvironmentalBus( AkEnvID in_EnvID, CAkPBI * in_pCtx );
	static CAkVPLMixBusNode *	CreateEnvMixBus( AkEnvID in_EnvID, CAkBusCtx in_BusContext );
	static void					DestroyVPLMixBus( AkVPL * in_pMixBus );
	static void					DestroyEnvMixBus( CAkVPLMixBusNode * in_pMixBus );
	static void					DestroyAllVPLMixBusses();
	static void					DestroyAllEnvMixBusses();
	static AkVPL *				GetVPLMixBus( CAkPBI * in_pCtx );

private:
	typedef CAkList2<AkLECmd, const AkLECmd&, AkFixedSize> AkListCmd;

private: 
	static CAkVPLFinalMixNode *	m_pFinalMixNode;			// Final mix.
	static AkArrayVPL			m_arrayVPLs;				// Array of VPLs.
    static AkEnvBusList			m_EnvBusList;				// List of environmental busses
	static AkUInt32				m_ulPlayID;					// Id of last source scheduled to play.
	static AkListCmd			m_listCmd;					// List of command posted by the upper engine.
    static AkListVPLSrcs		m_listSrcsNotConnected;		// List of sounds not yet connected to a bus.	 
	static AkUInt32				m_ulPlayEventID;			// Play event id.
	static AkChannelMask		m_uOutputChannelMask;		// Output channel configuration.
	static AkEvent				m_hEventPacketDone;         // Callback from Voice
	static AkEvent				m_EventStop;				// Event to stop the thread.
	static AkUniqueID			m_VPLPipelineID;			// Profiling vpl src id.
	static AkUInt32				m_uLastStarvationTime;		// Last time we sent a starvation notification

	typedef AkStaticArray<void *, void *, NUM_CACHED_BUFFERS> BufferCache;
	static BufferCache			m_CachedAudioBuffers[NUM_CACHED_BUFFER_SIZES];

	static CAkLock				m_LockFxParams;				// FX lock.

	static AkUInt16				m_cMarkers;					// Number of markers in m_pMarkers
	static AkBufferMarker *     m_pMarkers;

	static CAkFeedbackDeviceMgr* m_pDeviceMgr;				// Force feedback device manager
};
#endif
