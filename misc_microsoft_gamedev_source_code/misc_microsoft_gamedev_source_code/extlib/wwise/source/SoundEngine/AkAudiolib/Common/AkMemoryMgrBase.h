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
// AkMemoryMgrBase.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _MEMORY_MGR_BASE_H_
#define _MEMORY_MGR_BASE_H_

// AkMemoryMgr is the SDK-able part of the memory manager.
// Move the appropriate functions there.
#include <AK/SoundEngine/Common/AkMemoryMgr.h>

#include "AkListBare.h"

#ifdef RVL_OS
#include <AK/Tools/Wii/AkInterruptLock.h>
#define AUTOLOCKPOOL( _pool ) AkAutoInterruptLock lock
#else
#include <AK/Tools/Common/AkAutoLock.h>
#include <AK/Tools/Common/AkLock.h>
#define AUTOLOCKPOOL( _pool ) AkAutoLock<CAkLock> PoolLock( _pool->lock )
#endif

#define FREE_BLOCK_FLAG			0xFBFBFBFB
#define ALLOCATED_BLOCK_FLAG	0xABABABAB

struct AkLinkedBuffer
{
	AkLinkedBuffer * pNextItem;
};
typedef AkListBare<AkLinkedBuffer> BuffersList;

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
struct AkMemPool
{
	AkMemPool()
	{
		Init();
	}

	void Init();

	AkUInt32		ulNumBlocks;			// number of blocks
	AkUInt32		ulBlockSize;			// how big our blocks are
	AkChar*         pcAllocAddress;			// address used during allocation ( different from pcMemAddress when BlockAlign != 0 )
	AkChar*			pcMemAddress;			// starting address used for block allocation
	BuffersList		listBuffers;			// Fixed-sized blocks pool buffer list.
	AkUInt32/*AkMemPoolAttributes*/ eAttributes;	// pool attribute flags.
	AkUInt32		bAllocated :1;			// has to be freed when done
#ifndef RVL_OS
	CAkLock	    	lock;					// pool-level memory lock
#endif
	void			*pTlsfPool;				// points to start of memory pool for tlsf management
	AkUInt32		ulAlign;				// auto-memory alignment	

	// Profile info
#ifndef AK_OPTIMIZED

#define MAX_MEM_POOL_NAME_SIZE 64
	AkUInt32		ulAllocs;									// Number of Alloc calls since start
	AkUInt32		ulFrees;									// Number of Free calls since start
	AkUInt32        ulPeakUsed;									// Peak memory allocation (in bytes)
	AkTChar			ptcName[MAX_MEM_POOL_NAME_SIZE];	        // Name of pool
    bool            bDoMonitor;									// Flag for monitoring (do not monitor is false)
#endif

};

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

namespace AK
{
	namespace MemoryMgr
	{
		extern AKRESULT InitBase( AkInt32 in_iNumPools );

		extern AkInt32		s_iMaxNumPools;
		extern AkInt32		s_iNumPools;
		extern AkMemPool*	s_pMemPools; 

#ifndef AK_OPTIMIZED
		void CheckForMemUsage( AkMemPool* in_pMemPool );
#endif
	}
}

#endif // _MEMORY_MGR_BASE_H_

