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
// AkVPLFormatConvNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_VPL_FORMAT_CONV_NODE_H_
#define _AK_VPL_FORMAT_CONV_NODE_H_

#include "AkLEngineDefs.h"

#include "AkPBI.h"
#include "AkToNativeFormat.h"
#include "AkVPLNode.h"

class CAkVPLFormatConvNode : public CAkVPLNode
{
public:
	/*virtual*/ void	ConsumeBuffer( AkVPLState & io_state );
	/*virtual*/ void	ProcessDone( AkVPLState & io_state );
	virtual void		ReleaseBuffer();

	AKRESULT			TimeSkip( AkUInt32 & io_uFrames );
	AKRESULT			Init(	AkAudioFormat *		in_pFormat,		// Format.
								bool				bIsFull3D );	// Positionning type.	
	AKRESULT			Term( );									

private:
	CAkToNativeFormat		m_Conv;				// Int to float converter.
	AkUInt8*				m_pucData;			// Output buffer
};

#endif //_AK_VPL_FORMAT_CONV_NODE_H_
