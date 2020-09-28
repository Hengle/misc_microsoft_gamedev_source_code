#if !defined(GRANNY_CURVE_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_curve.h $
// $DateTime: 2007/11/27 12:25:30 $
// $Change: 16620 $
// $Revision: #25 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(AnimationGroup);


// This the old granny_curve type.
EXPTYPE struct old_curve
{
    // Note that the dimension is implied by the owner (or
    // you can figure it out by dividing the control count by
    // the knot count)
    int32 Degree;

    int32 KnotCount;
    real32 *Knots;

    int32 ControlCount;
    real32 *Controls;
};
EXPCONST EXPGROUP(old_curve) extern data_type_definition OldCurveType[];


// The universal curve header.
EXPTYPE struct curve2
{
    variant CurveData;
};
EXPCONST EXPGROUP(curve2) extern data_type_definition Curve2Type[];


// And the various structures that can live inside CurveData.
// The naming convention is somewhat crufty.


// Every CurveData structure must begin with this, and it must be the first item, so that
// you can take CurveData.Object and cast it to a curve_data_header*, and be able to read
// this data.
EXPTYPE struct curve_data_header
{
    // This is re-set every load time by comparing CurveData.Type against a table.
    uint8 Format;
    uint8 Degree;
};
EXPCONST EXPGROUP(curve_data_header) extern data_type_definition CurveDataHeaderType[];



// This type is not a B-spline, just a bunch of keyframes.
EXPTYPE struct curve_data_da_keyframes32f
{
    curve_data_header CurveDataHeader;
    int16  Dimension;
    int32  ControlCount;
    real32 *Controls;
};
EXPCONST EXPGROUP(curve_data_da_keyframes32f) extern data_type_definition CurveDataDaKeyframes32fType[];


// "di" = dimension implied by ControlCount/KnotCount.
// "k32f" = knots are 32-bit floats.
// "c32f" = control components are 32-bit floats.
// This type is the direct equivalent of the old granny_curve, and is what they will be converted to on load.
// You can also ask to convert any curve type to this type.
// Note that this type can be a bunch of keyframes (if KnotCount==0 and ControlCount>0)
// or it can be the identity curve (if KnotCount==0 and ControlCount==0)
// or it can be a constant value (if KnotCount==1)
// but it's a good idea not to do this usually:
// If you want keyframes, use DaKeyframes
// If you want the identity curve, use DaIdentity
// If you want a constant, use DXConstant32f
// They're all faster and smaller that using this one.
EXPTYPE struct curve_data_da_k32f_c32f
{
    curve_data_header CurveDataHeader;
    int16 Padding;
    int32  KnotCount;
    real32 *Knots;
    int32  ControlCount;
    real32 *Controls;
};
EXPCONST EXPGROUP(curve_data_da_k32f_c32f) extern data_type_definition CurveDataDaK32fC32fType[];

// "di" = dimension implied by ControlScaleOffsetCount/2.
// "k16u" = knots are 16-bit unsigned ints, scaled by KnotScale
// "c16u" = control components are 16-bit unsigned ints, modified by ControlScale and ControlOffset
EXPTYPE struct curve_data_da_k16u_c16u
{
    curve_data_header CurveDataHeader;
    uint16 OneOverKnotScaleTrunc;   // Is actually the top 16 bits of the IEEE float!
    int32  ControlScaleOffsetCount; // Dimension = ControlScaleOffsetSize/2
    real32 *ControlScaleOffsets;    // First half are Scales, second half are Offsets
    int32  KnotControlCount;        // KnotCount + ControlCount (and we know that ControlCount = KnotCount * Dimension)
    uint16 *KnotsControls;          // First part is Knots, last part is Controls.
};
EXPCONST EXPGROUP(curve_data_da_k16u_c16u) extern data_type_definition CurveDataDaK16uC16uType[];

// "di" = dimension implied by ControlScaleOffsetCount/2.
// "k8u" = knots are 8-bit unsigned ints, scaled by KnotScale
// "c8u" = control components are 8-bit unsigned ints, modified by ControlScale and ControlOffset
EXPTYPE struct curve_data_da_k8u_c8u
{
    curve_data_header CurveDataHeader;
    uint16 OneOverKnotScaleTrunc;   // Is actually the top 16 bits of the IEEE float!
    int32  ControlScaleOffsetCount; // Dimension = ControlScaleOffsetSize/2
    real32 *ControlScaleOffsets;    // First half are Scales, second half are Offsets
    int32  KnotControlCount;        // KnotCount + ControlCount (and we know that ControlCount = KnotCount * Dimension)
    uint8  *KnotsControls;          // First part is Knots, last part is Controls.
};
EXPCONST EXPGROUP(curve_data_da_k8u_c8u) extern data_type_definition CurveDataDaK8uC8uType[];



