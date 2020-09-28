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

////////////////////////////////////////////////////////////////////////
// RumbleBus.h
//
// Final mix bus for the rumble function of the game controllers.
//
// Copyright 2007 Audiokinetic Inc.
//
// Author:  mjean
// Version: 1.0
//
///////////////////////////////////////////////////////////////////////
#pragma once

#include <AK\SoundEngine\Common\IAkPlugin.h>
#include <IAkMotionMixBus.h>

#ifdef XBOX360
#define AK_DIRECTX
#include <Xtl.h>
#include <xboxmath.h>
#endif

#ifdef WIN32
#define AK_DIRECTX
#include <XInput.h>
#endif

#ifdef AK_PS3
#include <cell/pad.h>
#endif

#ifdef RVL_OS
#include <revolution/wpad.h>
#endif

#include "filterdecimator.h"

#define BUFFER_COUNT 8
#define CHANNEL_COUNT 2
#define SAMPLE_COUNT 2
#define BUFFER_SIZE (BUFFER_COUNT * CHANNEL_COUNT * SAMPLE_COUNT)
#define BUFFER_MASK (BUFFER_SIZE - 1)

class RumbleMixBus : public IAkMotionMixBus
{
	AK_USE_PLUGIN_ALLOCATOR();

public:
	RumbleMixBus();
	~RumbleMixBus();

	
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
	virtual void		Stop(){};

	virtual AkReal32	GetPeak();
	virtual	bool		IsStarving();
	virtual bool		IsActive();
	virtual AkChannelMask GetMixingFormat();
	virtual void		SetMasterVolume(AkReal32 in_fVol);

    // Execute effect processing.
	virtual void Execute( 
				AkAudioBuffer * io_pBufferOut // Output buffer interface
#ifdef AK_PS3
				, AK::MultiCoreServices::DspProcess*&	out_pDspProcess	///< Asynchronous DSP process utilities on PS3
#endif 
				) {}
	
private:
	void SendSample(AkReal32 in_fLarge, AkReal32 in_fSmall);

	template <class T>
	AkForceInline bool PulseMod( AkReal32 in_fControl, AkReal32 in_fPeriod, T in_valLow, T in_valHigh, T & out_val );

#ifdef AK_DIRECTX
	XINPUT_VIBRATION	m_oCurrent;		//Current motor speeds
#endif
#ifdef AK_PS3
	CellPadActParam		m_oCurrent;		//Current motor speeds
	AkReal32			m_fAverageSpeed;
#endif

#ifdef RVL_OS
	AkUInt32			m_oCurrent;		//Current motor speeds
	AkReal32			m_fAverageSpeed;
#endif

	AkReal32			m_pData[BUFFER_SIZE];		//Mixing buffer
	AkReal32			m_fLastBufferPower;	//Last audio buffer value
	AkUInt16			m_usWriteBuffer;//Next buffer for mixing
	AkUInt16			m_usReadBuffer;	//Next buffer to send
	AkReal32			m_fPeak;		//Peak value
	AkReal32			m_fVolume;		//Master volume for this player.
	AkUInt8				m_iPlayer;		//Player controller port number
	AkInt16				m_iDrift;		//Drift between the Render calls and the Tick calls
	bool				m_bGotData;		//Do we have anything to send
	bool				m_bStopped;		//Are the motors stopped.
};

template <class T>
bool RumbleMixBus::PulseMod( AkReal32 in_fControl, AkReal32 in_fPeriod, T in_valLow, T in_valHigh, T & out_val )
{
	T iLastVal = out_val;
	//Try to maintain an average speed by weighting the ON and OFF states differently.
	//Below 15% the motor only clicks uselessly.  Rescale the control value so we can use the full range.
	if (in_fControl > 0.15f)
		in_fControl = in_fControl * 0.85f + 0.15f;

	if (m_fAverageSpeed < in_fControl && in_fControl > 0.15f)
	{
		//Below the desired speed, start the motor.
		m_fAverageSpeed += (1 - in_fControl);
		out_val = in_valHigh;
		if (m_fAverageSpeed > 1)
			m_fAverageSpeed = 1;
	}
	else
	{
		//Above the desired speed, stop the motor.
		m_fAverageSpeed -= in_fControl;
		if (m_fAverageSpeed < 0 || in_fControl == 0)
			m_fAverageSpeed = 0;

		out_val = in_valLow;
	}

	return iLastVal != out_val;
}
