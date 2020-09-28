/**********************************************************************

Filename    :   GStandardAllocator_sys.cpp
Content     :   Standard Buddy system memory allocation
Created     :   December 28, 1998
Authors     :   Michael Antonov

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GTypes.h"

#ifdef GFC_OS_WIN32
#include <windows.h>
#endif
#ifdef GFC_OS_XBOX360
#include "PPCIntrinsics.h"
#endif

#if defined(GFC_OS_PS3) || defined(GFC_OS_DARWIN) || defined(GFC_OS_SYMBIAN) || defined(GFC_OS_WII) || defined(GFC_CC_RENESAS)
#include <stdlib.h>
#else
#include <malloc.h>
#endif

#include "GStandardAllocator.h"
#include "GMemory.h"
#include "GFunctions.h"

#define  GFC_MEMORY_TRACKSIZES



// ***** GAllocator 

// Allocate aligned memory, with optional extra header.
void*   GAllocator::AllocAligned(UPInt size, UPInt align, UPInt extra)
{
    GASSERT(align <= 0x10000);           // 65K align limit.
    GASSERT(((align - 1) & align) == 0); // Align must be a power of 2.
    // Extra must be a multiple of pointer size.
    GASSERT((extra & (sizeof(void*)-1)) == 0);

    if (align == 0)
        align = sizeof(void*);

    UPInt headerSize = extra + sizeof(UInt16);
    
    // The maximum adjustment that could be necessary for alignment
    // to take place is (align - 1).
    UPInt addr = (UPInt) Alloc(headerSize + align - 1 + size);
    if (addr == 0)
        return 0;

    // Calculate the aligned address and user address.
    UPInt closestAddr     = addr + headerSize;   
    UPInt alignedDataAddr = (closestAddr + align - 1) & ~(align-1);
    UPInt userAddr        = alignedDataAddr - extra;
   
    // Store offset that we will use during Free.
    UPInt offset = userAddr - addr;
    *(UInt16*)(userAddr - sizeof(UInt16)) = (UInt16)offset;

    return (void*) userAddr;
}

void    GAllocator::FreeAligned(void *pmemBlock)
{
    if (!pmemBlock) return;
    // Adjust back to recover from alignment and call user to free
    // with original pointer value.
    UPInt offset = *(((UInt16*)pmemBlock) - 1);
    Free( ((UByte*)pmemBlock) - offset);
}



// ***** GStandardBlockAllocator

GStandardBlockAllocator::GStandardBlockAllocator()    
{
    GMemUtil::Set(&Stats, 0, sizeof(GAllocatorStats));
}

void    GStandardBlockAllocator::Free(void *pmem)
{
#ifdef GFC_OS_WIN32
    ::VirtualFree(pmem, 0, MEM_RELEASE);
#else
    ::free(pmem);
#endif
}

void*   GStandardBlockAllocator::Alloc(UPInt size)
{
#ifdef GFC_OS_WIN32    
    return ::VirtualAlloc(0, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#else
    return ::malloc(size);
#endif
}

void*   GStandardBlockAllocator::Realloc(void *pmem, UPInt size)
{
#ifdef GFC_OS_WIN32
    return ::VirtualAlloc(pmem, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#else
    return ::realloc(pmem, size);
#endif
}



// ***** GStandardAllocator

GStandardAllocator::GStandardAllocator()
    : Lock(400)
{
    // Check-for and initialize buddy table
    GMemUtil::Set(&Stats, 0, sizeof(GAllocatorStats));
    InitBlockTable();
    pBlockAllocator     = 0;
    pSystemAllocator    = 0;
}

void    GStandardAllocator::InitBlockTable()
{
    // Setup node pointers
    SInt i,sz;

    // Initialize linked lists
    for(i=0; i< Buddy_TableSpace*2; i++)
    {
        BlockTable[i].Head.pPrev = &BlockTable[i].Head;
        BlockTable[i].Head.pNext = &BlockTable[i].Tail;
        BlockTable[i].Tail.pPrev = &BlockTable[i].Head;
        BlockTable[i].Tail.pNext = &BlockTable[i].Tail;
        // All hanging blocks will be 0
        BlockTable[i].BlockSize    = 0;
    }

    // And Setup Block sizes.    
    // NOTE: This is ok on 64-bit systems because blocks smaller then 24 bytes
    // are never found/applied on them (due to the mask used in AllocImpl).
    // 24-byte is the smallest usable node on 64-bit machines because the algorithm
    // requires sizeof(Node) + sizeof(Header) space at minimum, which is 18 bytes.
    for(i=Buddy_SmallTableIndex,sz = 12; i<=Buddy_SmallTableMaxIndex; i++, sz*=2)
        BlockTable[i].BlockSize = sz;
    for(i=Buddy_BigTableIndex,sz = 16; i<=Buddy_BigTableMaxIndex; i++, sz*=2)
        BlockTable[i].BlockSize = sz;
}


bool    GStandardAllocator::AllocBlock(UInt index)
{
    if (!pBlockAllocator)
        pBlockAllocator = GMemory::GetBlockAllocator(); 

    // Allocate system memory, with the following criteria:
    //  - 2x max block size
    //  - aligned to size of pointer, with sizeof(Header) extra bytes up front.
    void *pmem = pBlockAllocator->AllocAligned(BlockTable[index].BlockSize*2,
                                               Buddy_BlockAlign, Buddy_BlockAlign);

    GCOMPILER_ASSERT(sizeof(void*) >= sizeof(Header));

    if (pmem)
    {
        // We allocate sizeof(void*) instead of sizeof(Header) to meet our
        // pointer-aligned extra requirement.
        Header *pmemHeader = (Header*)
                 (((UByte*)pmem) + Buddy_BlockAlign - sizeof(Header));

        // Insert both nodes (GetNodePtr adjusts forward by sizeof(Header)).
        Node* pfirst = (Node*)pmemHeader->GetNodePtr();
        Node *psecond= pfirst->GetRightNode(BlockTable[index].BlockSize);

        // Link to tail
        BlockTable[index].Head.pNext->pPrev = psecond;
        psecond->pNext = BlockTable[index].Head.pNext;
        // each other
        psecond->pPrev = pfirst;
        pfirst->pNext  = psecond;
        // and head
        pfirst->pPrev = &BlockTable[index].Head;
        BlockTable[index].Head.pNext = pfirst;
        
        // Done, set values
        // Split counts 1 and 0, respectively
        pfirst->SetValue((UInt8)index, 0, 1);
        psecond->SetValue((UInt8)index, 0, 0);
        return 1;
    }
    return 0;
}

void    GStandardAllocator::FreeBlock(Node *pnode)
{
    // Adjust pointer back by sizeof(Header).
    pBlockAllocator->FreeAligned(((UByte*)pnode) - Buddy_BlockAlign);
}


// Used for big blocks
void*   GStandardAllocator::SystemAlloc(size_t size, size_t align, size_t uextra)
{
    if (!pSystemAllocator)
        pSystemAllocator = GMemory::GetBlockAllocator();

    // System allocation adds its own Header that it can mark it as system.
    GCOMPILER_ASSERT(sizeof(void*) >= sizeof(Header));

    UByte* pmem = 0;

    if (align != 0)
    {
        pmem = (UByte*) pSystemAllocator->AllocAligned(size, align,
                                                       uextra + sizeof(void*)*2);
        // NOTE: Technically we could save sizeof(void*) because  size of isn't
        // used (Realloc is forbidden on aligned memory by our APIs).
    }
    else
    {
        // Here, we can get uextra only due to TRACK_SIZES. In that case,
        // it is subtracted after it is read in Realloc.
        size += uextra;
        pmem = (UByte*) pSystemAllocator->Alloc(size + sizeof(void*)*2);
    }

    if (!pmem)
        return 0;

    UByte*   puser   = pmem + sizeof(void*)*2;
    Header*  pheader = (Header*)(puser - sizeof(Header)) ;

    // Record allocated size for Realloc. Note that this size will
    // not actually be used with aligned allocation.
    *((UPInt*)pmem) = size;
    // Mark as system-allocated node.
    pheader->SetValue(Buddy_BigTableMaxIndex+1,1,0);

    return puser;
}


void    GStandardAllocator::SystemFree(void* pmem, bool aligned)
{
    pmem = ((UByte*)pmem) - sizeof(void*)*2;

    if (!aligned)
        pSystemAllocator->Free(pmem);
    else
        pSystemAllocator->FreeAligned(pmem);
}


// AllocImpl handles alignment only when align != 0.
void*   GStandardAllocator::AllocImpl(UPInt usize, UPInt align, UPInt extra)
{
    void *pret;
    UPInt size = usize;

#ifdef GFC_MEMORY_TRACKSIZES    
    extra += Tracksizes_Header;
#endif

    if (align)
    {    
        if (align > 4096)
        {
#ifdef GFC_MEMORY_TRACKSIZES
            pret = SystemAlloc(usize,align,extra);
            if (pret)
            {       
                GLock::Locker lock(&Lock);
                Stats.AllocSizeUsed += usize+extra;
                Stats.AllocSize     += usize+extra - Tracksizes_Header;
                *((UPInt*)pret) = usize+extra - Tracksizes_Header;
                pret = ((UByte*)pret)+Tracksizes_Header;
                return pret;
            }
            return 0;
#else
            return SystemAlloc(usize,align,extra);
#endif
        }

    // Align value must be large enough to hold AlignHeader.
     GCOMPILER_ASSERT(sizeof(void*) >= sizeof(Header));
    if (align < sizeof(void*))
        align = sizeof(void*);
    }

    // When performing alignment, we do not need to add sizeof(AlignHeader)
    // to our computations because it can be dropped when alignment adjustment
    // is not actually done within the block (then only buddy header exists).
    // This is true as long as the following conditions hold:
    //  1) sizeof(AlignHeader) <= align, and
    //  2) extra is a multiple of pointer size.
    // Under these conditions the smallest possible alignment would be by
    // sizeof pointer and is thus enough to fit align header.
    size += extra + align;

    // Use Intel's bsr to find high order bit fast.
    // Note that on 64-bit systems we start with 24-byte block, so we use 0x1F so
    // ensure that lower bits are never found.
    SPInt   i, maxIndex;
#ifdef GFC_OS_PS2
    SPInt   roundedIndex = (size+sizeof(Header)-1) | 0x1F;
#else
    SPInt   roundedIndex = (size+sizeof(Header)-1) | ((sizeof(void*)>4) ? 0x1F : 0xF);
#endif

#if (defined(GFC_CC_MSVC) && defined(GFC_OS_XBOX360))
    // PowerPC cntlzw intrinsic
    roundedIndex  = 31 - (SInt)_CountLeadingZeros(roundedIndex);

#elif (defined(GFC_CC_MSVC) && defined(_M_IX86))

    GASM {
        bsr     eax, roundedIndex
        mov     roundedIndex, eax
        }

#else
    {
        SInt j, k;
        if (roundedIndex & 0xFFFF8000)
            { j = 31; k = 0x80000000; }
        else
            { j = 14; k = 0x4000; }

        do
        {
            if (roundedIndex & k)
            {
                roundedIndex = j;
                break;
            }
            k >>= 1;
            j--;
        } while (j >= 0);
    }
#endif
    // This will result in:
    //  3 for 0..14
    //  4 for 15..30  and so on
    roundedIndex -= 3;

    // Lock the threads. This is a no-op when threading is disabled.
    GLock::Locker lock(&Lock);

    // Counters must be accessed within a lock.
#ifdef GFC_MEMORY_TRACKSIZES
    Stats.AllocCount++;    
#endif

    // Check 75%-size table first
    if (BlockTable[roundedIndex].GetAllocSize() >= SPInt(size))
    {
        maxIndex     =  Buddy_SmallTableMaxIndex;
        roundedIndex += Buddy_SmallTableIndex;
    }
    else
    {
        // Note: the very first allocation ever will always end up here by default
        //       (Because GetAllocSize() will return -sizeof(Header) )
        // Otherwise, use a power of 2 table
        maxIndex     =  Buddy_BigTableMaxIndex;
        roundedIndex += Buddy_BigTableIndex;
    }
    
    // Search for a free link
    for (i=roundedIndex; (BlockTable[i].Head.pNext==BlockTable[i].Tail.pNext) && (i<=maxIndex); i++ )
        ;
    
    if (i>maxIndex)
    {
        // Check for block too large - for system allocation
        if (roundedIndex>maxIndex)
        {
            // Use system allocation.

    #ifdef GFC_MEMORY_TRACKSIZES
            pret = SystemAlloc(usize, align, extra);
            if (pret)
            {       
                Stats.AllocSizeUsed += usize+extra;
                Stats.AllocSize     += usize+extra - Tracksizes_Header;
                *((UPInt*)pret) = usize+extra - Tracksizes_Header;
                pret = ((UByte*)pret)+Tracksizes_Header;
                return pret;
            }
            return 0;
    #else
            return SystemAlloc(usize,align,extra);
    #endif
        }
                        
        // Allocate another chunk
        // NOTE: This will also initialize the BuddyTable, if needed
        i--;
        if (!AllocBlock(UInt(i)))
            return 0;       
    }

    Node *p = BlockTable[i].Head.pNext;
    // Delete link
    BlockTable[i].Head.pNext = p->pNext;
    p->pNext->pPrev = p->pPrev;

    // Go splitting
    while(i>roundedIndex)
    {
        i--;
        p->GetHeader()->SplitCount++;
        // Node to the right
        Node *q = p->GetRightNode(BlockTable[i].BlockSize);
        
        // Set index, mark as a free right-side node
        q->SetValue(UInt8(i), 0, 0);
        // Add to free list
        q->pNext                        = BlockTable[i].Head.pNext;
        BlockTable[i].Head.pNext->pPrev = q;
        q->pPrev                        = &BlockTable[i].Head;
        BlockTable[i].Head.pNext        = q;
    }

    // Mark as allocated
    p->SetIndex_And_AllocatedFlag(UInt8(i), 1);
    pret = p;

    if (align)
    {
        // Calculate the aligned address and user address.
        UPInt closestAddr     = UPInt(pret) + extra;
        UPInt alignedDataAddr = (closestAddr + align - 1) & ~(align-1);
        UPInt userAddr        = alignedDataAddr - extra;
        // Store offset that we will use during Free.
        UPInt offset          = userAddr - UPInt(pret);

        // Create align header if alignment took place. Since align header
        // matches a part of buddy header, we'll be able to distinguish
        // this later in Free/Realloc.
        if (offset)
        {
            GASSERT(offset >= sizeof(AlignHeader));
            AlignHeader* ph = (AlignHeader*)(userAddr - sizeof(AlignHeader));
            ph->SetAligned((UInt16)offset);
            // Check alignment of produced result address.
            GASSERT(((UPInt(pret) + offset + extra) & (align-1)) == 0);
            pret = (void*)userAddr;
        }
    }

#ifdef GFC_MEMORY_TRACKSIZES
    Stats.AllocSizeUsed += BlockTable[i].BlockSize;
    Stats.AllocSize     += size - Tracksizes_Header;
    *((UPInt*)pret) = size - Tracksizes_Header;
    pret = ((UByte*)pret)+Tracksizes_Header;
#endif

    return pret;
}


void    GStandardAllocator::FreeImpl(void *pmem, bool aligned)
{   
    // Have to check for null - by standard
    if (!pmem)
        return;

// System allocator & tracking option.
#ifdef  GFC_MEMORY_TRACKSIZES    
    UPInt sizeFreed = *((UPInt*)(((UByte*)pmem)-Tracksizes_Header));
    pmem = (((UByte*)pmem)-Tracksizes_Header);
#endif

    // Interpret align header. The returned offset will
    // be non-zero only if AlignHeader was used.
    AlignHeader* palign = ((AlignHeader*)pmem)-1;
    UInt         offset = palign->GetOffset();
    pmem = ((UByte*)pmem) - offset;
    
    // Get access to buddy node. If SystemAlloc was used this is
    // still ok since it has header with i>Buddy_BigTableMaxIndex.
    // Note that GetIndex() accesses header that is before the node.
    Node* pnode = (Node*)pmem;
    UInt  i     = pnode->GetIndex();

    // Lock the threads.
    GLock::Locker lock(&Lock);

// Tracking constants must be updated within a lock.
#ifdef  GFC_MEMORY_TRACKSIZES
    Stats.FreeCount++;
    Stats.FreeSize += sizeFreed;    

#ifdef GFC_BUILD_DEBUG
    memset(pmem, 0xFE, sizeFreed);
#endif
#endif
    
    // Detect & free system memory
    if (i>Buddy_BigTableMaxIndex)
    {   
    #ifdef GFC_MEMORY_TRACKSIZES
        Stats.FreeSizeUsed += sizeFreed;
    #endif  
        SystemFree(pmem, aligned);
        return;
    }

#ifdef  GFC_MEMORY_TRACKSIZES
    Stats.FreeSizeUsed += BlockTable[i].BlockSize;
#endif

    // Get Max index
    UInt    maxIndex    = (i&Buddy_TableSpace) + Buddy_TableSize - 1;   
    SPInt   size;
    Node*   pneighbor;  

    // Determine neighbors and merge them if we can
    while(1)
    {
        size      = BlockTable[i].BlockSize;
        pneighbor = pnode->IsRightNode() ? pnode->GetLeftNode(size) : pnode->GetRightNode(size);

        // Check Loop condition
        if ((i>maxIndex) || !pneighbor->IsFree_And_IndexIs(UInt8(i)))
            break;
        // Buddy is free

        // Delete neighbor from free list
        pneighbor->pPrev->pNext = pneighbor->pNext;
        pneighbor->pNext->pPrev = pneighbor->pPrev;

        if (pneighbor<pnode)
            pnode = pneighbor;
        i++;
        pnode->GetHeader()->SplitCount--;
    }

    // If bigger then index, system-pool free
    if (i>maxIndex)
        FreeBlock(pnode);        
    else
    {
        pnode->SetIndex_And_AllocatedFlag(UInt8(i), 0);
        // Insert into free list
        pnode->pNext = BlockTable[i].Head.pNext;
        pnode->pPrev = &BlockTable[i].Head;
        BlockTable[i].Head.pNext->pPrev = pnode;
        BlockTable[i].Head.pNext        = pnode;
    }
}


void*   GStandardAllocator::Realloc(void *pmem, UPInt size)
{

#ifdef GFC_MEMORY_TRACKSIZES
    UPInt trackedSize = 0;    
    if (pmem)
        trackedSize = *((UPInt*)(((UByte*)pmem)-Tracksizes_Header));
    // NOTE: We can't just modify size here because this 
    // function calls GMemory::Free and GMemory::Alloc,
    // which already do the adjustment
#endif

    // If new size is 0, same as free
    if (size==0)
    {
        Free(pmem);
        return 0;
    }

    UPInt oldSize, copySize;

    if (pmem)
    {
    #ifdef GFC_MEMORY_TRACKSIZES
        void *plookup = (((UByte*)pmem)-Tracksizes_Header);
    #else
        void *plookup = pmem;
    #endif

        // There should not be any align header on memory here
        // because Realloc can not be called on aligned memory.
    #ifdef GFC_BUILD_DEBUG
        AlignHeader* palign = ((AlignHeader*)plookup) - 1;
        UInt offset = palign->GetOffset();
        if (offset)
        {
            GFC_DEBUG_WARNING(1, "GStandardAllocator::Realloc called on aligned block");
            return 0;
        }
    #endif

        Node * pnode = (Node*)plookup;
        SInt   index = pnode->GetIndex();
        
        // If System allocation, fetch the stored size (before the node in memory)
        // Otherwise, use index for old size.
        if (index>Buddy_BigTableMaxIndex)
            oldSize = *(((UPInt*)pnode)-2);
        else
            oldSize = BlockTable[index].GetAllocSize();

        // oldSize included the 'extra' space for tracked header,
        // so adjust for that.
    #ifdef GFC_MEMORY_TRACKSIZES
        oldSize -= Tracksizes_Header;
    #endif
    }
    else
    {
        // Null pointer means allocation request.
        oldSize = 0;
    }

    // If block still fits, or no major change in size, just return back a pointer
    if (size <= oldSize)
    {
        if (size > (oldSize/2))
        {
    #ifdef GFC_MEMORY_TRACKSIZES
            GLock::Locker lock(&Lock);
            Stats.ReallocCount++;
            if (size > trackedSize)
                Stats.AllocSize += size - trackedSize;
            else
                Stats.FreeSize  += trackedSize - size;
            *((UPInt*)(((UByte*)pmem)-Tracksizes_Header)) += size - trackedSize;
    #endif
            // Done, no reallocation needs to take place.
            return pmem;
        }
        
        copySize = size;
    }
    else
    {
        copySize = oldSize;
    }

    // NOTE: more complicated reallocation can be done here
    //       We could try seeing if neighbor block is available for allocation
    //       or reducing the size of current block...

#ifdef GFC_MEMORY_TRACKSIZES
    { // Unfortunately, this increment needs to happen within a lock.
        GLock::Locker lock(&Lock);
        Stats.ReallocCount++;
    }
#endif

    void *pnewMem = Alloc(size);
#ifdef GFC_BUILD_DEBUG
    memset(pnewMem, 0xAA, size);
#endif
    
    if (!pnewMem)
    {
        // Realloc to reduce size can never fail
        return (size<=oldSize) ? pmem : 0;
    }   

    // The data & free old block
    GMemUtil::Copy(pnewMem, pmem, copySize);
    if (oldSize)
        Free(pmem);

    // Done, return pointer
    return pnewMem;
}



// Allocation interface.
void*   GStandardAllocator::Alloc(UPInt size)
{
	void* ret = AllocImpl(size, 0, 0);
	GFC_DEBUG_WARNING1(ret ==0, "GStandardAllocator::Alloc a block of size: %u has not been allocated", size);	
    return ret;
}
void    GStandardAllocator::Free(void *pmem)
{
    FreeImpl(pmem, 0);
}
void*   GStandardAllocator::AllocAligned(UPInt size, UPInt align, UPInt extra)
{
    if (align == 0)
        align = sizeof(void*);
    // Extra must be a multiple of pointer size.
    GASSERT((extra & (sizeof(void*)-1)) == 0);
    void* ret = AllocImpl(size, align, extra);
	GFC_DEBUG_WARNING1(ret ==0, "GStandardAllocator::AllocAligned a block of size: %u has not been allocated", size);	
    return ret;
}
void    GStandardAllocator::FreeAligned(void *pmem)
{
    FreeImpl(pmem, 1);
}


/* *****    Memory Allocation debugging code
        
    This code does allocation & verification of memory,
    my allocating and freeing random blocks & doing verified memory fills in them


    struct Chunk
        {
        UByte*  p;
        SInt    size;
        UByte   start;
        SInt    iteration;
        };

    Chunk   chunks[0x1000];
    int     i;

    GCString s;

    GMemUtil::Set(chunks, 0, sizeof(Chunk)*0x1000);

    for(i=0; i<100000; i++)
        {
        if (i%5000 == 0)
            {
            s.Format("%d\n",i);
            OutputDebugString(s.GetPtr());
            }

        int index = rand()&0xFFF;

        // If allocated, verify & free
        if (chunks[index].p)
            {
            // Verify           
            UByte v = chunks[index].start;
            int   j = 0;
            while(j<chunks[index].size)
                {
                if (chunks[index].p[j]!=v)
                    {
                    // Memory corrupted
                    _asm int 3;
                    }               
                j++;
                v++;
                }                   

            // Free or realloc?
            if (rand()&1)
                {
                // Free
                BuddyFree(chunks[index].p);
                chunks[index].p = 0;
                chunks[index].iteration = i;
                }
            else
                {               

                // Or reallocate
                SInt    newSize = rand()%32000;
                UByte  prevVal = chunks[index].size ? chunks[index].p[chunks[index].size-1] : chunks[index].start-1;
                chunks[index].p = (UByte*)BuddyRealloc(chunks[index].p,newSize);
                

                // Extend verifier data
                if (newSize>chunks[index].size)
                    {
                    UByte v = prevVal+1;
                    for(int j=chunks[index].size; j<newSize; j++)
                        {
                        chunks[index].p[j] = v;
                        v++;
                        }
                    }
                // Store size
                chunks[index].size = newSize;
                chunks[index].iteration = i;
                }

            }
        
        // Otherwise allocate & initialize
        else
            {
            // Allocate
            chunks[index].size  = rand()%32000;
            chunks[index].start = rand()&0xFF;
            chunks[index].p     = (UByte*)BuddyAlloc(chunks[index].size);

            // Initialize
            
            UByte v = chunks[index].start;          
            for(int j=0; j<chunks[index].size; j++)
                {
                chunks[index].p[j] = v;             
                v++;
                }
            chunks[index].iteration = i;
            }
        }
*/



