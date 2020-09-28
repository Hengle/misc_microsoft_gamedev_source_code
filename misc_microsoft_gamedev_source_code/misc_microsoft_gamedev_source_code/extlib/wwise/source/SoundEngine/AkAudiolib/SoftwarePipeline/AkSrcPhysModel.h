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
// AkSrcPhysModel.cpp
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_SRC_PHYSICAL_MODEL_H_
#define _AK_SRC_PHYSICAL_MODEL_H_

#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/Plugin/PluginServices/MultiCoreServices.h>

#include "AkSrcBase.h"
#include "AkFXContext.h"

class CAkSrcPhysModel : public CAkVPLSrcNode
{
public:

	//Constructor and destructor
	CAkSrcPhysModel( CAkPBI * in_pCtx );
	virtual ~CAkSrcPhysModel();

	// Data access.
	virtual AKRESULT StartStream();
	virtual void	 StopStream();
	virtual void	 GetBuffer( AkVPLState & io_state );
	virtual void	 ReleaseBuffer();
	virtual void     VirtualOn( AkVirtualQueueBehavior eBehavior );

	AkTimeMs		 GetDuration( void ) const;
	virtual AKRESULT StopLooping();

private:
	IAkSourcePlugin* 		m_pEffect;			// Pointer to a Physical Modelling effect.
	AkPipelineBuffer		m_pluginBuffer;		// Output plugin buffer.	
	CAkSourceFXContext *	m_pSourceFXContext;	// Source FX context (for physical model)
	AkAudioFormat			m_AudioFormat;		// Audio format output by source.
#ifndef AK_OPTIMIZED
	AkUInt32         m_uiID;        // Cached copy of fx id for profiling.
#endif
};

#endif // _AK_SRC_PHYSICAL_MODEL_H_
