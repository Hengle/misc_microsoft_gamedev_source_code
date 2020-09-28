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
// AkAudioLib.h
//
// AkAudioLib specific definitions
//
//////////////////////////////////////////////////////////////////////
#ifndef _AUDIOLIB_H_
#define _AUDIOLIB_H_

// Include exposed SDK interface.
#include <AK/SoundEngine/Common/AkSoundEngine.h>

using namespace AK;

#include <AK/Tools/Common/AkObject.h>
#include "AkAction.h"		// enum AkActionType;

class CAkIndexable;
namespace AkBank
{
	struct AKBKSubHircSection;
}
class CAkUsageSlot;

////////////////////////////////////////////////////////////////////////////
// Index ID to Ptr
////////////////////////////////////////////////////////////////////////////

enum AkIndexType
{
	AkIdxType_AudioNode,
	AkIdxType_State,
	AkIdxType_CustomState,
	AkIdxType_Action,
	AkIdxType_Event,
	AkIdxType_Layer,
	AkIdxType_Attenuation,
	AkIdxType_DynamicSequence,
};

////////////////////////////////////////////////////////////////////////////
// Behavioral extension type
////////////////////////////////////////////////////////////////////////////
typedef void	(*AkBehavioralExtensionCallback)();
typedef bool	(*AkExternalStateHandlerCallback)( AkStateGroupID in_stateGroupID, AkStateID in_stateID );
typedef AKRESULT	(*AkExternalBankHandlerCallback)( const AkBank::AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot, AkUInt32 in_dwBankID );
typedef void	(*AkExternalProfileHandlerCallback)();

namespace AK
{
	class IALMonitor;

	namespace SoundEngine
	{
		// Static bank callback function for synchronous bank load.
		// Casts the cookie as a structure that has an AkEvent and signals it.
		struct AkSyncLoadInfo
		{
			AKRESULT		eLoadResult;
			AkMemPoolId     memPoolId;
			AkEvent*		phEvent;
		};


		class AkSyncLoader
		{
		public:
			AKRESULT Init();
			AkSyncLoadInfo* GetCookie(){ return &m_SyncLoadInfo; }
			AKRESULT Wait( AKRESULT in_eResult );

		private:
			AkSyncLoadInfo	m_SyncLoadInfo;
			AkEvent			m_hEvent;
		};

		void DefaultBankCallbackFunc(
							AkBankID    in_bankID, 
							AKRESULT	in_eLoadResult,
							AkMemPoolId in_memPoolId,
							void *		in_pCookie );

		AkUInt32 HashName( char * in_pString );
		// Private, wwise-only methods

		extern AkPlayingID PostEvent(
	        AkUniqueID in_eventID,						///< Unique ID of the event
	        AkGameObjectID in_gameObjectID,				///< Associated game object ID
			AkUInt32 in_uFlags,							///< Bitmask: see \ref AkCallbackType
			AkCallbackFunc in_pfnCallback,				///< Callback function
			void * in_pCookie,							///< Callback cookie that will be sent to the callback function along with additional information
			AkCustomParamType * in_pCustomParam			///< Optional custom parameter
	        );

		extern AKRESULT SetState( 
				AkStateGroupID in_StateGroup,
				AkStateID in_State,
				bool in_bSkipTransitionTime,
                bool in_bSkipExtension
				);

		extern AKRESULT ResetSwitches( AkGameObjectID in_GameObjID = AK_INVALID_GAME_OBJECT );
		extern AKRESULT ResetRTPC( AkGameObjectID in_GameObjID = AK_INVALID_GAME_OBJECT );

		extern void SetVolumeThreshold( AkReal32 in_fVolumeThresholdDB );

		//////////////////////////////////////////////////////////////////////////////////
		//Monitoring
		//////////////////////////////////////////////////////////////////////////////////
		extern IALMonitor* GetMonitor( void );

        ////////////////////////////////////////////////////////////////////////////
		// Behavioral extensions registration
		////////////////////////////////////////////////////////////////////////////
        extern void AddBehavioralExtension( 
            AkBehavioralExtensionCallback in_pCallback
            );
        extern void AddExternalStateHandler( 
            AkExternalStateHandlerCallback in_pCallback
            );

		extern void AddExternalBankHandler(
			AkExternalBankHandlerCallback in_pCallback
			);
		extern void AddExternalProfileHandler(
			AkExternalProfileHandlerCallback in_pCallback
			);

		////////////////////////////////////////////////////////////////////////////
		// FXParameterSet
		////////////////////////////////////////////////////////////////////////////

		extern AKRESULT AddFXParameterSet(
			AkUniqueID		in_FXParameterSetID,	// FXParameterSet unique ID
			AkPluginID		in_EffectTypeID,		// Effect unique type ID. 
			void*			in_pvInitParamsBlock,	// FXParameterSet.
			unsigned long	in_ulParamBlockSize		// FXParameterSet size.
			);

		extern AKRESULT SetFXParameterSetParam( 
			AkPluginID			in_FXParameterSetID,		// FXParameterSet unique ID
			AkPluginParamID		in_ulParamID,				// ID of the parameter to modify.
			void*				in_pvParamsBlock,			// FXParameter bloc.
			unsigned long   	in_ulParamBlockSize			// FXParameter bloc size.
			);

		extern AKRESULT RemoveFXParameterSet(
			AkUniqueID	in_FXParameterSetID			// FXParameterSet unique ID.
			);

		extern CAkIndexable* GetIndexable(
			AkUniqueID	in_IndexableID, // Indexable ID
			AkIndexType	in_eIndexType	// Index to look into
			);

		extern CAkIndexable* GetStateIndexable( 
			AkUniqueID in_IndexableID,	// Indexable ID
			AkIndexType in_eIndexType,	// Index to look into
			AkStateGroupID in_StateGroupID // StateGroupID
			);
	}
};

#endif