// ****** Declared classes
class GDebugAllocator;
struct GAllocList;

static GAllocList*  GFC_pAllocList      = 0;
static GAllocator*  GFC_pBaseAllocator  = 0;

// Static lock used for allocators; uses ASSERT to guard improper use.
// 
class CheckedStaticLock
{
public:
    GLock   Lock;
    bool    Initialized; // Debug flag.

    CheckedStaticLock()  { Initialized = 1; }
    ~CheckedStaticLock() { Initialized = 0; }

    inline GLock* GetLock()
    {
        // If this ASSERT hits, it means that lock is being used while not
        // initialized. This can happen if clients try to create global 
        // GFx objects or allocate/release memory through GFx during static
        // initialization. Such use is not allowed.
        GASSERT(Initialized == 1);
        return &Lock;
    }    
};

CheckedStaticLock   AllocListLock;


// ***** Memory tracking


struct GAllocList
{

    // A simple doubly-linked list node.
    class AllocNode
    {
    public:
        union   {
            AllocNode *pPrev;
            AllocNode *pLast;
        };
        union   {
            AllocNode *pNext;
            AllocNode *pFirst;
        };

        // Initialize empty list root node.
        void EmptyList()
        { pNext = pPrev = this; }

        bool IsEmpty() const
        { return (pPrev == this); }

