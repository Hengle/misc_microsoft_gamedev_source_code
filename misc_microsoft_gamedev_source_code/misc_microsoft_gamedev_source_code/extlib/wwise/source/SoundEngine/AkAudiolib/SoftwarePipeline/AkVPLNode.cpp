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

/////////////////////////////////////////////////////////////////////
//
// AkVPLNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkVPLNode.h"
#include "AudiolibDefs.h"

//-----------------------------------------------------------------------------
// Name: Connect
// Desc: Connects the specified effect as input.
//
// Parameters:
//	CAkVPLNode * in_pInput : Pointer to the effect to connect.
//
// Return:
//	Ak_Success: Effect input was connected.
//  AK_Fail:    Failed to connect to the effect.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLNode::Connect( CAkVPLNode * in_pInput )
{
	AKASSERT( in_pInput != NULL );

	m_pInput = in_pInput;
	return AK_Success;
} // Connect

//-----------------------------------------------------------------------------
// Name: Disconnect
// Desc: Disconnects the specified effect as input.
//
// Parameters:
//	CAkVPLNode * in_pInput : Pointer to the effect to disconnect.
//
// Return:
//	Ak_Success: Effect input was disconnected.
//  AK_Fail:    Failed to disconnect to the effect.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLNode::Disconnect( CAkVPLNode * in_pInput )
{
	m_pInput = NULL;
	return AK_Success;
} // Disconnect

void CAkVPLNode::MergeMarkers( AkPipelineBuffer* in_pAudioBuffer1, AkUInt16 & io_cMarkers, AkBufferMarker *& io_pMarkers )
{
	if( in_pAudioBuffer1->uNumMarkers )
	{
		if( !io_pMarkers )
		{
			io_pMarkers = in_pAudioBuffer1->pMarkers;
			in_pAudioBuffer1->pMarkers = NULL;
		}
		else
		{
			//reallocate markers list
			AkBufferMarker* pNewMarkerList = (AkBufferMarker*)AkAlloc( AK_MARKERS_POOL_ID, sizeof(AkBufferMarker) * ( io_cMarkers + in_pAudioBuffer1->uNumMarkers ) );
			if ( pNewMarkerList )
			{
				//copy markers from original list in new list
				AKPLATFORM::AkMemCpy( pNewMarkerList, io_pMarkers, sizeof( AkBufferMarker ) * io_cMarkers );

				//append new markers
				AKPLATFORM::AkMemCpy( pNewMarkerList + io_cMarkers, 
										in_pAudioBuffer1->pMarkers, sizeof( AkBufferMarker ) * in_pAudioBuffer1->uNumMarkers );

				AkFree( AK_MARKERS_POOL_ID, io_pMarkers );
				AkFree( AK_MARKERS_POOL_ID, in_pAudioBuffer1->pMarkers );

				in_pAudioBuffer1->pMarkers = NULL;
				io_pMarkers = pNewMarkerList;
			}
		}

		io_cMarkers += in_pAudioBuffer1->uNumMarkers;
	}
}

