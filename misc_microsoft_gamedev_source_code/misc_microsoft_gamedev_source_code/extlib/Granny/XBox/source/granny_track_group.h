#if !defined(GRANNY_TRACK_GROUP_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_track_group.h $
// $DateTime: 2007/03/19 15:19:15 $
// $Change: 14572 $
// $Revision: #23 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

#if !defined(GRANNY_TRANSFORM_H)
#include "granny_transform.h"
#endif

#if !defined(GRANNY_CURVE_H)
#include "granny_curve.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(AnimationGroup);

struct periodic_loop;
struct skeleton;

EXPTYPE struct vector_track
{
    char const *Name;
    uint32 TrackKey;  // used to match up with the source bones/meshes

    // Don't strictly need to store the dimension, but it's handy.
    int32 Dimension;
    curve2 ValueCurve;
};
EXPCONST EXPGROUP(vector_track) extern data_type_definition VectorTrackType[];

EXPTYPE enum transform_track_flags
{
    UseAccumulatorNeighborhood = 0x1
};

EXPTYPE struct transform_track
{
    char const *Name;
    int32 Flags;

    // Reorganized for better prefetching.  Scale/Shear and Position
    // curves are much more likely than Orientation to be identity or
    // constant.  Since we only issue one prefetch for the curves, we
    // order these so that the fetch at the orientation curve object
    // pointer is most likely to bring in values that we will sample.
    // In typical use, if a curve is constant or identity, the value
    // is supplied from the LODTransform of the bound_transform_track,
    // rather than through ExtractKnots in EvaluateCurve.
    curve2 OrientationCurve;
    curve2 PositionCurve;
    curve2 ScaleShearCurve;
};
EXPCONST EXPGROUP(transform_track) extern data_type_definition TransformTrackType[];

EXPTYPE struct text_track_entry
{
    real32 TimeStamp;
    char const *Text;
};
EXPCONST EXPGROUP(text_track_entry) extern data_type_definition TextTrackEntryType[];

EXPTYPE struct text_track
{
    char const *Name;

    int32 EntryCount;
    text_track_entry *Entries;
};
EXPCONST EXPGROUP(text_track) extern data_type_definition TextTrackType[];

EXPTYPE enum track_group_flags
{
    AccumulationExtracted = 0x1,
    TrackGroupIsSorted = 0x2,
    AccumulationIsVDA = 0x4,

    // NOTE: The upper 16 bits of this value are a degree_of_freedom
    // for VDA.
};
EXPTYPE struct track_group
{
    char const *Name;

    int32 VectorTrackCount;
    vector_track *VectorTracks;

    int32 TransformTrackCount;
    transform_track *TransformTracks;

    int32 TransformLODErrorCount;
    real32 *TransformLODErrors;

    int32 TextTrackCount;
    text_track *TextTracks;

    transform InitialPlacement;

    int32 Flags;

    triple LoopTranslation;
    periodic_loop *PeriodicLoop;
    transform_track *RootMotion;

    variant ExtendedData;
};
EXPCONST EXPGROUP(track_group) extern data_type_definition TrackGroupType[];

EXPAPI GS_READ void GetTrackGroupInitialPlacement4x4(track_group &TrackGroup,
                                                     real32 *Placement4x4);

EXPAPI GS_PARAM void TransformCurve3(transform const &Transform,
                                     int32x Count, real32 *Curve3);
EXPAPI GS_PARAM void TransformCurve4(transform const &Transform,
                                     int32x Count, real32 *Curve4);
EXPAPI GS_PARAM void TransformCurve3x3(transform const &Transform,
                                       int32x Count, real32 *Curve3x3);

EXPAPI GS_READ void GetTrackInitialTransform(transform_track const &Track,
                                             transform &Transform);
EXPAPI GS_PARAM void RemoveTrackInitialTransform(transform_track &Track);

// Actually modifies the input matrices in certain cases.  todo: fix that
EXPAPI GS_SAFE bool BasisConversionRequiresCurveDecompression(real32 *Affine3,
                                                              real32 *Linear3x3,
                                                              real32 *InverseLinear3x3,
                                                              real32 AffineTolerance,
                                                              real32 LinearTolerance,
                                                              bool RoundToTolerance);