        // Inserts this node at the head of the list.
        void    AddToList(AllocNode *proot)
        {
            pNext = proot->pFirst;
            pPrev = proot;
            proot->pFirst->pPrev = this;
            proot->pFirst = this;
        }
        
        void    RemoveNode()
        {
            pPrev->pNext = pNext;
            pNext->pPrev = pPrev;
            // Null our pointers
            pNext = pPrev = 0;
        }
    };

    struct AllocInfo : public AllocNode
    {
        void*           pBlock;
        UPInt           Size;
        SInt            BlockType;
        SInt            Line;
        const char*     pFilename;
        const char*     pClassname;
    };

    // *** Extra data within user allocation
    
    enum
    {
        // Sizes of canaries.
        Alloc_HeadCanarySize = sizeof(void*),
        Alloc_TailCanarySize = sizeof(void*),

        // Header contains a pointer to data followed by canary bytes.
		//Align this to the platform alignment
        Alloc_BaseHeadSize   = Alloc_HeadCanarySize + sizeof(void*),
		Alloc_HeadSize       = (Alloc_BaseHeadSize + (SYSTEMALIGNMENT - 1)) & ~(SYSTEMALIGNMENT - 1),
        // Total allocation overhead added.
        Alloc_Overhead       = Alloc_HeadSize + Alloc_TailCanarySize
    };

    
private:    
    // All of the current allocations.
    AllocNode       Allocations;
    // Single-chained free list of nodes; 0 - terminated.
    AllocInfo*      pFree;

