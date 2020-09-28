// ========================================================================
// $File: //jeffr/granny/rt/granny_track_group_builder.cpp $
// $DateTime: 2007/08/23 09:22:51 $
// $Change: 15808 $
// $Revision: #33 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TRACK_GROUP_BUILDER_H)
#include "granny_track_group_builder.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_ANIMATION_H)
#include "granny_animation.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_MEMORY_ARENA_H)
#include "granny_memory_arena.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
#endif

#if !defined(GRANNY_FLOATS_H)
#include "granny_floats.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode TrackGroupLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct vector_track_builder
{
    vector_track Data;

    vector_track_builder *Left;
    vector_track_builder *Right;
};

#define CONTAINER_NAME vector_track_tree
#define CONTAINER_ITEM_TYPE vector_track_builder
#define CONTAINER_ADD_FIELDS char const *Name
#define CONTAINER_ADD_ASSIGN(Item) (Item)->Data.Name = Name;
#define CONTAINER_COMPARE_ITEMS(Item1, Item2) StringDifferenceOrCallback((Item1)->Data.Name, (Item2)->Data.Name)
#define CONTAINER_FIND_FIELDS char const *Name
#define CONTAINER_COMPARE_FIND_FIELDS(Item) StringDifferenceOrCallback(Name, (Item)->Data.Name)
#define CONTAINER_SORTED 1
#define CONTAINER_KEEP_LINKED_LIST 0
#define CONTAINER_SUPPORT_DUPES 1
#include "granny_contain.inl"

struct transform_track_builder
{
    transform_track Data;

    transform_track_builder *Left;
    transform_track_builder *Right;
};

#define CONTAINER_NAME transform_track_tree
#define CONTAINER_ITEM_TYPE transform_track_builder
#define CONTAINER_ADD_FIELDS char const *Name
#define CONTAINER_ADD_ASSIGN(Item) (Item)->Data.Name = Name;
#define CONTAINER_COMPARE_ITEMS(Item1, Item2) StringDifferenceOrCallback((Item1)->Data.Name, (Item2)->Data.Name)
#define CONTAINER_FIND_FIELDS char const *Name
#define CONTAINER_COMPARE_FIND_FIELDS(Item) StringDifferenceOrCallback(Name, (Item)->Data.Name)
#define CONTAINER_SORTED 1
#define CONTAINER_KEEP_LINKED_LIST 0
#define CONTAINER_SUPPORT_DUPES 1
#include "granny_contain.inl"

struct text_track_entry_builder
{
    real32 TimeStamp;
    char const *Text;
    text_track_entry_builder *Next;
};

struct text_track_builder
{
    char const *Name;
    int32x EntryCount;
    text_track_entry_builder *FirstEntry;
    text_track_entry_builder *LastEntry;

    text_track_builder *Left;
    text_track_builder *Right;
};

#define CONTAINER_NAME text_track_tree
#define CONTAINER_ITEM_TYPE text_track_builder
#define CONTAINER_ADD_FIELDS char const *Name
#define CONTAINER_ADD_ASSIGN(Item) (Item)->Name = Name;
#define CONTAINER_COMPARE_ITEMS(Item1, Item2) StringDifference((Item1)->Name, (Item2)->Name)
#define CONTAINER_FIND_FIELDS char const *Name
#define CONTAINER_COMPARE_FIND_FIELDS(Item) StringDifference(Name, (Item)->Name)
#define CONTAINER_SORTED 1
#define CONTAINER_KEEP_LINKED_LIST 0
#define CONTAINER_SUPPORT_DUPES 1
#include "granny_contain.inl"

struct track_group_builder
{
    memory_arena *Arena;

    char const *Name;

    int32x VectorTrackCount;
    vector_track_tree VectorTrackTree;

    int32x TransformTrackCount;
    transform_track_tree TransformTrackTree;

    int32x TextTrackCount;
    text_track_tree TextTrackTree;

    int32x TextEntryCount;
    //int32x KnotReal32Count;
    //int32x ControlReal32Count;
    int32x TotalCurveDataSize;

