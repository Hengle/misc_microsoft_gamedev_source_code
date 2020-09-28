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
// IAkTransportAware.h
//
// Audio context interface for Wwise transport, required by the
// PlayingMgr.
//
//////////////////////////////////////////////////////////////////////

#ifndef _IAK_TRANSPORT_AWARE_H_
#define _IAK_TRANSPORT_AWARE_H_

enum AkPBIStopMode
{
	AkPBIStopMode_Normal = 0,
	AkPBIStopMode_StopAndContinue = 1,
	AkPBIStopMode_StopAndContinueSequel = 2
};

class IAkTransportAware
{
public:
    virtual AKRESULT _Stop( AkPBIStopMode in_eStopMode = AkPBIStopMode_Normal, bool in_bIsFromTransition = false ) = 0;

#ifndef AK_OPTIMIZED
	virtual AKRESULT _StopAndContinue(
		AkUniqueID			in_ItemToPlay,
		AkUInt16				in_PlaylistPosition,
		CAkContinueListItem* in_pContinueListItem
		) = 0;
#endif
};

#endif // _IAK_TRANSPORT_AWARE_H_