    // *** Node block pool implementation.    

    enum { BlockInfoCount = 256 };
    struct AllocInfoBlock
    {
        AllocInfo        Info[BlockInfoCount];
        AllocInfoBlock*  pPrev;
    };
    // A linked list of allocated Info blocks; maintained so
    // that they can be released all at once.
    AllocInfoBlock*     pBlocks;



    // Grabs next available AllocInfo node.
    AllocInfo*  AllocInfoNode();
    // Places a node into the free pool.
    void        FreeInfoNode(AllocInfo* pnode);

    // Static creation / cleanup of this object.
    static void AllocList();
    static void FreeList();

    // Non-Static versions of APIS.
    void    AddBlock(UByte *pnew, UPInt size, int blocktype,
                     const char* pfilename, int line, const char *pclassname);
    void    RemoveBlock(UByte *pold, int blocktype);
    void    DumpBlocks();

    // Functions for setting & checking canaries.
    static void    AddCanary(UByte *poffset, UPInt size)
    { memset(poffset, 0xCC, size); }

    static bool    VerifyCanary(UByte *poffset, UPInt size)
    {
        static UByte data[16] = { 0xCC,0xCC,0xCC,0xCC, 0xCC,0xCC,0xCC,0xCC,
                                  0xCC,0xCC,0xCC,0xCC, 0xCC,0xCC,0xCC,0xCC };
        GCOMPILER_ASSERT(Alloc_HeadCanarySize <= 16);
        GCOMPILER_ASSERT(Alloc_TailCanarySize <= 16);
        GASSERT(size <= 16);
        return (memcmp(poffset, data, size) == 0);
    }
    
public:


