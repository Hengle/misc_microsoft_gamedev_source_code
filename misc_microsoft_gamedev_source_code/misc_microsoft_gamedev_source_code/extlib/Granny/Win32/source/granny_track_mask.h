#if !defined(GRANNY_TRACK_MASK_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_track_mask.h $
// $DateTime: 2007/04/23 12:48:54 $
// $Change: 14823 $
// $Revision: #9 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(TrackMaskGroup);

struct track_group;
struct model;
struct skeleton;

EXPTYPE struct track_mask;
struct track_mask
{
    real32 DefaultWeight;

    int32 BoneCount;
    real32 *BoneWeights;
};
EXPCONST EXPGROUP(track_mask) extern data_type_definition TrackMaskType[];

EXPTYPE EXPGROUP(unbound_track_mask) struct unbound_weight
{
    char const* Name;
    real32      Weight;
};

EXPTYPE struct unbound_track_mask
{
    real32 DefaultWeight;

    int32 WeightCount;
    unbound_weight* Weights;
};
EXPCONST EXPGROUP(unbound_track_mask) extern data_type_definition UnboundTrackMaskType[];


EXPTYPE enum extract_track_mask_result
{
    ExtractTrackMaskResult_AllDataPresent,
    ExtractTrackMaskResult_PartialDataPresent,
    ExtractTrackMaskResult_NoDataPresent
};


EXPAPI GS_SAFE track_mask *NewTrackMask(real32 DefaultWeight, int32x BoneCount);
EXPAPI GS_PARAM extract_track_mask_result ExtractTrackMask(track_mask *TrackMask,
                                                           int32x BoneCount,
                                                           skeleton const &Skeleton,
                                                           char *MaskName,
                                                           real32 DefaultWeight,
                                                           bool UseParentForDefault);
EXPAPI GS_READ real32 GetTrackMaskBoneWeight(track_mask const &Mask,
                                             int32x BoneIndex);
EXPAPI GS_PARAM void SetTrackMaskBoneWeight(track_mask &Mask, int32x BoneIndex,
                                            real32 Weight);
EXPAPI GS_PARAM void FreeTrackMask(track_mask *Mask);

EXPAPI GS_PARAM track_mask *CopyTrackMask(track_mask const &Mask);
EXPAPI GS_PARAM void InvertTrackMask(track_mask &Mask);

EXPAPI GS_PARAM void SetSkeletonTrackMaskFromTrackGroup(track_mask &Mask,
                                                        skeleton const &Skeleton,
                                                        track_group const &TrackGroup,
                                                        real32 IdentityValue,
                                                        real32 ConstantValue,
                                                        real32 AnimatedValue);

EXPAPI GS_PARAM void SetSkeletonTrackMaskChainUpwards(track_mask &Mask,
                                                      skeleton const &Skeleton,
                                                      int32x ChainLeafBoneIndex,
                                                      real32 Weight);
EXPAPI GS_PARAM void SetSkeletonTrackMaskChainDownwards(track_mask &Mask,
                                                        skeleton const &Skeleton,
                                                        int32x ChainRootBoneIndex,
                                                        real32 Weight);

EXPAPI GS_READ int32x FindMaskIndexForName(unbound_track_mask& UnboundMask,
                                           char const* Name);

EXPAPI GS_PARAM void BindTrackmaskToModel(unbound_track_mask& UnboundMask,
                                          model& Model,
                                          track_mask& Mask);
EXPAPI GS_PARAM void BindTrackmaskToTrackGroup(unbound_track_mask& UnboundMask,
                                               track_group& Model,
                                               track_mask& Mask);

EXPCONST extern track_mask IdentityTrackMask;
EXPCONST extern track_mask NullTrackMask;

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_TRACK_MASK_H
#endif
