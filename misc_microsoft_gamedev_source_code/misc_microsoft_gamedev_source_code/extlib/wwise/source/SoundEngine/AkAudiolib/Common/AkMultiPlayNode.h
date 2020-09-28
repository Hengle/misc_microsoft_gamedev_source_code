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
// AkMultiPlayNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_MULTI_PLAY_NODE_H_
#define _AK_MULTI_PLAY_NODE_H_

#include "AkContainerBase.h"
#include "AkContinuationList.h"

class SafeContinuationList
{
public:
	SafeContinuationList( AkPBIParams& in_rPBIParams, CAkMultiPlayNode* in_pMultiPlayNode );

	CAkContinuationList* Get(){ return m_spBackupContinuationList; }

private:
	CAkSmartPtr<CAkContinuationList> m_spBackupContinuationList;
};

class AkContParamsAndPath
{
public:
	AkContParamsAndPath( ContParams* in_pFrom );
	~AkContParamsAndPath();

	ContParams& Get(){ return m_continuousParams; }

private:
	ContParams	m_continuousParams;;
};

// class corresponding to a Container
//
// Author:  alessard
class CAkMultiPlayNode : public CAkContainerBase
{
public:

	CAkContinuationList*	ContGetList( CAkContinuationList* in_pList );
	AKRESULT				ContRefList( CAkContinuationList* in_pList );
	AKRESULT				ContUnrefList( CAkContinuationList* in_pList );

protected:

	// Constructors
    CAkMultiPlayNode( AkUniqueID in_ulID );

	//Destructor
    virtual ~CAkMultiPlayNode();

	AKRESULT	Init();
	void		Term();

	virtual bool IsContinuousPlayback();

	AKRESULT AddMultiplayItem( AkContParamsAndPath& in_rContParams, AkPBIParams& in_rParams, SafeContinuationList& in_rSafeContList /*warning, not necessarily the one in the in_rContParams*/ );

	AKRESULT PlayAndContinueAlternateMultiPlay( AkPBIParams& in_rPBIParams );

private:

	typedef CAkKeyList< CAkSmartPtr<CAkContinuationList>, AkUInt32, AkAllocAndKeep > AkListContParameters;
	AkListContParameters m_listContParameters;
};

#endif //_AK_MULTI_PLAY_NODE_H_