EXPAPI GS_PARAM void TransformCurveVec3(real32 const *Affine3,
                                        real32 const *Linear3x3,
                                        real32 AffineTolerance,
                                        real32 LinearTolerance,
                                        curve2 *Curve);

EXPAPI GS_PARAM void SimilarityTransformCurvePosition(real32 const *Affine3,
                                                      real32 const *Linear3x3,
                                                      real32 const *InverseLinear3x3,
                                                      real32 AffineTolerance,
                                                      real32 LinearTolerance,
                                                      curve2 *Curve);
EXPAPI GS_PARAM void SimilarityTransformCurveQuaternion(real32 const *Affine3,
                                                        real32 const *Linear3x3,
                                                        real32 const *InverseLinear3x3,
                                                        real32 AffineTolerance,
                                                        real32 LinearTolerance,
                                                        curve2 *Curve);
EXPAPI GS_PARAM void SimilarityTransformCurveScaleShear(real32 const *Affine3,
                                                        real32 const *Linear3x3,
                                                        real32 const *InverseLinear3x3,
                                                        real32 AffineTolerance,
                                                        real32 LinearTolerance,
                                                        curve2 *Curve);
EXPAPI GS_PARAM void SimilarityTransformTrackGroup(track_group &TrackGroup,
                                                   real32 const *Affine3,
                                                   real32 const *Linear3x3,
                                                   real32 const *InverseLinear3x3,
                                                   real32 AffineTolerance,
                                                   real32 LinearTolerance);

EXPTYPE enum vector_diff_mode
{
    DiffAsVectors,
    DiffAsQuaternions,
    ManhattanMetric
};

EXPAPI GS_PARAM void GetVectorDifferences(int32x VectorDimension,
                                          int32x VectorCount,
                                          real32 const *Vectors,
                                          real32 const *Identity,
                                          vector_diff_mode DiffMode,
                                          real32 &IdentityDifference,
                                          real32 &ConstantDifference);

EXPAPI GS_READ bool KnotsAreReducible(int32x Degree, int32x Dimension,
                                      int32x KnotCount,
                                      real32 const *Knots,
                                      real32 const *Controls,
                                      real32 const *Identity,
                                      real32 Epsilon,
                                      vector_diff_mode DiffMode,
                                      int32x &RequiredDegree,
                                      int32x &RequiredKnotCount);

EXPAPI GS_READ bool CurveIsReducible(curve2 const *Curve,
                                     real32 const *Identity,
                                     real32 Epsilon,
                                     vector_diff_mode DiffMode,
                                     int32x &RequiredDegree,
                                     int32x &RequiredKnotCount);

EXPAPI GS_READ bool TransformTrackHasKeyframedCurves(transform_track const &Track);
EXPAPI GS_READ bool TransformTrackIsAnimated(transform_track const &Track);
EXPAPI GS_READ bool TransformTrackIsIdentity(transform_track const &Track);

EXPAPI GS_READ bool FindTrackByName(track_group const *TrackGroup,
                                    char const *TrackName,
                                    int32x &TrackIndex);
EXPAPI GS_READ bool FindTrackByRule(track_group const *TrackGroup,
                                    char const *ProcessedTrackName,
                                    char const *TrackPattern,
                                    int32x &TrackIndex);
EXPAPI GS_READ bool FindVectorTrackByName(track_group const *TrackGroup,
                                          char const *TrackName,
                                          int32x &VectorTrackIndex);
EXPAPI GS_READ bool FindVectorTrackByRule(track_group const *TrackGroup,
                                          char const *ProcessedTrackName,
                                          char const *TrackPattern,
                                          int32x &VectorTrackIndex);

EXPAPI GS_READ void GetTrackGroupFlags(track_group const &TrackGroup,
                                       uint32 &Flags, uint32 &VDADOFs);
EXPAPI GS_READ void SetTrackGroupFlags(track_group &TrackGroup,
                                       uint32 Flags, uint32 VDADOFs);

EXPAPI GS_READ uint32 VectorTrackKeyForBone(skeleton&   Skeleton,
                                            int32x      BoneIndex,
                                            char const* TrackName);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_TRACK_GROUP_H
#endif