// "d3" = 3 dimensional.
// "k16u" = knots are 16-bit unsigned ints, scaled by KnotScale
// "c16u" = control components are 16-bit unsigned ints (you get signed by using the Scale and Offset)
// This is a special case of curve_data_da_k16u_c16u, where because of naming we know its dimension.
// This saves 8 bytes per curve, which turns out to be significant.
EXPTYPE struct curve_data_d3_k16u_c16u
{
    curve_data_header CurveDataHeader;
    uint16 OneOverKnotScaleTrunc;   // Is actually the top 16 bits of the IEEE float!
    real32 ControlScales[3];
    real32 ControlOffsets[3];
    int32  KnotControlCount;    // KnotCount + ControlCount (and we know that ControlCount = KnotCount * 3)
    uint16 *KnotsControls;      // First quarter is Knots, last 3/4 is Controls.
};
EXPCONST EXPGROUP(curve_data_d3_k16u_c16u) extern data_type_definition CurveDataD3K16uC16uType[];

// "d3" = 3 dimensional.
// "k8u" = knots are 8-bit unsigned ints, scaled by KnotScale
// "c8u" = control components are 8-bit unsigned ints (you get signed by using the Scale and Offset)
// This is a special case of curve_data_da_k8u_c8u, where because of naming we know its dimension.
// This saves 8 bytes per curve, which turns out to be significant.
EXPTYPE struct curve_data_d3_k8u_c8u
{
    curve_data_header CurveDataHeader;
    uint16 OneOverKnotScaleTrunc;   // Is actually the top 16 bits of the IEEE float!
    real32 ControlScales[3];
    real32 ControlOffsets[3];
    int32  KnotControlCount;    // KnotCount + ControlCount (and we know that ControlCount = KnotCount * 3)
    uint8 *KnotsControls;       // First quarter is Knots, last 3/4 is Controls.
};
EXPCONST EXPGROUP(curve_data_d3_k8u_c8u) extern data_type_definition CurveDataD3K8uC8uType[];


// "d4n" = 4 dimensional, normalised
// "k16u" = knots are 16-bit unsigned ints, scaled by KnotScale
// "c15u" = control components are 15-bit unsigned ints (you get signed by using the Scale and Offset)
//      Format is 15:1:15:1:15:1, and the extra three bits tell you which of the original
//      four components was discarded, and what its sign was, so it can be recovered during
//      the normalisation process.
//      Specifically, if the three uint16s are numbered 0, 1, 2 in memory, and the high bits
//      are numbered the same way, then:
//      bit 0: if 0, missing result is +ve. If 1, missing result is -ve
//      bit 1, bit 2: form a 2-bit value of 0-3 (bit 1 is the MSB, bit 2 is the LSB), saying which
//        of the four original values was discarded. The three specified values then follow on
//        afterwards in order, modulo 3. So for example if bit1=0, bit2=1, then component 1 (the Y)
//        is missing. The three uint16s Data[0], [1] and [2] then correspond to Z component = Data[0],
//        W component = Data[1], X component = Data[2].
//      Current default encoding is to always throw away the largest component, but this
//      is not mandatory (and may not be true in future, it may try throwing away each of the four
//      and seeing which generates the least error).
//      Note that there are four Scale+Offsets, even though we only store three of the components.
//      So you take the three, decode which one is missing, swizzle them into their correct places,
//      then apply Scale and Offset, then calculate the missing one. Basically, you don't want
//      to have a single Scale and Offset being applied to more than one axis, or you might as well
//      just have a single Scale and Offset that applied to all axes and save the memory!
EXPTYPE struct curve_data_d4n_k16u_c15u
{
    curve_data_header CurveDataHeader;
    uint16 ScaleOffsetTableEntries; // Four 4-bit fields to specify entries in a table of scales and offsets, one per dimension.
    real32 OneOverKnotScale;
    int32  KnotControlCount;    // KnotCount + ControlCount (and we know that ControlCount = KnotCount * 3)
    uint16 *KnotsControls;      // First quarter is Knots, last 3/4 is Controls.
};
EXPCONST EXPGROUP(curve_data_d4n_k16u_c15u) extern data_type_definition CurveDataD4nK16uC15uType[];

