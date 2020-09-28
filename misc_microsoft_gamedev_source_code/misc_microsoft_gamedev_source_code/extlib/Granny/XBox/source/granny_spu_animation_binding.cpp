// ========================================================================
// $File: //jeffr/granny/rt/granny_spu_animation_binding.cpp $
// $DateTime: 2007/08/31 17:29:59 $
// $Change: 15878 $
// $Revision: #2 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#if !defined(GRANNY_SPU_ANIMATION_BINDING_H)
#include "granny_spu_animation_binding.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_SKELETON_H)
#include "granny_skeleton.h"
#endif

#if !defined(GRANNY_MODEL_H)
#include "granny_model.h"
#endif

#if !defined(GRANNY_SPU_ANIMATION_H)
#include "granny_spu_animation.h"
#endif

#if !defined(GRANNY_SPU_TRACK_GROUP_H)
#include "granny_spu_track_group.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

#undef SubsystemCode
#define SubsystemCode AnimationLogMessage


inline intaddrx
SPUBindingDifference(spu_animation_binding_id& A,
                     spu_animation_binding_id& B)
{
    intaddrx Diff;

    Diff = PtrDiffSignOnly(A.Animation, B.Animation);
    if(Diff) return Diff;

    Diff = PtrDiffSignOnly(A.TrackGroup, B.TrackGroup);
    if(Diff) return Diff;

    Diff = PtrDiffSignOnly(A.Model, B.Model);
    if(Diff) return Diff;

    Diff = StringDifference(A.TrackPattern, B.TrackPattern);
    if(Diff) return Diff;

    Diff = StringDifference(A.BonePattern, B.BonePattern);
    return Diff;

    // Note that the cache pointers do NOT affect the difference...
}

#define CONTAINER_NAME spu_binding_cache
#define CONTAINER_ITEM_TYPE spu_animation_binding
#define CONTAINER_COMPARE_RESULT_TYPE intaddrx
#define CONTAINER_COMPARE_ITEMS(Item1, Item2) SPUBindingDifference((Item1)->ID, (Item2)->ID)
#define CONTAINER_FIND_FIELDS spu_animation_binding_id ID
#define CONTAINER_COMPARE_FIND_FIELDS(Item) SPUBindingDifference(ID, (Item)->ID)
#define CONTAINER_SORTED 1
#define CONTAINER_KEEP_LINKED_LIST 0
#define CONTAINER_SUPPORT_DUPES 0
#define CONTAINER_DO_ALLOCATION 0
#include "granny_contain.inl"

#define CONTAINER_NAME spu_binding_cache_free_list
#define CONTAINER_ITEM_TYPE spu_animation_binding
#define CONTAINER_SORTED 0
#define CONTAINER_KEEP_LINKED_LIST 1
#define CONTAINER_SUPPORT_DUPES 0
#define CONTAINER_PREV_NAME PreviousUnused
#define CONTAINER_NEXT_NAME NextUnused
#define CONTAINER_DO_ALLOCATION 0
#define CONTAINER_NEED_FIND 0
#define CONTAINER_NEED_FINDFIRSTLT 0
#define CONTAINER_NEED_FINDFIRSTGT 0
#include "granny_contain.inl"

// For the spu animations, we don't actually expose this structure
struct spu_animation_binding_cache_status
{
    int32x TotalBindingsCreated;
    int32x TotalBindingsDestroyed;

    int32x DirectAcquireCount;
    int32x IndirectAcquireCount;
    int32x ReleaseCount;

    int32x CurrentTotalBindingCount;
    int32x CurrentUsedBindingCount;

    int32x CacheHits;
    int32x CacheMisses;

    int32x ExplicitFlushCount;
    int32x ExplicitFlushFrees;
    int32x OverflowFrees;
};

static int32x SPUBindingCacheCountMax = 0;
static spu_binding_cache BindingCache;
static spu_binding_cache_free_list BindingCacheFreeList;
static spu_animation_binding_cache_status CacheStatus = { 0 };

int32x GRANNY
GetMaximumSPUAnimationBindingCount(void)
{
    return SPUBindingCacheCountMax;
}

void GRANNY
SetMaximumSPUAnimationBindingCount(int32x BindingCountMax)
{
    SPUBindingCacheCountMax = BindingCountMax;
}

static void
FreeAnimationBinding(spu_animation_binding *Binding)
{
    if(Binding)
    {
        Assert(Binding->UsedBy == 0);

        Remove(&BindingCache, Binding);
        Remove(&BindingCacheFreeList, Binding);

        Deallocate(Binding);

        --CacheStatus.CurrentTotalBindingCount;
        ++CacheStatus.TotalBindingsDestroyed;
    }
}

static void FreeCacheOverflow(void)
{
    while (CacheStatus.CurrentTotalBindingCount > SPUBindingCacheCountMax)
    {
        spu_animation_binding* FreeBinding = Last(&BindingCacheFreeList);
        if (FreeBinding)
        {
            ++CacheStatus.OverflowFrees;
            FreeAnimationBinding(FreeBinding);
        }
        else
        {
            break;
        }
    }
}

