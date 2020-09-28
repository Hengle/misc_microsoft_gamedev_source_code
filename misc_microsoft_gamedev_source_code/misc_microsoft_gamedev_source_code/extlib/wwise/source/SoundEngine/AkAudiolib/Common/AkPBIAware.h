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
// AkPBIAware.h
//
// Declaration of the CAkPBIAware.  This is a virtual base class
// taking care of the creation of the PBI.
//
//////////////////////////////////////////////////////////////////////
#ifndef _AKPBIAWARE_H_
#define _AKPBIAWARE_H_

#include <AK/SoundEngine/Common/AkTypes.h>

#include "AkIndexable.h"

class CAkPBI;
class CAkSoundBase;
class CAkSource;

struct AkPBIParams;

class CAkPBIAware : public CAkIndexable
{
public:
	CAkPBIAware( AkUniqueID in_ulID );
	~CAkPBIAware();

	virtual CAkPBI* CreatePBI( CAkSoundBase* in_pSound,
							   CAkSource*	 in_pSource,
                               AkPBIParams&  in_rPBIParams,
                               AkPriority    in_priority ) const;
};

#endif