    static void    AddCanaries(UByte *poffset, UPInt blockSize)
    {
        poffset += Alloc_HeadSize - Alloc_HeadCanarySize;
        AddCanary(poffset, Alloc_HeadCanarySize);
        AddCanary(poffset + blockSize + Alloc_HeadCanarySize, Alloc_TailCanarySize);
    }
    // Function to verify both canaries and complain to log.
    // Returns true for success, 0 if there was corruption.
    static bool    VerifyCanarys(UByte *poffset, AllocInfo* pinfo);
    
  
    // Externally used APIs (used by GDebugAllocator).
    // Returns a pointer to user data.
    static void*    Add(void *pnew, UPInt size, int blocktype,
                       const char* pfilename, int line, const char *pclassname)
    {
        // Prevent other threads from accessing GFC_pAllocList.
        // TODO: It would be more efficient to lock only once
        GLock::Locker lock(AllocListLock.GetLock());

        if (!GFC_pAllocList)
            AllocList();
        GFC_pAllocList->AddBlock((UByte*)pnew, size, blocktype, pfilename, line, pclassname);

        return ((UByte*)pnew) + Alloc_HeadSize;
    }

    static void*    Remove(void *pmem, int blocktype,
                          const char* pfilename, int line, const char *pclassname)
    {        
        GUNUSED3(pfilename, line, pclassname);
        
        UByte* poldMem = ((UByte*)pmem) - GAllocList::Alloc_HeadSize;

        GLock::Locker lock(AllocListLock.GetLock());
        if (GFC_pAllocList)
            GFC_pAllocList->RemoveBlock(poldMem, blocktype);
        return poldMem;
    }