static void IncUsedBy(spu_animation_binding* Binding)
{
    if(Binding)
    {
        if(Binding->UsedBy == 0)
        {
            ++CacheStatus.CurrentUsedBindingCount;
            Remove(&BindingCacheFreeList, Binding);
        }

        ++Binding->UsedBy;
        Assert(Binding->UsedBy > 0);
    }
}

static void DecUsedBy(spu_animation_binding* Binding)
{
    if(Binding)
    {
        Assert(Binding->UsedBy > 0);
        --Binding->UsedBy;

        if(Binding->UsedBy == 0)
        {
            --CacheStatus.CurrentUsedBindingCount;
            Add(&BindingCacheFreeList, Binding);
            FreeCacheOverflow();
        }
    }
}

static spu_animation_binding *
NewSPUAnimationBinding(spu_animation_binding_id &ID)
{
    Assert(ID.Animation);
    Assert(ID.TrackGroup);
    Assert(ID.Model);
    skeleton *OnSkeleton = ID.Model->Skeleton;

    Assert(OnSkeleton);
    int32x TrackCount = ID.TrackGroup->TrackNameCount;

    spu_animation_binding* Binding;
    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);
    SetAggrAlignment(Allocator, 16);

    AggrAllocPtr(Allocator, Binding);
    AggrAllocOffsetArrayPtr(Allocator, Binding, TrackCount,
                            TrackNameRemapCount, TrackNameRemaps);
    if(EndAggrAlloc(Allocator))
    {
        Binding->ID = ID;
        Binding->UsedBy = 0;

        {for(int32x TrackIndex = 0; TrackIndex < TrackCount; ++TrackIndex)
        {
            char const* TrackName = ID.TrackGroup->TrackNames[TrackIndex];
            bool Found = false;

            int32x SourceBoneIndex;
            if(StringComparisonCallback ||
               (IsPlainWildcard(Binding->ID.BonePattern) &&
                IsPlainWildcard(Binding->ID.TrackPattern)))
            {
                Found = FindBoneByName(OnSkeleton, TrackName, SourceBoneIndex);
            }
            else
            {
                char TrackNameBuffer[MaximumBoneNameLength + 1];
                WildCardMatch(TrackName, Binding->ID.TrackPattern,
                              TrackNameBuffer);
                Found = FindBoneByRule(OnSkeleton,
                                       TrackNameBuffer,
                                       Binding->ID.BonePattern,
                                       SourceBoneIndex);
            }

            if(Found)
            {
                Binding->TrackNameRemaps[TrackIndex] = SourceBoneIndex;
            }
            else
            {
                // Not found.
                Binding->TrackNameRemaps[TrackIndex] = -1;
            }
        }}

        ++CacheStatus.TotalBindingsCreated;
        ++CacheStatus.CurrentTotalBindingCount;
    }

    return Binding;
}

void GRANNY
MakeDefaultSPUAnimationBindingID(spu_animation_binding_id &ID,
                                 spu_animation const *Animation,
                                 int32x TrackGroupIndex,
                                 model const* Model)
{
    Assert(Animation);
    Assert(Animation->TrackGroups[TrackGroupIndex]);

    ID.Animation = Animation;
    ID.TrackGroup = Animation->TrackGroups[TrackGroupIndex];
    ID.Model = Model;
    ID.BonePattern = "*";
    ID.TrackPattern = "*";

    // Cache the pointers for DMA
    ID.TransformTrackCount = ID.TrackGroup->TransformTrackCount;
    ID.TransformTracks     = ID.TrackGroup->TransformTracks;
    ID.CurveByteCount      = ID.TrackGroup->CurveByteCount;
    ID.CurveBytes          = ID.TrackGroup->CurveBytes;
}

spu_animation_binding* GRANNY
AcquireSPUAnimationBindingFromID(spu_animation_binding_id &ID)
{
    ++CacheStatus.IndirectAcquireCount;
    spu_animation_binding *Binding = Find(&BindingCache, ID);

    if(Binding)
    {
        ++CacheStatus.CacheHits;
        IncUsedBy(Binding);
    }
    else
    {
        ++CacheStatus.CacheMisses;
        Binding = NewSPUAnimationBinding(ID);
        if(Binding)
        {
            Add(&BindingCache, Binding);
            Add(&BindingCacheFreeList, Binding);

            IncUsedBy(Binding);
            FreeCacheOverflow();
        }
    }

    return Binding;
}


spu_animation_binding* GRANNY
AcquireSPUAnimationBinding(spu_animation_binding *Binding)
{
    ++CacheStatus.DirectAcquireCount;
    ++CacheStatus.CacheHits;
    IncUsedBy(Binding);
    return Binding;
}


void GRANNY
ReleaseSPUAnimationBinding(spu_animation_binding *Binding)
{
    ++CacheStatus.ReleaseCount;
    DecUsedBy(Binding);
}