    bool IncludeLODErrorSpace;

    transform_track *LastTransformTrack;
    text_track_builder *LastTextTrack;
};

END_GRANNY_NAMESPACE;

track_group_builder *GRANNY
BeginTrackGroup(char const *Name,
                int32x VectorTrackCount,
                int32x TransformTrackCount,
                int32x TextTrackCount,
                bool IncludeLODErrorSpace)
{
    track_group_builder *Builder = Allocate(track_group_builder);
    if(Builder)
    {
        Builder->Arena = NewMemoryArena();
        Builder->Name = Name;

        Builder->VectorTrackCount = 0;
        Initialize(&Builder->VectorTrackTree, 0);

        Builder->TransformTrackCount = 0;
        Initialize(&Builder->TransformTrackTree, 0);

        Builder->TextTrackCount = 0;
        Initialize(&Builder->TextTrackTree, 0);

        Builder->TextEntryCount = 0;
        //Builder->KnotReal32Count = 0;
        //Builder->ControlReal32Count = 0;
        Builder->TotalCurveDataSize = 0;

        Builder->LastTransformTrack = 0;
        Builder->LastTextTrack = 0;

        Builder->IncludeLODErrorSpace = IncludeLODErrorSpace;
    }

    return(Builder);
}

track_group *GRANNY
EndTrackGroup(track_group_builder *Builder)
{
    track_group *TrackGroup = 0;

    if(Builder)
    {
        int32x TotalSize = GetResultingTrackGroupSize(*Builder);
        TrackGroup = EndTrackGroupInPlace(Builder, AllocateSize(TotalSize));
    }

    return(TrackGroup);
}

// NB!  This function must be kept synchronized with EndTrackGroupToArena for the exporter
// to function /at all/!
static void
AggrTrackGroup(aggr_allocator &Allocator,
               track_group_builder const &Builder, track_group *&TrackGroup,
               text_track_entry *&TextEntries,
               uint8 *&CurveDataMemory)
{
    int32x NumLODErrors = 0;
    if ( Builder.IncludeLODErrorSpace )
    {
        // Always either no LOD error data, or there's one per transform track.
        NumLODErrors = Builder.TransformTrackCount;
    }

    AggrAllocPtr(Allocator, TrackGroup);
    AggrAllocOffsetArrayPtr(Allocator, TrackGroup, Builder.VectorTrackCount,
                            VectorTrackCount, VectorTracks);
    // NOTE! To make sure the error-checking in FreeLODErrorSpace() works,
    // TransformLODErrors needs to be allocated before TransformTracks.
    AggrAllocOffsetArrayPtr(Allocator, TrackGroup, NumLODErrors,
                            TransformLODErrorCount, TransformLODErrors);
    AggrAllocOffsetArrayPtr(Allocator, TrackGroup, Builder.TransformTrackCount,
                            TransformTrackCount, TransformTracks);

    AggrAllocOffsetArrayPtr(Allocator, TrackGroup, Builder.TextTrackCount,
                            TextTrackCount, TextTracks);

    AggrAllocArrayPtr(Allocator, Builder.TextEntryCount, TextEntries);

    AggrAllocArrayPtr(Allocator, Builder.TotalCurveDataSize, CurveDataMemory);
}

int32x GRANNY
GetResultingTrackGroupSize(track_group_builder const &Builder)
{
    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    track_group *TrackGroup;
    text_track_entry *TextEntries;
    //real32 *Knots;
    //real32 *Controls;
    uint8 *CurveDataMemory;
    //AggrTrackGroup(Builder, TrackGroup, TextEntries, Knots, Controls);
    AggrTrackGroup(Allocator, Builder, TrackGroup, TextEntries, CurveDataMemory);

    int32x ResultingSize;
    CheckConvertToInt32(ResultingSize, EndAggrSize(Allocator), return 0);
    return ResultingSize;
}


