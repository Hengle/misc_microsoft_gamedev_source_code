#if !defined(GRANNY_MEMORY_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_memory.h $
// $DateTime: 2007/08/27 11:38:49 $
// $Change: 15837 $
// $Revision: #32 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

// We need this macro to be a compile time constant so we can use it
// in compile asserts.  In gcc 4.0+, it apparently isn't recognized as
// such with the normal macro, so use the version in stddef.h
#if !(COMPILER_GCC && (__GNUC__ >= 4))
  #define OffsetFromType(structure, Member) ((intaddrx)&((structure *)0)->Member)
#else
   #include <stddef.h>
   #define OffsetFromType(structure, Member) offsetof(structure, Member)
#endif

// By default, we turn on debug memory if DEBUG is on, but you can control
// it separately by defining DEBUG_MEMORY up front
#if !defined(DEBUG_MEMORY)
#define DEBUG_MEMORY DEBUG
#endif

BEGIN_GRANNY_NAMESPACE;

// TODO: It'd be nice if I added tags to the memory allocator such that
// any Granny allocation is marked as "transient" or "permanent", or
// some sort of arena system, so people who override our allocators
// can make smart decisions about that sort of thing.

// TODO: At some point, it might be nice to extend the allocator
// functionality to include automated initialization and such, for
// inserting counts into structures and initializing blocks to constant
// 8-bit or 32-bit value (since these are the operations that seem
// to be used in many places).

// TODO: Add a "clean up all" function, so the regression tester can
// flush everything before continuing if there are freeing errors.

// SizeOf and OffsetOf are provided as preferred macros to the standard
// sizeof and offsetof in ANSI C, since certain platforms may not
// actually have them, or they may return different types.
#define SizeOf(something) ((int32x)sizeof(something))
#define OffsetFromPtr(Ptr, Member) ((uint8 *)(&Ptr->Member) - (uint8 *)Ptr)
#define SizeOfMember(structure, member) SizeOf(((structure *)0)->member)
#define ArrayLength(Array) (SizeOf(Array)/SizeOf((Array)[0]))

// Use this macro instead of straight pointer arithmetic when you're
// doing undefined comparisons between pointers from different
// allocations ranges, and only need the sign of the result to be
// correct.  A bug has been observed when compiling with the MacOSX
// version of gcc in which the computation used for the difference
// yields incorrect results when the pointers are offset by a number
// of bytes not a multiple of the structure size.  This is just
// faster, anyways, if all you need is the sign, the implied divide is
// unnecessary.
#define PtrDiffSignOnly(PtrA, PtrB) (((intaddrx)(PtrA)) - ((intaddrx)(PtrB)))

// Evaluates to true if the two ranges overlap.  Handy for enforcing
// NOALIAS restrictions.  Note that the start and end follow STL
// conventions for this macro.
#define PtrRangesOverlap(AStart, AEnd, BStart, BEnd)    \
    (!(PtrDiffSignOnly(AEnd, BStart) <= 0 || PtrDiffSignOnly(AStart, BEnd) >= 0))

#define StructuresOverlap(A, B) \
    PtrRangesOverlap(&(A), &(A) + 1, &(B), &(B) + 1)

// The basic allocate operates via the Allocate() and Deallocate()
// functions, which manage memory and also track allocations and
// deallocations.  These are _SLOW BY DESIGN_, in an effort to
// discourage people from calling them very often.  If you have lots
// of allocations to do, consider using the aggregation macros
// described later.
#define Allocate(Type) (Type *)(*AllocateCallback)(__FILE__, __LINE__, DefaultAllocationAlignment, SizeOf(Type))
#define AllocateArray(Count, Type) (Type *)(*AllocateCallback)(__FILE__, __LINE__, DefaultAllocationAlignment, SizeOf(Type)*(Count))
#define AllocateSize(Size) (*AllocateCallback)(__FILE__, __LINE__, DefaultAllocationAlignment, Size)

#define AllocateAligned(Alignment, Type) (Type *)(*AllocateCallback)(__FILE__, __LINE__, Alignment, SizeOf(Type))
#define AllocateArrayAligned(Alignment, Count, Type) (Type *)(*AllocateCallback)(__FILE__, __LINE__, Alignment, SizeOf(Type)*(Count))
#define AllocateSizeAligned(Alignment, Size) (*AllocateCallback)(__FILE__, __LINE__, Alignment, Size)

#define Deallocate(Memory) (*DeallocateCallback)(__FILE__, __LINE__, Memory)
#define DeallocateSafe(Memory) { (*DeallocateCallback)(__FILE__, __LINE__, Memory); (Memory) = 0; }

