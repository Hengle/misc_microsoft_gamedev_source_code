// ========================================================================
// $File: //jeffr/granny/rt/granny_stat_hud.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #15 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_STAT_HUD_H)
#include "granny_stat_hud.h"
#endif

#if !defined(GRANNY_CONVERSIONS_H)
#include "granny_conversions.h"
#endif

#if !defined(GRANNY_MODEL_H)
#include "granny_model.h"
#endif

#if !defined(GRANNY_CONTROL_H)
#include "granny_control.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
#endif

#if !defined(GRANNY_STRING_FORMATTING_H)
#include "granny_string_formatting.h"
#endif

#if !defined(GRANNY_SKELETON_H)
#include "granny_skeleton.h"
#endif

#if !defined(GRANNY_MEMORY_ARENA_H)
#include "granny_memory_arena.h"
#endif

#if !defined(GRANNY_MODEL_INSTANCE_H)
#include "granny_model_instance.h"
#endif

#if !defined(GRANNY_MODEL_CONTROL_BINDING_H)
#include "granny_model_control_binding.h"
#endif

#if !defined(GRANNY_CONTROL_H)
#include "granny_control.h"
#endif

#if !defined(GRANNY_TRACK_GROUP_H)
#include "granny_track_group.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

struct model_instance_counter
{
    model *Model;
    int32x InstanceCount;

    model_instance_counter *Left;
    model_instance_counter *Right;
};

inline int32x ModelDifference(const void* a, const void* b)
{
    intaddrx Diff = ((const uint8 *)a - (const uint8 *)b);
    if (Diff < 0)
        return -1;
    else if (Diff > 0)
        return 1;
    return 0;
}


#define CONTAINER_NAME model_instance_counter_tree
#define CONTAINER_ITEM_TYPE model_instance_counter
#define CONTAINER_COMPARE_ITEMS(Item1, Item2) ModelDifference((Item1)->Model, (Item2)->Model)
#define CONTAINER_FIND_FIELDS model *Model
#define CONTAINER_COMPARE_FIND_FIELDS(Item) ModelDifference(Model, (Item)->Model)
#define CONTAINER_SUPPORT_DUPES 0
#include "granny_stat_hud_container.inl"

struct alloc_sorter
{
    stat_hud_alloc_point Point;

    alloc_sorter *Left;
    alloc_sorter *Right;
};
inline int32x
CompareSource(char const *NameA, int32x LineA,
              char const *NameB, int32x LineB)
{
    int32x Diff = StringDifference(NameA, NameB);
    if(Diff) return(Diff);

    return(LineA - LineB);
}

#define CONTAINER_NAME alloc_combiner_tree
#define CONTAINER_ITEM_TYPE alloc_sorter
#define CONTAINER_COMPARE_ITEMS(Item1, Item2)                                       \
    CompareSource((Item1)->Point.SourceFilename, (Item1)->Point.SourceLineNumber,   \
                  (Item2)->Point.SourceFilename, (Item2)->Point.SourceLineNumber);
#define CONTAINER_FIND_FIELDS char const *SourceFilename, int32x SourceLineNumber
#define CONTAINER_COMPARE_FIND_FIELDS(Item)                                     \
    CompareSource(SourceFilename, SourceLineNumber,                                 \
                  (Item)->Point.SourceFilename, (Item)->Point.SourceLineNumber);
#define CONTAINER_SUPPORT_DUPES 0
#include "granny_stat_hud_container.inl"

inline int32x BytesAllocDiff(intaddrx a, intaddrx b)
{
    intaddrx Diff = (a - b);
    if (Diff < 0)
        return -1;
    else if (Diff > 0)
        return 1;
    return 0;
}

#define CONTAINER_NAME alloc_sorter_tree
#define CONTAINER_ITEM_TYPE alloc_sorter
#define CONTAINER_COMPARE_ITEMS(Item1, Item2) BytesAllocDiff((Item2)->Point.BytesAllocated, (Item1)->Point.BytesAllocated)
#define CONTAINER_NEED_FIND        0
#define CONTAINER_NEED_FINDFIRSTLT 0
#define CONTAINER_NEED_FINDFIRSTGT 0
#define CONTAINER_SUPPORT_DUPES 1
#include "granny_stat_hud_container.inl"

struct perf_sorter
{
    stat_hud_perf_point Point;