// "d4n" = 4 dimensional, normalised
// "k8u" = knots are 8-bit unsigned ints, scaled by KnotScale
// "c7u" = control components are 7-bit unsigned ints (you get signed by using the Scale and Offset)
//      Format is 7:1:7:1:7:1, and the extra three bits tell you which of the original
//      four components was discarded, and what its sign was, so it can be recovered during
//      the normalisation process.
// Format is essentially the same as curve_data_d4n_k16u_c15u, just with fewer bits.
EXPTYPE struct curve_data_d4n_k8u_c7u
{
    curve_data_header CurveDataHeader;
    uint16 ScaleOffsetTableEntries; // Four 4-bit fields to specify entries in a table of scales and offsets, one per dimension.
    real32 OneOverKnotScale;
    int32  KnotControlCount;    // KnotCount + ControlCount (and we know that ControlCount = KnotCount * 3)
    uint8  *KnotsControls;      // First quarter is Knots, last 3/4 is Controls.
};
EXPCONST EXPGROUP(curve_data_d4n_k8u_c7u) extern data_type_definition CurveDataD4nK8uC7uType[];

// If actually sampled, this will return all-zeros.
// However, calling code can also call CurveIsIdentity, and this curve will produce TRUE.
// This allows the unit quaternion and identity scale/shear to be "stored".
EXPTYPE struct curve_data_da_identity
{
    curve_data_header CurveDataHeader;
    int16  Dimension;
};
EXPCONST EXPGROUP(curve_data_da_identity) extern data_type_definition CurveDataDaIdentityType[];

// A constant value.
// If sampled, will return the value.
// You can call CurveIsConstant, and this curve type will return TRUE.
EXPTYPE struct curve_data_da_constant32f
{
    curve_data_header CurveDataHeader;
    int16 Padding;
    int32  ControlCount;            // Same as Dimension
    real32 *Controls;
};
EXPCONST EXPGROUP(curve_data_da_constant32f) extern data_type_definition CurveDataDaConstant32fType[];

// Special versions for 3 and 4-dimensional data.
// This saves a surprising amount of space!
EXPTYPE struct curve_data_d3_constant32f
{
    curve_data_header CurveDataHeader;
    int16 Padding;
    real32 Controls[3];
};
EXPCONST EXPGROUP(curve_data_d3_constant32f) extern data_type_definition CurveDataD3Constant32fType[];

EXPTYPE struct curve_data_d4_constant32f
{
    curve_data_header CurveDataHeader;
    int16 Padding;
    real32 Controls[4];
};
EXPCONST EXPGROUP(curve_data_d4_constant32f) extern data_type_definition CurveDataD4Constant32fType[];

// "d9i1" = 9-dimensional implemented as 1-d
// "k16u" = knots are 16-bit unsigned ints, scaled by KnotScale
// "c16u" = control components are 16-bit unsigned ints (you get signed by using the Scale and Offset)
// This represents a 3x3 scale/shear matrix as a uniform scale
EXPTYPE struct curve_data_d9i1_k16u_c16u
{
    curve_data_header CurveDataHeader;
    uint16 OneOverKnotScaleTrunc;   // Is actually the top 16 bits of the IEEE float!
    real32 ControlScale;
    real32 ControlOffset;
    int32  KnotControlCount;    // KnotCount + ControlCount (and we know that ControlCount = KnotCount)
    uint16 *KnotsControls;      // First half is Knots, last half is Controls.
};
EXPCONST EXPGROUP(curve_data_d9i1_k16u_c16u) extern data_type_definition CurveDataD9I1K16uC16uType[];

// "d9i3" = 9-dimensional implemented as 3-d
// "k16u" = knots are 16-bit unsigned ints, scaled by KnotScale
// "c16u" = control components are 16-bit unsigned ints (you get signed by using the Scale and Offset)
// This represents a 3x3 scale/shear matrix as a uniform scale
EXPTYPE struct curve_data_d9i3_k16u_c16u
{
    curve_data_header CurveDataHeader;
    uint16 OneOverKnotScaleTrunc;   // Is actually the top 16 bits of the IEEE float!
    real32 ControlScales[3];
    real32 ControlOffsets[3];
    int32  KnotControlCount;    // KnotCount + ControlCount (and we know that ControlCount = KnotCount)
    uint16 *KnotsControls;      // First half is Knots, last half is Controls.
};
EXPCONST EXPGROUP(curve_data_d9i3_k16u_c16u) extern data_type_definition CurveDataD9I3K16uC16uType[];