// All allocations in Granny are tracked, period, whether you're running
// in release or debug or whatever.  The idea is that everybody should
// be very conscientious about memory usage, so every running program should
// be able to provide a profile of the memory.  The following functions
// allow you to iterate over the allocated memory blocks, and find out
// who allocated them (and how much they allocated).
EXPGROUP(MemoryIterationGroup)
EXPTYPE struct allocation_header;

// Iteration (STL style, only instead of ++ you use NextAllocation)
EXPAPI GS_MODIFY allocation_header *AllocationsBegin(void);
EXPAPI GS_MODIFY allocation_header *NextAllocation(allocation_header *Current);
EXPAPI GS_MODIFY allocation_header *AllocationsEnd(void);

// Accessors
EXPTYPE_EPHEMERAL struct allocation_information
{
    void *Memory;
    intaddrx RequestedSize;
    intaddrx ActualSize;
    char const *SourceFileName;
    int32x SourceLineNumber;
};
EXPAPI GS_READ void GetAllocationInformation(allocation_header const *Header,
                                             allocation_information &Information);

// Blockable checks
EXPAPI GS_MODIFY void *BeginAllocationCheck(void);
EXPAPI GS_MODIFY allocation_header *CheckedAllocationsEnd(void *CheckIdentifier);
EXPAPI GS_MODIFY bool EndAllocationCheck(void *CheckIdentifier);


struct aggregate_allocation
{
    bool UseRawOffset;     // If true, write offsets instead of pointers
    void **WriteToPointer; // If non-zero, write the pointer back here
    intaddrx WriteToOffset; // If WriteToPointer was zero, write to this offset
    intaddrx Offset;        // The offset of the allocation inside the block

    int32x Count;           // If count is not -1, write the count to this
    intaddrx CountOffset;   // offset

    intaddrx Alignment;     // Alignment for this sub-allocation
};

struct aggr_allocator
{
    intaddrx AllocationAlignment;
    intaddrx TotalAggregateSize;
    int32x NextUnusedAggregate;
    aggregate_allocation Aggregates[MaximumAggregateCount];
};


// Call this to initialize an aggregate allocator.  Note that the structure
//  returned is rendered useless by EndAggr*
// Use as:
//  aggr_allocator Allocator;
//  InitializeAggrAlloc(Allocator);
#define InitializeAggrAlloc(Allocator) InitializeAggregateAllocation_(&Allocator, __FILE__, __LINE__)

// The aggregation functions are useful for grouping small allocations into
// a large whole, or even just to ensure that several large allocations
// all occur successfully.  Call AggrAlloc*() as many times as you like,
// then finish it all with a call to EndAggrAlloc().  The memory will
// only be valid after EndAggrAlloc(), so don't try to use the memory
// in between AggrAlloc() and EndAggrAlloc().
#define AggrAllocPtr(Allocator, ReturnPointer)                                          \
    AggregateAllocate_(Allocator, (void **)&(ReturnPointer), SizeOf(*(ReturnPointer)))

#define AggrAllocArrayPtr(Allocator, Count, ReturnPointer)                                          \
    AggregateAllocate_(Allocator, (void **)&(ReturnPointer), (Count) * SizeOf(*(ReturnPointer)))

#define AggrAllocSizePtr(Allocator, Size, ReturnPointer)            \
    AggregateAllocate_(Allocator, (void **)&(ReturnPointer), Size)

#define AggrAllocOffset(Allocator, OwnerPointer, type, Member)          \
    AggregateAllocateOffset_(Allocator, OwnerPointer, SizeOf(type),     \
                             OffsetFromPtr(OwnerPointer, Member))

#define AggrAllocOffsetSize(Allocator, OwnerPointer, Size, Member)      \
    AggregateAllocateOffset_(Allocator, OwnerPointer, Size,             \
                             OffsetFromPtr(OwnerPointer, Member))

#define AggrAllocOffsetPtr(Allocator, OwnerPointer, Member)                                         \
    AggregateAllocate_(Allocator, OwnerPointer, OffsetFromPtr(OwnerPointer, Member), SizeOf(*OwnerPointer->Member))

#define AggrAllocOffsetArrayPtr(Allocator, OwnerPointer, Count, CountMember, PtrMember)    \
    AggregateAllocate_(Allocator, OwnerPointer, OffsetFromPtr(OwnerPointer, CountMember),      \
                       OffsetFromPtr(OwnerPointer, PtrMember),                     \
                       Count, SizeOf(*OwnerPointer->PtrMember))

