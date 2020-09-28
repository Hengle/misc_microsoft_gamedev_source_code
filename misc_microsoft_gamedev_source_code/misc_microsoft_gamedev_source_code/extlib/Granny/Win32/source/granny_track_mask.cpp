// ========================================================================
// $File: //jeffr/granny/rt/granny_track_mask.cpp $
// $DateTime: 2007/07/23 15:42:28 $
// $Change: 15572 $
// $Revision: #14 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TRACK_MASK_H)
#include "granny_track_mask.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_TRACK_GROUP_H)
#include "granny_track_group.h"
#endif

#if !defined(GRANNY_MODEL_H)
#include "granny_model.h"
#endif

#if !defined(GRANNY_SKELETON_H)
#include "granny_skeleton.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_DATA_TYPE_CONVERSION_H)
#include "granny_data_type_conversion.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode TrackMaskLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition TrackMaskType[] =
{
    { Real32Member, "DefaultWeight" },
    { ReferenceToArrayMember, "BoneWeights", Real32Type },
    { EndMember }
};


data_type_definition UnboundWeightType[] =
{
    { StringMember, "Name" },
    { Real32Member, "Weight" },
    { EndMember }
};

data_type_definition UnboundTrackMaskType[] =
{
    { Real32Member, "DefaultWeight" },
    { ReferenceToArrayMember, "Weights", UnboundWeightType },
    { EndMember }
};



track_mask IdentityTrackMask = { 1.0f, 0, NULL };
track_mask NullTrackMask     = { 0.0f, 0, NULL };
CompileAssert(SizeOf(track_mask) == (SizeOf(real32) +
                                     SizeOf(int32x) +
                                     SizeOf(real32*)));

END_GRANNY_NAMESPACE;

track_mask *GRANNY
NewTrackMask(real32 DefaultWeight, int32x BoneCount)
{
    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    track_mask *TrackMask;
    AggrAllocPtr(Allocator, TrackMask);
    AggrAllocOffsetArrayPtr(Allocator, TrackMask, BoneCount,
                            BoneCount, BoneWeights);
    if(EndAggrAlloc(Allocator))
    {
        TrackMask->DefaultWeight = DefaultWeight;

        {for(int32x BoneIndex = 0;
             BoneIndex < BoneCount;
             ++BoneIndex)
        {
            TrackMask->BoneWeights[BoneIndex] = TrackMask->DefaultWeight;
        }}
    }

    return(TrackMask);
}

real32 GRANNY
GetTrackMaskBoneWeight(track_mask const &Mask, int32x BoneIndex)
{
    return((BoneIndex < Mask.BoneCount) ?
           Mask.BoneWeights[BoneIndex] :
           Mask.DefaultWeight);
}

void GRANNY
SetTrackMaskBoneWeight(track_mask &Mask, int32x BoneIndex, real32 Weight)
{
    if(BoneIndex < Mask.BoneCount)
    {
        Mask.BoneWeights[BoneIndex] = Weight;
    }
    else
    {
        Log2(ErrorLogMessage, TrackMaskLogMessage,
             "BoneIndex %d is out-of-bounds "
             "(only %d bones in this track mask)",
             BoneIndex, Mask.BoneCount);
    }
}

void GRANNY
FreeTrackMask(track_mask *Mask)
{
    Deallocate(Mask);
}

track_mask *GRANNY
CopyTrackMask(track_mask const &Mask)
{
    track_mask *MaskCopy = NewTrackMask(Mask.DefaultWeight, Mask.BoneCount);
    if(MaskCopy)
    {
        Copy32(Mask.BoneCount, Mask.BoneWeights, MaskCopy->BoneWeights);
    }
    else
    {
        Log0(ErrorLogMessage, TrackMaskLogMessage,
             "Unable to create new track mask for copying");
    }

    return (MaskCopy);
}

void GRANNY
InvertTrackMask(track_mask &Mask)
{
    {for(int32x BoneIndex = 0;
         BoneIndex < Mask.BoneCount;
         ++BoneIndex)
    {
        real32 &Weight = Mask.BoneWeights[BoneIndex];
        Weight = 1.0f - Weight;
    }}
}

