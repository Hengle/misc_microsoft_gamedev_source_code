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
// AkURenderer.h
//
// Microsoft Windows (DirectSound) Implementation of the Audio renderer
//
//////////////////////////////////////////////////////////////////////
#ifndef _UPPER_RENDERER_H_
#define _UPPER_RENDERER_H_

#include "AkPBI.h"
#include "AkRTPC.h"
#include "PrivateStructures.h"
#include "AkList2.h"
#include "AkListBare.h"
#include <AK/Tools/Common/AkLock.h>

using namespace AK;

class CAkSoundBase;
class CAkSource;
struct AkRTPCFXSubscription;
struct AkMutedMapItem;
struct PlayHistory;
struct NotifParams;
struct AkRTPCEnvSubscription;

enum UEState
{	
	UEStatePlay			= 0,
	UEStatePlayPaused	= 1,
	UEStatePlayToEnd	= 2,
	UEStateStop			= 3,
	UEStatePause		= 4,
	UEStateResume		= 5
};

struct AkUEPlayEvent
{
	UEState			m_eActionType;
	CAkPBI *	m_pCtx;
};

struct FxParamRec
{
	IAkPluginParam * m_pMaster;		// The master of global parameter object.
	IAkPluginParam * m_pClone;		// A clone of the master.
};

//-----------------------------------------------------------------------------
// CAkURenderer class.
//-----------------------------------------------------------------------------
class CAkURenderer
{
public:

	// Store lower engine global settings.
	// AkPlatformInitSettings is platform specific.  In the DirectSound implementation,
    // it contains the GUID of the audio devices to initialize and the window 
    // handle of the application
	static void ApplyGlobalSettings( AkPlatformInitSettings *   io_pPDSettings );

    // Initialise the renderer
    //
    // Return - AKRESULT - AK_Success: everything succeed
    //                     AK_Fail: Was not able to initialize the renderer
    static AKRESULT Init();

    // Uninit the renderer
    //
    // Return - AKRESULT - AK_Success: everything succeed
    //                     AK_Fail: Was not able to uninitialize the renderer
    static AKRESULT Term();

	// Call a play on the definition directly
    //
    // Return - AKRESULT - Ak_Success if succeeded
	static AKRESULT Play(  CAkSoundBase*    in_pSound,          // Node instanciating the play
						   CAkSource*		in_pSource,			// Source
		                   AkPBIParams&     in_rPBIParams );

	static AKRESULT Play(	CAkPBI *		in_pContext, 
                    TransParams&    in_rTparameters,
					AkPlaybackState	in_ePlaybackState
					);

	// Stop All the specified node associated with the specified object
    //
    // Return - AKRESULT - Ak_Success if succeeded
	static AKRESULT Stop(	CAkSoundBase*	 in_pSound,
							CAkRegisteredObj * in_pGameObj,
                            TransParams&    in_rTparameters,
							AkPlayingID		in_pPlayingID = AK_INVALID_PLAYING_ID );

	// Pause All the specified node associated with the specified object
    //
    // Return - AKRESULT - Ak_Success if succeeded
	static AKRESULT Pause(	CAkSoundBase*	 in_pSound, 
							CAkRegisteredObj * in_pGameObj,
                            TransParams&    in_rTparameters,
							AkPlayingID		in_pPlayingID = AK_INVALID_PLAYING_ID );

	// Resume All the specified node associated with the specified object
    //
    // Return - AKRESULT - Ak_Success if succeeded
	static AKRESULT Resume(	CAkSoundBase*	  in_pSound,
							CAkRegisteredObj * in_pGameObj,
                            TransParams&    in_rTparameters,
							bool		  in_bIsMasterResume,
							AkPlayingID		in_pPlayingID = AK_INVALID_PLAYING_ID );

	static void StopAllPBIs( CAkUsageSlot* in_pUsageSlot );

    static void EnqueueContext( CAkPBI * in_pContext );


	// Asks to kick out the Oldest sound responding to the given IDs.
	// Theorically called to apply the Max num of instances per Node.
    //
    // Return - bool - true = caller survives | false = caller should kick himself
	static bool Kick( AkPriority in_Priority, CAkRegisteredObj * in_pGameObj, AkUniqueID in_NodeID, bool in_bKickNewest, CAkAudioNode*& out_pKicked );

#ifndef AK_OPTIMIZED