#define AggrAllocOffsetArraySizePtr(Allocator, OwnerPointer, Count, Size, CountMember, PtrMember)   \
    AggregateAllocate_(Allocator, OwnerPointer, OffsetFromPtr(OwnerPointer, CountMember),           \
                       OffsetFromPtr(OwnerPointer, PtrMember), Count, Size)

#define AggrAllocOffsetSizePtr(Allocator, OwnerPointer, Size, Member)                           \
    AggregateAllocate_(Allocator, OwnerPointer, OffsetFromPtr(OwnerPointer, Member), Size)

#define AggrAllocOffsetCountlessArrayPtr(Allocator, OwnerPointer, Count, Member)          \
        AggregateAllocate_(Allocator, OwnerPointer, OffsetFromPtr(OwnerPointer, Member), \
                           (Count) * SizeOf(*OwnerPointer->Member))

#define SubAggrAllocPtr(Allocator, OwnerOffset, Type, Member) \
        AggregateAllocate_(Allocator, OwnerOffset + (uint8)(&((Type *)0)->Member), SizeOf(((Type *)0)->Member))
#define SubAggrAllocArrayPtr(Allocator, OwnerOffset, Type, Count, Member) \
        AggregateAllocate_(Allocator, OwnerOffset + (uint8)(&((Type *)0)->Member), SizeOf(*((Type *)0)->Member) * (Count))
#define SubAggrAllocSizePtr(Allocator, OwnerOffset, Type, Size, Member) \
        AggregateAllocate_(Allocator, OwnerOffset + (uint8)(&((Type *)0)->Member), Size)

// Note that EndAggrAlloc() will return the correct pointer to pass
// to Deallocate(), which is always the first one that was AggrAlloc'd.
// However, if you don't want to care about which one that was, just
// keep an extra pointer around and store the result of EndAggrAlloc,
// then pass it to Deallocate when you're done.
#define EndAggrAlloc(Allocator) EndAggregateAllocation_(&Allocator, __FILE__, __LINE__)

// EndAggrPlacement() does just what EndAggrAlloc() does, only it expects
// that you've already allocated the memory.
#define EndAggrPlacement(Allocator, Memory) EndAggregatePlacement_(&Allocator, __FILE__, __LINE__, Memory)

// EndAggrSize() doesn't do any allocation or placing, it just lets
// you know how much you used.
#define EndAggrSize(Allocator) EndAggregateSize_(&Allocator, __FILE__, __LINE__)

// These calls calibrate the way the aggregate allocator works
void SetAggrAlignment(aggr_allocator &Allocator, intaddrx Alignment);

// These are optional replacements for memset, memcmp, etc.
void SetUInt8(intaddrx Count, uint8 Value, void *Buffer);
void SetInt32(intaddrx Count, int32 Value, void *Buffer);
void SetUInt32(intaddrx Count, uint32 Value, void *Buffer);
void SetInt32x(intaddrx Count, int32 Value, void *Buffer);
void SetUInt32x(intaddrx Count, uint32 Value, void *Buffer);
void SetReal32(intaddrx Count, real32 Value, void *Buffer);
void SetPtrNULL(intaddrx Count, void* Buffer);
void CopyStrided(intaddrx RowLength, intaddrx RowCount,
                 intaddrx SourceStride, void const *Source,
                 intaddrx DestStride, void *Dest);
bool Compare(intaddrx Count, void const *Buffer0, void const *Buffer1);


#define CopyArray(Count, Source, Dest) Copy((Count) * SizeOf(*(Source)), Source, Dest);
#define ZeroArray(Count, Array) SetUInt8((intaddrx)( (Count) * SizeOf(*(Array)) ), 0, Array)
#define ZeroStructure(Structure) SetUInt8(SizeOf(Structure), 0, &(Structure))
inline intaddrx Align32(intaddrx Size) {return((Size + 3) & ~3);}
inline intaddrx AlignN(intaddrx Value, intaddrx Alignment)
{intaddrx Pad = Alignment - (Value % Alignment);
 if(Pad != Alignment) {Value += Pad;}
 return(Value);}

// The Reverse() functions do byte-swapping such that files from little
// endian machines can be read on big endian machines and vice versa.
inline uint64 Reverse64(uint64 Value)
{
    // This is slow, but safe, given the PS3 compiler's twitchiness about
    //  type aliasing
    union
    {
        uint64 Result;
        uint8  SwitchArray[8];
    } Reverser;
    CompileAssert(SizeOf(Reverser) == SizeOf(uint64));

    Reverser.Result = Value;
    {for(int32x i = 0; i < 4; ++i)
    {
        uint8 temp = Reverser.SwitchArray[i];
        Reverser.SwitchArray[i] = Reverser.SwitchArray[7 - i];
        Reverser.SwitchArray[7 - i] = temp;
    }}

    return Reverser.Result;
}