static uint8 *
StoreCurve(curve2 const &Source, curve2 *Dest, uint8 *CurveDataMemory)
{
    if (CurveIsIdentity(Source))
    {
        Dest->CurveData.Type = Source.CurveData.Type;
        Dest->CurveData.Object = Source.CurveData.Object;
        return CurveDataMemory;
    }
    else
    {
        curve_builder Builder;
        BeginCurveCopyInPlace ( &Builder, Source );
        uint8 *NextMemory = CurveDataMemory + GetResultingCurveDataSize ( &Builder );
        curve2 *ResultCurve = EndCurveDataInPlace ( &Builder, Dest, CurveDataMemory );
        Assert ( ResultCurve == Dest );
        CurveParanoiaChecking ( *ResultCurve );
        return NextMemory;
    }
}

// NB!  This function must be kept synchronized with EndTrackGroupToArena for the exporter
// to function /at all/!
track_group *GRANNY
EndTrackGroupInPlace(track_group_builder *Builder, void *Memory)
{
    track_group *TrackGroup = 0;

    if(Builder)
    {
        aggr_allocator Allocator;
        InitializeAggrAlloc(Allocator);

        text_track_entry *TextEntries;
        //real32 *Knots;
        //real32 *Controls;
        uint8 *FirstCurveData;
        //AggrTrackGroup(*Builder, TrackGroup, TextEntries, Knots, Controls);
        AggrTrackGroup(Allocator, *Builder, TrackGroup, TextEntries, FirstCurveData);
        if(EndAggrPlacement(Allocator, Memory))
        {
            TrackGroup->Name = Builder->Name;

            //real32 *CurrentKnot = Knots;
            //real32 *CurrentControl = Controls;
            uint8 *CurrentCurveData = FirstCurveData;
            text_track_entry *CurrentEntry = TextEntries;

            int32x TrackIndex = 0;
            {for(vector_track_builder *SourceBuilder =
                     First(&Builder->VectorTrackTree);
                 SourceBuilder;
                 SourceBuilder = Next(&Builder->VectorTrackTree, SourceBuilder),
                     ++TrackIndex)
            {
                vector_track &Source = SourceBuilder->Data;
                vector_track &Dest = TrackGroup->VectorTracks[TrackIndex];

                Dest.Name = Source.Name;
                Dest.TrackKey = Source.TrackKey;
                Dest.Dimension = Source.Dimension;
                CurrentCurveData = StoreCurve ( Source.ValueCurve, &(Dest.ValueCurve), CurrentCurveData );
                //StoreCurve(Source.ValueCurve, Dest.ValueCurve,
                //           CurrentKnot, CurrentControl);
            }}

            TrackIndex = 0;
            {for(transform_track_builder *SourceBuilder =
                     First(&Builder->TransformTrackTree);
                 SourceBuilder;
                 SourceBuilder = Next(&Builder->TransformTrackTree, SourceBuilder),
                     ++TrackIndex)
            {
                transform_track &Source = SourceBuilder->Data;
                transform_track &Dest =
                    TrackGroup->TransformTracks[TrackIndex];

                Dest.Name  = Source.Name;
                Dest.Flags = Source.Flags;

                CurveParanoiaChecking ( Source.PositionCurve );
                CurveParanoiaChecking ( Source.OrientationCurve );
                CurveParanoiaChecking ( Source.ScaleShearCurve );

                CurrentCurveData = StoreCurve ( Source.PositionCurve,    &(Dest.PositionCurve),    CurrentCurveData );
                CurrentCurveData = StoreCurve ( Source.OrientationCurve, &(Dest.OrientationCurve), CurrentCurveData );
                CurrentCurveData = StoreCurve ( Source.ScaleShearCurve,  &(Dest.ScaleShearCurve),  CurrentCurveData );

                CurveParanoiaChecking ( Dest.PositionCurve );
                CurveParanoiaChecking ( Dest.OrientationCurve );
                CurveParanoiaChecking ( Dest.ScaleShearCurve );

                //StoreCurve(Source.PositionCurve, Dest.PositionCurve,
                //           CurrentKnot, CurrentControl);
                //StoreCurve(Source.OrientationCurve, Dest.OrientationCurve,
                //           CurrentKnot, CurrentControl);
                //StoreCurve(Source.ScaleShearCurve, Dest.ScaleShearCurve,
                //           CurrentKnot, CurrentControl);
            }}

            TrackIndex = 0;
            {for(text_track_builder *SourceBuilder =
                     First(&Builder->TextTrackTree);
                 SourceBuilder;
                 SourceBuilder = Next(&Builder->TextTrackTree, SourceBuilder),
                     ++TrackIndex)
            {
                text_track_builder &SourceTrack = *SourceBuilder;
                text_track &DestTrack = TrackGroup->TextTracks[TrackIndex];

                DestTrack.Name = SourceTrack.Name;
                DestTrack.EntryCount = SourceTrack.EntryCount;
                DestTrack.Entries = CurrentEntry;

                {for(text_track_entry_builder *SourceEntry =
                         SourceTrack.FirstEntry;
                     SourceEntry;
                     SourceEntry = SourceEntry->Next)
                {
                    CurrentEntry->TimeStamp = SourceEntry->TimeStamp;
                    CurrentEntry->Text = SourceEntry->Text;
                    ++CurrentEntry;
                }}
            }}

            {for ( int32x LODErrorNum = 0; LODErrorNum < TrackGroup->TransformLODErrorCount; LODErrorNum++ )
            {
                // This default means that no LOD will happen.
                TrackGroup->TransformLODErrors[LODErrorNum] = GetReal32AlmostInfinity();
            }}

            TrackGroup->Flags = TrackGroupIsSorted;
            TrackGroup->PeriodicLoop = 0;
            TrackGroup->RootMotion = 0;
            TrackGroup->ExtendedData.Object =
                TrackGroup->ExtendedData.Type = 0;

            //Assert((CurrentKnot - Knots) == Builder->KnotReal32Count);
            //Assert((CurrentControl - Controls) == Builder->ControlReal32Count);
            Assert ( ( CurrentCurveData - FirstCurveData ) == Builder->TotalCurveDataSize );

            Assert((CurrentEntry - TextEntries) == Builder->TextEntryCount);
        }

        FreeMemoryArena(Builder->Arena);
        FreeMemory(&Builder->VectorTrackTree);
        FreeMemory(&Builder->TransformTrackTree);
        FreeMemory(&Builder->TextTrackTree);
        Deallocate(Builder);
    }

    return(TrackGroup);
}


