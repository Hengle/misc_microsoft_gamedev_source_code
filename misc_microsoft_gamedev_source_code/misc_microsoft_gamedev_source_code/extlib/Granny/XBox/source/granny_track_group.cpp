// ========================================================================
// $File: //jeffr/granny/rt/granny_track_group.cpp $
// $DateTime: 2007/08/23 09:22:51 $
// $Change: 15808 $
// $Revision: #43 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TRACK_GROUP_H)
#include "granny_track_group.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_CRC_H)
#include "granny_crc.h"
#endif

#if !defined(GRANNY_FLOATS_H)
#include "granny_floats.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_PERIODIC_LOOP_H)
#include "granny_periodic_loop.h"
#endif

#if !defined(GRANNY_SKELETON_H)
#include "granny_skeleton.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
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

data_type_definition VectorTrackType[] =
{
    {StringMember, "Name"},
    {UInt32Member, "TrackKey"},

    {Int32Member, "Dimension"},
    {InlineMember, "ValueCurve", Curve2Type},

    {EndMember},
};

data_type_definition TransformTrackType[] =
{
    {StringMember, "Name"},
    {Int32Member, "Flags"},

    {InlineMember, "OrientationCurve", Curve2Type},
    {InlineMember, "PositionCurve", Curve2Type},
    {InlineMember, "ScaleShearCurve", Curve2Type},

    {EndMember},
};

data_type_definition TextTrackEntryType[] =
{
    {Real32Member, "TimeStamp"},
    {StringMember, "Text"},

    {EndMember},
};

data_type_definition TextTrackType[] =
{
    {StringMember, "Name"},

    {ReferenceToArrayMember, "Entries", TextTrackEntryType},

    {EndMember},
};

data_type_definition TrackGroupType[] =
{
    {StringMember, "Name"},

    {ReferenceToArrayMember, "VectorTracks", VectorTrackType},

    {ReferenceToArrayMember, "TransformTracks", TransformTrackType},

    {ReferenceToArrayMember, "TransformLODErrors", Real32Type},

    {ReferenceToArrayMember, "TextTracks", TextTrackType},

    {TransformMember, "InitialPlacement"},

    {Int32Member, "AccumulationFlags"},
    {Real32Member, "LoopTranslation", 0, 3},
    {ReferenceMember, "PeriodicLoop", PeriodicLoopType},
    {ReferenceMember, "RootMotion", TransformTrackType},

    {VariantReferenceMember, "ExtendedData"},

    {EndMember},
};

END_GRANNY_NAMESPACE;

void GRANNY
GetTrackGroupInitialPlacement4x4(track_group &TrackGroup,
                                 real32 *Placement4x4)
{
    BuildCompositeTransform4x4(TrackGroup.InitialPlacement, Placement4x4);
}

void GRANNY
TransformCurve3(transform const &Transform, int32x Count, real32 *Curve3)
{
    while(Count--)
    {
        TransformPointInPlace(Curve3, Transform);
        Curve3 += 3;
    }
}

void GRANNY
TransformCurve4(transform const &Transform, int32x Count, real32 *Curve4)
{
    while(Count--)
    {
        quad Temp;
        QuaternionMultiply4(Temp, Transform.Orientation, Curve4);
        VectorEquals4(Curve4, Temp);
        Curve4 += 4;
    }
}

void GRANNY
TransformCurve3x3(transform const &Transform, int32x Count, real32 *Curve3x3)
{
    triple Temp[3];
    triple ScaleShear[3];
    triple Rotation[3];

    MatrixEqualsQuaternion3x3((real32 *)Rotation, Transform.Orientation);
    MatrixMultiply3x3((real32 *)Temp, (real32 *)Transform.ScaleShear,
                      (real32 *)Rotation);
    TransposeMatrixMultiply3x3((real32 *)ScaleShear, (real32 *)Rotation,
                               (real32 *)Temp);

    while(Count--)
    {
        MatrixMultiply3x3((real32 *)Temp, (real32 *)ScaleShear,
                          (real32 *)Curve3x3);
        MatrixEquals3x3((real32 *)Curve3x3, (real32 *)Temp);
        Curve3x3 += 9;
    }
}

void GRANNY
GetTrackInitialTransform(transform_track const &Track, transform &Transform)
{
    real32 PositionData[3];
    real32 OrientationData[4];
    real32 ScaleShearData[9];
    CurveExtractKnotValue ( Track.PositionCurve   , 0, PositionData   , CurveIdentityPosition    );
    CurveExtractKnotValue ( Track.OrientationCurve, 0, OrientationData, CurveIdentityOrientation );
    CurveExtractKnotValue ( Track.ScaleShearCurve , 0, ScaleShearData , CurveIdentityScaleShear  );

    SetTransformWithIdentityCheck(
        Transform, PositionData, OrientationData, ScaleShearData);
}

