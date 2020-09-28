/**********************************************************************

Filename    :   GStandardAllocator.h
Content     :   Standard buddy system memory allocator
Created     :   December 28, 1998
Authors     :   Michael Antonov
Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GSTANDARDALLOCATOR_H
#define INC_GSTANDARDALLOCATOR_H

#include "GTypes.h"
#include "GAllocator.h"
// Include atomic ops for GLock implementation.
#include "GAtomic.h"

class GStandardAllocator : public GAllocator
{
private:

    // Header is stored in front of every allocation. Note that allocation is
    // aligned as required for pointers, while header is located before that
    // alignment.
    struct Header
    {
        UInt8   Value;
        UInt8   SplitCount;

        enum ValueConstants
        {
            IndexMask           = 0x7F,     
            AllocatedMask       = 0x80,
            AllocatedShift      = 7
        };
        enum SplitConstants
        {
            Aligned = 0x80
        };

        inline bool    IsRightNode()
        { return SplitCount==0; }   
        inline SInt    GetIndex()
        { return Value&IndexMask; } 
    
        // A high bit set in SplitCount interprets us as an AlignHeader component.
        inline bool    IsAlignHeader() const
        { return (SplitCount & Aligned) != 0; }

        // Returns and 1 if node is free and index == parameter
        inline bool    IsFree_And_IndexIs(UInt8 index)
        { return Value==index; }

        // No masking happens
        inline void    SetValue(UInt8 index, UInt8 allocated, UInt8 splitCount)
        { Value = UInt8(index | (allocated<<AllocatedShift)); SplitCount = splitCount; }    
        inline void    SetIndex_And_AllocatedFlag(UInt8 index, UInt8 allocated)
        { Value = UInt8(index | (allocated<<AllocatedShift)); }

        // Gets user pointer
        inline void*   GetNodePtr()
        { return ((UByte*)this)+sizeof(Header); }        
    };

    // AlignHeader can be overlapped with Header, indicating that block was
    // adjusted for alignment. In that case Offset can be used to seek back
    // in memory to where the real buddy header is. We keep past of the
    // header structure for a AlignHeader and buddy Header the same to
    // allow discrimination among the type of allocation that took place.
    struct AlignHeader 
    {
        UInt16  Offset;
        Header  HeaderVal;
   
        inline void     SetAligned(UInt16 offset)
        { GASSERT(offset); HeaderVal.SplitCount = Header::Aligned; Offset = offset; }
        inline UInt16   GetOffset()
        { return (HeaderVal.IsAlignHeader()) ? Offset : 0; }
    };

    // When used in memory, every node always has a header in front of it. However,
    // due to pointer alignment we can not deriver from Header, thus we just use
    // HeaderRef() function instead.
    struct Node
    {
        Node *pNext;
        Node *pPrev;

        inline Node* GetRightNode(SPInt size)
        { return (Node*) (((UByte*)this) + size); }
        inline Node* GetLeftNode(SPInt size)
        { return (Node*) (((UByte*)this) - size); }

        inline Header* GetHeader()      { return (Header*) (((UByte*)this) - sizeof(Header)); }

        // Inlines delegating to header functions.
        inline bool    IsRightNode()    { return GetHeader()->IsRightNode(); }   
        inline SInt    GetIndex()       { return GetHeader()->GetIndex(); }
        // Returns and 1 if node is free and index == parameter
        inline bool    IsFree_And_IndexIs(UInt8 index)
        { return GetHeader()->IsFree_And_IndexIs(index); }

        inline void    SetValue(UInt8 index, UInt8 allocated, UInt8 splitCount)
        { GetHeader()->SetValue(index, allocated, splitCount); }    
        inline void    SetIndex_And_AllocatedFlag(UInt8 index, UInt8 allocated)
        { GetHeader()->SetIndex_And_AllocatedFlag(index, allocated); }        
    };

    enum Constants
    {
        // 17 entries, but allocate all 32 nodes
        // (that will allow to access their indexes even though they are not used)
        Buddy_TableSize         = 17,
        Buddy_TableSpace        = 32,  // Have to be a power of 2
        // Table starting indexes
        Buddy_SmallTableIndex   = 0,
        Buddy_BigTableIndex     = Buddy_TableSpace,

        // Maximum Index that can actually contain free buddy nodes
        // New nodes are added here when needed
        Buddy_SmallTableMaxIndex= Buddy_TableSize - 1,
        Buddy_BigTableMaxIndex  = Buddy_BigTableIndex + Buddy_TableSize - 1,
    };

    // 
    struct IndexEntry
    {
        // Linked list structures
        Node   Head;
        Node   Tail;
        // And size of this block
        // (only BlockSize-sizeof(Header) can actually be allocated)
        SPInt  BlockSize;

        // How much can be allocated for this block
        inline SPInt       GetAllocSize()
        { return BlockSize - sizeof(Header); }
    };

    IndexEntry      BlockTable[Buddy_TableSpace*2];
    GAllocator*     pBlockAllocator;
    GAllocator*     pSystemAllocator;
    GAllocatorStats Stats;
    // Thread lock used to guard allocations. With no thread
    // support this is implemented as a no-op.
    GLock           Lock;

    // *** Functionality
    void    InitBlockTable();

    void    FreeBlock(Node *pnode);
    bool    AllocBlock(UInt index);
    // Called when chunk is larger than the biggest block size.
    void*   SystemAlloc(size_t size, size_t align, size_t extra);
    void    SystemFree(void* pmem, bool aligned);
    // Allocation implementation.
    void*   AllocImpl(UPInt size, UPInt align, UPInt extra);
    void    FreeImpl(void *pmem, bool aligned);

public:
    GStandardAllocator();

    // Allocation interface.    
    void*   Alloc(UPInt size);
    void*   Realloc(void *pmem, UPInt size);
    void    Free(void *pmem);
    // Aligned Version.
    void*   AllocAligned(UPInt size, UPInt align, UPInt extra);
    void    FreeAligned(void *pmem);

    const GAllocatorStats* GetStats() const { return &Stats; }
};


class GStandardBlockAllocator : public GAllocator
{
public:
    GStandardBlockAllocator();

    void    Free(void *pmem);
    void*   Alloc(UPInt size);
    void*   Realloc(void *pmem, UPInt size);
    
    GAllocatorStats Stats;
    const GAllocatorStats* GetStats() const { return &Stats; }
};



#endif // INC_GSTANDARDALLOCATOR_H