track_group *GRANNY
EndTrackGroupToArena(track_group_builder *Builder,
                     memory_arena& Arena)
{
    track_group *TrackGroup = 0;
    bool AllSuccessful = true;

    if(Builder)
    {
        TrackGroup = ArenaPush(Arena, track_group);
        ZeroStructure(*TrackGroup);

        if(TrackGroup)
        {
            TrackGroup->Name = Builder->Name;

            TrackGroup->VectorTrackCount = Builder->VectorTrackCount;
            if (TrackGroup->VectorTrackCount != 0)
            {
                TrackGroup->VectorTracks = ArenaPushArray(Arena,
                                                          Builder->VectorTrackCount,
                                                          vector_track);
                if (TrackGroup->VectorTracks)
                {
                    int32x TrackIndex = 0;
                    {for(vector_track_builder *SourceBuilder = First(&Builder->VectorTrackTree);
                         SourceBuilder;
                         SourceBuilder = Next(&Builder->VectorTrackTree, SourceBuilder),
                             ++TrackIndex)
                    {
                        vector_track &Source = SourceBuilder->Data;
                        vector_track &Dest = TrackGroup->VectorTracks[TrackIndex];

                        int32x CurveSize = CurveGetSize(Source.ValueCurve);
                        uint8* CurveMemory = ArenaPushArray(Arena, CurveSize, uint8);
                        if (!CurveMemory)
                        {
                            AllSuccessful = false;
                            break;
                        }

                        Dest.Name = Source.Name;
                        Dest.TrackKey = Source.TrackKey;
                        Dest.Dimension = Source.Dimension;
                        uint8* NextMemory = StoreCurve ( Source.ValueCurve, &(Dest.ValueCurve), CurveMemory );
                        Assert(NextMemory == CurveMemory + CurveSize);
                    }}
                }
                else
                {
                    AllSuccessful = false;
                }
            }

            TrackGroup->TransformTrackCount = Builder->TransformTrackCount;
            if (TrackGroup->TransformTrackCount != 0)
            {
                TrackGroup->TransformTracks = ArenaPushArray(Arena,
                                                             Builder->TransformTrackCount,
                                                             transform_track);
                if (TrackGroup->TransformTracks)
                {
                    int32x TrackIndex = 0;
                    {for(transform_track_builder *SourceBuilder = First(&Builder->TransformTrackTree);
                         SourceBuilder;
                         SourceBuilder = Next(&Builder->TransformTrackTree, SourceBuilder),
                             ++TrackIndex)
                    {
                        transform_track &Source = SourceBuilder->Data;
                        transform_track &Dest = TrackGroup->TransformTracks[TrackIndex];

                        Dest.Name = Source.Name;
                        Dest.Flags = Source.Flags;

                        CurveParanoiaChecking ( Source.PositionCurve );
                        CurveParanoiaChecking ( Source.OrientationCurve );
                        CurveParanoiaChecking ( Source.ScaleShearCurve );

                        int32x TotalCurveSize = (CurveGetSize(Source.PositionCurve) +
                                                 CurveGetSize(Source.OrientationCurve) +
                                                 CurveGetSize(Source.ScaleShearCurve));
                        uint8* CurveMemory = ArenaPushArray(Arena, TotalCurveSize, uint8);
                        if (!CurveMemory)
                        {
                            AllSuccessful = false;
                            break;
                        }

                        uint8* CurrentMemory = CurveMemory;
                        CurrentMemory = StoreCurve ( Source.PositionCurve,    &(Dest.PositionCurve),    CurrentMemory );
                        CurrentMemory = StoreCurve ( Source.OrientationCurve, &(Dest.OrientationCurve), CurrentMemory );
                        CurrentMemory = StoreCurve ( Source.ScaleShearCurve,  &(Dest.ScaleShearCurve),  CurrentMemory );

                        CurveParanoiaChecking ( Dest.PositionCurve );
                        CurveParanoiaChecking ( Dest.OrientationCurve );
                        CurveParanoiaChecking ( Dest.ScaleShearCurve );
                    }}
                }
                else
                {
                    AllSuccessful = false;
                }
            }

            TrackGroup->TextTrackCount = Builder->TextTrackCount;
            if (TrackGroup->TextTrackCount != 0)
            {
                TrackGroup->TextTracks = ArenaPushArray(Arena,
                                                        Builder->TextTrackCount,
                                                        text_track);
                if (TrackGroup->TextTracks)
                {
                    int32x TrackIndex = 0;
                    {for(text_track_builder *SourceBuilder = First(&Builder->TextTrackTree);
                         SourceBuilder;
                         SourceBuilder = Next(&Builder->TextTrackTree, SourceBuilder),
                             ++TrackIndex)
                    {
                        text_track_builder &SourceTrack = *SourceBuilder;
                        text_track &DestTrack = TrackGroup->TextTracks[TrackIndex];

                        DestTrack.Name = SourceTrack.Name;
                        DestTrack.EntryCount = SourceTrack.EntryCount;
                        DestTrack.Entries = ArenaPushArray(Arena,
                                                           SourceTrack.EntryCount,
                                                           text_track_entry);;
                        if (!DestTrack.Entries)
                        {
                            AllSuccessful = false;
                            break;
                        }

                        text_track_entry* CurrentEntry = DestTrack.Entries;
                        {for(text_track_entry_builder *SourceEntry =
                                 SourceTrack.FirstEntry;
                             SourceEntry;
                             SourceEntry = SourceEntry->Next)
                        {
                            CurrentEntry->TimeStamp = SourceEntry->TimeStamp;
                            CurrentEntry->Text = SourceEntry->Text;
                            ++CurrentEntry;
                        }}
                    }}
                }
                else
                {
                    AllSuccessful = false;
                }
            }

            if (Builder->IncludeLODErrorSpace)
            {
                TrackGroup->TransformLODErrorCount = Builder->TransformTrackCount;
                if (TrackGroup->TransformLODErrorCount)
                {
                    TrackGroup->TransformLODErrors = ArenaPushArray(Arena,
                                                                    TrackGroup->TransformLODErrorCount,
                                                                    real32);
                    if (TrackGroup->TransformLODErrors)
                    {
                        {for ( int32x LODErrorNum = 0; LODErrorNum < TrackGroup->TransformLODErrorCount; LODErrorNum++ )
                        {
                            // This default means that no LOD will happen.
                            TrackGroup->TransformLODErrors[LODErrorNum] = GetReal32AlmostInfinity();
                        }}
                    }
                    else
                    {
                        AllSuccessful = false;
                    }
                }
            }

            TrackGroup->Flags = TrackGroupIsSorted;
            TrackGroup->PeriodicLoop = 0;
            TrackGroup->RootMotion = 0;
            TrackGroup->ExtendedData.Object = TrackGroup->ExtendedData.Type = 0;
        }

        FreeMemoryArena(Builder->Arena);
        FreeMemory(&Builder->VectorTrackTree);
        FreeMemory(&Builder->TransformTrackTree);
        FreeMemory(&Builder->TextTrackTree);
        Deallocate(Builder);
    }

    return AllSuccessful ? TrackGroup : NULL;
}


