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
// AkMemoryMgrBase.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h" 

#include "AkMemoryMgrBase.h"
#include "AkMonitor.h"

#include <new>
#undef new

// Header to the actual memory manager
#include "tlsf/tlsf.h"

#if defined (AK_MEMDEBUG)
#include "AkMemTracker.h"

AkMemTracker g_memTracker;
#endif

void AkMemPool::Init()
{
	ulNumBlocks				= 0;
	ulBlockSize				= 0;
	pcAllocAddress			= NULL;
	pcMemAddress			= NULL;
	eAttributes				= AkMalloc;
	pTlsfPool				= NULL;
	ulAlign					= 0;

#ifndef AK_OPTIMIZED
	ulAllocs         = 0;
	ulFrees          = 0;
	ulPeakUsed		 = 0;
	ptcName[0]       = 0;
	bDoMonitor		 = true;
#endif
}

namespace AK
{

namespace MemoryMgr
{

// "Protected"

AkInt32 s_iMaxNumPools = 0;		// how many pools we can manage at most
AkInt32 s_iNumPools = 0;		// how many pools we have
AkMemPool* s_pMemPools = NULL;	// the pools we manage

// Privates

static bool s_bInitialized = false;

// used for getting information about a given tlsf pool
struct tlsf_pool_info
{
	AkUInt32 uUsed;
	AkUInt32 uMaxFreeBlock;
};

// function that walks the tlsf pool for profiling
static void tlsf_pool_walker_profiling(void* ptr, size_t size, int used, void* user, char *fileName, int lineNumber)
{
	tlsf_pool_info *info = (tlsf_pool_info *)user;
	
	if(used)
	{
		info->uUsed += (AkUInt32)size;
	}
	
	if(size > info->uMaxFreeBlock)
	{
		info->uMaxFreeBlock = (AkUInt32)size;
	}
}

//////////////////////////////////////////////////////////////////////////////////
// MEMORY MANAGER BASE IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////////////

AKRESULT InitBase( AkInt32 in_iNumPools )
{
	// create the pool ids array
	s_pMemPools = (AkMemPool *) AK::AllocHook( sizeof( AkMemPool ) * in_iNumPools );

	if(s_pMemPools == NULL)
	{
		return AK_Fail;
	}

	for ( AkInt32 lPool = 0; lPool < in_iNumPools; lPool++ )
	{
		::new( s_pMemPools + lPool ) AkMemPool; // placement new
	}

	// set the max number of pools
	s_iMaxNumPools = in_iNumPools;

	// none created for now
	s_iNumPools = 0;

	s_bInitialized = true;

	return AK_Success;
}

//////////////////////////////////////////////////////////////////////////////////
// MAIN PUBLIC INTERFACE IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////////////

bool IsInitialized()
{
	return s_bInitialized;
}

//====================================================================================================
//====================================================================================================
void Term()
{
	if ( IsInitialized() )
	{
		// scan all pools
		for(AkInt32 lPoolCounter = 0 ; lPoolCounter < s_iMaxNumPools ; ++lPoolCounter)
		{
			AkMemPool & pool = s_pMemPools[ lPoolCounter ];

			// have we got blocks hanging out there ?
			if( pool.ulNumBlocks != 0 )
			{
				DestroyPool( lPoolCounter );
			}
			
			pool.~AkMemPool();
		}

		AK::FreeHook(s_pMemPools);

		s_iMaxNumPools = 0;
		s_iNumPools = 0;
		s_pMemPools = NULL;

		s_bInitialized = false;
	}
}
//====================================================================================================
//====================================================================================================
AKRESULT SetPoolName(
	AkMemPoolId     in_PoolId,
	AkTChar*        in_tcsPoolName
)
{
#if defined (_DEBUG)
	AKASSERT( CheckPoolId(in_PoolId) == AK_Success );
#endif

#ifndef AK_OPTIMIZED
	size_t stringsize = wcslen( in_tcsPoolName );
	if( stringsize > MAX_MEM_POOL_NAME_SIZE - 1 )
		stringsize = MAX_MEM_POOL_NAME_SIZE - 1;
	memcpy( s_pMemPools[ in_PoolId ].ptcName, in_tcsPoolName, stringsize * sizeof( wchar_t ) );
	s_pMemPools[ in_PoolId ].ptcName[ stringsize ] = 0;

	AkMonitor::Monitor_SetPoolName( in_PoolId, in_tcsPoolName );
#endif

	return AK_Success;
}
//====================================================================================================
//====================================================================================================
AkTChar * GetPoolName(
	AkMemPoolId     in_PoolId
)
{
#ifndef AK_OPTIMIZED
	if ( s_pMemPools[ in_PoolId ].ulNumBlocks )
		return s_pMemPools[ in_PoolId ].ptcName;
	else
		return NULL;
#else
	return NULL;
#endif
}

//====================================================================================================
//====================================================================================================
AkMemPoolAttributes GetPoolAttributes(
	AkMemPoolId		in_poolId			///< ID of memory pool to test
	)
{
	return (AkMemPoolAttributes)s_pMemPools[ in_poolId ].eAttributes;
}

//====================================================================================================
//====================================================================================================
AKRESULT SetMonitoring(
    AkMemPoolId     in_poolId,			// ID of memory pool
    bool            in_bDoMonitor       // enables error monitoring (has no effect in AK_OPTIMIZED build)
    )
{
#ifndef AK_OPTIMIZED
#if defined (_DEBUG)
	AKASSERT( CheckPoolId(in_poolId) == AK_Success );
#endif
    // get a pointer to our pool
	AkMemPool*	pMemPool = s_pMemPools + in_poolId;
    pMemPool->bDoMonitor = in_bDoMonitor;
#endif
    return AK_Success;
}
//====================================================================================================
//====================================================================================================
#if defined (AK_MEMDEBUG)
void * dMalloc(AkMemPoolId	in_PoolId,
			   size_t	in_ulSize,
			   AkTChar*	ptcFile,
			   AkUInt32	ulLine)
#else
void * Malloc(AkMemPoolId in_PoolId, 
			  size_t    in_ulSize)
#endif
{
#if defined (_DEBUG)
	AKASSERT( CheckPoolId(in_PoolId) == AK_Success );
#endif

	AKASSERT(in_ulSize != 0);

	if(in_ulSize == 0)
	{
		return NULL;
	}

	// assume things went wrong
	AkChar*		pcMemAddress = NULL;

	// get a pointer to our pool
	AkMemPool*	pMemPool = s_pMemPools + in_PoolId;

	// lock the pool from this point on
	AUTOLOCKPOOL( pMemPool );

	AKASSERT( pMemPool->pTlsfPool );
				
	if(pMemPool->ulAlign)
	{
		pcMemAddress = (AkChar*)tlsf_memalign( pMemPool->pTlsfPool, pMemPool->ulAlign, in_ulSize );
	}
	else
	{
		pcMemAddress = (AkChar*)tlsf_malloc( pMemPool->pTlsfPool, in_ulSize );
	}

#ifndef AK_OPTIMIZED
	if ( pcMemAddress )
	{
		++pMemPool->ulAllocs;
#ifdef AK_MEMDEBUG
		g_memTracker.Add( pcMemAddress, ptcFile, ulLine );
#endif
	}
	else if ( pMemPool->bDoMonitor )
	{
		MONITOR_ERRORMSG2( L"Insufficient memory in pool: ", pMemPool->ptcName );
	}
#endif
	
	return pcMemAddress;
}

//====================================================================================================
//====================================================================================================
void * Malign(
	AkMemPoolId in_poolId,
    size_t		in_uSize,
    AkUInt32	in_uAlignment
    )
{
#if defined (_DEBUG)
	AKASSERT( CheckPoolId(in_poolId) == AK_Success );
#endif

	if(in_uSize == 0)
	{
		AKASSERT(false);
		return NULL;
	}

	// get a pointer to our pool
	AkMemPool*	pMemPool = s_pMemPools + in_poolId;

	// lock the pool from this point on
	AUTOLOCKPOOL( pMemPool );

	AKASSERT( pMemPool->pTlsfPool );
				
	AkChar* pcMemAddress = (AkChar*)tlsf_memalign(pMemPool->pTlsfPool, in_uAlignment, in_uSize);

#ifndef AK_OPTIMIZED
	if ( pcMemAddress )
	{
		++pMemPool->ulAllocs;
#ifdef AK_MEMDEBUG
		g_memTracker.Add( pcMemAddress, 0, 0 );
#endif
	}
	else if ( pMemPool->bDoMonitor )
	{
		MONITOR_ERRORMSG2( L"Insufficient memory in pool: ", pMemPool->ptcName );
	}
#endif
	
	return pcMemAddress;
}

AKRESULT Falign(AkMemPoolId in_PoolId,void* in_pvMemAddress)
{
	return Free( in_PoolId, in_pvMemAddress ); // in this implementation, same as free
}

//====================================================================================================
//====================================================================================================
AKRESULT Free(AkMemPoolId in_PoolId,void* in_pvMemAddress)
{
#if defined (_DEBUG)
	AKASSERT( CheckPoolId(in_PoolId) == AK_Success );
#endif

	// get a pointer to our pool
	AkMemPool* pMemPool = s_pMemPools + in_PoolId;

	// lock the pool from this point on
	AUTOLOCKPOOL( pMemPool );

#ifndef AK_OPTIMIZED
	// Update statistics here
	++pMemPool->ulFrees;
#ifdef AK_MEMDEBUG
	g_memTracker.Remove( in_pvMemAddress );
#endif
#endif

	// Optimzation for one block in pool
	tlsf_free(pMemPool->pTlsfPool, in_pvMemAddress);
	
	return AK_Success;
}

//====================================================================================================
//====================================================================================================

AKMEMORYMGR_API void * GetBlock(
	AkMemPoolId in_poolId				///< ID of the memory pool
	)
{
#if defined (_DEBUG)
	AKASSERT( CheckPoolId(in_poolId) == AK_Success );
#endif

	// get a pointer to our pool
	AkMemPool*	pMemPool = s_pMemPools + in_poolId;
	
	void * pBuffer = pMemPool->listBuffers.First();
	if ( pBuffer )
	{
		pMemPool->listBuffers.RemoveFirst();		
		
#ifndef AK_OPTIMIZED
		++pMemPool->ulAllocs;
#ifdef AK_MEMDEBUG
		g_memTracker.Add( pcMemAddress, 0, 0 );
#endif
#endif
	}

	return pBuffer;
}

//====================================================================================================
//====================================================================================================
AKRESULT ReleaseBlock(AkMemPoolId in_PoolId,void* in_pvMemAddress)
{
#if defined (_DEBUG)
	AKASSERT( CheckPoolId(in_PoolId) == AK_Success );
#endif

	// get a pointer to our pool
	AkMemPool* pMemPool = s_pMemPools + in_PoolId;

#ifndef AK_OPTIMIZED
	// Update statistics here
	++pMemPool->ulFrees;
#ifdef AK_MEMDEBUG
	g_memTracker.Remove( in_pvMemAddress );
#endif
#endif

	AKASSERT( in_pvMemAddress );
	pMemPool->listBuffers.AddLast( reinterpret_cast<AkLinkedBuffer*>( in_pvMemAddress ) );
	
	return AK_Success;
}

//====================================================================================================
//====================================================================================================
AkInt32 GetNumPools()
{
	return s_iNumPools;
}

//====================================================================================================
//====================================================================================================
AkInt32 GetMaxPools()
{
	return s_iMaxNumPools;
}
//====================================================================================================
// look for this id in the manager's list
//====================================================================================================
AKRESULT CheckPoolId(AkMemPoolId in_PoolId)
{
	// has this pool been created ?
	if((in_PoolId < s_iMaxNumPools)
		&& ((s_pMemPools + in_PoolId)->ulNumBlocks != 0))
	{
		// yes, we know this one
		return AK_Success;
	}

	return AK_InvalidID;
}

//====================================================================================================
//====================================================================================================
AKRESULT GetPoolStats(
	AkMemPoolId     in_PoolId,
	PoolStats&      out_stats
	)
{
	if ( in_PoolId >= s_iMaxNumPools )
		return AK_Fail;

	AkMemPool*	pMemPool = s_pMemPools + in_PoolId;
	AUTOLOCKPOOL( pMemPool );

	out_stats.uReserved = pMemPool->ulNumBlocks * pMemPool->ulBlockSize;

#ifndef AK_OPTIMIZED

	out_stats.uAllocs   = pMemPool->ulAllocs;
	out_stats.uFrees    = pMemPool->ulFrees;
	
	// Walk the heap (if the pool exists)
	tlsf_pool_info info;

	info.uMaxFreeBlock = 0;
	info.uUsed = 0;
	
	if(pMemPool->pTlsfPool)
	{
		tlsf_walk_heap(pMemPool->pTlsfPool, tlsf_pool_walker_profiling, &info);
	}
	else
	{
		AkUInt32 uNumFreeBlocks = pMemPool->listBuffers.Length();
		info.uMaxFreeBlock = ( uNumFreeBlocks > 0 ) ? pMemPool->ulBlockSize : 0;
		info.uUsed = ( pMemPool->ulNumBlocks - uNumFreeBlocks ) * pMemPool->ulBlockSize;
	}

	out_stats.uUsed         = info.uUsed;
	out_stats.uMaxFreeBlock = info.uMaxFreeBlock;

	if(pMemPool->ulPeakUsed < out_stats.uUsed)
	{
		pMemPool->ulPeakUsed = out_stats.uUsed;
	}

	out_stats.uPeakUsed = pMemPool->ulPeakUsed;

#else

	out_stats.uAllocs       = 0;
	out_stats.uFrees        = 0;
	out_stats.uUsed			= 0;
	out_stats.uMaxFreeBlock = 0;

#endif // AK_OPTIMZED
	
	return AK_Success;
}

#ifndef AK_OPTIMIZED

// function that walks the tlsf pool for checking memory
static void tlsf_pool_walker_check(void* ptr, size_t size, int used, void* user, char *fileName, int lineNumber)
{
	// AKASSERT( used == false && "Memory leak detected" );
	// dummy method, used to walk the heap and detect inconsistencies (by crashing).
}

// Finds memory leaks
void CheckForMemUsage(AkMemPool* in_pMemPool)
{
	if( in_pMemPool )
	{
		if ( in_pMemPool->pTlsfPool )
			tlsf_walk_heap( in_pMemPool->pTlsfPool, tlsf_pool_walker_check, NULL );
		
		AKASSERT( in_pMemPool->ulAllocs == in_pMemPool->ulFrees );
		if ( in_pMemPool->ulAllocs != in_pMemPool->ulFrees )
		{
			MONITOR_ERRORMSG2( L"Memory leak in pool: ", in_pMemPool->ptcName );
		}
	}
}

#endif // AK_OPTIMIZED

} // namespace MemoryMgr

} // namespace AK
