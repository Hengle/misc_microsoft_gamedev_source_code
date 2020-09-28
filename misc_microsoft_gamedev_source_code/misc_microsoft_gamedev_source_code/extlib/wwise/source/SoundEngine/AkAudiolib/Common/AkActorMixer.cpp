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
// AkActorMixer.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkBankFloatConversion.h"
#include "AkActorMixer.h"
#include "AkAudioLibIndex.h"

extern AkMemPoolId g_DefaultPoolId;

CAkActorMixer::CAkActorMixer(AkUniqueID in_ulID)
:CAkActiveParent<CAkParameterNode>(in_ulID)
{
}

CAkActorMixer::~CAkActorMixer()
{
}

CAkActorMixer* CAkActorMixer::Create(AkUniqueID in_ulID)
{
	CAkActorMixer* pActorMixer = AkNew(g_DefaultPoolId,CAkActorMixer(in_ulID));
	if( pActorMixer )
	{
		if( pActorMixer->Init() != AK_Success )
		{
			pActorMixer->Release();
			pActorMixer = NULL;
		}
	}
	return pActorMixer;
}

AkNodeCategory CAkActorMixer::NodeCategory()
{
	return AkNodeCategory_ActorMixer;
}

AKRESULT CAkActorMixer::CanAddChild( CAkAudioNode * in_pAudioNode )
{
	AKASSERT(in_pAudioNode);

	AKRESULT eResult = AK_Success;	
	if( in_pAudioNode->NodeCategory() == AkNodeCategory_Bus )
	{
		eResult = AK_NotCompatible;
	}
	else if(in_pAudioNode->Parent() != NULL)
	{
		eResult = AK_ChildAlreadyHasAParent;
	}
	else if(m_mapChildId.Exists(in_pAudioNode->ID()))
	{
		eResult = AK_AlreadyConnected;
	}
	else if(ID() == in_pAudioNode->ID())
	{
		eResult = AK_CannotAddItseflAsAChild;
	}
	return eResult;	
}

AKRESULT CAkActorMixer::Play( AkPBIParams& in_rPBIParams )
{
	AKASSERT(!"Shouldn't be called on an Actor mixer"); 
	return AK_NotImplemented;
}

AKRESULT CAkActorMixer::SetInitialValues(AkUInt8* in_pData, AkUInt32 in_ulDataSize)
{
	AKRESULT eResult = AK_Success;

	//Read ID
	// We don't care about the ID, just skip it
	SKIPBANKDATA(AkUInt32, in_pData, in_ulDataSize);

	//ReadParameterNode
	eResult = SetNodeBaseParams(in_pData, in_ulDataSize, false);

	if( eResult == AK_Success )
	{
		eResult = SetChildren( in_pData, in_ulDataSize );
	}

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}