// TODO: version of this that handles all curve types?
// But then you're re-quantising a quantised curve and the aliasing is going to suck.
// I think better to make it explicit that hey - don't do this unless they're uncompressed curves.
// TODO: use FindSwizzlesFromLinear and BasisConversionRequiresCurveDecompression to handle all formats anyway?
void GRANNY
RemoveTrackInitialTransform(transform_track &Track)
{
    curve_data_da_k32f_c32f *UncompressedPositionCurve = CurveGetContentsOfDaK32fC32f ( Track.PositionCurve );
    curve_data_da_k32f_c32f *UncompressedOrientationCurve = CurveGetContentsOfDaK32fC32f ( Track.OrientationCurve );
    curve_data_da_k32f_c32f *UncompressedScaleShearCurve = CurveGetContentsOfDaK32fC32f ( Track.ScaleShearCurve );
    // This function won't work unless all the inputs are of uncompressed type, or identity.
    if ( ( UncompressedPositionCurve == NULL ) && !CurveIsIdentity ( Track.PositionCurve ) )
    {
        Log0 ( ErrorLogMessage, SubsystemCode, "Track contains compressed position curve type - can't remove initial transform" );
        return;
    }
    if ( ( UncompressedOrientationCurve == NULL ) && !CurveIsIdentity ( Track.OrientationCurve ) )
    {
        Log0 ( ErrorLogMessage, SubsystemCode, "Track contains compressed orientation curve type - can't remove initial transform" );
        return;
    }
    if ( ( UncompressedScaleShearCurve == NULL ) && !CurveIsIdentity ( Track.ScaleShearCurve ) )
    {
        Log0 ( ErrorLogMessage, SubsystemCode, "Track contains compressed scale-shear curve type - can't remove initial transform" );
        return;
    }



    transform InitialTransform;
    GetTrackInitialTransform(Track, InitialTransform);

    transform InverseInitialTransform;
    BuildInverse(InverseInitialTransform, InitialTransform);

    if(!CurveIsIdentity ( Track.PositionCurve ))
    {
        Assert ( UncompressedPositionCurve != NULL );
        TransformCurve3(InverseInitialTransform,
                        UncompressedPositionCurve->KnotCount,
                        UncompressedPositionCurve->Controls);
    }

    if(!CurveIsIdentity ( Track.OrientationCurve ))
    {
        Assert ( UncompressedOrientationCurve != NULL );
        TransformCurve4(InverseInitialTransform,
                        UncompressedOrientationCurve->KnotCount,
                        UncompressedOrientationCurve->Controls);
    }

    if(!CurveIsIdentity ( Track.ScaleShearCurve ))
    {
        Assert ( UncompressedScaleShearCurve != NULL );
        TransformCurve3x3(InverseInitialTransform,
                          UncompressedScaleShearCurve->KnotCount,
                          UncompressedScaleShearCurve->Controls);
    }
}



// SourceComponent is a vec3 of either 0 (comes from X), 1 (comes from Y), 2 (comes from Z).
// So a Linear3x3 matrix of:
// 0.0, 2.0, 0.0
// 0.0, 0.0, -5.0
// 3.0, 0.0, 0.0
// Gives a result of:
// SourceComponent = { 1, 2, 0 }
// SourceScale = { 2.0, -5.0, 3.0 }
// The idea is that:
// Dest[0] = SourceScale[0] * Source[SourceComponent[0]];
// Dest[1] = SourceScale[1] * Source[SourceComponent[1]];
// Dest[2] = SourceScale[2] * Source[SourceComponent[2]];
// Should give the same result as:
// VectorTransform3 ( Dest, Linear3x3, Source );
void FindSwizzlesFromLinear ( real32 const *Linear3x3,
                              int32x *SourceComponent,
                              real32 *SourceScale )
{
    {for ( int32x Component = 0; Component < 3; Component++ )
    {
        real32 const *LinearVector = Linear3x3 + Component * 3;
        // Find the largest component.
        SourceComponent[Component] = 0;
        {for ( int32x Component2 = 1; Component2 < 3; Component2++ )
        {
            if ( Square ( LinearVector[SourceComponent[Component]] ) < Square ( LinearVector[Component2] ) )
            {
                SourceComponent[Component] = Component2;
            }
        }}
        SourceScale[Component] = LinearVector[SourceComponent[Component]];
    }}
}

bool GRANNY
BasisConversionRequiresCurveDecompression(real32 *Affine3,
                                          real32 *Linear3x3,
                                          real32 *InverseLinear3x3,
                                          real32 AffineTolerance,
                                          real32 LinearTolerance,
                                          bool RoundToTolerance)
{
    // AffineTolerance is not actually used - all formats of curve can handle positional offsets.

    // Check our assumption that InvLin3x3 is the inverse of Linear3x3.  If it's not,
    // we'll just have to do the decompression and trust that the application knows what
    // it's doing.
    if (InverseLinear3x3 != NULL)
    {
        real32 CheckInverse[9];
        MatrixMultiply3x3(CheckInverse, Linear3x3, InverseLinear3x3);
        {for ( int32x Component = 0; Component < 3; Component++ )
        {
            real32 const *LinearVector = CheckInverse + Component * 3;
            {for ( int32x Component2 = 0; Component2 < 3; Component2++ )
            {
                if (Component == Component2)
                {
                    // The inverse isn't precise enough
                    if ( Square ( LinearVector[Component2] - 1.0f ) > Square ( LinearTolerance ) )
                        return true;
                }
                else
                {
                    // The inverse isn't precise enough?
                    if ( Square ( LinearVector[Component2] ) > Square ( LinearTolerance ) )
                        return true;
                }
            }}
        }}
    }


    real32 SourceScale[3];
    int32x SourceComponent[3];
    FindSwizzlesFromLinear ( Linear3x3, SourceComponent, SourceScale );
    // Find the components that are not going to be used.
    int32x SourceNotComponents[3][2];
    {for ( int32x Component = 0; Component < 3; Component++ )
    {
        SourceNotComponents[Component][0] = (SourceComponent[Component] + 1) % 3;
        SourceNotComponents[Component][1] = (SourceComponent[Component] + 2) % 3;
    }}

    if ( ( SourceComponent[0] == SourceComponent[1] ) ||
         ( SourceComponent[1] == SourceComponent[2] ) ||
         ( SourceComponent[2] == SourceComponent[0] ) )
    {
        // Replication swizzle!
        return true;
    }

    {for ( int32x Component = 0; Component < 3; Component++ )
    {
        real32 const *LinearVector = Linear3x3 + Component * 3;
        {for ( int32x Component2 = 0; Component2 < 2; Component2++ )
        {
            if ( Square ( LinearVector[SourceNotComponents[Component][Component2]] ) > Square ( LinearTolerance ) )
            {
                // One of the lesser values is too large.
                return true;
            }
        }}
    }}

    // And make sure all the scales are the same (sign flips are allowed).
    // Non-equal scales mean that the axes of quaternion rotations would need
    // to change, causing decompression & recompression. You could avoid that
    // by allowing non-unit quaternions, but that's moderately insane.
    if ( ( AbsoluteValue ( AbsoluteValue ( SourceScale[0] ) - AbsoluteValue ( SourceScale[1] ) ) > LinearTolerance ) ||
         ( AbsoluteValue ( AbsoluteValue ( SourceScale[1] ) - AbsoluteValue ( SourceScale[2] ) ) > LinearTolerance ) ||
         ( AbsoluteValue ( AbsoluteValue ( SourceScale[2] ) - AbsoluteValue ( SourceScale[0] ) ) > LinearTolerance ) )
    {
        // Deviance!
        return true;
    }


    // Looks like this is a transform that only has scales and swizzles of axes.
    // Which means the animation curves don't need decompressing and
    // won't loose precision under transformation.

    if ( RoundToTolerance )
    {
        // To avoid small numbers resulting from round-off errors from polluting stuff, just set them to zero.
        real32 MasterScale = AbsoluteValue ( SourceScale[0] );
        {for ( int32x Component = 0; Component < 3; Component++ )
        {
            real32 *LinearVector = Linear3x3 + Component * 3;
            {for ( int32x Component2 = 0; Component2 < 2; Component2++ )
            {
                LinearVector[SourceNotComponents[Component][Component2]] = 0.0f;
            }}

            if ( LinearVector[SourceComponent[Component]] > 0.0f )
            {
                Assert ( AbsoluteValue ( MasterScale - LinearVector[SourceComponent[Component]] ) <= LinearTolerance );
                LinearVector[SourceComponent[Component]] = MasterScale;
            }
            else
            {
                Assert ( AbsoluteValue ( MasterScale + LinearVector[SourceComponent[Component]] ) <= LinearTolerance );
                LinearVector[SourceComponent[Component]] = -MasterScale;
            }
        }}
        MatrixInvert3x3(InverseLinear3x3, Linear3x3);

        real32 SourceScale2[3];
        int32x SourceComponent2[3];
        FindSwizzlesFromLinear ( Linear3x3, SourceComponent2, SourceScale2 );
        {for ( int32x Component = 0; Component < 3; Component++ )
        {
            Assert ( SourceComponent2[Component] == SourceComponent[Component] );
            Assert ( SourceScale2[Component] == SourceScale[Component] );
        }}
    }

    return false;
}