    // Keep static implementation for Update since it doesn't really need to access lists.
    static void*    Update(AllocInfo *pold, void *pnew, UPInt size, int blocktype,
                           const char* pfilename, int line, const char *pclassname);
    
    static void Dump()
    {        
        GLock::Locker lock(AllocListLock.GetLock());
        if (GFC_pAllocList)
            GFC_pAllocList->DumpBlocks();
    }
};


// Grabs next available AllocInfo node.
GAllocList::AllocInfo*  GAllocList::AllocInfoNode()
{
    if (!pFree)
    {        
        GAllocator*     pinfoAllocator = GMemory::GetBlockAllocator();
        AllocInfoBlock* pblock = 
            (GAllocList::AllocInfoBlock*)pinfoAllocator->Alloc(sizeof(AllocInfoBlock));
        if (!pblock)
        {
            // Out of memory!!
            return 0;
        }

        pblock->pPrev   = pBlocks;
        pBlocks         = pblock;
                        
        // Add nodes in the block to free pool.
        for (UInt i=0; i<BlockInfoCount; i++)
            FreeInfoNode(&pBlocks->Info[i]);
    }

    AllocInfo* pnode = pFree;
    pFree = (AllocInfo*)pFree->pNext;
    return pnode;   
}

// Places a node into the free pool.
void        GAllocList::FreeInfoNode(GAllocList::AllocInfo* pnode)
{
    pnode->pPrev = 0 ; // If pPrev == 0, we'll know it's a free node.
    pnode->pNext = pFree;
    pFree = pnode;
}


// Create global allocator list if it doesn't exist.
void    GAllocList::AllocList()
{
    if (!GFC_pAllocList)
    {
        GAllocator*  pinfoAllocator = GMemory::GetBlockAllocator();

        GFC_pAllocList = (GAllocList*)pinfoAllocator->Alloc(sizeof(GAllocList));
        GFC_pAllocList->Allocations.EmptyList();
        GFC_pAllocList->pFree       = 0;
        GFC_pAllocList->pBlocks     = 0;
   }
}

void    GAllocList::FreeList()
{
    if (!GFC_pAllocList) 
        return;

    GAllocator*  pinfoAllocator = GMemory::GetBlockAllocator();

    // Free all the blocks.
    AllocInfoBlock* pblock = GFC_pAllocList->pBlocks;
    while (pblock)
    {
        AllocInfoBlock *pprev = pblock->pPrev;
        pinfoAllocator->Free(pblock);
        pblock = pprev;
    }
    GFC_pAllocList->pBlocks = 0; // Not technically necessary.
    GFC_pAllocList->pFree   = 0;

    pinfoAllocator->Free(GFC_pAllocList);
    GFC_pAllocList = 0;
}



