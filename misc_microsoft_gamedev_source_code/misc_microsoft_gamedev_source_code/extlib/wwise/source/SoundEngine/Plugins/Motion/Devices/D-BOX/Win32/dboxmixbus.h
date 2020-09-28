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

#include <IAkMotionMixBus.h>
#include "..\Common\filterdecimator.h"
#include "sink.h"
#include "VistaSink.h"
#include "..\Common\DBoxDefs.h"

class DBoxMixBus : public IAkMotionMixBus
{
public:
	AK_USE_PLUGIN_ALLOCATOR();

	DBoxMixBus();
	~DBoxMixBus();

	//IAkPlugin
	/// Release the resources upon termination of the plug-in.
	/// \return AK_Success if successful, AK_Fail otherwise
	/// \aknote The self-destruction of the plug-in must be done using AK_PLUGIN_DELETE() macro. \endaknote
	/// \sa
	/// - \ref iakeffect_term
	virtual AKRESULT Term( 
		AK::IAkPluginMemAlloc * in_pAllocator 	///< AkInterface to memory allocator to be used by the plug-in
		) ;

	/// The reset action should perform any actions required to reinitialize the state of the plug-in 
	/// to its original state.
	/// \return AK_Success if successful, AK_Fail otherwise.
	/// \sa
	/// - \ref iakeffect_reset
	virtual AKRESULT Reset( ) ;

	/// Plug-in information query mechanism used when the sound engine requires information 
	/// about the plug-in to determine its behavior
	/// \return AK_Success if successful.
	/// \sa
	/// - \ref iakeffect_geteffectinfo
	virtual AKRESULT GetPluginInfo( 
		AkPluginInfo & out_rPluginInfo	///< Reference to the plug-in information structure to be retrieved
		) ;

	//IAkMotionMixBus
	virtual AKRESULT 	Init(AK::IAkPluginMemAlloc * in_pAllocator, AkPlatformInitSettings * io_pPDSettings, AkUInt8 in_iPlayer);

	virtual AKRESULT	MixAudioBuffer( AkAudioBufferFinalMix &io_rBuffer );
	virtual AKRESULT	MixFeedbackBuffer( AkAudioBufferFinalMix &io_rBuffer );
	virtual AKRESULT	RenderData() ;
	virtual void		CommandTick();
	virtual void		Stop();

	virtual AkReal32	GetPeak();
	virtual	bool		IsStarving();
	virtual bool		IsActive();
	virtual AkChannelMask GetMixingFormat();
	virtual void		SetMasterVolume(AkReal32 in_fVol);

	virtual void Execute( 
				AkAudioBuffer * io_pBuffer		///< Out audio buffer data structure
				) {}

private:
	void MixBuffer(AkAudioBuffer &io_rBuffer);
	AKRESULT MixFilteredBuffer(AkAudioBufferFinalMix &io_rBuffer);
__declspec(align(16)) AkReal32 m_aCurrentMixBuffer[DBOX_CHANNELS*DBOX_MAX_REFILL_FRAMES];	//Buffer for current mix
	AkPipelineBuffer	m_oBuffer;
	FilterDecimator		m_oDecimator;
	AccelerationFilter	m_oAccelFilter;

	CSinkBase *			m_pSink;
	AkReal32			m_fVolume;	//Master Volume for this player

	bool				m_bGotData;
};