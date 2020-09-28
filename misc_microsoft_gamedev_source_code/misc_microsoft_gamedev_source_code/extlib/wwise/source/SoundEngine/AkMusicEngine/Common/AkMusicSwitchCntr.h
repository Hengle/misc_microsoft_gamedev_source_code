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
// AkMusicSwitchCntr.h
//
// Music Switch container definition.
//
//////////////////////////////////////////////////////////////////////
#ifndef _MUSIC_SWITCH_CNTR_H_
#define _MUSIC_SWITCH_CNTR_H_

#include "AkMusicTransAware.h"
#include "AkSwitchAware.h"
#include "AkPreparationAware.h"

class CAkMusicCtx;
class CAkMusicSwitchCtx;

class CAkMusicSwitchCntr : public CAkMusicTransAware
						 , public CAkPreparationAware
{
public:

    // Thread safe version of the constructor.
	static CAkMusicSwitchCntr * Create(
        AkUniqueID in_ulID = 0
        );

	AKRESULT SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize );

    // Return the node category.
	virtual AkNodeCategory NodeCategory();

    virtual AKRESULT CanAddChild(
        CAkAudioNode * in_pAudioNode 
        );

    // Context factory.
    virtual CAkMatrixAwareCtx * CreateContext( 
        CAkMatrixAwareCtx * in_pParentCtx,
        CAkRegisteredObj * in_GameObject,
        UserParams &  in_rUserparams,
        CAkSegmentBucket *& out_pFirstRelevantBucket
        );

    // Play the specified node
    //
    // Return - AKRESULT - Ak_Success if succeeded
	virtual AKRESULT Play( AkPBIParams& in_rPBIParams );

    void SetSwitchGroup( 
        AkUInt32 in_ulGroup, 
        AkGroupType in_eGroupType 
        );

    void SetDefaultSwitch( AkUInt32 in_Switch ) { m_ulDefaultSwitch = in_Switch; }

	AKRESULT SetSwitchAssocs(
		AkUInt32 in_uNumAssocs,
		AkMusicSwitchAssoc* in_pAssocs
		);

	virtual AKRESULT ModifyActiveState( AkUInt32 in_stateID, bool in_bSupported );
	virtual AKRESULT PrepareData();
	virtual void UnPrepareData();

	bool ContinuePlayback(){ return m_bIsContinuePlayback; }
	void ContinuePlayback( bool in_bContinuePlayback ){ m_bIsContinuePlayback = in_bContinuePlayback; }

    // Interface for Wwise
    // ----------------------
    AKRESULT ObsoleteAddSwitch( 
        AkSwitchStateID in_switchID 
        );

	void ObsoleteRemoveSwitch( 
        AkSwitchStateID in_switchID 
        );

	AKRESULT ObsoleteSetNodeInSwitch(
		AkSwitchStateID in_switchID,
		AkUniqueID		in_nodeID
		);


    // Interface for Contexts
    // ----------------------

    AkUInt32 GetSwitchGroup() { return m_ulGroupID; }
    AkGroupType GetSwitchGroupType() { return m_eGroupType; }
    AKRESULT GetSwitchNode(
        AkUniqueID in_switchID,
        AkUniqueID & out_nodeID
        );


protected:
    CAkMusicSwitchCntr( 
        AkUniqueID in_ulID
        );
    virtual ~CAkMusicSwitchCntr();
    AKRESULT Init() { return CAkMusicNode::Init(); }
	void	 Term();

private:

    // Switch ID to node ID association.
    typedef CAkKeyArray<AkUInt32,AkUniqueID> SwitchNodeAssoc;
    SwitchNodeAssoc m_arSwitchNode;

    // Switch/state data.
    AkUInt32	m_ulGroupID;				// May either be a state group or a switch group
    AkUInt32	m_ulDefaultSwitch;			// Default value, to be used if none is available, 
    AkGroupType m_eGroupType;				// Is it binded to state or to switches
	bool		m_bIsContinuePlayback;
};

#endif //_MUSIC_SWITCH_CNTR_H_