static bool
BasisConversionRequiresCurveDecompressionConst(real32 const *Affine3,
                                               real32 const *Linear3x3,
                                               real32 const *InverseLinear3x3,
                                               real32 AffineTolerance,
                                               real32 LinearTolerance)
{
    // Shush, Mr. Compiler.
    return BasisConversionRequiresCurveDecompression (
        (real32*)Affine3, (real32*)Linear3x3, (real32*)InverseLinear3x3, AffineTolerance, LinearTolerance, false );
}



void GRANNY
TransformCurveVec3(real32 const *Affine3,
                   real32 const *Linear3x3,
                   real32 AffineTolerance,
                   real32 LinearTolerance,
                   curve2 *Curve)
{
    if ( CurveIsIdentity ( *Curve ) )
    {
        // Can't transform the identity curve.
        Log0 ( ErrorLogMessage, SubsystemCode, "Curve is the identity value - can't transform it" );
        return;
    }
    if ( CurveGetDimensionUnchecked ( *Curve ) != 3 )
    {
        // Can't transform non-vec3s
        Log0 ( ErrorLogMessage, SubsystemCode, "Curve does not have dimension=3" );
        return;
    }

    if ( !BasisConversionRequiresCurveDecompressionConst ( Affine3, Linear3x3, NULL, AffineTolerance, LinearTolerance ) )
    {
        // We can just do a swizzle.
        real32 SourceScale[3];
        int32x SourceComponent[3];
        int32x Dimension = CurveGetDimensionUnchecked ( *Curve );
        Assert ( Dimension == 3 );
        FindSwizzlesFromLinear ( Linear3x3, SourceComponent, SourceScale );

#if DEBUG
        int32 KnotCount = CurveGetKnotCount ( *Curve );
        real32 *Knots = NULL;
        if ( !CurveIsKeyframed ( *Curve ) )
        {
            Knots = AllocateArray ( KnotCount, real32 );
        }
        real32 *Controls = AllocateArray ( 3 * KnotCount, real32 );
        CurveExtractKnotValues ( *Curve, 0, KnotCount, Knots, Controls, CurveIdentityPosition );
        real32 *CurControl = Controls;
        {for ( int32x KnotNum = 0; KnotNum < KnotCount; KnotNum++ )
        {
            InPlaceSimilarityTransformPosition(Affine3, Linear3x3, CurControl);
            CurControl += Dimension;
        }}
#endif

        CurveScaleOffsetSwizzle ( Curve, Dimension, SourceScale, Affine3, SourceComponent );

#if DEBUG
        CurControl = Controls;
        {for ( int32x KnotNum = 0; KnotNum < KnotCount; KnotNum++ )
        {
            real32 Controls2[3];
            CurveExtractKnotValue ( *Curve, KnotNum, Controls2, CurveIdentityOrientation );

            VectorSubtract3 ( Controls2, CurControl );
            real32 DifferenceSq = VectorLengthSquared3 ( Controls2 );
            Assert ( DifferenceSq <= AffineTolerance * AffineTolerance );
            CurControl += Dimension;
        }}
        Deallocate ( Controls );
        Deallocate ( Knots );
#endif

    }
    else
    {
        // Going to have to do it the slow (and lossy) way.
        int32x NumKnots = CurveGetKnotCount ( *Curve );
        int32x Dimension = CurveGetDimensionUnchecked ( *Curve );
        Assert ( Dimension == 3 );
        real32 *Knots = NULL;
        if ( !CurveIsKeyframed ( *Curve ) )
        {
            Knots = AllocateArray ( NumKnots, real32 );
        }
        real32 *Controls = AllocateArray ( NumKnots * Dimension, real32 );
        if ( Controls != NULL )
        {
            CurveExtractKnotValues ( *Curve, 0, NumKnots, Knots, Controls, CurveIdentityPosition );
            real32 *CurControl = Controls;
            {for ( int32x KnotNum = 0; KnotNum < NumKnots; KnotNum++ )
            {
                InPlaceSimilarityTransformPosition(Affine3, Linear3x3, CurControl);
                CurControl += Dimension;
            }}
            CurveSetAllKnotValues ( Curve, NumKnots, Dimension, Knots, Controls );
        }
        Deallocate ( Knots );
        Deallocate ( Controls );
    }
}