	static void NotifBusUnsetRTPC( AkUniqueID in_MixBusID, AkPluginID in_FXID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID );
	static void NotifMasterBusUnsetRTPC( AkPluginID in_FXID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID );

	static void UpdateBusRTPC( AkUniqueID in_MixBusID, AkRTPCFXSubscription& in_rSubsItem );
	static void UpdateMasterBusRTPC( AkRTPCFXSubscription& in_rSubsItem );

	static void UpdateBusFxParam(
			AkUniqueID 		in_MixBusID,
			AkPluginID		in_FXID,
			AkPluginParamID	in_ulParamID,
			void*			in_pvParamsBlock,
			AkUInt32		in_ulParamBlockSize
			);

	static void UpdateMasterBusFxParam(
			AkPluginID		in_FXID,
			AkPluginParamID	in_ulParamID,
			void*			in_pvParamsBlock,
			AkUInt32		in_ulParamBlockSize
			);

	static void NotifEnvUnsetRTPC( 
		AkEnvID in_envID, 
		AkRTPC_ParameterID in_ParamID,
		AkUniqueID in_RTPCCurveID
		);

	static void UpdateEnvRTPC( 
		AkEnvID in_envID, 
		AkRTPCEnvSubscription& in_rSubsItem 
		);

	static void UpdateEnvParam(
		AkEnvID			in_envID,
		AkPluginParamID	in_ulParamID,
		void*			in_pvParamsBlock,
		AkUInt32		in_ulParamBlockSize
		);
#endif

	struct ContextNotif
	{
		CAkPBI* pPBI;
		AkCtxState state;
		AkCtxDestroyReason DestroyReason;
		AkTimeMs EstimatedLength;
	};

	static AKRESULT	EnqueueContextNotif( CAkPBI* in_pPBI, AkCtxState in_State, AkCtxDestroyReason in_eDestroyReason = CtxDestroyReasonFinished, AkTimeMs in_EstimatedTime = 0 );

	// Fx Management.
	static AKRESULT	AddFxParam( IAkPluginParam * in_pMaster, IAkPluginParam * in_pClone );
	static AKRESULT	RemoveFxParam( IAkPluginParam * in_pParam );
	static AKRESULT	SetParam( IAkPluginParam *	in_pMaster,		// Pointer to the master parm of the cloned param object.
						  		  AkPluginParamID		in_ulParamID,	// Parameter id.
								  void *			in_vpParam,		// Pointer to a setup param block.
								  AkUInt32			in_ulSize		// Size of the parameter block.
								);

	typedef CAkList2<AkUEPlayEvent, const AkUEPlayEvent&, AkAllocAndFree> AkListUEPlayEvent;
	typedef CAkList2<FxParamRec, const FxParamRec&, AkAllocAndFree> AkListFxParams;
	typedef CAkList2<ContextNotif, const ContextNotif&, AkAllocAndFree> AkContextNotifQueue;
	typedef AkListBare<CAkPBI> AkListCtxs;
	 
	static void PerformContextNotif();

    // Lock/unlock
    static AKRESULT	Lock();
    static AKRESULT	Unlock();

public:
	static AkPriority _CalcInitialPriority( CAkSoundBase * in_pSound, CAkRegisteredObj * in_pGameObj );

private:

	static AKRESULT	DestroyPBI( CAkPBI * in_pPBI );
	static AKRESULT	DestroyAllPBIs( void );

	// Source management.
	static AKRESULT	ProcessCommand( UEState		 in_eCommand, 
								CAkSoundBase*	 in_pSound, 
								CAkRegisteredObj * in_pGameObj, 
								AkPlayingID		in_PlayingID,
								TransParams&    in_rTparameters,
								bool       in_bIsMasterResume);

private:	 
	static AkListCtxs				m_listCtxs;					// List of PBIs/Contexts.	 

	static AkListFxParams			m_listFxParam;				// List of tupples of param master/clones.
	static CAkLock					m_Lock;	

	static AkContextNotifQueue		m_CtxNotifQueue;
};

#endif //#define _UPPER_RENDERER_H_
