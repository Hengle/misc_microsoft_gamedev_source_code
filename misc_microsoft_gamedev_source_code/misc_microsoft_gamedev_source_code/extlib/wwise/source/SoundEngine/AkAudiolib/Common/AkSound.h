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
// AkSound.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _SOUND_H_
#define _SOUND_H_

#include "AkMonitorData.h"
#include "AkSoundBase.h"
#include "AkParams.h"
#include "AkParameters.h"
#include "AkBankMgr.h"
#include "AkCommon.h"
#include "AkSource.h"

// class corresponding to a Sound
//
// Author:  alessard
class CAkSound : public CAkSoundBase
{
public:
	//Thread safe version of the constructor
	static CAkSound* Create(AkUniqueID in_ulID = 0);

	virtual AkNodeCategory NodeCategory();

	bool IsZeroLatency() const 
{
		return m_Source.IsZeroLatency();
	}

	void IsZeroLatency( bool in_bIsZeroLatency )
	{
		m_Source.IsZeroLatency( in_bIsZeroLatency ); 
	}

	// Call a play on the definition directly
    //
    // Return - AKRESULT - Ak_Success if succeeded
	virtual AKRESULT Play( AkPBIParams& in_rPBIParams );

	virtual AKRESULT ExecuteAction( ActionParams& in_rAction );
	virtual AKRESULT ExecuteActionExcept( ActionParamsExcept& in_rAction );

	void SetSource( AkUInt32 in_PluginID, AkLpCtstr in_pszFilePath, AkAudioFormat in_AudioFormat )
	{
		return m_Source.SetSource( in_PluginID, in_pszFilePath, in_AudioFormat );
	}

	void SetSource( AkUInt32 in_PluginID, const AkMediaInformation& in_rMediaInfo, AkAudioFormat in_AudioFormat )
	{
		return m_Source.SetSource( in_PluginID, in_rMediaInfo, in_AudioFormat );
	}

	void SetSource( AkPluginID in_ulID, void * in_pParam, AkUInt32 in_uSize )
	{
		return m_Source.SetSource( in_ulID, in_pParam, in_uSize );
	}

	AKRESULT SetSrcParam(					// Set the parameter on an physical model source.
		AkPluginID		in_ID,				// Plug-in id.  Necessary for validation that the param is set on the current FX.
		AkPluginParamID in_ulParamID,		// Parameter id.
		void *			in_vpParam,			// Pointer to a setup param block.
		AkUInt32			in_ulSize			// Size of the parameter block.
		)
	{
		return m_Source.SetSrcParam( in_ID, in_ulParamID, in_vpParam, in_ulSize );
	}

	void FreeCurrentSource()
	{
		m_Source.FreeSource();
	}

	virtual AKRESULT RemoveChild(
        AkUniqueID in_ulID				// Input node ID to remove
		) {AKASSERT(!"Cannot Remove a child on a sound"); return AK_NotImplemented;}


	// Get/Set source info and format structs.
    AkSrcTypeInfo * GetSrcTypeInfo()
	{     
		return m_Source.GetSrcTypeInfo();
	}

	AkAudioFormat * GetMediaFormat()
	{
		return m_Source.GetMediaFormat();
	}

	void SetMediaFormat( AkAudioFormat & in_rMediaFormat )
	{
		m_Source.SetMediaFormat( in_rMediaFormat );
	}

	virtual AkObjectCategory Category();

	AKRESULT SetInitialValues( AkUInt8* pData, AkUInt32 ulDataSize, CAkUsageSlot* in_pUsageSlot, bool in_bIsPartialLoadOnly );

	bool HasBankSource()	{ return m_Source.HasBankSource(); }

	virtual AKRESULT PrepareData();
	virtual void UnPrepareData();

	bool SourceLoaded(){ return m_Source.GetSrcTypeInfo()->mediaInfo.Type == SrcTypeNone; }

protected:
	// Constructors
    CAkSound( AkUniqueID in_ulID );

	//Destructor
    virtual ~CAkSound();

	AKRESULT Init(){ return CAkParameterNode::Init(); }

// members
private:
	CAkSource					m_Source;
};
#endif