void GRANNY
SimilarityTransformCurvePosition(real32 const *Affine3,
                                 real32 const *Linear3x3,
                                 real32 const *InverseLinear3x3,
                                 real32 AffineTolerance,
                                 real32 LinearTolerance,
                                 curve2 *Curve)
{
    // Same as a standard vec3
    TransformCurveVec3 ( Affine3, Linear3x3, AffineTolerance, LinearTolerance, Curve );
}



void GRANNY
SimilarityTransformCurveQuaternion(real32 const *Affine3,
                                   real32 const *Linear3x3,
                                   real32 const *InverseLinear3x3,
                                   real32 AffineTolerance,
                                   real32 LinearTolerance,
                                   curve2 *Curve)
{
    if ( CurveIsIdentity ( *Curve ) )
    {
        // Can't transform the identity curve.
        Log0 ( ErrorLogMessage, SubsystemCode, "Curve is the identity value - can't transform it" );
        return;
    }
    if ( CurveGetDimensionUnchecked ( *Curve ) != 4 )
    {
        // Can't transform non-vec4s
        Log0 ( ErrorLogMessage, SubsystemCode, "Curve does not have dimension=4" );
        return;
    }

    // The Affine3 term is ignored for quaternions.

    if ( !BasisConversionRequiresCurveDecompressionConst ( GlobalZeroVector, Linear3x3, InverseLinear3x3, 1.0f, LinearTolerance ) )
    {
        // We can just do a swizzle. At least I think so!
        // So one way to find out what swizzle and scale we do is to transform the
        // four basis quaternions. But that has the problem that
        // the result may be Q or -Q, we don't know which, so the scales come out to
        // -1 or +1 at random.
        // So the solution is that we know that under any transformation, W stays as W
        // So we make sure we always have a W component, and that it is made +ve
        // after the transformation. Then we can find the swizzle and sign value of the other
        // component.
        const real32 RefCompValue = 4.0f / 5.0f;
        const real32 RefWValue = 3.0f / 5.0f;
        real32 SourceScale[4];
        int32x SourceComponent[4];
        SourceComponent[0] = -1;
        SourceComponent[1] = -1;
        SourceComponent[2] = -1;
        SourceComponent[3] = 3;
        SourceScale[3] = 1.0f;
        {for ( int32x ComponentNum = 0; ComponentNum < 3; ComponentNum++ )
        {
            real32 Quat[4];
            Quat[0] = 0.0f;
            Quat[1] = 0.0f;
            Quat[2] = 0.0f;
            Quat[3] = RefWValue;
            Quat[ComponentNum] = RefCompValue;
            InPlaceSimilarityTransformOrientation ( Linear3x3, InverseLinear3x3, Quat );
            if ( Quat[3] < 0.0f )
            {
                VectorNegate4 ( Quat );
            }
            Assert ( AbsoluteValue ( Quat[3] - RefWValue ) < 0.0001f );

            int32x TargetComponent = -1;
            real32 TargetScale = 0.0f;
            {for ( int32x ComponentNum2 = 0; ComponentNum2 < 3; ComponentNum2++ )
            {
                if ( AbsoluteValue ( Quat[ComponentNum2] ) > RefCompValue - 0.05f )
                {
                    Assert ( TargetComponent == -1 );
                    TargetComponent = ComponentNum2;
                    if ( Quat[ComponentNum2] > 0.0f )
                    {
                        Assert ( AbsoluteValue ( Quat[ComponentNum2] - RefCompValue ) < 0.0001f );
                        TargetScale = 1.0f;
                    }
                    else
                    {
                        Assert ( AbsoluteValue ( Quat[ComponentNum2] + RefCompValue ) < 0.0001f );
                        TargetScale = -1.0f;
                    }
                }
                else
                {
                    Assert ( AbsoluteValue ( Quat[ComponentNum2] ) < 0.1f );
                }
            }}
            Assert ( TargetComponent >= 0 );

            // OK, so we know that a 1.0 in source component [ComponentNum] becomes a TargetScale in target component [TargetScale].
            // And the swizzle tells us where to get each target component from, and what scale to apply it to.
            Assert ( SourceComponent[TargetComponent] == -1 );
            SourceComponent[TargetComponent] = ComponentNum;
            SourceScale[TargetComponent] = TargetScale;
        }}

        int32x Dimension = CurveGetDimensionUnchecked ( *Curve );
        Assert ( Dimension == 4 );

#if DEBUG
        int32 KnotCount = CurveGetKnotCount ( *Curve );
        real32 *Knots = AllocateArray ( KnotCount, real32 );
        real32 *Controls = AllocateArray ( 4 * KnotCount, real32 );
        CurveExtractKnotValues ( *Curve, 0, KnotCount, Knots, Controls, CurveIdentityOrientation );
        real32 *CurControl = Controls;
        {for ( int32x KnotNum = 0; KnotNum < KnotCount; KnotNum++ )
        {
            InPlaceSimilarityTransformOrientation(Linear3x3, InverseLinear3x3, CurControl);
            CurControl += Dimension;
        }}
#endif

        CurveScaleOffsetSwizzle ( Curve, Dimension, SourceScale, GlobalZeroVector, SourceComponent );

#if DEBUG
        CurControl = Controls;
        {for ( int32x KnotNum = 0; KnotNum < KnotCount; KnotNum++ )
        {
            real32 Controls2[4];
            CurveExtractKnotValue ( *Curve, KnotNum, Controls2, CurveIdentityOrientation );

            real32 DotProd = InnerProduct4 ( Controls2, CurControl );
            real32 LengthSq = VectorLengthSquared4 ( CurControl );
            real32 LengthSq2 = VectorLengthSquared4 ( Controls2 );
            // In theory both lengths should be 1.0, but curve compression can drive them a fair way off.
            Assert ( AbsoluteValue ( LengthSq2 - LengthSq ) < 0.0001f );
            // And then the dot product can be either 1.0 or -1.0 times the length, we don't mind.
            Assert ( AbsoluteValue ( AbsoluteValue ( DotProd ) - LengthSq ) < 0.0001f );
            CurControl += Dimension;
        }}
        Deallocate ( Controls );
        Deallocate ( Knots );
#endif
    }
    else
    {
        // Going to have to do it the slow (and lossy) way.
        int32x NumKnots = CurveGetKnotCount ( *Curve );
        int32x Dimension = CurveGetDimensionUnchecked ( *Curve );
        Assert ( Dimension == 4 );
        real32 *Knots = NULL;
        if ( !CurveIsKeyframed ( *Curve ) )
        {
            Knots = AllocateArray ( NumKnots, real32 );
        }
        real32 *Controls = AllocateArray ( NumKnots * Dimension, real32 );
        if ( Controls != NULL )
        {
            CurveExtractKnotValues ( *Curve, 0, NumKnots, Knots, Controls, CurveIdentityOrientation );
            real32 *CurControl = Controls;
            {for ( int32x KnotNum = 0; KnotNum < NumKnots; KnotNum++ )
            {
                InPlaceSimilarityTransformOrientation(Linear3x3, InverseLinear3x3, CurControl);
                CurControl += Dimension;
            }}
            CurveSetAllKnotValues ( Curve, NumKnots, Dimension, Knots, Controls );
        }
        Deallocate ( Knots );
        Deallocate ( Controls );
    }
}


