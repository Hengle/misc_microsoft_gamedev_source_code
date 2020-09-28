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
// AkBanks.h
//
//////////////////////////////////////////////////////////////////////
#ifndef AK_BANKS_H_
#define AK_BANKS_H_

#include "AkParameters.h"
#include "AkCommon.h"

#define AK_BANK_INVALID_OFFSET (-1)

// Version History:
//   1:   Original
//   2:   Added Layers to Layer Containers
//   3:   Remove unused isTransitionEnabled flag
//   4:   Removed Divergence Front/Rear
//   5:   Added Crossfading data to Layers
//   6:   - Moved interpolation type from curves to curve points (for segment-specific interpolation)
//        - Added scaling mode to RTPC curves (for optional dB scaling of volume-related curves curves)
//        - Unifying enum used to specify interpolation in curves and in fade actions
//   7:   Added curve scaling type to radius and obs/occ curves
//   8:   Support more interpolation types in curves. Note that this does not change the format
//        of the bank, only the possible values for interpolation.
//   9:   Support multiple RTPC curves on the same parameter (added curve ID to each RTPC)
//  10:   Add RTPC for environmentals (WG-4485)
//  11:	  Removed unused DeviceID for sources
//  12:   Move FX at beginning of sound (WG-4513)
//  13:   Removed UseState flag from SoundBanks (WG-4597)
//  14:   Cleanup (WG-4716)
//          - Removed bIsDopplerEnabled from AKBKPositioningInfo
//          - Removed bSendOverrideParent, Send, SendModMin and SendModMax from AKBKParameterNodeParams
//  15:   Added State synchronization type.
//  16:   Added Triggers Strings in banks and trigger actions.
//  17:   Added Interactive music bank packaging.
//		  ( all banks gets invalidated since the source content format changed ).
//  18:   Added Random/Sequence support at track level for IM
//  19:   Removed Override Parent in AkStinger
//  20:   Added new positioning information
//	21:	  Added new advanced settings information for kick newest/oldest option
//  22:   Removed old positioning info
//  23:   ---
//  24:	  Fixed Trigger action packaging
//  25:   Added Interactive music track look ahead time.
//  26:   Added Wii compressor for all busses
//  27:   MAJOR BANK REFACTORING
//	28:	  MAJOR BANK REFACTORING( Now hashing strings )	
//	29:	  Changed sources' format bits packaging in accordance to new AkAudioFormat.
//  30:	  Added Feedback Generator
//  31:	  Added Feedback flag in bank header.
//  32:   If the feedback bus isn't set, the feedback properties aren't saved anymore
//  33:   Some plug-ins have additional properties that must be packaged into banks
//	34:   Music tracks: move playlist before base node params (bug fix WG-10631)

#define AK_BANK_READER_VERSION 34

class CAkFileBase;

#pragma pack(push, 1) // pack all structures read from disk

namespace AkBank
{
	static const AkUInt32 BankHeaderChunkID		= AkmmioFOURCC('B', 'K', 'H', 'D');
	static const AkUInt32 BankDataIndexChunkID	= AkmmioFOURCC('D', 'I', 'D', 'X');
	static const AkUInt32 BankDataChunkID		= AkmmioFOURCC('D', 'A', 'T', 'A');
	static const AkUInt32 BankHierarchyChunkID	= AkmmioFOURCC('H', 'I', 'R', 'C');
	static const AkUInt32 BankStrMapChunkID		= AkmmioFOURCC('S', 'T', 'I', 'D');
	static const AkUInt32 BankStateMgrChunkID	= AkmmioFOURCC('S', 'T', 'M', 'G');
	static const AkUInt32 BankFXParamsChunkID	= AkmmioFOURCC('F', 'X', 'P', 'R');
	static const AkUInt32 BankEnvSettingChunkID	= AkmmioFOURCC('E', 'N', 'V', 'S');


	struct AkBankHeader
	{
		AkUInt32 dwBankGeneratorVersion;
		AkUInt32 dwSoundBankID;	 // Required for in-memory banks
        AkUInt32 dwLanguageID;   // Wwise Language ID in witch the bank was created ( 0 for SFX )
		AkUInt32 bFeedbackSupported;	//This bank contain feedback information. (Boolean, but kept at 32 bits for alignment)
	};

	struct AkSubchunkHeader
	{
		AkUInt32 dwTag;			// 4 character TAG
		AkUInt32 dwChunkSize;	// Size of the SubSection in bytes
	};

	struct AkDataIndexStruct
	{
		AkUInt32 ulHeaderOffset;	// Offset since the beginning of the Header Block
		AkUInt32 ulHeaderSize;	// Size of the Header Block
		AkUInt32 ulFileOffset;	// Offset since the beginning of the File Block
		AkUInt32 ulFileSize;		// Size of the File Block
		AkUInt32 ulDataOffset;	// Offset since the beginning of the Data Block
		AkUInt32 ulDataSize;		// Size of the Data Block
	};

