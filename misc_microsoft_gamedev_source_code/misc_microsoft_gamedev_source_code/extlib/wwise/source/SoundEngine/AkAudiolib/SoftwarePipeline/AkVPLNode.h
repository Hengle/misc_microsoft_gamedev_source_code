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
// AkVPLNode.h
//
// Mananges the execution of the effects applied to a sound or bus.
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_VPL_NODE_H_
#define _AK_VPL_NODE_H_

//-----------------------------------------------------------------------------
// Enums.
//-----------------------------------------------------------------------------
enum VPLNodeState
{
	NodeStateInit		= 0,
	NodeStatePlay		= 1,
	NodeStateStop		= 2,
	NodeStatePause		= 3,

	NodeStateIdle		= 4, // only used for busses for idle time due to virtual voices / wait for first output
#ifdef AK_PS3
	NodeStateProcessing	= 5, // PS3-specific state to handle calling ProcessDone
#endif
};

#include <AK/Tools/Common/AkObject.h>
#include "AkCommon.h"
#include "AkLEngineStructs.h"

using namespace AK;

class CAkVPLNode : public CAkObject
{
public:
	CAkVPLNode()
		:m_pInput( NULL )
	{}

//	Following two virtual methods have been made non-virtual due to significant impact on performance of RunVPL
//	virtual void		GetBuffer( AkVPLState & io_state ) {}			// request data from node
//	virtual void		ConsumeBuffer( AkVPLState & io_state ) {}		// give data to node
	virtual void		ProcessDone( AkVPLState & io_state ) {}			// called when processing has completed
	virtual void		ReleaseBuffer() = 0;							// called when returned data has been consumed downstream
	virtual AKRESULT	TimeSkip( AkUInt32 & io_uFrames ) = 0;
	virtual void        VirtualOn( AkVirtualQueueBehavior eBehavior ) { if ( m_pInput ) m_pInput->VirtualOn( eBehavior ); }
	virtual AKRESULT    VirtualOff( AkVirtualQueueBehavior eBehavior ) { if ( m_pInput ) return m_pInput->VirtualOff( eBehavior ); return AK_Success; }
	virtual AKRESULT	Connect( CAkVPLNode * in_pInput );
	AKRESULT			Disconnect( CAkVPLNode * in_pInput );

	static void			MergeMarkers( AkPipelineBuffer* in_pAudioBuffer1, AkUInt16 & io_cMarkers, AkBufferMarker *& io_pMarkers );

protected:
	CAkVPLNode *		m_pInput;		// Pointer to connected input node.
};

#endif  // _AK_VPL_NODE_H_