void GRANNY
SimilarityTransformCurveScaleShear(real32 const *Affine3,
                                   real32 const *Linear3x3,
                                   real32 const *InverseLinear3x3,
                                   real32 AffineTolerance,
                                   real32 LinearTolerance,
                                   curve2 *Curve)
{
    if ( CurveIsIdentity ( *Curve ) )
    {
        // Can't transform the identity curve.
        Log0 ( ErrorLogMessage, SubsystemCode, "Curve is the identity value - can't transform it" );
        return;
    }
    if ( CurveGetDimensionUnchecked ( *Curve ) != 9 )
    {
        // Can't transform non-3x3s
        Log0 ( ErrorLogMessage, SubsystemCode, "Curve does not have dimension=9" );
        return;
    }

    // The Affine3 term is ignored for Scale/Shear

    if ( !BasisConversionRequiresCurveDecompressionConst ( GlobalZeroVector, Linear3x3, InverseLinear3x3, 1.0f, LinearTolerance ) )
    {
        // We can just do a swizzle.
        // Easiest way to find out which swizzle is to transform the things.
        // Although it's a 3x3, the transform does the same things to all 3 vec3s, so
        // we only need to do 3 swizzles.
        real32 SourceScale[9];
        int32x SourceComponent[9];
        SourceComponent[0] = -1;
        SourceComponent[1] = -1;
        SourceComponent[2] = -1;
        SourceComponent[3] = -1;
        SourceComponent[4] = -1;
        SourceComponent[5] = -1;
        SourceComponent[6] = -1;
        SourceComponent[7] = -1;
        SourceComponent[8] = -1;
        {for ( int32x ComponentNum = 0; ComponentNum < 9; ComponentNum++ )
        {
            real32 Vec[9];
            Vec[0] = 0.0f;
            Vec[1] = 0.0f;
            Vec[2] = 0.0f;
            Vec[3] = 0.0f;
            Vec[4] = 0.0f;
            Vec[5] = 0.0f;
            Vec[6] = 0.0f;
            Vec[7] = 0.0f;
            Vec[8] = 0.0f;
            // Set the right component to 1.
            Vec[ComponentNum] = 1.0f;
            InPlaceSimilarityTransformScaleShear ( Linear3x3, InverseLinear3x3, Vec );
            int32x TargetComponent = -1;
            real32 TargetScale = 0.0f;
            {for ( int32x ComponentNum2 = 0; ComponentNum2 < 9; ComponentNum2++ )
            {
                if ( AbsoluteValue ( Vec[ComponentNum2] ) > LinearTolerance )
                {
                    Assert ( TargetComponent == -1 );
                    // I _think_ these are right, but I'm really not that sure.
                    Assert ( AbsoluteValue ( Vec[ComponentNum2] ) > 0.999f );
                    Assert ( AbsoluteValue ( Vec[ComponentNum2] ) < 1.001f );
                    TargetComponent = ComponentNum2;
                    if ( Vec[ComponentNum2] > 0.0f )
                    {
                        Assert ( AbsoluteValue ( 1.0f - Vec[ComponentNum2] ) < 0.0001f );
                        TargetScale = 1.0f;
                    }
                    else
                    {
                        Assert ( AbsoluteValue ( -1.0f - Vec[ComponentNum2] ) < 0.0001f );
                        TargetScale = -1.0f;
                    }
                }
            }}
            Assert ( TargetComponent >= 0 );

            // OK, so we know that a 1.0 in source component [ComponentNum] becomes a TargetScale in target component [TargetScale].
            // And the swizzle tells us where to get each target component from, and what scale to apply it to.
            Assert ( SourceComponent[TargetComponent] == -1 );
            SourceComponent[TargetComponent] = ComponentNum;
            SourceScale[TargetComponent] = TargetScale;
        }}

#if DEBUG
        // Some sanity checks.
        {for ( int32x i = 0; i < 3; i++ )
        {
            int32x ColumnSwizzle = SourceComponent[i*3+0] / 3;
            int32x RowSwizzle = SourceComponent[0+i] % 3;
            {for ( int32x j = 0; j < 3; j++ )
            {
                Assert ( ColumnSwizzle == SourceComponent[i*3+j] / 3 );
                Assert ( RowSwizzle == SourceComponent[j*3+i] % 3 );
            }}
        }}
#endif

        int32x Dimension = CurveGetDimensionUnchecked ( *Curve );
        Assert ( Dimension == 9 );


#if DEBUG
        int32 KnotCount = CurveGetKnotCount ( *Curve );
        real32 *Knots = AllocateArray ( KnotCount, real32 );
        real32 *Controls = AllocateArray ( 9 * KnotCount, real32 );
        CurveExtractKnotValues ( *Curve, 0, KnotCount, Knots, Controls, CurveIdentityScaleShear );
        real32 *CurControl = Controls;
        {for ( int32x KnotNum = 0; KnotNum < KnotCount; KnotNum++ )
        {
            InPlaceSimilarityTransformScaleShear(Linear3x3, InverseLinear3x3, CurControl);
            CurControl += Dimension;
        }}
#endif

        CurveScaleOffsetSwizzle ( Curve, Dimension, SourceScale, GlobalZero16, SourceComponent );

#if DEBUG
        CurControl = Controls;
        {for ( int32x KnotNum = 0; KnotNum < KnotCount; KnotNum++ )
        {
            real32 Controls2[9];
            CurveExtractKnotValue ( *Curve, KnotNum, Controls2, CurveIdentityScaleShear );

            VectorSubtract3 ( Controls2+0, CurControl+0 );
            real32 DifferenceSq = VectorLengthSquared3 ( Controls2+0 );
            Assert ( DifferenceSq < LinearTolerance * LinearTolerance );
            VectorSubtract3 ( Controls2+3, CurControl+3 );
            DifferenceSq = VectorLengthSquared3 ( Controls2+3 );
            Assert ( DifferenceSq < LinearTolerance * LinearTolerance );
            VectorSubtract3 ( Controls2+6, CurControl+6 );
            DifferenceSq = VectorLengthSquared3 ( Controls2+6 );
            Assert ( DifferenceSq < LinearTolerance * LinearTolerance );
            CurControl += Dimension;
        }}
        Deallocate ( Controls );
        Deallocate ( Knots );
#endif

    }
    else
    {
        // Going to have to do it the slow (and lossy) way.
        int32x NumKnots = CurveGetKnotCount ( *Curve );
        int32x Dimension = CurveGetDimensionUnchecked ( *Curve );
        Assert ( Dimension == 9 );
        real32 *Knots = NULL;
        if ( !CurveIsKeyframed ( *Curve ) )
        {
            Knots = AllocateArray ( NumKnots, real32 );
        }
        real32 *Controls = AllocateArray ( NumKnots * Dimension, real32 );
        if ( Controls != NULL )
        {
            CurveExtractKnotValues ( *Curve, 0, NumKnots, Knots, Controls, CurveIdentityPosition );
            real32 *CurControl = Controls;
            {for ( int32x KnotNum = 0; KnotNum < NumKnots; KnotNum++ )
            {
                InPlaceSimilarityTransformScaleShear ( Linear3x3, InverseLinear3x3, CurControl );
                CurControl += Dimension;
            }}
            CurveSetAllKnotValues ( Curve, NumKnots, Dimension, Knots, Controls );
        }
        Deallocate ( Knots );
        Deallocate ( Controls );
    }
}


