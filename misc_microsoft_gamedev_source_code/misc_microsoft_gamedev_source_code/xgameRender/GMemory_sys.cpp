/**********************************************************************

Filename    :   GMemory_sys.cpp
Content     :   Win32 Fast memory allocation functions implementation
Created     :   December 28, 1998
Authors     :   Michael Antonov

History     :   22/02/2002 MA   Rewrote to use a buddy system,
                                replacing the older version

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#define BUILD_GMEMORY

#include "xgameRender.h"
#include "scaleformIncludes.h"

//#include "GTypes.h"

// Define to use system allocator instead
//#define GFC_USE_SYSTEM_ALLOCATOR    1

#if defined(GFC_OS_PS3) || defined(GFC_OS_DARWIN) || defined(GFC_OS_SYMBIAN) || defined(GFC_OS_WII) || defined(GFC_CC_RENESAS)
#include <stdlib.h>
#else
#include <malloc.h>
#endif

#include "GStandardAllocator.h"

#include "GMemory.h"
#include "GFunctions.h"


// ****** Declared classes
class GMemory;
class GSystemAllocator;


// ***** System Allocator

class GSystemAllocator : public GAllocator
{
public:

    GAllocatorStats  Stats;

    GSystemAllocator()
    {
        GMemUtil::Set(&Stats, 0, sizeof(GAllocatorStats));
    }

    const GAllocatorStats* GetStats() const
    {
        return &Stats;
    }

    void    Free(void *pmem)
    {
        // System allocator & tracking option
#ifdef  GFC_MEMORY_TRACKSIZES
        // Have to check for null - by standard
        if (!pmem) return;
        Stats.FreeCount++;
        UPInt sizeFreed = *(((UPInt*)pmem)-1);
        Stats.FreeSize += sizeFreed;
        pmem = (((UPInt*)pmem)-1);
#endif
        free(pmem);
    }

    void*   Alloc(UPInt size)
    {
#ifdef GFC_MEMORY_TRACKSIZES
        void *pret;

        Stats.AllocCount++;
        pret = malloc(size+sizeof(UPInt));
        if (pret)
        {
            Stats.AllocSize += size;
            *((UPInt*)pret) = size;
            return ((UPInt*)pret)+1;
        }
        return 0;

#else // NO MEMORY TRACKING
        return malloc(size);    
#endif
    }

    void*   Realloc(void *pmem, UPInt size)
    {
#ifdef  GFC_MEMORY_TRACKSIZES
        Stats.ReallocCount++;
        UPInt oldSize = pmem ? *((UPInt*)pmem-1) : 0;
        void *pret =  realloc(((UPInt*)pmem-1),size + sizeof(UPInt));
        if (pret)
        {
            if (oldSize>size)
                Stats.FreeSize +=(oldSize - size);
            else
                Stats.AllocSize+=(size - oldSize);
            *((UPInt*)pret) = size;
            return ((UPInt*)pret) + 1;
        }   
        return 0;
#else
        return realloc(pmem,size);  
#endif
    }

};


#ifdef GFC_USE_SYSTEM_ALLOCATOR
static GSystemAllocator         GFC_Allocator;
static GSystemAllocator         GFC_BlockAllocator;
#else
static GStandardAllocator       GFC_Allocator;
static GStandardBlockAllocator  GFC_BlockAllocator;
#endif
static GDebugAllocator          GFC_DebugAllocator;

static GAllocator*              GFC_pAllocator      = &GFC_Allocator;
static GDebugAllocator*         GFC_pDebugAllocator = &GFC_DebugAllocator;
static GAllocator*              GFC_pBlockAllocator = &GFC_BlockAllocator;

#ifdef GFC_BUILD_DEBUG
static bool InitAlloc = 0;
#endif

// *** Allocator

// Overrides every individual allocation
// - avoids using the default memory system
void         GMemory::SetAllocator(GAllocator *pallocator)
{
#ifdef GFC_BUILD_DEBUG
    if (InitAlloc)
        GFC_DEBUG_ERROR(1,"GMemory::SetAllocator - failed to set allocator; memory was already allocated");
#endif
    if (pallocator)
        GFC_pAllocator = pallocator;
    else
        GFC_pAllocator = &GFC_Allocator;
}
GAllocator*  GMemory::GetAllocator()
{
    return GFC_pAllocator;
}

// Optional: overrides every individual allocation in Debug mode
// - by default, the debug calls fallback on the normal allocator
// - avoids using the default memory system
void             GMemory::SetDebugAllocator(GDebugAllocator *pallocator)
{
#ifdef GFC_BUILD_DEBUG
    if (InitAlloc)
        GFC_DEBUG_ERROR(1,"GMemory::SetDebugAllocator - failed to set allocator; memory was already allocated");
#endif
    if (pallocator)
        GFC_pDebugAllocator = pallocator;
    else
        GFC_pDebugAllocator = &GFC_DebugAllocator;
}
GDebugAllocator* GMemory::GetDebugAllocator()
{
    return GFC_pDebugAllocator;
}


// *** Block Allocator

// Overrides only the block allocation made by the default memory system
// - will not be used if the 
void          GMemory::SetBlockAllocator(GAllocator *pallocator)
{
#ifdef GFC_BUILD_DEBUG
    if (InitAlloc)
        GFC_DEBUG_ERROR(1,"GMemory::SetBlockAllocator - failed to set allocator; memory was already allocated");
#endif
    if (pallocator)
        GFC_pBlockAllocator = pallocator;
    else
        GFC_pBlockAllocator = &GFC_BlockAllocator;
}

GAllocator*   GMemory::GetBlockAllocator()
{
    return GFC_pBlockAllocator;
}



// ***** Release Alloc/Realloc/Free

#ifndef GFC_BUILD_DEBUG

void*   GMemory::Alloc(UPInt size)
{
    return GFC_pAllocator->Alloc(size);
}
void*   GMemory::Realloc(void *pmem, UPInt size)
{
    return GFC_pAllocator->Realloc(pmem, size);
}
void    GMemory::Free(void *pmem)
{   
    GFC_pAllocator->Free(pmem);
}

void*   GMemory::AllocAligned(UPInt size, UPInt align, UPInt extra)
{
    return GFC_pAllocator->AllocAligned(size, align, extra);
}
void    GMemory::FreeAligned(void *pmem)
{   
    GFC_pAllocator->FreeAligned(pmem);
}

// *** Debug Alloc/Realloc/Free

#else //ifdef GFC_BUILD_DEBUG

void*   GMemory::DebugAlloc(size_t size, int blocktype, const char* pfilename, int line, const char *pclassname)
{
    InitAlloc=1;
    return GFC_pDebugAllocator->Alloc(size,blocktype, pfilename, line, pclassname);
}
void*   GMemory::DebugRealloc(void *pmem, size_t size, int blocktype, const char* pfilename, int line, const char *pclassname)
{
    InitAlloc=1;
    return GFC_pDebugAllocator->Realloc(pmem, size, blocktype, pfilename, line, pclassname);
}
void    GMemory::DebugFree(void *pmem, int blocktype, const char* pfilename, int line, const char *pclassname)
{
    GFC_pDebugAllocator->Free(pmem, blocktype, pfilename, line, pclassname);
}

void*   GMemory::DebugAllocAligned(size_t size, UPInt align, UPInt extra, int blocktype, const char* pfilename, int line, const char *pclassname)
{
    InitAlloc=1;
    return GFC_pDebugAllocator->AllocAligned(size, align, extra, blocktype, pfilename, line, pclassname);
}
void    GMemory::DebugFreeAligned(void *pmem, int blocktype, const char* pfilename, int line, const char *pclassname)
{
    GFC_pDebugAllocator->FreeAligned(pmem, blocktype, pfilename, line, pclassname);
}


void*   GMemory::Alloc(size_t size)
{
    InitAlloc=1;
    return GFC_pDebugAllocator->Alloc(size, 1,0,0,0);
}
void*   GMemory::Realloc(void *pmem, size_t size)
{
    InitAlloc=1;
    return GFC_pDebugAllocator->Realloc(pmem, size, 1,0,0,0);
}
void    GMemory::Free(void *pmem)
{
    GFC_pDebugAllocator->Free(pmem, 1,0,0,0);
}

void*   GMemory::AllocAligned(size_t size, UPInt align, UPInt extra)
{
    InitAlloc=1;
    return GFC_pDebugAllocator->AllocAligned(size, align, extra, 1,0,0,0);
}
void    GMemory::FreeAligned(void *pmem)
{
    GFC_pDebugAllocator->FreeAligned(pmem, 1,0,0,0);
}


void    GMemory::DetectMemoryLeaks()
{
    GFC_pDebugAllocator->Dump();
}

#endif // GFC_BUILD_DEBUG