void GRANNY
SetSkeletonTrackMaskFromTrackGroup(track_mask &Mask,
                                   skeleton const &Skeleton,
                                   track_group const &TrackGroup,
                                   real32 IdentityValue,
                                   real32 ConstantValue,
                                   real32 AnimatedValue)
{
    {for(int32x TrackIndex = 0;
         TrackIndex < TrackGroup.TransformTrackCount;
         ++TrackIndex)
    {
        transform_track &Track = TrackGroup.TransformTracks[TrackIndex];
        int32x BoneIndex;
        if(FindBoneByName(&Skeleton, Track.Name, BoneIndex))
        {
            if(TransformTrackIsAnimated(Track))
            {
                SetTrackMaskBoneWeight(Mask, BoneIndex, AnimatedValue);
            }
            else if(TransformTrackIsIdentity(Track))
            {
                SetTrackMaskBoneWeight(Mask, BoneIndex, IdentityValue);
            }
            else
            {
                SetTrackMaskBoneWeight(Mask, BoneIndex, ConstantValue);
            }
        }
    }}
}

void GRANNY
SetSkeletonTrackMaskChainUpwards(track_mask &Mask,
                                 skeleton const &Skeleton,
                                 int32x ChainLeafBoneIndex,
                                 real32 Weight)
{
    while(ChainLeafBoneIndex != NoParentBone)
    {
        SetTrackMaskBoneWeight(Mask, ChainLeafBoneIndex, Weight);
        ChainLeafBoneIndex = Skeleton.Bones[ChainLeafBoneIndex].ParentIndex;
    }
}

void GRANNY
SetSkeletonTrackMaskChainDownwards(track_mask &Mask,
                                   skeleton const &Skeleton,
                                   int32x ChainRootBoneIndex,
                                   real32 Weight)
{
    real32 KeyWeight = -12345.0f;

    SetTrackMaskBoneWeight(Mask, ChainRootBoneIndex, KeyWeight);
    {for(int32x Index = ChainRootBoneIndex + 1;
         Index < Skeleton.BoneCount;
         ++Index)
    {
        int32x ParentIndex = Skeleton.Bones[Index].ParentIndex;
        if((ParentIndex != NoParentBone) &&
           (GetTrackMaskBoneWeight(Mask, ParentIndex) == KeyWeight))
        {
            SetTrackMaskBoneWeight(Mask, Index, KeyWeight);
        }
    }}

    SetTrackMaskBoneWeight(Mask, ChainRootBoneIndex, Weight);
    {for(int32x Index = ChainRootBoneIndex + 1;
         Index < Skeleton.BoneCount;
         ++Index)
    {
        if(GetTrackMaskBoneWeight(Mask, Index) == KeyWeight)
        {
            SetTrackMaskBoneWeight(Mask, Index, Weight);
        }
    }}
}




struct granny_bone_track_mask_data
{
    real32 BoneWeight;
};