	struct AkPathHeader
	{
		AkUniqueID	ulPathID;		// Id
		AkInt		iNumVertices;	// How many vertices there are
	};
	//////////////////////////////////////////////////////////////
	// HIRC structures
	//////////////////////////////////////////////////////////////

	enum AKBKHircType
	{
		HIRCType_State			= 1,
		HIRCType_Sound			= 2,
		HIRCType_Action			= 3,
		HIRCType_Event			= 4,
		HIRCType_RanSeqCntr		= 5,
		HIRCType_SwitchCntr		= 6,
		HIRCType_ActorMixer		= 7,
		HIRCType_Bus			= 8,
		HIRCType_LayerCntr		= 9,
		HIRCType_Segment		= 10,
		HIRCType_Track			= 11,
		HIRCType_MusicSwitch	= 12,
		HIRCType_MusicRanSeq	= 13,
		HIRCType_Attenuation	= 14,
		HIRCType_DialogueEvent	= 15,
		HIRCType_FeedbackBus	= 16,
		HIRCType_FeedbackNode	= 17,
	};

	enum AKBKStringType
	{
		StringType_None			= 0,
		StringType_Bank         = 1,
	};

	struct AKBKSubHircSection
	{
		AKBKHircType	eHircType;
		AkUInt32			dwSectionSize;
	};

	enum AKBKSourceType
	{
		SourceType_Data					= 0,
		SourceType_Streaming			= 1,
		SourceType_PrefetchStreaming	= 2,
	};


	struct AKBKStateItem
	{
		AkUInt32 State;	// Type, sunny or something else
		AkUInt8	 bIsCustom;
		AkUInt32 ID;	// Unique ID of the state
	//Constructor
		AKBKStateItem()
			:State( AK_INVALID_UNIQUE_ID )
			,bIsCustom(false)
			,ID( AK_INVALID_UNIQUE_ID )
		{}
	//Writer
		void Write(CAkFileBase* in_pFIO);
	};

	struct AKBKFileFormat
	{
		AkUInt32 eFileFormat;
		AkUInt32 uiBitsPerSample;
		AkUInt32 uiBlockAlign;
		AkUInt32 uiNbChannels;
		AkUInt32 ulDataOffset;
		AkUInt32 ulDataSize;
		AkUInt32 ulSampleRate;
		AKBKFileFormat()
			:eFileFormat(0)
			,uiBitsPerSample(0)
			,uiBlockAlign(0)
			,uiNbChannels(0)
			,ulDataOffset(0)
			,ulDataSize(0)
			,ulSampleRate(0)
		{}
		void Write(CAkFileBase* in_pFIO);
	};

	struct AKBKSource
	{
		AkUInt32	TypeID; 		// Typically zero if not a plug in
		AkUInt32	StreamingType; 	// In-mem/prefetch/streamed
		AkAudioFormat audioFormat;
		AkUInt32	SourceID;
		AkUInt32	FileID;			// FileID when streaming/prefetch, BankID when from bank
		AkUInt32	uFileOffset;
		AkUInt32	uInMemoryMediaSize;
		AkUInt8		bIsLanguageSpecific;
		AkUInt32	SourceParamsSize;	
		AkUInt8*	pSourceParams;
	//Constructor
		AKBKSource()
			:TypeID(0)
			,StreamingType( SourceType_Data )
			,SourceID( AK_INVALID_UNIQUE_ID )
			,FileID( 0 )
			,uFileOffset( 0 )
			,uInMemoryMediaSize( 0 )
			,bIsLanguageSpecific( 0 )
			,SourceParamsSize(0)
			,pSourceParams(NULL)
		{
			audioFormat.SetAll( 
				48000,
				AK_SPEAKER_SETUP_MONO,
				AK_LE_NATIVE_BITSPERSAMPLE,
				sizeof(AkReal32),	
				AK_LE_NATIVE_SAMPLETYPE,
				AK_LE_NATIVE_INTERLEAVE
			  );}
	//Writer
		void Write(CAkFileBase* in_pFIO);
	};

	struct AKBKFeedbackSource : public AKBKSource
	{
		AKBKFeedbackSource() : AKBKSource()
			, usDeviceID (0)
			, usCompanyID (0)
		{}

		AkUInt16 usDeviceID;
		AkUInt16 usCompanyID;
		AkReal32 fVolumeOffset;
	};

	struct AKBKHashHeader
	{
		AkUInt32 uiType;
		AkUInt32 uiSize;
	};

}//end namespace "AkBank"

#pragma pack(pop)

#endif
