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

#ifndef _AKDIALOGUEEVENT_H
#define _AKDIALOGUEEVENT_H

#include <AK/SoundEngine/Common/AkTypes.h>
#include "AkIndexable.h"

#include "AkDecisionTree.h"

class CAkDialogueEvent
	: public CAkIndexable
{
public:
	static CAkDialogueEvent* Create( AkUniqueID in_ulID = 0 );

	virtual AkUInt32 AddRef();
	virtual AkUInt32 Release();

	AKRESULT SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize );

	AKRESULT ResolveArgumentValueNames( AkLpCtstr * in_aNames, AkArgumentValueID * out_pPath, AkUInt32 in_cPath );

	AkForceInline AkUniqueID ResolvePath( AkArgumentValueID * in_pPath, AkUInt32 in_cPath )
	{
		return m_decisionTree.ResolvePath( in_pPath, in_cPath );
	}
	
private:
    CAkDialogueEvent( AkUniqueID in_ulID );
    virtual ~CAkDialogueEvent();

	AKRESULT Init();

	AkUniqueID *	m_pArguments;
	AkDecisionTree	m_decisionTree;
};

#endif // _AKDIALOGUEEVENT_H