extract_track_mask_result GRANNY
ExtractTrackMask(track_mask *TrackMask,
                 int32x BoneCount,
                 skeleton const &Skeleton,
                 char *MaskName,
                 real32 DefaultWeight,
                 bool UseParentForDefault)
{
    // Can't ask for more bones than the skeleton has.
    CheckBoundedInt32 ( 0, BoneCount, Skeleton.BoneCount, return ExtractTrackMaskResult_NoDataPresent );
    // Can't ask for more bones than the track_mask has.
    CheckBoundedInt32 ( 0, BoneCount, TrackMask->BoneCount, return ExtractTrackMaskResult_NoDataPresent );

    TrackMask->DefaultWeight = DefaultWeight;

    const data_type_definition GrannyBoneTrackMaskDataType[] =
        {
            {Real32Member, MaskName},
            {EndMember}
        };


    bool FoundNoData = true;
    bool FoundAllData = true;
    {for ( int32x BoneNum = 0; BoneNum < BoneCount; BoneNum++ )
    {
        // Look for extended data called MaskName in each bone.
        bone &Bone = Skeleton.Bones[BoneNum];

        variant Result;
        FindMatchingMember ( Bone.ExtendedData.Type, Bone.ExtendedData.Object, MaskName, &Result );
        if ( Result.Object != NULL )
        {
            // Found it
            granny_bone_track_mask_data TheData;
            ConvertSingleObject ( Bone.ExtendedData.Type, Bone.ExtendedData.Object, GrannyBoneTrackMaskDataType, &TheData );
            FoundNoData = false;
            TrackMask->BoneWeights[BoneNum] = TheData.BoneWeight;
        }
        else
        {
            // Not found.
            FoundAllData = false;
            if ( UseParentForDefault )
            {
                // Use the parent's weight (which in turn will have used its parent's weight, etc).
                int32x ParentIndex = Skeleton.Bones[BoneNum].ParentIndex;
                Assert ( ParentIndex < BoneNum );
                if ( ParentIndex == -1 )
                {
                    // I am a root bone.
                    TrackMask->BoneWeights[BoneNum] = DefaultWeight;
                }
                else
                {
                    TrackMask->BoneWeights[BoneNum] = TrackMask->BoneWeights[ParentIndex];
                }
            }
            else
            {
                TrackMask->BoneWeights[BoneNum] = DefaultWeight;
            }
        }
    }}

    if ( FoundNoData )
    {
        return ExtractTrackMaskResult_NoDataPresent;
    }
    else if ( FoundAllData )
    {
        return ExtractTrackMaskResult_AllDataPresent;
    }
    else
    {
        return ExtractTrackMaskResult_PartialDataPresent;
    }
}

int32x GRANNY
FindMaskIndexForName(unbound_track_mask& UnboundMask,
                     char const* Name)
{
    CheckPointerNotNull(Name, return -1);
    CheckCondition(UnboundMask.Weights || UnboundMask.WeightCount == 0, return -1);

    {for(int32x Mask = 0; Mask < UnboundMask.WeightCount; )
    {
        if (StringsAreEqualOrCallback(UnboundMask.Weights[Mask].Name, Name))
        {
            return Mask;
        }
    }}

    return -1;
}


void GRANNY
BindTrackmaskToModel(unbound_track_mask& UnboundMask,
                     model& Model,
                     track_mask& Mask)
{
    CheckPointerNotNull(Model.Skeleton, return);
    CheckCondition(Model.Skeleton->BoneCount == Mask.BoneCount, return);

    Mask.DefaultWeight = UnboundMask.DefaultWeight;
    {for(int32x Bone = 0; Bone < Model.Skeleton->BoneCount; ++Bone)
    {
        char const* BoneName = Model.Skeleton->Bones[Bone].Name;
        int32x IndexForName = FindMaskIndexForName(UnboundMask, BoneName);

        if (IndexForName != -1)
            Mask.BoneWeights[Bone] = UnboundMask.Weights[IndexForName].Weight;
        else
            Mask.BoneWeights[Bone] = UnboundMask.DefaultWeight;
    }}
}

void GRANNY
BindTrackmaskToTrackGroup(unbound_track_mask& UnboundMask,
                          track_group& TrackGroup,
                          track_mask& Mask)
{
    CheckPointerNotNull(TrackGroup.TransformTracks || TrackGroup.TransformTrackCount == 0, return);
    CheckCondition(TrackGroup.TransformTrackCount == Mask.BoneCount, return);

    Mask.DefaultWeight = UnboundMask.DefaultWeight;
    {for(int32x Track = 0; Track < TrackGroup.TransformTrackCount; ++Track)
    {
        char const* TrackName = TrackGroup.TransformTracks[Track].Name;
        int32x IndexForName = FindMaskIndexForName(UnboundMask, TrackName);

        if (IndexForName != -1)
            Mask.BoneWeights[Track] = UnboundMask.Weights[IndexForName].Weight;
        else
            Mask.BoneWeights[Track] = UnboundMask.DefaultWeight;
    }}
}
