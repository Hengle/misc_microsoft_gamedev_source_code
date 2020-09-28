/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1 Patch 4  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

#include "stdafx.h"
#include "AkSequencableSegmentCtx.h"
#include "AkMatrixSequencer.h"
#include "AkMusicSegment.h"

CAkSequencableSegmentCtx::CAkSequencableSegmentCtx(
    CAkMusicSegment *   in_pSegmentNode,
    CAkMusicCtx *       in_pParentCtx
    )
:CAkMatrixAwareCtx( in_pParentCtx )
,m_pSegmentNode( in_pSegmentNode )
{
	if( m_pSegmentNode )
		m_pSegmentNode->AddRef();
}

CAkSequencableSegmentCtx::~CAkSequencableSegmentCtx()
{
    CAkSegmentChain::Term();
	if( m_pSegmentNode )
		m_pSegmentNode->Release();
}

AKRESULT CAkSequencableSegmentCtx::Init( 
    CAkRegisteredObj *  in_GameObject,
    UserParams &        in_rUserparams,
    CAkSegmentBucket *& out_pFirstBucket    // First (relevant) bucket.
    )
{
    CAkSegmentChain::Init();
    AKRESULT eResult = CAkMatrixAwareCtx::Init( in_GameObject, in_rUserparams );
    if ( eResult == AK_Success )
    {
        // Create segment context.
        CAkSegmentCtx * pChildCtx = m_pSegmentNode->CreateSegmentCtxAndAddRef( this, in_GameObject, in_rUserparams );
        if ( pChildCtx )
        {
            out_pFirstBucket = EnqueueSegment( 0, pChildCtx );
			
			// Release child segment context (only) once it has been enqueued in the chain.
			pChildCtx->Release();

			if ( out_pFirstBucket )
            {
				InitActiveSegment( out_pFirstBucket );

				// Attach a play action and a stop action to this segment item.
				AkMusicFade fadeInfo = { 0, };
                out_pFirstBucket->AttachPlayCmd( fadeInfo, 0 );

				AkInt32 iStmLookAhead;
#ifndef AK_OPTIMIZED				
				if( in_rUserparams.CustomParam.ui32Reserved & AK_EVENTFROMWWISE_RESERVED_BIT )
				{
					iStmLookAhead = out_pFirstBucket->Prepare( m_pSegmentNode->StartPos() - m_pSegmentNode->PreEntryDuration() );
				}
				else
				{
#endif
                // Default prepare segment context (at entry cue, play pre-entry).
					iStmLookAhead = out_pFirstBucket->Prepare( 0, fadeInfo, true );
#ifndef AK_OPTIMIZED
				}
#endif
				AkInt32 iRelativeStopTime = m_pSegmentNode->ActiveDuration() + m_pSegmentNode->PostExitDuration();
#ifndef AK_OPTIMIZED
				if( in_rUserparams.CustomParam.ui32Reserved & AK_EVENTFROMWWISE_RESERVED_BIT )
				{
					if( m_pSegmentNode->StartPos() >= (AkUInt32)m_pSegmentNode->PreEntryDuration() )
					{
						iRelativeStopTime += ( m_pSegmentNode->PreEntryDuration() - m_pSegmentNode->StartPos() );
					}
				}
#endif

				// Attach a stop action to this segment. To occur at the end of post-exit.
				out_pFirstBucket->AttachStopCmd( fadeInfo.transitionTime, 
                                                fadeInfo.eFadeCurve,
                                                iRelativeStopTime );

                // Set chain's top-level property. 
                if ( !m_pParentCtx )
                    eResult = SetAsTopLevel();

                // Enqueue a NULL, ending SEGMENT bucket.
                if ( eResult == AK_Success )
                {
                    CAkSegmentBucket * pEndingBucket = EnqueueSegment( m_pSegmentNode->ActiveDuration(), NULL );
                    if ( !pEndingBucket )
                    {
                        eResult = AK_Fail;
                    }
                }
                
            }
			else
			{
				eResult = AK_Fail;
			}
        }
		else
		{
			eResult = AK_Fail;
		}
    }
    return eResult;
}


// Matrix Aware Context implementation.
// ----------------------------------------------------------

// Sequencer access.
void CAkSequencableSegmentCtx::Process(
	AkInt32 in_iFrameDuration				// Number of samples to process.
	)
{
    AddRef();
    ProcessChainItems( in_iFrameDuration );
    Release();
}
void CAkSequencableSegmentCtx::ProcessRelative(
    AkInt32 in_iSyncTime,					// Sync time to use (controlled from above).
	AkInt32 in_iFrameDuration				// Number of samples to process.
    )
{
	ProcessRelativeChainItems( in_iSyncTime,	in_iFrameDuration );
}

CAkSegmentChain * CAkSequencableSegmentCtx::GetActiveChain()
{
    return this;
}

bool CAkSequencableSegmentCtx::CanRestartPlaying()
{
	return true;
}

// Revert playback commands that were disabled. Only possible if it can restart playing.
// Default implementation does nothing.
void CAkSequencableSegmentCtx::RestartPlaying()
{
	AKASSERT( CanRestartPlaying() );
}

// For Music Renderer's music contexts look-up: concrete contexts must return their own node.
CAkMusicNode * CAkSequencableSegmentCtx::Node()
{
    return m_pSegmentNode;
}

// Override SegmentCtx OnStopped(): Need to unregister ourselves to sequencer.
void CAkSequencableSegmentCtx::OnStopped( 
	AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually stop playing.
    )
{
    // Add ref ourselves locally in case child stopping destroys us.
    AddRef();

    CAkMusicCtx::OnStopped( in_uSubFrameOffset );

	ClearChain();

    Release();
}

// CAkSegmentChain implementation
// ----------------------------------------------------------
CAkMatrixAwareCtx * CAkSequencableSegmentCtx::MatrixAwareCtx()
{
    return this;
}