// "d9i1" = 9-dimensional implemented as 1-d
// "k8u" = knots are 8-bit unsigned ints, scaled by KnotScale
// "c8u" = control components are 8-bit unsigned ints (you get signed by using the Scale and Offset)
// This represents a 3x3 scale/shear matrix as a uniform scale
EXPTYPE struct curve_data_d9i1_k8u_c8u
{
    curve_data_header CurveDataHeader;
    uint16 OneOverKnotScaleTrunc;   // Is actually the top 16 bits of the IEEE float!
    real32 ControlScale;
    real32 ControlOffset;
    int32  KnotControlCount;    // KnotCount + ControlCount (and we know that ControlCount = KnotCount)
    uint8  *KnotsControls;      // First half is Knots, last half is Controls.
};
EXPCONST EXPGROUP(curve_data_d9i1_k8u_c8u) extern data_type_definition CurveDataD9I1K8uC8uType[];

// "d9i3" = 9-dimensional implemented as 3-d
// "k8u" = knots are 8-bit unsigned ints, scaled by KnotScale
// "c8u" = control components are 8-bit unsigned ints (you get signed by using the Scale and Offset)
// This represents a 3x3 scale/shear matrix as a uniform scale
EXPTYPE struct curve_data_d9i3_k8u_c8u
{
    curve_data_header CurveDataHeader;
    uint16 OneOverKnotScaleTrunc;   // Is actually the top 16 bits of the IEEE float!
    real32 ControlScales[3];
    real32 ControlOffsets[3];
    int32  KnotControlCount;    // KnotCount + ControlCount (and we know that ControlCount = KnotCount)
    uint8  *KnotsControls;      // First half is Knots, last half is Controls.
};
EXPCONST EXPGROUP(curve_data_d9i3_k8u_c8u) extern data_type_definition CurveDataD9I3K8uC8uType[];


// "d3i1" = 3-dimensional data implemented as 1-d
// "k32f" = 32-bit float knots
// "c32f" = 32-bit float controls
// This represents points that can be represented by a linear function of one variable)
EXPTYPE struct curve_data_d3i1_k32f_c32f
{
    curve_data_header CurveDataHeader;
    int16 Padding;
    real32 ControlScales[3];
    real32 ControlOffsets[3];
    int32  KnotControlCount;    // KnotCount + ControlCount (and we know that ControlCount = KnotCount)
    real32 *KnotsControls;      // First half is Knots, last half is Controls.
};
EXPCONST EXPGROUP(curve_data_d3i1_k32f_c32f) extern data_type_definition CurveDataD3I1K32fC32fType[];


// "d3i1" = 3-dimensional data implemented as 1-d
// "k16u" = 16-bit unsigned int knots
// "c16u" = 16-bit unsigned int controls
// This represents points that can be represented by a linear function of one variable)
EXPTYPE struct curve_data_d3i1_k16u_c16u
{
    curve_data_header CurveDataHeader;
    uint16 OneOverKnotScaleTrunc;
    real32 ControlScales[3];
    real32 ControlOffsets[3];
    int32  KnotControlCount;    // KnotCount + ControlCount (and we know that ControlCount = KnotCount)
    uint16 *KnotsControls;      // First half is Knots, last half is Controls.
};
EXPCONST EXPGROUP(curve_data_d3i1_k16u_c16u) extern data_type_definition CurveDataD3I1K16uC16uType[];

// "d3i1" = 3-dimensional data implemented as 1-d
// "k8u" = 16-bit unsigned int knots
// "c8u" = 16-bit unsigned int controls
// This represents points that can be represented by a linear function of one variable)
EXPTYPE struct curve_data_d3i1_k8u_c8u
{
    curve_data_header CurveDataHeader;
    uint16 OneOverKnotScaleTrunc;
    real32 ControlScales[3];
    real32 ControlOffsets[3];
    int32  KnotControlCount;    // KnotCount + ControlCount (and we know that ControlCount = KnotCount)
    uint8 *KnotsControls;       // First half is Knots, last half is Controls.
};
EXPCONST EXPGROUP(curve_data_d3i1_k8u_c8u) extern data_type_definition CurveDataD3I1K8uC8uType[];

// nb: removed hinge orientation curve types, can be found in db. 2007/11/27


#if DEBUG
void CurveParanoiaChecking ( curve2 const &Curve );
#else
#define CurveParanoiaChecking(ignored)
#endif