void GRANNY
SimilarityTransformTrackGroup(track_group &TrackGroup,
                              real32 const *Affine3,
                              real32 const *Linear3x3,
                              real32 const *InverseLinear3x3,
                              real32 AffineTolerance,
                              real32 LinearTolerance )
{
    {for(int32x TrackIndex = 0;
         TrackIndex < TrackGroup.TransformTrackCount;
         ++TrackIndex)
    {
        transform_track &Track = TrackGroup.TransformTracks[TrackIndex];

        // Actually, we _don't_ want to transform the animation tracks
        // by the Affine3 part at all. The affine part should only be applied to InitialPlacement.
        if(!CurveIsIdentity ( Track.PositionCurve ))
        {
            SimilarityTransformCurvePosition(GlobalZeroVector,
                                             Linear3x3, InverseLinear3x3,
                                             1.0f, LinearTolerance,
                                             &(Track.PositionCurve) );
        }

        if(!CurveIsIdentity ( Track.OrientationCurve ))
        {
            SimilarityTransformCurveQuaternion(GlobalZeroVector,
                                               Linear3x3, InverseLinear3x3,
                                               1.0f, LinearTolerance,
                                               &(Track.OrientationCurve) );
        }

        if(!CurveIsIdentity ( Track.ScaleShearCurve ))
        {
            SimilarityTransformCurveScaleShear(GlobalZeroVector,
                                               Linear3x3, InverseLinear3x3,
                                               1.0f, LinearTolerance,
                                               &(Track.ScaleShearCurve) );
        }
    }}

    // Actually, we _don't_ want to transform the animation tracks
    // by the Affine3 part at all. The affine part should only be applied to InitialPlacement.
    InPlaceSimilarityTransformPosition(GlobalZeroVector, Linear3x3,
                                       TrackGroup.LoopTranslation);
    if(TrackGroup.PeriodicLoop)
    {
        periodic_loop &Loop = *TrackGroup.PeriodicLoop;
        InPlaceSimilarityTransformPosition(GlobalZeroVector, Linear3x3,
                                           Loop.BasisX);
        InPlaceSimilarityTransformPosition(GlobalZeroVector, Linear3x3,
                                           Loop.BasisY);
        InPlaceSimilarityTransformPosition(GlobalZeroVector, Linear3x3,
                                           Loop.Axis);

        // Note: I use BasisX here to determine what the scale factor
        // should be for the radius, although in general it would be
        // more correct to re-solve the circle after the transform so
        // that non-uniform scaling and shearing could be accounted
        // for.  Same with dZ, really.
        Loop.dZ *= NormalizeOrZero3(Loop.Axis);
        Loop.Radius *= NormalizeOrZero3(Loop.BasisX);
        NormalizeOrZero3(Loop.BasisY);
    }

    // Here we _do_ use Affine3.
    SimilarityTransform(TrackGroup.InitialPlacement,
                        Affine3, Linear3x3, InverseLinear3x3);
}

