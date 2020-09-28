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

#include "stdafx.h" 

#include "AkFXMemAlloc.h"

#include <AK/SoundEngine/Common/AkMemoryMgr.h>

AkFXMemAlloc AkFXMemAlloc::m_instanceUpper;
AkFXMemAlloc AkFXMemAlloc::m_instanceLower;

#if defined (AK_MEMDEBUG)

// debug malloc
void * AkFXMemAlloc::dMalloc( 
    size_t   in_ulSize,		// size
    AkTChar* in_pszFile,	// file name
	AkUInt32 in_uLine		// line number
	)
{
#ifdef AK_PS3
	return AK::MemoryMgr::Malign( m_poolId, in_uSize, 16 ); // FX memory needs to always be 16-byte aligned on PS3
#else
	return AK::MemoryMgr::dMalloc( m_poolId, in_ulSize, in_pszFile, in_uLine );
#endif
}

#else

// release malloc
void * AkFXMemAlloc::Malloc( 
    size_t in_uSize		// size
    )
{
#ifdef AK_PS3
	return AK::MemoryMgr::Malign( m_poolId, in_uSize, 16 ); // FX memory needs to always be 16-byte aligned on PS3
#else
	return AK::MemoryMgr::Malloc( m_poolId, in_uSize );
#endif
}

#endif

void AkFXMemAlloc::Free(
    void * in_pMemAddress	// starting address
    )
{
#ifdef AK_PS3
	AK::MemoryMgr::Falign( m_poolId, in_pMemAddress );
#else
	AK::MemoryMgr::Free( m_poolId, in_pMemAddress );
#endif
}