EXPAPI GS_PARAM void CurveInitializeFormat(curve2 *Curve);
EXPAPI GS_READ bool CurveFormatIsInitializedCorrectly(curve2 const &Curve, bool CheckTypes);

EXPAPI GS_READ bool CurveIsKeyframed(curve2 const &Curve);
EXPAPI GS_READ bool CurveIsIdentity(curve2 const &Curve);
EXPAPI GS_READ bool CurveIsConstantOrIdentity(curve2 const &Curve);
EXPAPI GS_READ bool CurveIsConstantNotIdentity(curve2 const &Curve);

EXPAPI GS_READ int32x CurveGetKnotCount(curve2 const &Curve);
EXPAPI GS_READ int32x CurveGetDimension(curve2 const &Curve);
int32x CurveGetDimensionUnchecked(curve2 const &Curve);
EXPAPI GS_READ int32x CurveGetDegree(curve2 const &Curve);
EXPAPI GS_READ data_type_definition const *CurveGetDataTypeDefinition(curve2 const &Curve);

// Building a curve.
EXPTYPE struct curve_builder;
EXPAPI GS_SAFE curve_builder *BeginCurve(data_type_definition const *TypeDefinition,
                                         int32x Degree, int32x Dimension,
                                         int32x KnotCount);
EXPAPI GS_SAFE curve_builder *BeginCurveCopy(curve2 const &SourceCurve);
EXPAPI GS_PARAM void PushCurveKnotArray(curve_builder *Builder, real32 const *KnotArray);
EXPAPI GS_PARAM void PushCurveControlArray(curve_builder *Builder, real32 const *ControlArray);
EXPAPI GS_PARAM void PushCurveSampleArrays(curve_builder *Builder,
                                           int32x SampleCount, int32x Dimension,
                                           real32 const *TransformedSamples,
                                           real32 const *OriginalSamples);
EXPAPI GS_READ int32x GetResultingCurveSize(curve_builder const *Builder);
EXPAPI GS_PARAM curve2 *EndCurveInPlace(curve_builder *Builder, void *Memory);
EXPAPI GS_PARAM curve2 *EndCurve(curve_builder *Builder);
EXPAPI GS_PARAM void AbortCurveBuilder(curve_builder *Builder);
EXPAPI GS_PARAM void FreeCurve(curve2 *Curve);


// If you already have memory defined for the curve2 (e.g. it's static or part
// of an existing granny_animation or something), and you just want to allocate
// the variant part of the curve, use this pair instead of the above.
EXPAPI GS_READ int32x GetResultingCurveDataSize(curve_builder const *Builder);
// The return value will be the same as Curve of course...
EXPAPI GS_PARAM curve2 *EndCurveDataInPlace(curve_builder *Builder,
                                            curve2 *Curve,
                                            void *CurveDataMemory);



struct curve_builder
{
    bool CurveBuilderNeedsFreeing;
    data_type_definition *TypeDefinition;
    int32x FormatEnum;
    int32x Degree;
    int32x Dimension;
    int32x KnotCount;
    real32 const *KnotArray;
    real32 const *ControlArray;
    curve2 const *SourceCurve;

    int32x SampleCount;
    int32x SampleDimension;
    real32 const *TransformedSamples;
    real32 const *OriginalSamples;
};

// Allows the curve to massage the data into a better format for
// compression, before the spline least squares fitter is run.  Mostly
// this is for the rotational dimension-reduced formats, though it
// might expand in the future.  If the curve transforms the data, it
// is responsible for reextracting the /same/ transform in SetAllKnotValues
void TransformSamplesForCurve(data_type_definition const *TypeDefinition,
                              int32x Dimension, int32x SampleCount,
                              real32 *Samples);

void BeginCurveInPlace(curve_builder *Builder,
                       data_type_definition const *TypeDefinition,
                       int32x Degree, int32x Dimension, int32x KnotCount);
void BeginCurveCopyInPlace(curve_builder *Builder, curve2 const &SourceCurve);



// Handy conversion functions from any arbitrary compressed curve to an easily-readable one.
EXPAPI GS_READ bool CurveIsTypeDaK32fC32f(curve2 const &SrcCurve);
// And this checks and returns the actual curve data. If the curve is not of type DaK32fC32f, it will return NULL!
// This is a nice safe way of taking CurveData and casting it.
EXPAPI GS_PARAM curve_data_da_k32f_c32f *CurveGetContentsOfDaK32fC32f(curve2 const &SrcCurve);