// Function to verify both canaries and complain to log.
// Returns true for success, 0 if there was corruption.
bool GAllocList::VerifyCanarys(UByte *poffset, AllocInfo* pinfo)
{
    poffset += Alloc_HeadSize - Alloc_HeadCanarySize;
    void *pblock = (void*)(((UByte*)pinfo->pBlock) + GAllocList::Alloc_HeadSize);
    GUNUSED(pblock);

    if (!VerifyCanary(poffset, Alloc_HeadCanarySize))
    {
        if (pinfo->pClassname)
        {   // Use %p for pointer formatting to maintain 64-bit compatibility.
            GFC_DEBUG_MESSAGE6(pinfo->pFilename, "%s(%d) : memory corruption at 0x%p before allocation 0x%p, %d bytes, class '%s'", 
                               pinfo->pFilename, pinfo->Line, poffset, pblock, pinfo->Size, pinfo->pClassname);
        }
        else 
        {
            GFC_DEBUG_MESSAGE5(pinfo->pFilename, "%s(%d) : memory corruption at 0x%p before allocation 0x%p, %d bytes", 
                               pinfo->pFilename, pinfo->Line, poffset, pblock, pinfo->Size);
        }
        GFC_DEBUG_MESSAGE3(!pinfo->pFilename,"Memory corruption at 0x%p before allocation in unknown file 0x%p, %d bytes", 
                           poffset, pblock, pinfo->Size);

        return 0;
    }

    UByte *pendOffset = poffset + Alloc_HeadCanarySize + pinfo->Size;
    if (!VerifyCanary(pendOffset, Alloc_TailCanarySize))
    {
        if (pinfo->pClassname)
        {   // Use %p for pointer formatting to maintain 64-bit compatibility.
            GFC_DEBUG_MESSAGE6(pinfo->pFilename, "%s(%d) : memory corruption at 0x%p after allocation 0x%p, %d bytes, class '%s'", 
                               pinfo->pFilename, pinfo->Line, pendOffset, pblock, pinfo->Size, pinfo->pClassname);
        }
        else 
        {
            GFC_DEBUG_MESSAGE5(pinfo->pFilename, "%s(%d) : memory corruption at 0x%p after allocation 0x%p, %d bytes", 
                               pinfo->pFilename, pinfo->Line, pendOffset, pblock, pinfo->Size);
        }

        GFC_DEBUG_MESSAGE3(!pinfo->pFilename,"Memory corruption at 0x%p after allocation in unknown file 0x%p, %d bytes", 
                           pendOffset, pblock, pinfo->Size);

        return 0;
    }

    return 1;
}



void    GAllocList::AddBlock(UByte *pnew, UPInt size, int blocktype,
                             const char* pfilename, int line, const char *pclassname)
{
    if (!pnew) 
        return;
  
#ifdef GFC_BUILD_DEBUG
    // Set memory to constant value for debugging.
    memset(pnew + Alloc_HeadSize, 0xBB, size);
#endif

    if (!blocktype)
    {
        *((AllocInfo**)pnew) = 0;
        return;
    }

    // Insert canaries around the block.
    AddCanaries(pnew, size);

    // Get node and fill it in with data.
    AllocInfo* pinfo = AllocInfoNode();
    
    pinfo->pBlock       = pnew;
    pinfo->Size         = size;
    pinfo->BlockType    = blocktype;
    pinfo->Line         = line;
    pinfo->pFilename    = pfilename;
    pinfo->pClassname   = pclassname;
    pinfo->pNext        = 0;

    *((AllocInfo**)pnew) = pinfo;

    // Add node to list.
    pinfo->AddToList(&Allocations);
}


void    GAllocList::RemoveBlock(UByte *poldMem, int blocktype)
{
    GAllocList::AllocInfo* pinfo = *(GAllocList::AllocInfo**)(poldMem);

    if (!blocktype || !pinfo)
        return;

    // Verify the allocation address; if this is not correct
    // there must have been memory corruption.
    GASSERT(pinfo->pBlock == poldMem);

    // Verify canaries here.
    // If this assertion hits, there was a memory corruption around the
    // block that is being released. Look into log for the message.
    GASSERT(VerifyCanarys(poldMem, pinfo));    

    // Remove allocation node and add it to free list.
    pinfo->RemoveNode();
    FreeInfoNode(pinfo);
    
    // Free the list in the end, so that we don't get external debug library warnings.
    if (Allocations.IsEmpty())
        FreeList();
}

// static, since we don't need 'this' for update.
void*    GAllocList::Update(AllocInfo *pinfo, void *pnewMem, UPInt size, 
                            int blocktype, const char* pfilename, int line, const char *pclassname)
{
    UByte* pnew = (UByte*)pnewMem;

    if (!blocktype)
    {
        *((AllocInfo**)pnew) = 0;
        return pnew + Alloc_HeadSize;
    }
    
    // Handle Alloc/Free style of requests.
    if (!pinfo)
    {
        return Add(pnew,size,blocktype,pfilename,line,pclassname);
    }
    // This is checked for by the caller.
    GASSERT(pnew != 0);   

    // Previous pointer should already be there since this is called
    // from Realloc, which does a memory copy.
    GASSERT(*((AllocInfo**)pnew) == pinfo);

    pinfo->pBlock   = pnew;
    pinfo->Size     = size;
    
    // Insert canaries around the new block size.
    AddCanaries(pnew, size);

    return pnew + Alloc_HeadSize;
}