static curve2
PushCurve(track_group_builder &Builder, curve2 const *SourceCurve )
{
    // Make sure this doesn't cause threading problems...
    Assert(CurveFindFormatFromDefinition(*CurveDataDaIdentityType) == 2);
    CompileAssert(MaximumBSplineDimension == 16);
    static curve_data_da_identity IdCurve[MaximumBSplineDimension + 1] = {
        { { 2, 0 },  0 + 0 }, { { 2, 0 },  0 + 1 }, { { 2, 0 },  0 + 2 }, { { 2, 0 },  0 + 3 },
        { { 2, 0 },  4 + 0 }, { { 2, 0 },  4 + 1 }, { { 2, 0 },  4 + 2 }, { { 2, 0 },  4 + 3 },
        { { 2, 0 },  8 + 0 }, { { 2, 0 },  8 + 1 }, { { 2, 0 },  8 + 2 }, { { 2, 0 },  8 + 3 },
        { { 2, 0 }, 12 + 0 }, { { 2, 0 }, 12 + 1 }, { { 2, 0 }, 12 + 2 }, { { 2, 0 }, 12 + 3 },
        { { 2, 0 }, 16 + 0 }
    };

    curve2 TheCurve;
    CurveParanoiaChecking ( *SourceCurve );
    if (CurveIsIdentity(*SourceCurve))
    {
        // Trick.  If the curve is identity, set it to a known instance of the identity
        // curve.  There's no need to have multiple copies of this object, and it can save
        // a surprising amount of space in the final file.  Note that there's no need to
        // get the dimension field right in the identity curve, since CurveGetDimension
        // will always return 0 for identity curves.
        int32x Dim = CurveGetDimensionUnchecked(*SourceCurve);
        Assert(Dim >= 0 && Dim <= MaximumBSplineDimension);

        TheCurve.CurveData.Type = CurveDataDaIdentityType;
        TheCurve.CurveData.Object = &IdCurve[Dim];
        return TheCurve;
    }
    else
    {
        // Otherwise, take a copy of this curve. All the other calls copy the data here
        // and now, in case the caller changes the data, so might as well join in the fun.
        curve_builder *CurveBuilder = BeginCurveCopy ( *SourceCurve );
        if ( CurveBuilder != NULL )
        {
            int32x CurveDataSize = GetResultingCurveDataSize ( CurveBuilder );
            void *CurveDataMemory = ArenaPushSize ( *Builder.Arena, CurveDataSize );
            if ( CurveDataMemory != NULL )
            {
                curve2 *ResultCurve = EndCurveDataInPlace ( CurveBuilder, &TheCurve, CurveDataMemory );
                if ( ResultCurve != NULL )
                {
                    Assert ( ResultCurve == &TheCurve );
                    Builder.TotalCurveDataSize += CurveDataSize;
                    CurveParanoiaChecking ( TheCurve );
                    return TheCurve;
                }
            }
        }
    }

    // Failed. Try to cope.
    Assert ( !"PushCurve failed horribly" );
    TheCurve.CurveData.Type = NULL;
    TheCurve.CurveData.Object = NULL;
    return TheCurve;
}