EXPAPI GS_PARAM curve2 *CurveConvertToDaK32fC32f(curve2 const &SrcCurve, real32 const *IdentityVector);
EXPAPI GS_READ int32x GetResultingDaK32fC32fCurveSize(curve2 const &SrcCurve);
EXPAPI GS_PARAM curve2 *CurveConvertToDaK32fC32fInPlace(curve2 const &SrcCurve, void *Memory, real32 const *IdentityVector);



// Specialised GetResultingCurveDataSize and EndCurveDataInPlace for DaK32fC32f.
// This needs to be a fairly fast path, since it's used for rebasing.
EXPAPI GS_PARAM void CurveMakeStaticDaK32fC32f(curve2 *Curve,
                                               curve_data_da_k32f_c32f *CurveData,
                                               int32x KnotCount,
                                               int32x Degree, int32x Dimension,
                                               real32 const *Knots,
                                               real32 const *Controls);
int32x GetResultingDaK32fC32fCurveDataSize(int32x KnotCount, int32 Dimension);
void CurveCreateDaK32fC32fInPlace(int32x KnotCount, int32x Degree, int32 Dimension, curve2 *Curve, void *CurveDataMemory);

int32x CurveFindFormatFromDefinition(data_type_definition const &CurveTypeDefinition);


// Returns the value of the indexed knot value and corresponding control value.
// ControlResult can be NULL if you wish, but otherwise must be an array of at least as many dimensions as the curve has.
EXPAPI GS_READ real32 CurveExtractKnotValue(curve2 const &Curve, int32x KnotIndex, real32 *ControlResult, real32 const *IdentityVector);

// Returns a series of knot values and controls, starting at index KnotIndexStart and going on for KnotCount knots.
// KnotResults must be an array of size KnotCount, and ControlResults must be an array of size KnotCount*Dimension.
EXPAPI GS_READ void CurveExtractKnotValues(curve2 const &Curve,
                                           int32x KnotIndexStart, int32x KnotCount,
                                           real32 *KnotResults, real32 *ControlResults,
                                           real32 const *IdentityVector);

// Sets the contents of an existing curve. Note that all the contents
// have to be set at once, because compressed curves cannot be
// partially updated. KnotCount and Dimension must match the existing
// curve values. To change then, you need to create an entirely new
// curve.
EXPAPI GS_PARAM void CurveSetAllKnotValues(curve2 *Curve,
                                           int32x KnotCount, int32x Dimension,
                                           real32 const *KnotSrc,
                                           real32 const *ControlSrc);


// Allows you to swizzle, scale and offset the various channels of the curve
// without losing any significant precision.
// Scales, Offsets and Swizzles are all vectors of size Dimension.
// Swizzles[Dim] holds the dimension that this dimension should be taken from,
// e.g. Swizzles[3] = 1 means that the W dimension (the 3rd one) should be copied from
// the original's Y dimension (the 1st one) and then scaled and offset by Scales[3] and Offsets[3].
// Note that some formats (notably the D3n and D4n ones) only allow a scale of +1 or -1, and an offset of 0.
EXPAPI GS_PARAM void CurveScaleOffsetSwizzle(curve2 *Curve,
                                             int32x Dimension,
                                             real32 const *Scales, real32 const *Offsets,
                                             int32x const *Swizzles);


EXPAPI GS_READ int32x CurveFindKnot(curve2 const &Curve, real32 t);
EXPAPI GS_READ int32x CurveFindCloseKnot(curve2 const &Curve, real32 t, int32x StartingKnotIndex);

// Equivalent to the GetResultingCurveSize of the builder
EXPAPI GS_READ int32 CurveGetSize(curve2 const &Curve);


// (0,0,0)
EXPCONST extern real32 CurveIdentityPosition[];
// (0,0,1)
EXPCONST extern real32 CurveIdentityNormal[];
// (0,0,0,1)
EXPCONST extern real32 CurveIdentityOrientation[];
// |1,0,0|
// |0,1,0|
// |0,0,1|
EXPCONST extern real32 CurveIdentityScaleShear[];
// (1,1,1)
EXPCONST extern real32 CurveIdentityScale[];
// |0,0,0|
// |0,0,0|
EXPCONST extern real32 CurveIdentityShear[];

inline int32x
CurveGetDegreeInline(curve2 const &Curve)
{
    curve_data_header* CurveDataHeader = (curve_data_header*)Curve.CurveData.Object;
    return ( CurveDataHeader->Degree );
}

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_CURVE_H
#endif