    perf_sorter *Left;
    perf_sorter *Right;
};
#define CONTAINER_NAME perf_sorter_tree
#define CONTAINER_ITEM_TYPE perf_sorter
#define CONTAINER_COMPARE_ITEMS(Item1, Item2) ((Item2)->Point.TotalCycles - (Item1)->Point.TotalCycles);
#define CONTAINER_NEED_FIND        0
#define CONTAINER_NEED_FINDFIRSTLT 0
#define CONTAINER_NEED_FINDFIRSTGT 0
#define CONTAINER_SUPPORT_DUPES 1
#define CONTAINER_COMPARE_RESULT_TYPE real64x
#include "granny_stat_hud_container.inl"


stat_hud *GRANNY
CaptureCurrentStats(int32x FramesSinceLastCapture)
{
    memory_arena *Arena = NewMemoryArena();

    model_instance_counter_tree ModelInstanceTree;
    Initialize(&ModelInstanceTree, 0);

    alloc_combiner_tree AllocCombinerTree;
    Initialize(&AllocCombinerTree, 0);

    alloc_sorter_tree AllocSorterTree;
    Initialize(&AllocSorterTree, 0);

    perf_sorter_tree PerfSorterTree;
    Initialize(&PerfSorterTree, 0);

    //
    // Model instances
    //
    stat_hud_model_instances ModelInstances = {0};
    {for(model_instance *Instance = GetGlobalModelInstancesBegin();
         Instance != GetGlobalModelInstancesEnd();
         Instance = GetGlobalNextModelInstance(Instance))
    {
        model *Model = &GetSourceModel(*Instance);
        int32x BoneCount = Model->Skeleton->BoneCount;

        model_instance_counter *Counter = Find(&ModelInstanceTree, Model);
        if(!Counter)
        {
            ++ModelInstances.TotalUsedModelCount;
            if(ModelInstances.MaxBoneCount < BoneCount)
            {
                ModelInstances.MaxBoneCount = BoneCount;
            }

            Counter = ArenaPush(*Arena, model_instance_counter);
            Counter->Model = Model;
            Counter->InstanceCount = 0;
            Add(&ModelInstanceTree, Counter);
        }

        int32x ControlCount = 0;
        {for(model_control_binding *Binding = &ModelControlsBegin(*Instance);
             Binding != &ModelControlsEnd(*Instance);
             Binding = &ModelControlsNext(*Binding))
        {
            ++ControlCount;
        }}
        if(ModelInstances.MaxInstanceControlCount < ControlCount)
        {
            ModelInstances.MaxInstanceControlCount = ControlCount;
        }

        ++Counter->InstanceCount;
        ++ModelInstances.TotalInstanceCount;
        ModelInstances.TotalInstancedBoneCount += BoneCount;
    }}

    {for(model_instance_counter *Counter = First(&ModelInstanceTree);
         Counter;
         Counter = Next(&ModelInstanceTree, Counter))
    {
        if(ModelInstances.MaxInstanceCount < Counter->InstanceCount)
        {
            ModelInstances.MaxInstanceCount = Counter->InstanceCount;
        }
    }}

    //
    // Animations
    //
    stat_hud_model_controls Controls = {0};
    if(GetGlobalControlsBegin() != GetGlobalControlsEnd())
    {
        Controls.MinClockTime = Real32Maximum;
        Controls.MaxClockTime = -Real32Maximum;
        Controls.MinCompletionTime = Real32Maximum;
        Controls.MaxCompletionTime = -Real32Maximum;
    }
    {for(control *Control = GetGlobalControlsBegin();
         Control != GetGlobalControlsEnd();
         Control = GetGlobalNextControl(Control))
    {
        ++Controls.TotalControlCount;
        if(ControlIsActive(*Control))
        {
            ++Controls.ActiveControlCount;
            if(ControlHasEffect(*Control))
            {
                ++Controls.ActiveAndWeightedControlCount;
                if(GetControlEffectiveWeight(*Control) == 0.0f)
                {
                    ++Controls.ActiveAndWeightedButEasedOutControlCount;
                }
            }
        }

        real32 ControlClock = GetControlClock(*Control);
        Controls.MinClockTime = Minimum(ControlClock, Controls.MinClockTime);
        Controls.MaxClockTime = Maximum(ControlClock, Controls.MaxClockTime);

        if(GetControlCompletionCheckFlag(*Control))
        {
            real32 CompletionTime = GetControlCompletionClock(*Control);
            Controls.MinCompletionTime =
                Minimum(CompletionTime, Controls.MinCompletionTime);
            Controls.MaxCompletionTime =
                Maximum(CompletionTime, Controls.MaxCompletionTime);

            ++Controls.CompletableControlCount;
        }

        if(ControlIsComplete(*Control))
        {
            ++Controls.CompletedControlCount;
        }

        if(ControlIsUnused(*Control))
        {
            ++Controls.UnusedControlCount;
        }
    }}

    animation_binding_cache_status AnimBindingCacheStatus;
    GetAnimationBindingCacheStatus(AnimBindingCacheStatus);

    stat_hud_animation_types AnimTypes = {0};
    {for(animation_binding *Binding = GetFirstAnimationBinding();
         Binding;
         Binding = GetNextAnimationBinding(Binding))
    {
        bool Rebased = (Binding->RebasingMemory != 0);

        {for(int32x TrackIndex = 0;
             TrackIndex < Binding->TrackBindingCount;
             ++TrackIndex)
        {
            bound_transform_track &Track = Binding->TrackBindings[TrackIndex];
            if(Track.SourceTrack)
            {
                transform_track const &SourceTrack = *Track.SourceTrack;

                ++AnimTypes.TrackTypes
                    [CurveKnotCharacter(SourceTrack.PositionCurve)]
                    [CurveKnotCharacter(SourceTrack.OrientationCurve)]
                    [CurveKnotCharacter(SourceTrack.ScaleShearCurve)]
                    [Rebased];
                ++AnimTypes.TotalTrackCount;
            }
        }}
    }}

    //
    // Memory footprint
    //
    stat_hud_footprint Footprint = {0};

    {for(allocation_header *Header = AllocationsBegin();
         Header != AllocationsEnd();
         Header = NextAllocation(Header))
    {
        allocation_information Info;
        GetAllocationInformation(Header, Info);

        alloc_sorter *Sorter = Find(&AllocCombinerTree,
                                    Info.SourceFileName,
                                    Info.SourceLineNumber);
        if(!Sorter)
        {
            Sorter = ArenaPush(*Arena, alloc_sorter);
            Sorter->Point.SourceFilename = (char *)Info.SourceFileName;
            Sorter->Point.SourceLineNumber = Info.SourceLineNumber;
            Sorter->Point.AllocationCount = 0;
            Sorter->Point.BytesRequested = 0;
            Sorter->Point.BytesAllocated = 0;

            Add(&AllocCombinerTree, Sorter);
        }

        ++Sorter->Point.AllocationCount;
        Sorter->Point.BytesRequested += Info.RequestedSize;
        Sorter->Point.BytesAllocated += Info.ActualSize;

        ++Footprint.TotalAllocationCount;
        Footprint.TotalBytesRequested += Info.RequestedSize;
        Footprint.TotalBytesAllocated += Info.ActualSize;
    }}

    int32x AllocPointCount = 0;
    {for(alloc_sorter *Sorter = First(&AllocCombinerTree);
         Sorter;
         )
    {
        alloc_sorter *NextSorter = Next(&AllocCombinerTree, Sorter);

        Remove(&AllocCombinerTree, Sorter);
        Add(&AllocSorterTree, Sorter);
        ++AllocPointCount;

        Sorter = NextSorter;
    }}

    //
    // Performance
    //
    stat_hud_perf Perf = {0};

    int32x PerfPointCount = 0;
    {for(int32x CounterIndex = 0;
         CounterIndex < GetCounterCount();
         ++CounterIndex)
    {
        counter_results Results;
        GetCounterResults(CounterIndex, Results);

        perf_sorter *Sorter = ArenaPush(*Arena, perf_sorter);
        Sorter->Point.Name = (char *)Results.Name;
        Sorter->Point.Count = Results.Count;
        Sorter->Point.TotalSeconds = (real32)Results.SecondsWithoutChildren;
        Sorter->Point.TotalCycles = (real32)Results.TotalCyclesWithoutChildren;
        Add(&PerfSorterTree, Sorter);

        Perf.TotalSeconds += Sorter->Point.TotalSeconds;
        Perf.TotalCycles += Sorter->Point.TotalCycles;

        ++PerfPointCount;
    }}

    //
    // Gathering
    //
    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    stat_hud *HUD;
    AggrAllocPtr(Allocator, HUD);
    AggrAllocOffsetArrayPtr(Allocator, HUD, AllocPointCount, AllocPointCount, AllocPoints);
    AggrAllocOffsetArrayPtr(Allocator, HUD, PerfPointCount, PerfPointCount, PerfPoints);
    if(EndAggrAlloc(Allocator))
    {
        HUD->FrameCount = FramesSinceLastCapture;
        HUD->ModelInstances = ModelInstances;
        HUD->Controls = Controls;
        HUD->AnimBindingCacheStatus = AnimBindingCacheStatus;
        HUD->AnimTypes = AnimTypes;
        HUD->Footprint = Footprint;
        HUD->Perf = Perf;

        stat_hud_alloc_point *AllocPoint = HUD->AllocPoints;
        int32x AllocPointCountCheck = 0;
        {for(alloc_sorter *Sorter = First(&AllocSorterTree);
             Sorter;
             Sorter = Next(&AllocSorterTree, Sorter),
                 ++AllocPointCountCheck)
        {
            *AllocPoint++ = Sorter->Point;
        }}
        Assert(AllocPointCount == AllocPointCountCheck);

        stat_hud_perf_point *PerfPoint = HUD->PerfPoints;
        int32x PerfPointCountCheck = 0;
        {for(perf_sorter *Sorter = First(&PerfSorterTree);
             Sorter;
             Sorter = Next(&PerfSorterTree, Sorter),
                 ++PerfPointCountCheck)
        {
            *PerfPoint++ = Sorter->Point;
        }}
        Assert(PerfPointCount == PerfPointCountCheck);
    }

    //
    // Cleanup
    //
    FreeMemoryArena(Arena);

    return(HUD);
}