void GRANNY
PushVectorTrackCurve(track_group_builder &Builder,
                     char const *Name,
                     uint32 TrackKey,
                     curve2 const *SourceCurve)
{
    vector_track_builder *TrackBuilder =
        Add(&Builder.VectorTrackTree, Name);
    if(TrackBuilder)
    {
        vector_track &Track = TrackBuilder->Data;
        ZeroStructure(Track);

        Track.Name = Name;
        Track.TrackKey = TrackKey;
        Track.Dimension = CurveGetDimension ( *SourceCurve );
        Track.ValueCurve = PushCurve(Builder, SourceCurve);

        ++Builder.VectorTrackCount;
    }
}


void GRANNY
BeginTransformTrack(track_group_builder &Builder,
                    char const *Name,
                    int32x Flags)
{
    transform_track_builder *TrackBuilder =
        Add(&Builder.TransformTrackTree, Name);
    if(TrackBuilder)
    {
        transform_track &Track = TrackBuilder->Data;
        ZeroStructure(Track);

        Track.Name = Name;
        Track.Flags = Flags;
        Builder.LastTransformTrack = &Track;
    }
}

void GRANNY
SetTransformTrackPositionCurve(track_group_builder &Builder, curve2 const *SourceCurve)
{
    if(Builder.LastTransformTrack)
    {
        transform_track &Track = *Builder.LastTransformTrack;
        Track.PositionCurve = PushCurve(Builder, SourceCurve);
    }
}