void GRANNY
GetVectorDifferences(int32x VectorDimension,
                     int32x VectorCount,
                     real32 const *Vectors,
                     real32 const *Identity,
                     vector_diff_mode DiffMode,
                     real32 &IdentityDifference,
                     real32 &ConstantDifference)
{
    IdentityDifference = 0.0f;
    ConstantDifference = 0.0f;

    real32 const *RealIdentity = Identity;
    if ( Identity == NULL )
    {
        // Caller doesn't want to compare against an identity.
        // Splice in a placeholder
        Identity = GlobalZero16;
    }

    {for(int32x Index = 0;
         Index < VectorCount;
         ++Index)
    {
        real32 ThisIdentityDifference;
        real32 ThisConstantDifference;

        real32 const *CurrentVector = Vectors + Index*VectorDimension;

        switch (DiffMode)
        {
            case DiffAsQuaternions:
            {
                ThisConstantDifference = AbsoluteValue (
                    1.0f - InnerProductN(VectorDimension, CurrentVector, Vectors));
                ThisIdentityDifference = AbsoluteValue (
                    1.0f - InnerProductN(VectorDimension, CurrentVector, Identity));
            } break;

            case DiffAsVectors:
            {
                ThisConstantDifference = SquaredDistanceBetweenN (
                    VectorDimension, CurrentVector, Vectors);
                ThisIdentityDifference = SquaredDistanceBetweenN (
                    VectorDimension, CurrentVector, Identity);
            } break;

            case ManhattanMetric:
            {
                ThisConstantDifference = 0.0f;
                ThisIdentityDifference = 0.0f;
                {for(int32x ElementIndex = 0;
                     ElementIndex < VectorDimension;
                     ++ElementIndex)
                {
                    ThisConstantDifference += AbsoluteValue(CurrentVector[ElementIndex] -
                                                            Vectors[ElementIndex]);
                    ThisIdentityDifference += AbsoluteValue(CurrentVector[ElementIndex] -
                                                            Identity[ElementIndex]);
                }}
            } break;

            default:
            {
                ThisIdentityDifference = 0.0f;
                ThisConstantDifference = 0.0f;
                InvalidCodePath("Bad vector difference type!");
            } break;
        }


        IdentityDifference = Maximum(IdentityDifference, ThisIdentityDifference);
        ConstantDifference = Maximum(ConstantDifference, ThisConstantDifference);
    }}

    if ( DiffMode == DiffAsVectors )
    {
        // We actually just calculated the square of the error.
        ConstantDifference = SquareRoot ( ConstantDifference );
        IdentityDifference = SquareRoot ( IdentityDifference );
    }

    if ( RealIdentity == NULL )
    {
        // Caller doesn't want to compare against an identity.
        IdentityDifference = GetReal32AlmostInfinity();
    }
}

bool GRANNY
KnotsAreReducible(int32x Degree, int32x Dimension,
                  int32x KnotCount,
                  real32 const *Knots,
                  real32 const *Controls,
                  real32 const *Identity,
                  real32 Epsilon,
                  vector_diff_mode DiffMode,
                  int32x &RequiredDegree,
                  int32x &RequiredKnotCount)
{
    real32 IdentityDifference = 0.0f;
    real32 ConstantDifference = 0.0f;

    GetVectorDifferences(Dimension, KnotCount, Controls, Identity,
                         DiffMode, IdentityDifference, ConstantDifference);

    if(IdentityDifference < Epsilon)
    {
        // This isn't guaranteed, but it seems unlikely that the
        // curve solver would have created so many knots if so.
        //   arg: and in fact, it happens with fair regularity...
        //Assert ( KnotCount <= 4 );
        Assert ( Identity != NULL );
        RequiredDegree = 0;
        RequiredKnotCount = 0;
        return(true);
    }
    else if(ConstantDifference < Epsilon)
    {
        // This isn't guaranteed, but it seems unlikely that the
        // curve solver would have created so many knots if so.
        //   arg: and in fact, it happens with fair regularity...
        //Assert ( KnotCount <= 4 );
        RequiredDegree = 0;
        RequiredKnotCount = 1;
        return(true);
    }
    else
    {
        RequiredDegree    = Degree;
        RequiredKnotCount = KnotCount;
        return(false);
    }
}

bool GRANNY
CurveIsReducible(curve2 const *Curve,
                 real32 const *Identity,
                 real32 Epsilon,
                 vector_diff_mode DiffMode,
                 int32x &RequiredDegree,
                 int32x &RequiredKnotCount)
{
    CheckPointerNotNull(Curve, return false);

    // Extract the controls and check
    int CurveKnotCount = CurveGetKnotCount(*Curve);
    int CurveDimension = CurveGetDimension(*Curve);

    real32 *Controls = AllocateArray(CurveKnotCount * CurveDimension, real32);
    if (Controls == 0)
        return false;

    CurveExtractKnotValues(*Curve, 0, CurveKnotCount,
                           NULL, Controls, Identity);

    bool Result = KnotsAreReducible(CurveGetDegree(*Curve),
                                    CurveDimension,
                                    CurveKnotCount,
                                    NULL,
                                    Controls,
                                    Identity,
                                    Epsilon,
                                    DiffMode,
                                    RequiredDegree,
                                    RequiredKnotCount);

    Deallocate(Controls);

    return Result;
}

bool GRANNY
TransformTrackHasKeyframedCurves(transform_track const &Track)
{
    return ( CurveIsKeyframed(Track.PositionCurve) ||
             CurveIsKeyframed(Track.OrientationCurve) ||
             CurveIsKeyframed(Track.ScaleShearCurve) );
}