void GRANNY
FreeStats(stat_hud *StatHUD)
{
    Deallocate(StatHUD);
}

char **GRANNY
DumpStatHUD(stat_hud &StatHUD)
{
    int32x LineCount = 256;
    int32x LineLength = 256;

    char **StatHUDDump;
    char *CharArray;

    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    AggrAllocArrayPtr(Allocator, LineCount, StatHUDDump);
    AggrAllocArrayPtr(Allocator, LineCount*LineLength, CharArray);
    if(EndAggrAlloc(Allocator))
    {
        {for(int32x LineIndex = 0;
             LineIndex < LineCount;
             ++LineIndex)
        {
            StatHUDDump[LineIndex] = CharArray;
            CharArray += LineLength;
        }}

        char **Line = StatHUDDump;

        //
        // Model Instances
        //
        stat_hud_model_instances &ModelInstances = StatHUD.ModelInstances;
        ConvertToStringVar(LineLength, *Line++,
                           "Model Instances:");
        ConvertToStringVar(LineLength, *Line++,
                           "  Total instances: %d   Bones: %d   Source models: %d",
                           ModelInstances.TotalInstanceCount,
                           ModelInstances.TotalInstancedBoneCount,
                           ModelInstances.TotalUsedModelCount);
        ConvertToStringVar(LineLength, *Line++,
                           "  Max instances/m: %d   Max bones/m: %d   Max controls/i: %d",
                           ModelInstances.MaxInstanceCount,
                           ModelInstances.MaxBoneCount,
                           ModelInstances.MaxInstanceControlCount);

        //
        // Animations
        //
        stat_hud_model_controls &Controls = StatHUD.Controls;
        ConvertToStringVar(LineLength, *Line++,
                           "Controls:");
        ConvertToStringVar(LineLength, *Line++,
                           "  Total controls: %d   Active: %d   A/Weighted: %d",
                           Controls.TotalControlCount,
                           Controls.ActiveControlCount,
                           Controls.ActiveAndWeightedControlCount);
        ConvertToStringVar(LineLength, *Line++,
                           "  Completable: %d   Completed: %d   Unused: %d",
                           Controls.CompletableControlCount,
                           Controls.CompletedControlCount,
                           Controls.UnusedControlCount);
        ConvertToStringVar(LineLength, *Line++,
                           "  Clock range: %fs - %fs",
                           Controls.MinClockTime,
                           Controls.MaxClockTime);
        if(Controls.MinCompletionTime == Controls.MaxCompletionTime)
        {
            ConvertToStringVar(LineLength, *Line++,
                               "  Completion range: (none)",
                               Controls.MinCompletionTime);
        }
        else if(Controls.MaxCompletionTime == Real32Maximum)
        {
            ConvertToStringVar(LineLength, *Line++,
                               "  Completion range: %fs - Infinity",
                               Controls.MinCompletionTime);
        }
        else
        {
            ConvertToStringVar(LineLength, *Line++,
                               "  Completion range: %fs - %fs",
                               Controls.MinCompletionTime,
                               Controls.MaxCompletionTime);
        }

        animation_binding_cache_status &ABCS = StatHUD.AnimBindingCacheStatus;
        ConvertToStringVar(LineLength, *Line++,
                           "Animation binding cache:");
        ConvertToStringVar(LineLength, *Line++,
                           "  Bindings created: %d   Destroyed: %d",
                           ABCS.TotalBindingsCreated,
                           ABCS.TotalBindingsDestroyed);
        ConvertToStringVar(LineLength, *Line++,
                           "  Acquire direct: %d   Indirect: %d   Released: %d",
                           ABCS.DirectAcquireCount,
                           ABCS.IndirectAcquireCount,
                           ABCS.ReleaseCount);
        ConvertToStringVar(LineLength, *Line++,
                           "  Current bindings: %d   Used: %d",
                           ABCS.CurrentTotalBindingCount,
                           ABCS.CurrentUsedBindingCount);
        ConvertToStringVar(LineLength, *Line++,
                           "  Cache hits: %d   Misses: %d",
                           ABCS.CacheHits, ABCS.CacheMisses);
        ConvertToStringVar(LineLength, *Line++,
                           "  Flushes: %d   Flush frees: %d   Overflows: %d",
                           ABCS.ExplicitFlushCount,
                           ABCS.ExplicitFlushFrees,
                           ABCS.OverflowFrees);

        stat_hud_animation_types &AnimTypes = StatHUD.AnimTypes;
        {for(int32x P = 0;
             P < 3;
             ++P)
        {
            {for(int32x R = 0;
                 R < 3;
                 ++R)
            {
                {for(int32x S = 0;
                     S < 3;
                     ++S)
                {
                    int32x Count = AnimTypes.TrackTypes[P][R][S][0];
                    int32x RebasedCount = AnimTypes.TrackTypes[P][R][S][1];
                    int32x TotalCount = Count + RebasedCount;
                    if(TotalCount)
                    {
                        char Letter[] = {'I', 'C', 'A'};
                        ConvertToStringVar(
                            LineLength, *Line++,
                            "  (%d%%) %c%c%c bindings: %4d + %4dr = %4d",
                            (100 * TotalCount) / AnimTypes.TotalTrackCount,
                            Letter[P],
                            Letter[R],
                            Letter[S],
                            Count,
                            RebasedCount,
                            TotalCount);
                    }
                }}
            }}
        }}

        //
        // Memory footprint
        //
        stat_hud_footprint Footprint = StatHUD.Footprint;
        ConvertToStringVar(LineLength, *Line++,
                           "Allocations: %d (%dk / %dk)",
                           Footprint.TotalAllocationCount,
                           Footprint.TotalBytesRequested / 1024,
                           Footprint.TotalBytesAllocated / 1024);

        {for(int32x Index = 0;
             Index < StatHUD.AllocPointCount;
             ++Index)
        {
            stat_hud_alloc_point &Point = StatHUD.AllocPoints[Index];

            if(Point.BytesAllocated)
            {
                ConvertToStringVar(LineLength, *Line++,
                                   "  %2d%% %s(%d): %d (%dk / %dk)",
                                   RoundReal32ToInt32(100.0f * ((real32)Point.BytesAllocated / (real32)Footprint.TotalBytesAllocated)),
                                   FindLastSlash(Point.SourceFilename),
                                   Point.SourceLineNumber,
                                   Point.AllocationCount,
                                   Point.BytesRequested / 1024,
                                   Point.BytesAllocated / 1024);
            }
        }}

        //
        // Perf
        //
        stat_hud_perf Perf = StatHUD.Perf;
        ConvertToStringVar(LineLength, *Line++,
                           "Clocks/frame: %fs (%.0f cycles)",
                           Perf.TotalSeconds / (real64x)StatHUD.FrameCount,
                           Perf.TotalCycles / (real64x)StatHUD.FrameCount);

        {for(int32x Index = 0;
             Index < StatHUD.PerfPointCount;
             ++Index)
        {
            stat_hud_perf_point &Point = StatHUD.PerfPoints[Index];

            if(Point.Count)
            {
                ConvertToStringVar(LineLength, *Line++,
                                   "  %2d%% %s: %dh/f %fs/f (%.0f cycles/hit)",
                                   RoundReal32ToInt32(100.0f * (real32)(Point.TotalSeconds / Perf.TotalSeconds)),
                                   Point.Name,
                                   Point.Count / StatHUD.FrameCount,
                                   Point.TotalSeconds / (real64x)StatHUD.FrameCount,
                                   Point.TotalCycles / (real64x)Point.Count);
            }
        }}

        *Line = 0;
    }

    return(StatHUDDump);
}

void GRANNY
FreeStatHUDDump(char **StatHUDDump)
{
    Deallocate(StatHUDDump);
}