void GRANNY
SetTransformTrackOrientationCurve(track_group_builder &Builder, curve2 const *SourceCurve)
{
    if(Builder.LastTransformTrack)
    {
        transform_track &Track = *Builder.LastTransformTrack;
        Track.OrientationCurve = PushCurve(Builder, SourceCurve);
    }
}

void GRANNY
SetTransformTrackScaleShearCurve(track_group_builder &Builder, curve2 const *SourceCurve)
{
    if(Builder.LastTransformTrack)
    {
        transform_track &Track = *Builder.LastTransformTrack;
        Track.ScaleShearCurve = PushCurve(Builder, SourceCurve);
    }
}

void GRANNY
EndTransformTrack(track_group_builder &Builder)
{
    if(Builder.LastTransformTrack)
    {
        Builder.LastTransformTrack = 0;
        ++Builder.TransformTrackCount;
    }
}

void GRANNY
BeginTextTrack(track_group_builder &Builder, char const *Name)
{
    text_track_builder *TrackBuilder =
        Add(&Builder.TextTrackTree, Name);
    if(TrackBuilder)
    {
        TrackBuilder->Name = Name;
        TrackBuilder->EntryCount = 0;
        TrackBuilder->FirstEntry = TrackBuilder->LastEntry = 0;
        Builder.LastTextTrack = TrackBuilder;
    }
}

void GRANNY
AddTextEntry(track_group_builder &Builder,
             real32 TimeStamp, char const *Text)
{
    if(Builder.LastTextTrack)
    {
        text_track_builder &Track = *Builder.LastTextTrack;

        text_track_entry_builder *Entry = ArenaPush(*Builder.Arena,
                                                    text_track_entry_builder);
        Entry->TimeStamp = TimeStamp;
        Entry->Text = Text;
        Entry->Next = 0;

        if(Track.FirstEntry)
        {
            Track.LastEntry = Track.LastEntry->Next = Entry;
        }
        else
        {
            Track.FirstEntry = Track.LastEntry = Entry;
        }

        ++Track.EntryCount;
        ++Builder.TextEntryCount;
    }
}