void    GAllocList::DumpBlocks()
{
    if (Allocations.IsEmpty()) 
        return;

    UPInt       leakedBytes     = 0;
    UPInt       leakedAllocs    = 0;
    
    GFC_DEBUG_ERROR(1, "Memory leaks detected!");

    AllocInfo*  pinfo = (AllocInfo*)Allocations.pFirst;
    while (pinfo != &Allocations)
    {
        void *pblock = (void*)(((UByte*)pinfo->pBlock) + GAllocList::Alloc_HeadSize);
        GUNUSED(pblock);
        
        if (pinfo->pClassname)
        {   // Use %p for pointer formatting to maintain 64-bit compatibility.
            GFC_DEBUG_MESSAGE5(pinfo->pFilename, "%s(%d) : 0x%p, %d bytes, class '%s'", 
                               pinfo->pFilename, pinfo->Line, pblock, pinfo->Size, pinfo->pClassname);
            GFC_DEBUG_MESSAGE2(!pinfo->pFilename,"Memory leak in unknown file: 0x%08X, %d bytes", 
                               pblock, pinfo->Size);
        }
        else 
        {           
            GFC_DEBUG_MESSAGE4(pinfo->pFilename, "%s(%d) : 0x%p, %d bytes", 
                               pinfo->pFilename, pinfo->Line, pblock, pinfo->Size);
            GFC_DEBUG_MESSAGE2(!pinfo->pFilename,"Memory leak in unknown file: 0x%08X, %d bytes", 
                               pblock, pinfo->Size);
        }
        leakedBytes += pinfo->Size;
        leakedAllocs++;
        pinfo = (AllocInfo*)pinfo->pNext;
    }

    GFC_DEBUG_ERROR2(1,"Total memory leaked: %d bytes in %d allocations", 
                        leakedBytes, leakedAllocs);
}


// *** Debug Alloc/Realloc/Free

void*   GDebugAllocator::Alloc(size_t size,
                               int blocktype, const char* pfilename,
                               int line, const char *pclassname)
{
    if (!GFC_pBaseAllocator)
        GFC_pBaseAllocator = GMemory::GetAllocator();
  
    void *pnewMem = GFC_pBaseAllocator->Alloc(size + GAllocList::Alloc_Overhead);
    if (!pnewMem) return 0;       
    return GAllocList::Add(pnewMem, size, blocktype, pfilename, line, pclassname);
}

void*   GDebugAllocator::Realloc(void *pmem, size_t size,
                                 int blocktype, const char* pfilename,
                                 int line, const char *pclassname)
{
    if (!GFC_pBaseAllocator)
        GFC_pBaseAllocator = GMemory::GetAllocator();
    
    if (!pmem)
    {
        void* pnewMem = GFC_pBaseAllocator->Alloc(size + GAllocList::Alloc_Overhead);
        return GAllocList::Add(pnewMem, size, blocktype, pfilename, line,pclassname);
    }
    if (size == 0)
    {
        void* poldMem = GAllocList::Remove(pmem, blocktype, pfilename, line,pclassname);
        GFC_pBaseAllocator->Free(poldMem);
        return 0;
    }

    // Reallocate memory to new size.
    UByte*                  poldMem  = ((UByte*)pmem) - GAllocList::Alloc_HeadSize;
    GAllocList::AllocInfo*  poldInfo = *(GAllocList::AllocInfo**)(poldMem);

    // Check old canaries around the block.
    // If this assertion hits, there was a memory corruption around the
    // block that is being reallocated. Look into log for the message.
    GASSERT(GAllocList::VerifyCanarys(poldMem, poldInfo));

    void* pnewMem  = GFC_pBaseAllocator->Realloc(poldMem, size + GAllocList::Alloc_Overhead);    
    if (!pnewMem) return 0;
    return GAllocList::Update(poldInfo, pnewMem, size, blocktype, pfilename, line, pclassname);    
}

void    GDebugAllocator::Free(void *pmem,
                              int blocktype, const char* pfilename,
                              int line, const char *pclassname)
{
    if (!pmem) return;
    void* poldMem = GAllocList::Remove(pmem, blocktype, pfilename, line, pclassname);
    if (!GFC_pBaseAllocator)
        GFC_pBaseAllocator = GMemory::GetAllocator();
    GFC_pBaseAllocator->Free(poldMem);
}

// *** Aligned versions

void*   GDebugAllocator::AllocAligned(size_t size, size_t align, size_t extra,
                                      int blocktype, const char* pfilename,
                                      int line, const char *pclassname)
{
    if (!GFC_pBaseAllocator)
        GFC_pBaseAllocator = GMemory::GetAllocator();

    void *pnewMem = GFC_pBaseAllocator->AllocAligned(size + GAllocList::Alloc_TailCanarySize,
                                                     align, extra + GAllocList::Alloc_HeadSize);
    if (!pnewMem) return 0;
    return GAllocList::Add(pnewMem, size, blocktype, pfilename, line, pclassname);
}

void    GDebugAllocator::FreeAligned(void *pmem,
                                     int blocktype, const char* pfilename,
                                     int line, const char *pclassname)
{
    if (!pmem) return;
    void* poldMem = GAllocList::Remove(pmem, blocktype, pfilename, line, pclassname);
    if (!GFC_pBaseAllocator)
        GFC_pBaseAllocator = GMemory::GetAllocator();
    GFC_pBaseAllocator->FreeAligned(poldMem);
}



const GAllocatorStats*    GDebugAllocator::GetStats() const
{
    if (!GFC_pBaseAllocator)
        GFC_pBaseAllocator = GMemory::GetAllocator();
    return GFC_pBaseAllocator->GetStats();
}

void    GDebugAllocator::Dump()
{
    GAllocList::Dump();
}



