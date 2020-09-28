/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1 Patch 4  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkMemoryMgr.cpp
//
// XBox360 implementation.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h" 
#include "AkMemoryMgr.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

// Header to the actual memory manager
#include "tlsf/tlsf.h"

namespace AK
{

namespace MemoryMgr
{

//-----------------------------------------------------------------------------
// Factory.
//-----------------------------------------------------------------------------
AKRESULT Init( AkMemSettings * in_pSettings )
{
	AKASSERT( !IsInitialized() );
	if ( IsInitialized() )
		return AK_Fail;

    // Check parameters.
    if ( in_pSettings == NULL )
    {
        AKASSERT( !"Invalid memory manager settings" );
		return AK_Fail;
    }

	return InitBase( in_pSettings->uMaxNumPools );
}

//====================================================================================================
//====================================================================================================

static void DeallocatePool( AkMemPool & in_pool )
{
	switch(in_pool.eAttributes & AkAllocMask)
	{
	case AkMalloc:
		AK::FreeHook(in_pool.pcAllocAddress);
		in_pool.pcAllocAddress = NULL;
		break;
	case AkVirtualAlloc:
		AK::VirtualFreeHook(in_pool.pcMemAddress,0,MEM_RELEASE);
		in_pool.pcMemAddress = NULL;
		break;
	case AkPhysicalAlloc:
		AK::PhysicalFreeHook(in_pool.pcMemAddress);
		in_pool.pcMemAddress = NULL;
		break;
	}
}

AkMemPoolId CreatePool(void*		in_pvMemAddress,
					   AkUInt32		in_ulMemSize,
					   AkUInt32		in_ulBlockSize,
					   AkUInt32		in_eAttributes,
					   AkUInt32     in_ulBlockAlign
					  )
{
	AKASSERT( AK::MemoryMgr::IsInitialized() );

	// check no allocation case
	if( ( in_eAttributes & AkAllocMask ) == AkNoAlloc )
	{
		if ( in_pvMemAddress == NULL )
			return AK_INVALID_POOL_ID;
	}
	else // check allocation case
	{
		if(((in_eAttributes&AkAllocMask) != AkMalloc)        && 
		   ((in_eAttributes&AkAllocMask) != AkVirtualAlloc)  &&
		   ((in_eAttributes&AkAllocMask) != AkPhysicalAlloc))

		{
			return AK_INVALID_POOL_ID;
		}
	}

	// no free ones ?
	if(s_iNumPools >= s_iMaxNumPools)
	{
		return AK_INVALID_POOL_ID;
	}

	// start at first pool
	AkMemPool*	pThisPool = s_pMemPools;
	AkMemPoolId	PoolId;
	AkInt32 lCounter;
	// find a free one
	for(lCounter = 0 ; lCounter < s_iMaxNumPools ; ++lCounter)
	{
		// lock the pool
		pThisPool->lock.Lock();

		// is it used ?
		if(pThisPool->ulNumBlocks == 0)
		{
			// this is the pool id
			PoolId = lCounter;
			break;
		}
		// unlock the pool
		pThisPool->lock.Unlock();
		// next one
		++pThisPool;
	}
	// found a free one ?
	if(lCounter < s_iMaxNumPools)
	{
//----------------------------------------------------------------------------------------------------
// if we get there pThisPool is locked and can be created
//----------------------------------------------------------------------------------------------------
		// WG-8829: round *down* the number of blocks
		AkUInt32 ulNumBlocks    = in_ulMemSize / in_ulBlockSize;
		AkUInt32 ulAdjustedSize = ulNumBlocks * in_ulBlockSize;

		// should we allocate ?
		if(in_pvMemAddress == NULL)
		{
			// figure out what alloc should be done
			switch(in_eAttributes&AkAllocMask)
			{
			case AkMalloc:
				if( ulAdjustedSize + in_ulBlockAlign )
				{
					pThisPool->pcAllocAddress = static_cast<AkChar*>(AK::AllocHook(ulAdjustedSize + in_ulBlockAlign));
				}
				pThisPool->pcMemAddress = pThisPool->pcAllocAddress;
				if ( in_ulBlockAlign )
				{
					size_t offset = (size_t) pThisPool->pcAllocAddress % in_ulBlockAlign;
					if ( offset )
						pThisPool->pcMemAddress += in_ulBlockAlign - offset;
				}
				break;
			case AkVirtualAlloc:
				AKASSERT( !in_ulBlockAlign ); // not supported yet
				if( ulAdjustedSize )
				{
					pThisPool->pcMemAddress = static_cast<AkChar*>(AK::VirtualAllocHook(NULL,
																	ulAdjustedSize,
																	MEM_COMMIT | MEM_LARGE_PAGES,
																	PAGE_EXECUTE_READWRITE));
				}
				break;
			// PhM : add this one ?
/*
			case AkPhysicalAllocNoCache:
				pThisPool->pcMemAddress = static_cast<AkChar*>(AK::PhysicalAllocHook(ulAdjustedSize,
																MAXULONG_PTR,
																0,
																PAGE_READWRITE | PAGE_NOCACHE | MEM_LARGE_PAGES));
				break;
*/
			case AkPhysicalAlloc:
				if( ulAdjustedSize )
				{
					#define PHYSALLOC_ADJUST_FOR_SIMD 16 // unaligned read at end of memory can cause 16 bytes read past end. This happens in multi-channel pitch conversion.
					pThisPool->pcMemAddress = static_cast<AkChar*>(AK::PhysicalAllocHook(ulAdjustedSize + PHYSALLOC_ADJUST_FOR_SIMD,
																	MAXULONG_PTR,
																	in_ulBlockAlign,
																	PAGE_READWRITE /*| MEM_LARGE_PAGES*/));
					AKASSERT( ( in_ulBlockAlign == 0 ) || ( ( (uintptr_t)pThisPool->pcMemAddress % in_ulBlockAlign ) == 0 ) ); // make sure memory is aligned properly
				}
				break;
			}

			// did we get the needed memory ?
			if(pThisPool->pcMemAddress == NULL)
			{
				pThisPool->lock.Unlock();
				return AK_INVALID_POOL_ID;
			}

			// this one will have to be freed later on
			pThisPool->bAllocated = true;

#ifdef _DEBUG
			// flag it as free
			memset(pThisPool->pcMemAddress,FREE_BLOCK_FLAG,ulAdjustedSize);
#endif
		}
		// use the provided address
		else
		{
			pThisPool->pcMemAddress = static_cast<AkChar*>(in_pvMemAddress);
			pThisPool->bAllocated = false;
		}

		// remember how memory was allocated
		pThisPool->eAttributes = in_eAttributes;

		// setup pool
		if((in_eAttributes&AkBlockMgmtMask)==AkFixedSizeBlocksMode)
		{
			// Fixed-size blocks allocator mode
			pThisPool->listBuffers.Init();

			// Fill list.
			AkChar * pBuffer = pThisPool->pcMemAddress;
			AkChar * pEndAddress = pBuffer + ulAdjustedSize;

			while ( pBuffer != pEndAddress )
			{
				pThisPool->listBuffers.AddLast( reinterpret_cast<AkLinkedBuffer*>( pBuffer ) );
				pBuffer += in_ulBlockSize;
			}
		}
		else
		{
			// tlsf allocator mode
			// Create the tlsf pool from the memory allocated
			pThisPool->pTlsfPool = tlsf_create(pThisPool->pcMemAddress, ulAdjustedSize);
			if ( !pThisPool->pTlsfPool )
			{
				DeallocatePool( *pThisPool );
				pThisPool->lock.Unlock();
				return AK_INVALID_POOL_ID;
			}
		}

		pThisPool->ulNumBlocks = ulNumBlocks;
		pThisPool->ulBlockSize = in_ulBlockSize;
		pThisPool->ulAlign = in_ulBlockAlign;

		// one more pool
		++s_iNumPools;

		// we're done
		pThisPool->lock.Unlock();

		return PoolId;
	}
	AKASSERT(!"No free pool");
	return AK_INVALID_POOL_ID;
}
//====================================================================================================
//====================================================================================================
AKRESULT DestroyPool(AkMemPoolId in_PoolId)
{
	AKASSERT( AK::MemoryMgr::IsInitialized() );

	AkMemPool*	pMemPool;

	AKRESULT eResult;

	// is it a valid id ?
	eResult = CheckPoolId(in_PoolId);

	// if yes then go on
	if(eResult == AK_Success)
	{
		// get a pointer to our pool
		pMemPool = s_pMemPools + in_PoolId;

		// lock the pool from this point on
		AkAutoLock<CAkLock> PoolLock( pMemPool->lock );

#ifndef AK_OPTIMIZED
		AK::MemoryMgr::CheckForMemUsage( pMemPool );
#endif

		if((pMemPool->eAttributes&AkBlockMgmtMask)==AkFixedSizeBlocksMode)
		{
			// Fixed-size blocks allocator
			pMemPool->listBuffers.Term();
		}
		else
		{
			// Tlsf allocator
			// Clear the tlsf pool
			tlsf_destroy(pMemPool->pTlsfPool);
		}		
		
		// should we free this memory ?
		if(pMemPool->bAllocated)
		{
			DeallocatePool( *pMemPool );
		}
		// reset everything
		pMemPool->Init();

		// one less pool
		--s_iNumPools;
	}
	return eResult;
}

} // namespace MemoryMgr

} // namespace AK
