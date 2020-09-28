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
// AkSource.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AKSOURCE_H_
#define _AKSOURCE_H_

#include "AkCommon.h"
#include <AK/Tools/Common/AkArray.h>
#include "AkPoolSizes.h"
#include "AudioLibDefs.h"

class CAkRegisteredObj;

//-----------------------------------------------------------------------------
// Name: struct AkSrcTypeInfo
// Desc: Defines the parameters of an audio source: e.g. file, physical 
//       modelling, or memory.
//-----------------------------------------------------------------------------

struct AkMediaInformation
{
	// Not using constructor as we don't want the bank reader to uselessly init those values before filling them for real.
	void SetDefault( AkSrcType in_SrcType )
	{
		sourceID			= 0;
		uFileID				= AK_INVALID_FILE_ID;
		uFileOffset			= 0;
		uInMemoryMediaSize	= 0;
		bIsLanguageSpecific = false;
		bPrefetch			= false;
		Type				= in_SrcType;
	}

	AkUniqueID	sourceID;
	AkFileID	uFileID;
	AkUInt32	uFileOffset;
	AkUInt32	uInMemoryMediaSize;
	AkUInt32	bIsLanguageSpecific : 1;
	AkUInt32	bPrefetch    : 1;       // True=data in-memory, false=data not prefetched.
	AkUInt32	/*AkSrcType*/  Type    :SRC_TYPE_NUM_BITS; // File, physical modelling, etc source.
};

struct AkSrcTypeInfo
{	
	AkMediaInformation		mediaInfo;

    void *					pvSrcDesc;              // Pointer to the source descriptor, e.g.: filename for file, init params blob for plugin sources.
    AkUInt32				ulSrcDescSize;          // Size of the source descriptor (in bytes).
    AkUInt32				dwID;             		// Device ID for stream files, effect ID for source plugins.
    AkUInt32				uStreamingLookAhead;	// Streaming look-ahead (in samples, at the native frame rate).
};

class CAkSource : public CAkObject
{
public:
	// Constructor and destructor
	CAkSource();
	~CAkSource();

	void SetSource(			            // File sources.
		AkUInt32  in_PluginID,			// Conversion plug-in id.
		AkLpCtstr in_szFileName,		// Filename.
		AkAudioFormat in_AudioFormat
		);

	void SetSource(			            // File sources.
		AkUInt32  in_PluginID,			// Conversion plug-in id.
		AkMediaInformation in_MediaInfo,
		AkAudioFormat in_AudioFormat
		);

	void SetSource(			            // Physical modelling source.
		AkPluginID		in_ID,			// Plug-in id.
		void *		    in_pParam,		// Pointer to a setup param block.
		AkUInt32		in_uSize		// Size of the parameter block.
		);

	AKRESULT SetSrcParam(		// Set the parameter on an physical model source.
		AkPluginID		in_ID,				// Plug-in id.  Necessary for validation that the param is set on the current FX.
		AkPluginParamID in_ParamID,			// Parameter id.
		void *			in_vpParam,			// Pointer to a setup param block.
		AkUInt32			in_ulSize			// Size of the parameter block.
		);

	void FreeSource();

	// Get/Set source info and format structs.
	AkSrcTypeInfo *		GetSrcTypeInfo() { return &m_sSrcTypeInfo; }

	AkAudioFormat *		GetMediaFormat() { return &m_sMediaFormat; }
    void				SetMediaFormat( AkAudioFormat & in_rMediaFormat );

	void LockDataPtr( void *& out_ppvBuffer, AkUInt32 & out_ulDataSize, CAkPBI* in_pPBI );
	void UnLockDataPtr();

	bool IsZeroLatency() const 
	{
		return m_sSrcTypeInfo.mediaInfo.bPrefetch;
	}

	void IsZeroLatency( bool in_bIsZeroLatency )
	{
		m_sSrcTypeInfo.mediaInfo.bPrefetch = in_bIsZeroLatency; 
	}

    AkUInt32 StreamingLookAhead()
    {
        return m_sSrcTypeInfo.uStreamingLookAhead;
    }

    void StreamingLookAhead( AkUInt32 in_uStmLookAhead )
    {
        m_sSrcTypeInfo.uStreamingLookAhead = in_uStmLookAhead;
    }

	bool HasBankSource()	{ return m_bHasSource; }

	AKRESULT PrepareData();
	void UnPrepareData();

private:

	void SetDefaultParams();

	AkSrcTypeInfo				m_sSrcTypeInfo;			// Source info description.
	AkAudioFormat				m_sMediaFormat;			// Audio format.

	bool						m_bHasSource;
};

#endif //_AKSOURCE_H_
