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
#ifndef _AK_FX_MEM_ALLOC_H_
#define _AK_FX_MEM_ALLOC_H_

#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>

class AkFXMemAlloc
	: public AK::IAkPluginMemAlloc
{
public:
	AkFXMemAlloc() : m_poolId( AK_INVALID_POOL_ID ) {}
	
	void SetPoolId( AkMemPoolId in_poolId ) { m_poolId = in_poolId; }

	static AkFXMemAlloc * GetUpper() { return &m_instanceUpper; }
	static AkFXMemAlloc * GetLower() { return &m_instanceLower; }

	// AK::IAkPluginMemAlloc

#if defined (AK_MEMDEBUG)
	    // debug malloc
	    virtual void * dMalloc( 
            size_t in_ulSize,		// size
            AkTChar* in_pszFile,	// file name
		    AkUInt32 in_uLine		// line number
		    );
#else
	    // release malloc
	    virtual void * Malloc( 
            size_t in_uSize		// size
            );
#endif
	    
        virtual void Free(
            void * in_pMemAddress	// starting address
            );

private:
	AkMemPoolId m_poolId;

	static AkFXMemAlloc m_instanceUpper;
	static AkFXMemAlloc m_instanceLower;
};
#endif
