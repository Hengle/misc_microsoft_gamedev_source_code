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
// AkContextualMusicSequencer.h
//
// Action sequencer for music contexts.
// Holds a list of pending musical actions, stamped with sample-based
// timing. 
// For example, a sequence context would enqueue actions on children 
// nodes that are scheduled to play or stop in the near future.
//
//////////////////////////////////////////////////////////////////////
#ifndef _CTX_MUSIC_SEQUENCER_H_
#define _CTX_MUSIC_SEQUENCER_H_

#include "AkList2.h"
#include "AkMusicStructs.h"
#include "AudiolibDefs.h"

class CAkMusicPBI;

struct AkMusicAction
{
    AkInt32             iTime;

    CAkMusicPBI * 	    pTargetPBI;
    bool operator==( const AkMusicAction & in_cmp )
    {
        return ( pTargetPBI == in_cmp.pTargetPBI );
    }
};

// TODO Rename this class as a Music PBI sequencer.
class CAkContextualMusicSequencer
{
public:
    CAkContextualMusicSequencer();
    virtual ~CAkContextualMusicSequencer();

    AKRESULT Init();
    void Term();

    // Returns a pointer to the action enqueued if successful, NULL otherwise.
    const AkMusicAction * ScheduleAction( 
        AkMusicAction & in_action
        );

    // Returns AK_NoMoreData when there is no action to be executed in next frame (out_action is invalid).
    // Otherwise, returns AK_DataReady.
    // NOTE: When actions are dequeued with this method, they are still referenced. Caller needs to
    // release them explicitly.
    AKRESULT PopImminentAction(
		AkInt32 in_iFrameDuration,			// Number of samples to process.
        AkMusicAction & out_action
        );

    // Removes from sequencer all actions that reference the specified target. 
    void ClearActionsByTarget( 
        const void * in_pTargetAddress
        );

    // Remove all actions from sequencer (ref counted targets are released).
    void Flush();

    // Sequencer is empty. For debug purposes.
    bool IsEmpty() { return m_listActions.IsEmpty(); }

    // Time control.
    // Call Tick at each audio frame, to be able to dequeue subsequent actions.
    inline void Tick( 
		AkInt32 in_iFrameDuration 
		) 
	{ 
		m_iNow += in_iFrameDuration; 
	}
    
    // Get Now (in samples).
    AkInt32 Now() { return m_iNow; }

    // Change Now (in samples).
    void Now( 
        AkInt32 in_iNow                     // New absolute time. 
        ) { m_iNow = in_iNow; }

private:
    typedef CAkList2<AkMusicAction, const AkMusicAction&, AkAllocAndFree> SeqActionsList;
    SeqActionsList m_listActions;

    AkInt32    m_iNow; // Current time, in samples.
};

#endif //_CTX_MUSIC_SEQUENCER_H_