bool GRANNY
TransformTrackIsAnimated(transform_track const &Track)
{
    return ( !CurveIsConstantOrIdentity(Track.PositionCurve) ||
             !CurveIsConstantOrIdentity(Track.OrientationCurve) ||
             !CurveIsConstantOrIdentity(Track.ScaleShearCurve) );
}

bool GRANNY
TransformTrackIsIdentity(transform_track const &Track)
{
    return ( CurveIsIdentity(Track.PositionCurve) &&
             CurveIsIdentity(Track.OrientationCurve) &&
             CurveIsIdentity(Track.ScaleShearCurve) );
}

bool GRANNY
FindTrackByName(track_group const *TrackGroup, char const *TrackName,
                int32x &Result)
{
    COUNT_BLOCK("FindTrackByName");

    if(TrackGroup)
    {
        if(TrackGroup->Flags & TrackGroupIsSorted)
        {
            // We can do binary search
            int32x TrackCount = TrackGroup->TransformTrackCount;

            int32x TrackWindow = TrackCount;
            int32x TrackIndex = 0;
            while(1)
            {
                if((TrackWindow < 1) || (TrackIndex >= TrackCount))
                {
                    Result = -1;
                    return(false);
                }

                int32x HalfWay = TrackIndex + (TrackWindow / 2);
                char const *CompareName = TrackGroup->TransformTracks[HalfWay].Name;

                int32x const Diff = StringDifferenceOrCallback(TrackName, CompareName);

                if(Diff == 0)
                {
                    Result = HalfWay;
                    return(true);
                }
                else if(Diff > 0)
                {
                    TrackWindow -= (HalfWay - TrackIndex + 1);
                    TrackIndex = HalfWay + 1;
                }
                else if(Diff < 0)
                {
                    TrackWindow = HalfWay - TrackIndex;
                }
            }
        }
        else
        {
            // We have to do linear search
            {for(Result = 0;
                 Result < TrackGroup->TransformTrackCount;
                 ++Result)
            {
                if(StringsAreEqualOrCallback(TrackName,
                                             TrackGroup->TransformTracks[Result].Name))
                {
                    return(true);
                }
            }}
        }
    }

    Result = -1;
    return(false);
}

bool GRANNY
FindTrackByRule(track_group const *TrackGroup,
                char const *ProcessedTrackName,
                char const *TrackPattern,
                int32x &TrackIndex)
{
    COUNT_BLOCK("FindTrackByRule");

    char TrackNameBuffer[MaximumBoneNameLength + 1];
    if(TrackGroup)
    {
        {for(TrackIndex = 0;
             TrackIndex < TrackGroup->TransformTrackCount;
             ++TrackIndex)
        {
            WildCardMatch(TrackGroup->TransformTracks[TrackIndex].Name,
                          TrackPattern, TrackNameBuffer);
            if(StringsAreEqual(ProcessedTrackName,
                               TrackNameBuffer))
            {
                return(true);
            }
        }}
    }

    TrackIndex = -1;
    return(false);
}

void GRANNY
GetTrackGroupFlags(track_group const &TrackGroup,
                   uint32 &Flags, uint32 &VDADOFs)
{
    Flags = TrackGroup.Flags & 0xFFFF;
    VDADOFs = TrackGroup.Flags >> 16;
}

void GRANNY
SetTrackGroupFlags(track_group &TrackGroup,
                   uint32 Flags, uint32 VDADOFs)
{
    TrackGroup.Flags = Flags | (VDADOFs << 16);
}


bool GRANNY
FindVectorTrackByName(track_group const *TrackGroup, char const *TrackName,
                      int32x &Result)
{
    COUNT_BLOCK("FindVectorTrackByName");

    if(TrackGroup)
    {
        // Vector tracks are not sorted (yet?)

        // We have to do linear search
        {for(Result = 0;
                Result < TrackGroup->VectorTrackCount;
                ++Result)
        {
            if(StringsAreEqualOrCallback(TrackName,
                                         TrackGroup->VectorTracks[Result].Name))
            {
                return(true);
            }
        }}
    }

    Result = -1;
    return(false);
}

bool GRANNY
FindVectorTrackByRule(track_group const *TrackGroup,
                      char const *ProcessedTrackName,
                      char const *TrackPattern,
                      int32x &TrackIndex)
{
    COUNT_BLOCK("FindVectorTrackByRule");

    char TrackNameBuffer[MaximumBoneNameLength + 1];
    if (TrackGroup)
    {
        {for(TrackIndex = 0;
             TrackIndex < TrackGroup->VectorTrackCount;
             ++TrackIndex)
        {
            WildCardMatch(TrackGroup->VectorTracks[TrackIndex].Name,
                          TrackPattern, TrackNameBuffer);
            if (StringsAreEqual(ProcessedTrackName,
                                TrackNameBuffer))
            {
                return true;
            }
        }}
    }

    TrackIndex = -1;
    return false;
}

uint32 GRANNY
VectorTrackKeyForBone(skeleton& Skeleton,
                      int32x BoneIndex,
                      char const* TrackName)
{
    COUNT_BLOCK("VectorTrackKeyForBone");
    CheckBoundedInt32(0, BoneIndex, Skeleton.BoneCount, return 0);
    CheckPointerNotNull(TrackName, return 0);

    bone* WalkBone = &Skeleton.Bones[BoneIndex];
    Assert(WalkBone);

    uint32 CRC;
    BeginCRC32(CRC);
    for (;;)
    {
        AddToCRC32(CRC, StringLength(WalkBone->Name), WalkBone->Name);

        if (WalkBone->ParentIndex == NoParentBone)
            break;
        WalkBone = &Skeleton.Bones[WalkBone->ParentIndex];
    }

    AddToCRC32(CRC, StringLength(TrackName), TrackName);
    EndCRC32(CRC);
    return CRC;
}