void GRANNY
EndTextTrack(track_group_builder &Builder)
{
    if(Builder.LastTextTrack)
    {
        Builder.LastTextTrack = 0;
        ++Builder.TextTrackCount;
    }
}

void GRANNY
ResortTrackGroup(track_group &Group)
{
    transform_track_tree TransformTrackTree;
    Initialize(&TransformTrackTree, 0);

    {for(int32x TrackIndex = 0;
         TrackIndex < Group.TransformTrackCount;
         ++TrackIndex)
    {
        transform_track &Track = Group.TransformTracks[TrackIndex];
        transform_track_builder *TrackBuilder =
            Add(&TransformTrackTree, Track.Name);
        if(TrackBuilder)
        {
            TrackBuilder->Data = Track;
        }
    }}

    int32x TrackIndex = 0;
    {for(transform_track_builder *SourceBuilder =
             First(&TransformTrackTree);
         SourceBuilder;
         SourceBuilder = Next(&TransformTrackTree, SourceBuilder),
             ++TrackIndex)
    {
        Group.TransformTracks[TrackIndex] = SourceBuilder->Data;
    }}

    if ( StringComparisonCallback )
    {
        // It's far too easy to pick a string remapping that gets duplicate values!
        // Even this checking doesn't guarantee that you don't have duplicates
        // across your entire world, but hey - better than nothing.
        {for(int32x TrackIndex = 1;
            TrackIndex < Group.TransformTrackCount;
            ++TrackIndex)
        {
            transform_track &Track1 = Group.TransformTracks[TrackIndex];
            transform_track &Track2 = Group.TransformTracks[TrackIndex-1];
            if ( Track1.Name == Track2.Name )
            {
                Log1(WarningLogMessage, NameMapLogMessage,
                     "ResortTrackGroup: found duplicate remapped track value - 0x%08x",
                     (int32)(intaddrx)Track2.Name);
            }
        }}
    }

    Group.Flags |= TrackGroupIsSorted;

    FreeMemory(&TransformTrackTree);

    // TODO: This won't reorder the associated TransformLODErrors array!
    // Need to store some indices as well or something.
    Assert ( Group.TransformLODErrorCount == 0 );
}


void GRANNY
AllocateLODErrorSpace(track_group &Group)
{
    if ( Group.TransformLODErrorCount == 0 )
    {
        Group.TransformLODErrorCount = Group.TransformTrackCount;
        Group.TransformLODErrors = AllocateArray ( Group.TransformLODErrorCount, real32 );
    }
}

void GRANNY
FreeLODErrorSpace(track_group &Group)
{
    if ( Group.TransformLODErrorCount > 0 )
    {
        Assert ( Group.TransformLODErrorCount == Group.TransformTrackCount );
        // Make sure the chunk of memory was actually allocated
        // separately by AddLODErrorSpace(), and not in a single chunk
        // by AggrTrackGroup (or loading a file).
        Assert ( ( (uintaddrx)Group.TransformLODErrors < (uintaddrx)&Group ) ||
                 ( (uintaddrx)Group.TransformLODErrors > (uintaddrx)Group.TransformTracks ) );
        DeallocateSafe ( Group.TransformLODErrors );
        Group.TransformLODErrorCount = 0;
    }
}

void GRANNY
SetAllLODErrorSpace (track_group &Group, real32 Value)
{
    {for ( int32x LODNum = 0; LODNum < Group.TransformLODErrorCount; LODNum++ )
    {
        Group.TransformLODErrors[LODNum] = Value;
    }}
}

void GRANNY
ResetLODErrorSpace (track_group &Group)
{
    SetAllLODErrorSpace ( Group, GetReal32AlmostInfinity() );
}