inline uint32 Reverse32(uint32 Value)
{
    return((Value << 24)               |
           ((Value & 0x0000FF00) << 8) |
           ((Value & 0x00FF0000) >> 8) |
           (Value >> 24));
}

inline uint32 Reverse16_32(uint32 Value)
{
    return(((Value & 0x00FF00FF) << 8) |
           ((Value & 0xFF00FF00) >> 8));
}

inline uint16 Reverse16_16(uint16 Value)
{
    return(uint16)(((Value & (uint16)0x00FF) << 8) |
                   ((Value & (uint16)0xFF00) >> 8));
}

inline void Copy(intaddrx Count, void const *FromInit, void *ToInit)
{
    uint8 const *From = (uint8 const *)FromInit;
    uint8 *To = (uint8 *)ToInit;
    while(Count--)
    {
        *To++ = *From++;
    }
}

inline void Copy32(intaddrx Count, void const *FromInit, void *ToInit)
{
    uint32 const *From = (uint32 const *)FromInit;
    uint32 *To = (uint32 *)ToInit;
    while(Count--)
    {
        *To++ = *From++;
    }
}

inline void SetReal32(intaddrx Count, real32 Value, void *BufferInit)
{
    real32 *Buffer = (real32 *)BufferInit;

    while(Count--)
    {
        *Buffer++ = Value;
    }
}


void Reverse64(intaddrx Count, void *BufferInit);
void Reverse32(intaddrx Count, void *BufferInit);
void Reverse16(intaddrx Count, void *BufferInit);

// The following functions should generally not be called directly,
// but rather through their macros defined above.
intaddrx AggregateAllocate_(aggr_allocator &Allocator,
                            void **ReturnPointer, intaddrx Size);

intaddrx AggregateAllocate_(aggr_allocator &Allocator,
                            intaddrx Offset, intaddrx Size);

intaddrx AggregateAllocate_(aggr_allocator &Allocator,
                            void *OwnerPointer,
                            intaddrx Offset, intaddrx Size);

intaddrx AggregateAllocate_(aggr_allocator &Allocator,
                            void *OwnerPointer, intaddrx CountOffset,
                            intaddrx PtrOffset, int32x Count, intaddrx Size);

intaddrx AggregateAllocateOffset_(aggr_allocator &Allocator,
                                  void *OwnerPointer, intaddrx Size,
                                  intaddrx Offset);

intaddrx AggregateAllocateOffset_(aggr_allocator &Allocator,
                                  void *OwnerPointer,
                                  int32x Count, intaddrx UnitSize,
                                  intaddrx CountOffset,
                                  intaddrx OffsetOffset);

void InitializeAggregateAllocation_(aggr_allocator *Allocator,
                                    char const *File, int32 Line);

void *EndAggregateAllocation_(aggr_allocator *Allocator,
                              char const *File, int32x Line);
void *EndAggregatePlacement_(aggr_allocator *Allocator,
                             char const *File, int32x Line,
                             void *Memory);
intaddrx EndAggregateSize_(aggr_allocator *Allocator,
                           char const *File, int32x Line);

// For overriding the standard allocators, you can use the following
// types and functions.
// TODO: Define these directly as __cdecl?
EXPGROUP(MemoryCallbackGroup)
EXPAPI typedef void *allocate_callback(char const *File, int32x Line,
                                       intaddrx Alignment, intaddrx Size);
EXPAPI typedef void deallocate_callback(char const *File, int32x Line,
                                        void *Memory);
extern allocate_callback *AllocateCallback;
extern deallocate_callback *DeallocateCallback;

EXPAPI GS_READ void GetAllocator(allocate_callback *&AllocateCallback,
                                 deallocate_callback *&DeallocateCallback);
EXPAPI GS_MODIFY void SetAllocator(allocate_callback *AllocateCallback,
                                   deallocate_callback *DeallocateCallback);

// For freeing objects allocated by the builders
EXPAPI GS_PARAM void FreeBuilderResult(void *Result);


EXPAPI GS_MODIFY void AcquireMemorySpinlock();
EXPAPI GS_MODIFY void ReleaseMemorySpinlock();


// For computing offset sizes, these macros are helpful
#define MAXOFFSET(Store, Offset, Size) \
    if(Store < (Offset + Size)) {Store = Offset + Size;}

// Platform-specific operations
void *PlatformAllocate(intaddrx Size);
void PlatformDeallocate(void *Memory);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_MEMORY_H
#endif
