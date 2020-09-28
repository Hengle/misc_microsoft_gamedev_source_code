// ========================================================================
// $File: //jeffr/granny/rt/granny_curve.cpp $
// $DateTime: 2007/11/27 12:25:30 $
// $Change: 16620 $
// $Revision: #45 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_CURVE_H)
#include "granny_curve.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_FIND_KNOT_H)
#include "granny_find_knot.h"
#endif

#if !defined(GRANNY_CONVERSIONS_H)
#include "granny_conversions.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif

#if !defined(GRANNY_FLOATS_H)
#include "granny_floats.h"
#endif

#if !defined(GRANNY_QUATERNION_SCALEOFFSET_H)
#include "granny_quaternion_scaleoffset.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode CurveLogMessage


USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;


// You can turn this on, but everything slows to a real crawl in debug mode.  It's also
// undefined when you may have called FreeFileSection(..., StandardDiscardableSection),
// since it looks at the types of the curve.  Use MILD_PARANOIA if you are calling
// FreeFileSection.
#define EXCESSIVE_PARANOIA 0

// This is supported when FreeFileSection is called, but will still slow everything down
#define MILD_PARANOIA 0


#if EXCESSIVE_PARANOIA && defined(DEBUG)
#define PARANOID_CURVE_CHECK(curve) Assert ( CurveFormatIsInitializedCorrectly ( curve, true ) )
#elif MILD_PARANOIA && defined(DEBUG)
#define PARANOID_CURVE_CHECK(curve) Assert ( CurveFormatIsInitializedCorrectly ( curve, false ) )
#else
#define PARANOID_CURVE_CHECK(curve)
#endif


#if defined(_MSC_VER)
#if !GRANNY_SHOW_HARMLESS_WARNINGS
// Because a lot of this file is cut'n'paste and macros and stuff, these warnings get generated all over the place.
#pragma warning( disable : 4189 )   // warning C4189: 'CurveDimension' : local variable is initialized but not referenced
#pragma warning( disable : 4100 )   // warning C4100: 'IdentityVector' : unreferenced formal parameter
#endif
#endif



// This the old granny_curve type.
data_type_definition OldCurveType[] =
{
    {Int32Member, "Degree"},

    {ReferenceToArrayMember, "Knots", Real32Type},
    {ReferenceToArrayMember, "Controls", Real32Type},

    {EndMember},
};


data_type_definition Curve2Type[] =
{
    {VariantReferenceMember, "CurveData"},

    {EndMember},
};


data_type_definition CurveDataHeaderType[] =
{
    {UInt8Member, "Format"},
    {UInt8Member, "Degree"},
    {EndMember},
};


// Note that all CurveData*Types include a CurveDataHeaderType as their first item,
// and it is tagged with the the name of the CurveData type itself.
// This allows DataTypesAreEqualWithNames to tell them apart, even if the components
// are the same types (e.g CurveDataD3nK16uC14uType and CurveDataD3K16uC16uType).
// This is vital because the two are decompressed in different ways!

// This type is not a B-spline, just a bunch of keyframes.
data_type_definition CurveDataDaKeyframes32fType[] =
{
    {InlineMember, "CurveDataHeader_DaKeyframes32f", CurveDataHeaderType},
    {Int16Member, "Dimension"},
    {ReferenceToArrayMember, "Controls", Real32Type},

    {EndMember},
};


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
data_type_definition CurveDataDaK32fC32fType[] =
{
    {InlineMember, "CurveDataHeader_DaK32fC32f", CurveDataHeaderType},
    {Int16Member, "Padding"},
    {ReferenceToArrayMember, "Knots", Real32Type},
    {ReferenceToArrayMember, "Controls", Real32Type},

    {EndMember},
};

data_type_definition CurveDataDaK16uC16uType[] =
{
    {InlineMember, "CurveDataHeader_DaK16uC16u", CurveDataHeaderType},
    {UInt16Member, "OneOverKnotScaleTrunc"},
    {ReferenceToArrayMember, "ControlScaleOffsets", Real32Type},
    {ReferenceToArrayMember, "KnotsControls", UInt16Type},

    {EndMember},
};

data_type_definition CurveDataDaK8uC8uType[] =
{
    {InlineMember, "CurveDataHeader_DaK8uC8u", CurveDataHeaderType},
    {UInt16Member, "OneOverKnotScaleTrunc"},
    {ReferenceToArrayMember, "ControlScaleOffsets", Real32Type},
    {ReferenceToArrayMember, "KnotsControls", UInt8Type},

    {EndMember},
};

data_type_definition CurveDataD3K16uC16uType[] =
{
    {InlineMember, "CurveDataHeader_D3K16uC16u", CurveDataHeaderType},
    {UInt16Member, "OneOverKnotScaleTrunc"},
    {Real32Member, "ControlScales", 0, 3},
    {Real32Member, "ControlOffsets", 0, 3},
    {ReferenceToArrayMember, "KnotsControls", UInt16Type},

    {EndMember},
};

data_type_definition CurveDataD3K8uC8uType[] =
{
    {InlineMember, "CurveDataHeader_D3K8uC8u", CurveDataHeaderType},
    {UInt16Member, "OneOverKnotScaleTrunc"},
    {Real32Member, "ControlScales", 0, 3},
    {Real32Member, "ControlOffsets", 0, 3},
    {ReferenceToArrayMember, "KnotsControls", UInt8Type},

    {EndMember},
};


data_type_definition CurveDataD4nK16uC15uType[] =
{
    {InlineMember, "CurveDataHeader_D4nK16uC15u", CurveDataHeaderType},
    {UInt16Member, "ScaleOffsetTableEntries"},
    {Real32Member, "OneOverKnotScale"},
    {ReferenceToArrayMember, "KnotsControls", UInt16Type},

    {EndMember},
};

data_type_definition CurveDataD4nK8uC7uType[] =
{
    {InlineMember, "CurveDataHeader_D4nK8uC7u", CurveDataHeaderType},
    {UInt16Member, "ScaleOffsetTableEntries"},
    {Real32Member, "OneOverKnotScale"},
    {ReferenceToArrayMember, "KnotsControls", UInt8Type},

    {EndMember},
};

data_type_definition CurveDataDaIdentityType[] =
{
    {InlineMember, "CurveDataHeader_DaIdentity", CurveDataHeaderType},
    {Int16Member, "Dimension"},
    {EndMember},
};

data_type_definition CurveDataDaConstant32fType[] =
{
    {InlineMember, "CurveDataHeader_DaConstant32f", CurveDataHeaderType},
    {Int16Member, "Padding"},
    {ReferenceToArrayMember, "Controls", Real32Type},
    {EndMember},
};

data_type_definition CurveDataD3Constant32fType[] =
{
    {InlineMember, "CurveDataHeader_D3Constant32f", CurveDataHeaderType},
    {Int16Member, "Padding"},
    {Real32Member, "Controls", 0, 3},
    {EndMember},
};

data_type_definition CurveDataD4Constant32fType[] =
{
    {InlineMember, "CurveDataHeader_D4Constant32f", CurveDataHeaderType},
    {Int16Member, "Padding"},
    {Real32Member, "Controls", 0, 4},
    {EndMember},
};

data_type_definition CurveDataD9I1K16uC16uType[] =
{
    {InlineMember, "CurveDataHeader_D9I1K16uC16u", CurveDataHeaderType},
    {UInt16Member, "OneOverKnotScaleTrunc"},
    {Real32Member, "ControlScale"},
    {Real32Member, "ControlOffset"},
    {ReferenceToArrayMember, "KnotsControls", UInt16Type},

    {EndMember},
};
data_type_definition CurveDataD9I3K16uC16uType[] =
{
    {InlineMember, "CurveDataHeader_D9I3K16uC16u", CurveDataHeaderType},
    {UInt16Member, "OneOverKnotScaleTrunc"},
    {Real32Member, "ControlScales", 0, 3},
    {Real32Member, "ControlOffsets", 0, 3},
    {ReferenceToArrayMember, "KnotsControls", UInt16Type},

    {EndMember},
};

data_type_definition CurveDataD9I1K8uC8uType[] =
{
    {InlineMember, "CurveDataHeader_D9I1K8uC8u", CurveDataHeaderType},
    {UInt16Member, "OneOverKnotScaleTrunc"},
    {Real32Member, "ControlScale"},
    {Real32Member, "ControlOffset"},
    {ReferenceToArrayMember, "KnotsControls", UInt8Type},

    {EndMember},
};
data_type_definition CurveDataD9I3K8uC8uType[] =
{
    {InlineMember, "CurveDataHeader_D9I3K8uC8u", CurveDataHeaderType},
    {UInt16Member, "OneOverKnotScaleTrunc"},
    {Real32Member, "ControlScales", 0, 3},
    {Real32Member, "ControlOffsets", 0, 3},
    {ReferenceToArrayMember, "KnotsControls", UInt8Type},

    {EndMember},
};

data_type_definition CurveDataD3I1K32fC32fType[] =
{
    {InlineMember, "CurveDataHeader_D3I1K32fC32f", CurveDataHeaderType},
    {UInt16Member, "Padding"},
    {Real32Member, "ControlScales", 0, 3},
    {Real32Member, "ControlOffsets", 0, 3},
    {ReferenceToArrayMember, "KnotsControls", Real32Type},

    {EndMember},
};

data_type_definition CurveDataD3I1K16uC16uType[] =
{
    {InlineMember, "CurveDataHeader_D3I1K16uC16u", CurveDataHeaderType},
    {UInt16Member, "OneOverKnotScaleTrunc"},
    {Real32Member, "ControlScales", 0, 3},
    {Real32Member, "ControlOffsets", 0, 3},
    {ReferenceToArrayMember, "KnotsControls", UInt16Type},

    {EndMember},
};

data_type_definition CurveDataD3I1K8uC8uType[] =
{
    {InlineMember, "CurveDataHeader_D3I1K8uC8u", CurveDataHeaderType},
    {UInt16Member, "OneOverKnotScaleTrunc"},
    {Real32Member, "ControlScales", 0, 3},
    {Real32Member, "ControlOffsets", 0, 3},
    {ReferenceToArrayMember, "KnotsControls", UInt8Type},

    {EndMember},
};

// data_type_definition CurveDataD4nI1K16uC16uType[] =
// {
//     {InlineMember, "CurveDataHeader_D4nI1K16uC16u", CurveDataHeaderType},
//     {UInt16Member, "OneOverKnotScaleTrunc"},
//     {UInt16Member, "BaseQuaternion", 0, 4},
//     {UInt16Member, "RotationAxis", 0, 3},
//     {UInt16Member, "ValueRange"},
//  {Real32Member, "ValueStart"},
//     {ReferenceToArrayMember, "KnotsControls", UInt16Type},

//     {EndMember},
// };

// data_type_definition CurveDataD4nI1K8uC8uType[] =
// {
//     {InlineMember, "CurveDataHeader_D4nI1K8uC8u", CurveDataHeaderType},
//     {UInt16Member, "OneOverKnotScaleTrunc"},
//     {UInt16Member, "BaseQuaternion", 0, 4},
//     {UInt16Member, "RotationAxis", 0, 3},
//     {UInt16Member, "ValueRange"},
//  {Real32Member, "ValueStart"},
//     {ReferenceToArrayMember, "KnotsControls", UInt8Type},

//     {EndMember},
// };

// If you add a curve type, you must add an entry to CurveTypeTable to deal with it.



// Now the big table of types and functions and so on.
// TODO: split this table up into common calls and uncommon calls, to minimise
// the chance of the common ones falling out of the cache (and minimise the penalty when they do).
struct curve_type_table
{
    data_type_definition *TypeDefinition;
    char *ReadableName;

    int32x (*GetDimension) ( curve2 const &Curve );
    int32x (*GetKnotCount) ( curve2 const &Curve );

    int32x (*FindKnot) ( curve2 const &Curve, real32 t );
    int32x (*FindCloseKnot) ( curve2 const &Curve, real32 t, int32x StartingIndex );

    void (*ExtractKnotValues) ( curve2 const &Curve,
                                int32x KnotIndexStart,
                                int32x KnotCount,
                                real32* NOALIAS KnotResults,
                                real32* NOALIAS ControlResults,
                                real32 const* NOALIAS IdentityVector );
    void (*TransformSamples) (int32x SampleCount, int32x Dimension,
                              real32 *Samples );
    void (*NoteSampleTransform) (curve2 &Curve,
                                 int32x SampleCount, int32x Dimension,
                                 real32 const *OriginalSamples );
    void (*SetAllKnotValues) ( curve2 &Curve,
                               int32x KnotCount, int32x Dimension,
                               real32 const *KnotSrc, real32 const *ControlSrc );

    void (*AggrCurveData) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData );
    void (*CopyCurve) ( curve2 &Curve, curve2 const &SourceCurve);

    void (*ScaleOffsetSwizzle) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles );

    // Add extra functions here, and to the TABLE_ENTRY macro, and to the DECLARE_FUNCTIONS macro.
};


#define DECLARE_FUNCTIONS(name)                                                             \
    int32x CurveGetDimensionUnchecked##name ( curve2 const &Curve );                        \
    int32x CurveGetKnotCount##name ( curve2 const &Curve );                                 \
    int32x CurveFindKnot##name ( curve2 const &Curve, real32 t );                           \
    int32x CurveFindCloseKnot##name ( curve2 const &Curve, real32 t,                        \
                                      int32x StartingIndex );                               \
    void CurveExtractKnotValues##name ( curve2 const &Curve,                                \
                                        int32x KnotIndexStart, int32x KnotCount,            \
                                        real32 * NOALIAS KnotResults,                       \
                                        real32 * NOALIAS ControlResults,                    \
                                        real32 const * NOALIAS IdentityVector);             \
    void CurveTransformSamples##name (int32x SampleCount, int32x Dimension,                 \
                                      real32 *Samples);                                     \
    void CurveNoteSampleTransform##name (curve2 &Curve,                                     \
                                         int32x SampleCount, int32x Dimension,              \
                                         real32 const *OriginalSamples);                    \
    void CurveSetAllKnotValues##name ( curve2 &Curve,                                       \
                                       int32x KnotCount, int32x Dimension,                  \
                                       real32 const *KnotSrc, real32 const *ControlSrc );   \
    void CurveAggrCurveData##name (aggr_allocator &Allocator,                               \
                                   curve_builder const *Builder,                            \
                                   void *&CurveData);                                       \
    void CurveCopyCurve##name ( curve2 &Curve, curve2 const &SourceCurve);                  \
    void CurveScaleOffsetSwizzle##name ( curve2 &Curve, int32x Dimension,                   \
                                         real32 const *Scales, real32 const *Offsets,       \
                                         int32x const *Swizzles )

// Add extra functions here, and to the curve_type_table structure.
// (remember the slash characters, and make sure there's a blank line above this comment
// block).

DECLARE_FUNCTIONS(DaKeyframes32f);
DECLARE_FUNCTIONS(DaK32fC32f);
DECLARE_FUNCTIONS(DaIdentity);
DECLARE_FUNCTIONS(DaConstant32f);
DECLARE_FUNCTIONS(D3Constant32f);
DECLARE_FUNCTIONS(D4Constant32f);
DECLARE_FUNCTIONS(DaK16uC16u);
DECLARE_FUNCTIONS(DaK8uC8u);
DECLARE_FUNCTIONS(D4nK16uC15u);
DECLARE_FUNCTIONS(D4nK8uC7u);
DECLARE_FUNCTIONS(D3K16uC16u);
DECLARE_FUNCTIONS(D3K8uC8u);
DECLARE_FUNCTIONS(D9I1K16uC16u);
DECLARE_FUNCTIONS(D9I3K16uC16u);
DECLARE_FUNCTIONS(D9I1K8uC8u);
DECLARE_FUNCTIONS(D9I3K8uC8u);
DECLARE_FUNCTIONS(D3I1K32fC32f);
DECLARE_FUNCTIONS(D3I1K16uC16u);
DECLARE_FUNCTIONS(D3I1K8uC8u);
// DECLARE_FUNCTIONS(D4nI1K16uC16u);
// DECLARE_FUNCTIONS(D4nI1K8uC8u);

#undef DECLARE_FUNCTIONS

// Nobody mention the word "Vtable", OK?
#define TABLE_ENTRY(name) {                     \
        CurveData##name##Type,                  \
            #name,                              \
            CurveGetDimensionUnchecked##name,   \
            CurveGetKnotCount##name,            \
            CurveFindKnot##name,                \
            CurveFindCloseKnot##name,           \
            CurveExtractKnotValues##name,       \
            CurveTransformSamples##name,        \
            CurveNoteSampleTransform##name,     \
            CurveSetAllKnotValues##name,        \
            CurveAggrCurveData##name,           \
            CurveCopyCurve##name,               \
            CurveScaleOffsetSwizzle##name,      \
            }


// Special positions in the table.
enum
{
    CurveTypeTableEnum_DaKeyframes32f = 0,
    CurveTypeTableEnum_DaK32fC32f,
    CurveTypeTableEnum_DaIdentity,
    CurveTypeTableEnum_DaConstant32f,   // All the Constant entries must be contiguous.
    CurveTypeTableEnum_D3Constant32f,
    CurveTypeTableEnum_D4Constant32f,
};

const int CurveTypeTableEnum_FirstConstant = CurveTypeTableEnum_DaConstant32f;
const int CurveTypeTableEnum_LastConstant  = CurveTypeTableEnum_D4Constant32f;


// If you move, add or subtract entries from here, you MUST
// change the file version number. These enums get re-set when
// the file is loaded (it looks for the matching data_type_definition), but
// only if a different file version number actually triggered a re-parse.
curve_type_table CurveTypeTable[] =
{
    TABLE_ENTRY(DaKeyframes32f),        // Must always be first
    TABLE_ENTRY(DaK32fC32f),
    TABLE_ENTRY(DaIdentity),
    TABLE_ENTRY(DaConstant32f),         // The Constant entries must be contiguous.
    TABLE_ENTRY(D3Constant32f),
    TABLE_ENTRY(D4Constant32f),
    // All the above entries must be in the same order as the CurveTypeTable_... enum above

    TABLE_ENTRY(DaK16uC16u),
    TABLE_ENTRY(DaK8uC8u),
    TABLE_ENTRY(D4nK16uC15u),
    TABLE_ENTRY(D4nK8uC7u),
    TABLE_ENTRY(D3K16uC16u),
    TABLE_ENTRY(D3K8uC8u),

    TABLE_ENTRY(D9I1K16uC16u),
    TABLE_ENTRY(D9I3K16uC16u),
    TABLE_ENTRY(D9I1K8uC8u),
    TABLE_ENTRY(D9I3K8uC8u),

    TABLE_ENTRY(D3I1K32fC32f),
    TABLE_ENTRY(D3I1K16uC16u),
    TABLE_ENTRY(D3I1K8uC8u),

//  TABLE_ENTRY(D4nI1K16uC16u),
//  TABLE_ENTRY(D4nI1K8uC8u),
};

#undef TABLE_ENTRY

END_GRANNY_NAMESPACE;

#if DEBUG
void GRANNY
CurveParanoiaChecking ( curve2 const &Curve )
{
    // This is just about as much paranoia as I can pack into a single routine!
    PARANOID_CURVE_CHECK ( Curve );
    int32x KnotCount = CurveGetKnotCount ( Curve );
    int32x Dimension = 1;
    if ( KnotCount > 0 )
    {
        Dimension = CurveGetDimensionUnchecked ( Curve );
    }
    int32x Degree = CurveGetDegree ( Curve );
    Assert ( Degree >= 0 );
    Assert ( Degree <= 3 );
    Assert ( Dimension > 0 );
    Assert ( Dimension <= MaximumBSplineDimension );
    if ( CurveIsKeyframed ( Curve ) )
    {
        // Not really sure about these....
        Assert ( Degree == 0 );
        Assert ( KnotCount > 0 );
    }
    else if ( CurveIsIdentity ( Curve ) )
    {
        Assert ( Degree == 0 );
        Assert ( KnotCount == 0 );
    }
    else if ( CurveIsConstantOrIdentity ( Curve ) )
    {
        Assert ( Degree == 0 );
        Assert ( KnotCount == 1 );
    }
    else
    {
        Assert ( Degree >= 0 );
        Assert ( KnotCount > 1 );
        // And check the knot values are monotonic.
        real32 CurKnotValue = 0.0f;
        {for ( int32x KnotNum = 0; KnotNum < KnotCount; KnotNum++ )
        {
            real32 NewKnotValue = CurveExtractKnotValue ( Curve, KnotNum, NULL, NULL );
            Assert ( NewKnotValue >= CurKnotValue );
            CurKnotValue = NewKnotValue;
        }}
    }
}
#endif

int32x GRANNY
CurveFindFormatFromDefinition(data_type_definition const &CurveTypeDefinition)
{
    // Just a simple linear check through the list.
    {for ( int32x FormatNum = 0; FormatNum < ArrayLength ( CurveTypeTable ); FormatNum++ )
    {
        // We check with the type names so you can tell things like curve_data_d3n_k16u_c14u and curve_data_d3_k16u_c16u apart.
        if ( DataTypesAreEqualWithNames ( CurveTypeTable[FormatNum].TypeDefinition, &CurveTypeDefinition ) )
        {
            return FormatNum;
        }
    }}
    InvalidCodePath ( "Unknown curve type" );
    return -1;
}

int32x
CurveFindFormatFromCurve(curve2 const &Curve)
{
    return ( CurveFindFormatFromDefinition ( *Curve.CurveData.Type ) );
}


void GRANNY
CurveInitializeFormat(curve2 *Curve)
{
    int32x FormatNum = CurveFindFormatFromCurve ( *Curve );
    Assert ( FormatNum >= 0 );
    curve_data_header *CurveDataHeader = (curve_data_header *)Curve->CurveData.Object;
    CurveDataHeader->Format = (uint8)FormatNum;
}

bool GRANNY
CurveFormatIsInitializedCorrectly(curve2 const &Curve, bool CheckTypes)
{
    if ( Curve.CurveData.Object == NULL )
    {
        return false;
    }
    curve_data_header *CurveDataHeader = (curve_data_header *)Curve.CurveData.Object;
    int32x CurveFormat = CurveDataHeader->Format;

    if ( CurveFormat < 0 )
    {
        return false;
    }
    else if ( CurveFormat >= ArrayLength ( CurveTypeTable ) )
    {
        return false;
    }
    else if (CheckTypes)
    {
        // The current format value is in range, but is it right?
        return ( DataTypesAreEqualWithNames ( CurveTypeTable[CurveFormat].TypeDefinition, Curve.CurveData.Type ) );
    }
    else
    {
        return true;
    }
}


bool GRANNY
CurveIsKeyframed(curve2 const &Curve)
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveTypeTable[CurveTypeTableEnum_DaKeyframes32f].TypeDefinition == CurveDataDaKeyframes32fType );
    Assert ( CurveTypeTable[CurveTypeTableEnum_DaK32fC32f].TypeDefinition == CurveDataDaK32fC32fType );
    curve_data_header *CurveDataHeader = (curve_data_header *)Curve.CurveData.Object;
    int32x CurveFormat = CurveDataHeader->Format;
    if ( CurveFormat == CurveTypeTableEnum_DaKeyframes32f )
    {
        return true;
    }
    if ( CurveFormat == CurveTypeTableEnum_DaK32fC32f )
    {
        curve_data_da_k32f_c32f *CurveDataRaw = (curve_data_da_k32f_c32f *)(Curve.CurveData.Object);
        if ( ( CurveDataRaw->KnotCount == 0 ) && ( CurveDataRaw->ControlCount > 0 ) )
        {
            return true;
        }
    }
    return false;
}

bool GRANNY
CurveIsIdentity(curve2 const &Curve)
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveTypeTable[CurveTypeTableEnum_DaIdentity].TypeDefinition == CurveDataDaIdentityType );
    Assert ( CurveTypeTable[CurveTypeTableEnum_DaK32fC32f].TypeDefinition == CurveDataDaK32fC32fType );
    curve_data_header *CurveDataHeader = (curve_data_header *)Curve.CurveData.Object;
    int32x CurveFormat = CurveDataHeader->Format;
    if ( CurveFormat == CurveTypeTableEnum_DaIdentity )
    {
        return true;
    }
    if ( CurveFormat == CurveTypeTableEnum_DaK32fC32f )
    {
        curve_data_da_k32f_c32f *CurveDataRaw = (curve_data_da_k32f_c32f *)(Curve.CurveData.Object);
        if ( ( CurveDataRaw->KnotCount == 0 ) && ( CurveDataRaw->ControlCount == 0 ) )
        {
            return true;
        }
    }
    return false;
}

bool GRANNY
CurveIsConstantOrIdentity(curve2 const &Curve)
{
    PARANOID_CURVE_CHECK ( Curve );
    // Change these if necessary.
    Assert ( CurveTypeTableEnum_FirstConstant == CurveTypeTableEnum_DaConstant32f );
    Assert ( CurveTypeTableEnum_LastConstant  == CurveTypeTableEnum_D4Constant32f );
    Assert ( CurveTypeTable[CurveTypeTableEnum_DaConstant32f].TypeDefinition == CurveDataDaConstant32fType );
    Assert ( CurveTypeTable[CurveTypeTableEnum_D3Constant32f].TypeDefinition == CurveDataD3Constant32fType );
    Assert ( CurveTypeTable[CurveTypeTableEnum_D4Constant32f].TypeDefinition == CurveDataD4Constant32fType );

    Assert ( CurveTypeTable[CurveTypeTableEnum_DaIdentity].TypeDefinition == CurveDataDaIdentityType );
    Assert ( CurveTypeTable[CurveTypeTableEnum_DaK32fC32f].TypeDefinition == CurveDataDaK32fC32fType );

    curve_data_header *CurveDataHeader = (curve_data_header *)Curve.CurveData.Object;
    int32x CurveFormat = CurveDataHeader->Format;
    if ( CurveFormat == CurveTypeTableEnum_DaIdentity )
    {
        return true;
    }
    if ( ( CurveFormat >= CurveTypeTableEnum_FirstConstant ) && ( CurveFormat <= CurveTypeTableEnum_LastConstant ) )
    {
        return true;
    }
    if ( CurveFormat == CurveTypeTableEnum_DaK32fC32f )
    {
        curve_data_da_k32f_c32f *CurveDataRaw = (curve_data_da_k32f_c32f *)(Curve.CurveData.Object);
        if ( CurveDataRaw->KnotCount == 0 )
        {
            if ( CurveDataRaw->ControlCount == 0 )
            {
                // Identity.
                return true;
            }
            else
            {
                // Keyframed.
                return false;
            }
        }
        else if ( CurveDataRaw->KnotCount == 1 )
        {
            // Constant value.
            return true;
        }
        else
        {
            // A B-spline curve.
            return false;
        }
    }
    return false;
}

bool GRANNY
CurveIsConstantNotIdentity(curve2 const &Curve)
{
    return ( CurveIsConstantOrIdentity ( Curve ) && !CurveIsIdentity ( Curve ) );
}


int32x GRANNY
CurveGetKnotCount(curve2 const &Curve)
{
    PARANOID_CURVE_CHECK ( Curve );
    curve_data_header *CurveDataHeader = (curve_data_header *)Curve.CurveData.Object;
    int32x CurveFormat = CurveDataHeader->Format;
    return ( CurveTypeTable[CurveFormat].GetKnotCount ( Curve ) );
}

int32x GRANNY
CurveGetDimensionUnchecked(curve2 const &Curve)
{
    PARANOID_CURVE_CHECK ( Curve );
    curve_data_header *CurveDataHeader = (curve_data_header *)Curve.CurveData.Object;
    int32x CurveFormat = CurveDataHeader->Format;
    return ( CurveTypeTable[CurveFormat].GetDimension ( Curve ) );
}

int32x GRANNY
CurveGetDimension(curve2 const &Curve)
{
    if ( CurveGetKnotCount ( Curve ) == 0 )
    {
        // Might have resulted in a divide-by-zero.
        return 0;
    }
    else
    {
        return CurveGetDimensionUnchecked ( Curve );
    }
}


int32x GRANNY
CurveGetDegree(curve2 const &Curve)
{
    return CurveGetDegreeInline(Curve);
}

data_type_definition const *GRANNY
CurveGetDataTypeDefinition(curve2 const &Curve)
{
    return Curve.CurveData.Type;
}



void GRANNY
TransformSamplesForCurve(data_type_definition const *TypeDefinition,
                         int32x Dimension, int32x SampleCount,
                         real32 *Samples)
{
    int32x Format = CurveFindFormatFromDefinition ( *TypeDefinition );
    if ( Format < 0 )
    {
        Log0 ( ErrorLogMessage, CurveLogMessage, "Invalid format specified for a granny_curve2 - creating this curve will fail" );
        return;
    }

    CurveTypeTable[Format].TransformSamples(SampleCount, Dimension, Samples);
}


void GRANNY
BeginCurveInPlace(curve_builder *Builder, data_type_definition const *TypeDefinition, int32x Degree, int32x Dimension, int32x KnotCount)
{
    if ( Builder != NULL )
    {
        Builder->CurveBuilderNeedsFreeing = false;
        // De-constify the thing.
        Builder->TypeDefinition = (data_type_definition *)TypeDefinition;
        Builder->FormatEnum = CurveFindFormatFromDefinition ( *Builder->TypeDefinition );
        if ( Builder->FormatEnum < 0 )
        {
            Log0 ( ErrorLogMessage, CurveLogMessage, "Invalid format specified for a granny_curve2 - creating this curve will fail" );
            // ...but it doesn't fail just yet :-)
        }
        Builder->Degree = Degree;
        Builder->Dimension = Dimension;
        Builder->KnotCount = KnotCount;
        Builder->KnotArray = NULL;
        Builder->ControlArray = NULL;
        Builder->SourceCurve = NULL;

        Builder->SampleCount = -1;
        Builder->SampleDimension = -1;
        Builder->TransformedSamples = NULL;
        Builder->OriginalSamples  = NULL;
    }
}

void GRANNY
BeginCurveCopyInPlace(curve_builder *Builder, curve2 const &SourceCurve)
{
    if ( Builder != NULL )
    {
        Assert ( CurveFormatIsInitializedCorrectly ( SourceCurve, true ) );
        BeginCurveInPlace (
            Builder,
            SourceCurve.CurveData.Type,
            CurveGetDegree ( SourceCurve ),
            CurveGetDimension ( SourceCurve ),
            CurveGetKnotCount ( SourceCurve ) );
        Builder->SourceCurve = &SourceCurve;
    }
}


curve_builder *GRANNY
BeginCurve(data_type_definition const *TypeDefinition, int32x Degree, int32x Dimension, int32x KnotCount )
{
    curve_builder *Builder = NULL;

    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    AggrAllocPtr(Allocator, Builder);
    if(EndAggrAlloc(Allocator))
    {
        BeginCurveInPlace ( Builder, TypeDefinition, Degree, Dimension, KnotCount );
        Builder->CurveBuilderNeedsFreeing = true;
    }
    return Builder;
}

curve_builder *GRANNY
BeginCurveCopy(curve2 const &SourceCurve)
{
    Assert ( CurveFormatIsInitializedCorrectly ( SourceCurve, true ) );
    curve_builder *Builder = BeginCurve (
        SourceCurve.CurveData.Type,
        CurveGetDegree ( SourceCurve ),
        CurveGetDimension ( SourceCurve ),
        CurveGetKnotCount ( SourceCurve ) );
    if ( Builder != NULL )
    {
        Builder->SourceCurve = &SourceCurve;
        Builder->CurveBuilderNeedsFreeing = true;
    }
    return Builder;
}

void GRANNY
PushCurveKnotArray(curve_builder *Builder, real32 const *KnotArray)
{
    if ( Builder != NULL )
    {
        Builder->SourceCurve = NULL;
        Builder->KnotArray = KnotArray;
    }
}

void GRANNY
PushCurveSampleArrays(curve_builder *Builder,
                      int32x SampleCount, int32x Dimension,
                      real32 const *TransformedSamples,
                      real32 const *OriginalSamples)
{
    if ( Builder != NULL )
    {
        Builder->SampleCount = SampleCount;
        Builder->SampleDimension = Dimension;
        Builder->TransformedSamples = TransformedSamples;
        Builder->OriginalSamples  = OriginalSamples;
    }
}

void GRANNY
PushCurveControlArray(curve_builder *Builder, real32 const *ControlArray)
{
    if ( Builder != NULL )
    {
        Builder->SourceCurve = NULL;
        Builder->ControlArray = ControlArray;
    }
}


static void
AggrCurveData(aggr_allocator &Allocator,
              curve_builder const *Builder,
              curve2 *&Curve, void *&CurveData)
{
    if ( Builder != NULL )
    {
        if ( Builder->FormatEnum < 0 )
        {
            // Invalid type.
            Assert ( false );
            return;
        }

        CurveTypeTable[Builder->FormatEnum].AggrCurveData(Allocator, Builder, CurveData);
    }
}

static void
AggrCurve(aggr_allocator &Allocator,
          curve_builder const *Builder,
          curve2 *&Curve,
          void *&CurveData)
{
    if ( Builder != NULL )
    {
        if ( Builder->FormatEnum < 0 )
        {
            // Invalid type.
            Assert ( false );
            return;
        }

        AggrAllocPtr(Allocator, Curve);
        AggrCurveData(Allocator, Builder, Curve, CurveData);
    }
}



int32x GRANNY
GetResultingCurveSize(curve_builder const *Builder)
{
    if ( Builder != NULL )
    {
        if ( Builder->FormatEnum < 0 )
        {
            Log0 ( ErrorLogMessage, CurveLogMessage, "Invalid format specified for a granny_curve2 - returning size of 0" );
            return 0;
        }

        aggr_allocator Allocator;
        InitializeAggrAlloc(Allocator);

        curve2 *Ignored;
        void *Ignored2;
        AggrCurve ( Allocator, Builder, Ignored, Ignored2 );

        int32x ResultingSize;
        CheckConvertToInt32(ResultingSize, EndAggrSize(Allocator), return 0);
        return ResultingSize;
    }
    else
    {
        return 0;
    }
}

int32x GRANNY
GetResultingCurveDataSize(curve_builder const *Builder)
{
    if ( Builder != NULL )
    {
        if ( Builder->FormatEnum < 0 )
        {
            Log0 ( ErrorLogMessage, CurveLogMessage, "Invalid format specified for a granny_curve2 - returning size of 0" );
            return 0;
        }

        aggr_allocator Allocator;
        InitializeAggrAlloc(Allocator);

        curve2 *Ignored;
        void *Ignored2;
        AggrCurveData ( Allocator, Builder, Ignored, Ignored2 );

        int32x ResultingSize;
        CheckConvertToInt32(ResultingSize, EndAggrSize(Allocator), return 0);
        return ResultingSize;
    }
    else
    {
        return 0;
    }
}


static curve2 *
EndCurveDataInPlaceInternal(curve_builder *Builder, curve2 *Curve,
                            void *Memory, bool AllocateCurveMemory)
{
    if ( Curve != NULL )
    {
        // Safety net - if all else fails, *Curve will still be "valid",
        // in that it can be saved and freed without explosions.
        Curve->CurveData.Object = NULL;
        Curve->CurveData.Type = NULL;
    }

    if ( Builder == NULL )
    {
        Log0 ( ErrorLogMessage, CurveLogMessage, "Invalid curve_builder address - returning NULL" );
        return NULL;
    }

    if ( Builder->FormatEnum < 0 )
    {
        // Invalid format for a curve.
        Log0 ( ErrorLogMessage, CurveLogMessage, "Invalid format specified for a granny_curve2 - returning NULL" );
        if ( Builder->CurveBuilderNeedsFreeing )
        {
            Deallocate ( Builder );
        }
        return NULL;
    }

    if ( Memory == NULL )
    {
        Log0 ( ErrorLogMessage, CurveLogMessage, "Out of memory or invalid address for curve - returning NULL" );
        if ( Builder->CurveBuilderNeedsFreeing )
        {
            Deallocate ( Builder );
        }
        return NULL;
    }

    if ( !AllocateCurveMemory && ( Curve == NULL ) )
    {
        Log0 ( ErrorLogMessage, CurveLogMessage, "Out of memory or invalid address for curve - returning NULL" );
        if ( Builder->CurveBuilderNeedsFreeing )
        {
            Deallocate ( Builder );
        }
        return NULL;
    }

    if ( Builder->SourceCurve != NULL )
    {
        // I know how to copy an existing curve.
    }
    else if ( ( Builder->ControlArray != NULL ) && ( Builder->KnotArray != NULL ) )
    {
        // Both arrays present - that's fine.
    }
    else if ( ( Builder->ControlArray != NULL ) && ( Builder->KnotArray == NULL ) )
    {
        if ( Builder->KnotCount == 1 )
        {
            // One knot, no knot array, so they want a constant curve.
            // The knot is implicitly at time 0.0
        }
        else
        {
            if ( ( Builder->Degree == 0 ) && ( Builder->FormatEnum == CurveTypeTableEnum_DaKeyframes32f ) )
            {
                // Controls but no knots, and multiple "knots" = keyframed data.
            }
            else
            {
                Log0 ( ErrorLogMessage, CurveLogMessage, "Unknown curve type - returning NULL" );
                if ( Builder->CurveBuilderNeedsFreeing )
                {
                    Deallocate ( Builder );
                }
                return NULL;
            }
        }
    }
    else if ( Builder->KnotCount == 0 )
    {
        // No knots, so this is the identity curve. Don't need any data.
    }
    else
    {
        Log0 ( ErrorLogMessage, CurveLogMessage, "Need either a source curve to copy, or both knot and control arrays - returning NULL" );
        if ( Builder->CurveBuilderNeedsFreeing )
        {
            Deallocate ( Builder );
        }
        return NULL;
    }


    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    if ( AllocateCurveMemory )
    {
        // Allocating both Curve and CurveData.
        Assert ( Curve == NULL );
        Curve = NULL;
        AggrAllocPtr(Allocator, Curve);
    }
    else
    {
        // Curve already points to some good memory I hope.
        Assert ( Curve != NULL );
    }
    void *CurveData = NULL;
    // Allocate the CurveData part.
    AggrCurveData ( Allocator, Builder, Curve, CurveData );
    if ( EndAggrPlacement ( Allocator, Memory ) )
    {
        Curve->CurveData.Type = Builder->TypeDefinition;
        // The aggregation system is unable to set this pointer up for us, which is a shame.
        Curve->CurveData.Object = CurveData;

        curve_data_header *CurveDataHeader = (curve_data_header *)Curve->CurveData.Object;
        CurveDataHeader->Degree = (uint8)Builder->Degree;
        CurveDataHeader->Format = (uint8)Builder->FormatEnum;
        int32x CurveFormat = Builder->FormatEnum;

        // Make sure Format and CurveData.Type agree.
        Assert ( CurveFormatIsInitializedCorrectly ( *Curve, true ) );

        if (Builder->SampleCount != -1 && Builder->OriginalSamples != NULL)
        {
            CurveTypeTable[CurveFormat].NoteSampleTransform(*Curve,
                                                            Builder->SampleCount, Builder->SampleDimension,
                                                            Builder->OriginalSamples);
        }

        if ( Builder->SourceCurve != NULL )
        {
            Assert ( ( Builder->KnotArray == NULL ) && ( Builder->ControlArray == NULL ) );
            CurveTypeTable[CurveFormat].CopyCurve ( *Curve, *Builder->SourceCurve );
        }
        else if ( Builder->FormatEnum == CurveTypeTableEnum_DaKeyframes32f )
        {
            // Keyframed data.
            Assert ( Builder->Degree == 0 );
            Assert ( Builder->KnotCount > 1 );
            Assert ( Builder->KnotArray == NULL );
            CurveTypeTable[CurveFormat].SetAllKnotValues ( *Curve,
                                                           Builder->KnotCount, Builder->Dimension,
                                                           Builder->KnotArray, Builder->ControlArray );
        }
        else if ( Builder->KnotCount == 0 )
        {
            // Identity curve. Still need to call SetAllKnotValues to make sure it's initialised.
            real32 FakeKnotArray = 0.0f;
            if ( Builder->KnotArray == NULL )
            {
                Builder->KnotArray = &FakeKnotArray;
            }
            CurveTypeTable[CurveFormat].SetAllKnotValues ( *Curve, Builder->KnotCount, Builder->Dimension, Builder->KnotArray, Builder->ControlArray );
        }
        else if ( Builder->ControlArray != NULL )
        {
            // Copy the data into the CurveData chunk.
            real32 FakeKnotArray = 0.0f;
            if ( Builder->KnotArray == NULL )
            {
                Assert ( Builder->KnotCount == 1 );
                Builder->KnotArray = &FakeKnotArray;
            }
            CurveTypeTable[CurveFormat].SetAllKnotValues ( *Curve, Builder->KnotCount, Builder->Dimension, Builder->KnotArray, Builder->ControlArray );
        }
        else
        {
            InvalidCodePath ( "Don't know how to make this curve" );
        }
    }

    if ( Builder->CurveBuilderNeedsFreeing )
    {
        Deallocate ( Builder );
    }
    CurveParanoiaChecking ( *Curve );
    return Curve;
}

curve2 *GRANNY
EndCurveDataInPlace(curve_builder *Builder, curve2 *Curve, void *CurveDataMemory)
{
    if ( Builder == NULL )
    {
        return NULL;
    }
    curve2 *Result = EndCurveDataInPlaceInternal ( Builder, Curve, CurveDataMemory, false );
    Assert ( Result == Curve );
    return Result;
}

curve2 *GRANNY
EndCurveInPlace(curve_builder *Builder, void *Memory)
{
    return EndCurveDataInPlaceInternal ( Builder, NULL, Memory, true );
}

curve2 *GRANNY
EndCurve(curve_builder *Builder)
{
    if ( Builder == NULL )
    {
        return NULL;
    }

    if ( Builder->FormatEnum < 0 )
    {
        // Invalid format for a curve.
        Log0 ( ErrorLogMessage, CurveLogMessage, "Invalid format specified for a granny_curve2 - returning NULL" );
        if ( Builder->CurveBuilderNeedsFreeing )
        {
            Deallocate ( Builder );
        }
        return NULL;
    }

    curve2 *Curve = NULL;
    int32x TotalSize = GetResultingCurveSize ( Builder );
    void *CurveMemory = AllocateSize ( TotalSize );
    Curve = EndCurveInPlace ( Builder, CurveMemory );
    if ( Curve == NULL )
    {
        // Erk... that didn't go so well. Clean up.
        if ( CurveMemory != NULL )
        {
            Deallocate ( CurveMemory );
        }
    }
    return Curve;
}

void GRANNY
AbortCurveBuilder(curve_builder *Builder)
{
    if ( Builder != NULL )
    {
        if ( Builder->CurveBuilderNeedsFreeing )
        {
            Deallocate ( Builder );
        }
    }
}

void GRANNY
FreeCurve(curve2 *Curve)
{
    Deallocate ( Curve );
}




bool GRANNY
CurveIsTypeDaK32fC32f(curve2 const &SrcCurve)
{
    PARANOID_CURVE_CHECK ( SrcCurve );
    Assert ( CurveTypeTable[CurveTypeTableEnum_DaK32fC32f].TypeDefinition == CurveDataDaK32fC32fType );
    curve_data_header *CurveDataHeader = (curve_data_header *)SrcCurve.CurveData.Object;
    int32x CurveFormat = CurveDataHeader->Format;
    return ( CurveFormat == CurveTypeTableEnum_DaK32fC32f );
}

curve_data_da_k32f_c32f *GRANNY
CurveGetContentsOfDaK32fC32f(curve2 const &SrcCurve)
{
    if ( CurveIsTypeDaK32fC32f ( SrcCurve ) )
    {
        return (curve_data_da_k32f_c32f *)(SrcCurve.CurveData.Object);
    }
    else
    {
        // Not the right type!
        return NULL;
    }
}

curve2 *GRANNY
CurveConvertToDaK32fC32f(curve2 const &SrcCurve, real32 const *IdentityVector)
{
    return CurveConvertToDaK32fC32fInPlace ( SrcCurve,
                                             AllocateSize ( GetResultingDaK32fC32fCurveSize ( SrcCurve ) ),
                                             IdentityVector);
}

int32x GRANNY
GetResultingDaK32fC32fCurveSize(curve2 const &SrcCurve)
{
    int32x TotalSize = sizeof ( curve2 );
    int32x Dimension = 0;
    if ( !CurveIsIdentity ( SrcCurve ) )
    {
        Dimension = CurveGetDimensionUnchecked ( SrcCurve );
    }
    TotalSize += GetResultingDaK32fC32fCurveDataSize (
        CurveGetKnotCount ( SrcCurve ),
        Dimension );

#if DEBUG
    // Make sure this value is right by checking with the slower normal creation mechanism.
    curve_builder Builder;
    Builder.KnotCount = CurveGetKnotCount ( SrcCurve );
    Builder.Dimension = Dimension;
    Builder.TypeDefinition = CurveDataDaK32fC32fType;
    Builder.FormatEnum = CurveTypeTableEnum_DaK32fC32f;

    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    curve2 *Ignored;
    void *Ignored2;
    AggrCurve ( Allocator, &Builder, Ignored, Ignored2 );
    Assert ( EndAggrSize(Allocator) == TotalSize );
#endif

    return TotalSize;
}

curve2 *GRANNY
CurveConvertToDaK32fC32fInPlace(curve2 const &SrcCurve, void *Memory, real32 const *IdentityVector)
{
    int32x KnotCount = CurveGetKnotCount ( SrcCurve );
    int32x Dimension = CurveGetDimension ( SrcCurve );
    int32x Degree = CurveGetDegree ( SrcCurve );
    curve2 *TheCurve = (curve2*)Memory;
    curve_data_da_k32f_c32f *TheCurveData = (curve_data_da_k32f_c32f*)( (uint8*)Memory + sizeof ( curve2 ) );
    CurveCreateDaK32fC32fInPlace ( KnotCount, Degree, Dimension, TheCurve, TheCurveData );
    CurveExtractKnotValues ( SrcCurve, 0, KnotCount, TheCurveData->Knots, TheCurveData->Controls, IdentityVector );
    CurveParanoiaChecking ( *TheCurve );
    return TheCurve;
}

void GRANNY
CurveMakeStaticDaK32fC32f ( curve2 *Curve, curve_data_da_k32f_c32f *CurveData,
                            int32x KnotCount, int32x Degree, int32x Dimension,
                            real32 const *Knots, real32 const *Controls )
{
    // Some basic sanity checking. These are not strictly true, but
    // should catch the crazier bugs.
    Assert ( KnotCount >= 0 );
    Assert ( KnotCount < 0x10000 );
    Assert ( Dimension >= 0 );
    Assert ( Dimension <= MaximumBSplineDimension );
    Assert ( Degree >= 0 );
    Assert ( Degree <= 3 );

    Curve->CurveData.Object = CurveData;
    Curve->CurveData.Type = CurveDataDaK32fC32fType;
    curve_data_header *CurveDataHeader = (curve_data_header *)Curve->CurveData.Object;
    CurveDataHeader->Degree = (uint8)Degree;
    CurveDataHeader->Format = CurveTypeTableEnum_DaK32fC32f;
    CurveData->KnotCount = KnotCount;
    CurveData->Knots = (real32*)Knots;
    CurveData->ControlCount = KnotCount * Dimension;
    CurveData->Controls = (real32*)Controls;
    Assert ( CurveFormatIsInitializedCorrectly ( *Curve, true ) );
    CurveParanoiaChecking ( *Curve );
}



int32x GRANNY
GetResultingDaK32fC32fCurveDataSize(int32x KnotCount, int32 Dimension)
{
    int32x NumberOfBytes = sizeof (curve_data_da_k32f_c32f);
    NumberOfBytes += KnotCount * sizeof ( ((curve_data_da_k32f_c32f *)0)->Knots[0] );
    NumberOfBytes += KnotCount * Dimension * sizeof ( ((curve_data_da_k32f_c32f *)0)->Controls[0] );

#if DEBUG
    // Make sure this value is right by checking with the slower normal creation mechanism.
    curve_builder Builder;
    Builder.KnotCount = KnotCount;
    Builder.Dimension = Dimension;
    Builder.TypeDefinition = CurveDataDaK32fC32fType;
    Builder.FormatEnum = CurveTypeTableEnum_DaK32fC32f;

    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);
    curve2 *Ignored;
    void *Ignored2;
    AggrCurveData ( Allocator, &Builder, Ignored, Ignored2 );
    Assert ( EndAggrSize(Allocator) == NumberOfBytes );
#endif
    return NumberOfBytes;
}

void GRANNY
CurveCreateDaK32fC32fInPlace(int32x KnotCount, int32x Degree, int32 Dimension, curve2 *Curve, void *CurveDataMemory)
{
    curve_data_da_k32f_c32f *CurveData;

    Assert ( KnotCount >= 1 );

    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);
    AggrAllocPtr ( Allocator, CurveData );
    AggrAllocOffsetArrayPtr ( Allocator, CurveData, KnotCount, KnotCount, Knots );
    AggrAllocOffsetArrayPtr ( Allocator, CurveData, KnotCount * Dimension, ControlCount, Controls );
    Curve->CurveData.Object = EndAggrPlacement ( Allocator, CurveDataMemory );

    Curve->CurveData.Type = CurveDataDaK32fC32fType;
    curve_data_header *CurveDataHeader = (curve_data_header *)Curve->CurveData.Object;
    CurveDataHeader->Degree = (uint8)Degree;
    CurveDataHeader->Format = CurveTypeTableEnum_DaK32fC32f;
    SetReal32(KnotCount, 0.0f, CurveData->Knots);
    SetReal32(KnotCount*Dimension, 0.0f, CurveData->Controls);
    Assert ( CurveData->KnotCount == KnotCount );
    Assert ( CurveData->ControlCount == KnotCount * Dimension );
    Assert ( CurveFormatIsInitializedCorrectly ( *Curve, true ) );
    CurveParanoiaChecking ( *Curve );
}


real32 GRANNY
CurveExtractKnotValue(curve2 const &Curve, int32x KnotIndex, real32 *ControlResult, real32 const *IdentityVector)
{
    PARANOID_CURVE_CHECK ( Curve );
    curve_data_header *CurveDataHeader = (curve_data_header *)Curve.CurveData.Object;
    int32x CurveFormat = CurveDataHeader->Format;
    real32 KnotValue = 0.0f;
    CurveTypeTable[CurveFormat].ExtractKnotValues ( Curve, KnotIndex, 1, &KnotValue, ControlResult, IdentityVector );
    return KnotValue;
}

void GRANNY
CurveExtractKnotValues(curve2 const &Curve, int32x KnotIndexStart, int32x KnotCount, real32 *KnotResults, real32 *ControlResults, real32 const *IdentityVector)
{
    PARANOID_CURVE_CHECK ( Curve );
    curve_data_header *CurveDataHeader = (curve_data_header *)Curve.CurveData.Object;
    int32x CurveFormat = CurveDataHeader->Format;
    CurveTypeTable[CurveFormat].ExtractKnotValues ( Curve, KnotIndexStart, KnotCount, KnotResults, ControlResults, IdentityVector );
}


void GRANNY
CurveSetAllKnotValues(curve2 *Curve,
                      int32x KnotCount, int32x Dimension, real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( *Curve );
    curve_data_header *CurveDataHeader = (curve_data_header *)Curve->CurveData.Object;
    int32x CurveFormat = CurveDataHeader->Format;
    if ( Dimension != CurveGetDimension ( *Curve ) )
    {
        Log0 ( ErrorLogMessage, CurveLogMessage, "Curve and argument dimensions do not match" );
        return;
    }
    if ( KnotCount != CurveGetKnotCount ( *Curve ) )
    {
        Log0 ( ErrorLogMessage, CurveLogMessage, "Curve and argument knot counts do not match" );
        return;
    }
    CurveTypeTable[CurveFormat].SetAllKnotValues ( *Curve, KnotCount, Dimension, KnotSrc, ControlSrc );
}


void GRANNY
CurveScaleOffsetSwizzle(curve2 *Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles)
{
    PARANOID_CURVE_CHECK ( *Curve );
    CurveParanoiaChecking ( *Curve );
    curve_data_header *CurveDataHeader = (curve_data_header *)Curve->CurveData.Object;
    int32x CurveFormat = CurveDataHeader->Format;
    CurveTypeTable[CurveFormat].ScaleOffsetSwizzle ( *Curve, Dimension, Scales, Offsets, Swizzles );
}


int32x GRANNY
CurveFindKnot(curve2 const &Curve, real32 t)
{
    PARANOID_CURVE_CHECK ( Curve );
    CurveParanoiaChecking ( Curve );
    curve_data_header *CurveDataHeader = (curve_data_header *)Curve.CurveData.Object;
    int32x CurveFormat = CurveDataHeader->Format;
    return CurveTypeTable[CurveFormat].FindKnot ( Curve, t );
}

int32x GRANNY
CurveFindCloseKnot(curve2 const &Curve, real32 t, int32x StartingKnotIndex)
{
    PARANOID_CURVE_CHECK ( Curve );
    CurveParanoiaChecking ( Curve );
    curve_data_header *CurveDataHeader = (curve_data_header *)Curve.CurveData.Object;
    int32x CurveFormat = CurveDataHeader->Format;
    return ( CurveTypeTable[CurveFormat].FindCloseKnot ( Curve, t, StartingKnotIndex ) );
}

int32 GRANNY
CurveGetSize(curve2 const &Curve)
{
    curve_builder Builder;
    BeginCurveCopyInPlace(&Builder, Curve);
    int32x Size = GetResultingCurveSize(&Builder);
    AbortCurveBuilder(&Builder);

    return Size;
}


// Handy function for some of the dimension reduction functions
static void
ExtractPCALine3Dim(int32x const NumControls,
                   real32 const *ControlSrc,
                   int32x const ControlStrideInDwords,
                   real32 *Offsets,
                   real32 *Scales,
                   bool RescaleResults)
{
    // Compute the mean
    real32 Mean[3] = { 0, 0, 0 };
    {for(int Control = 0; Control < NumControls; ++Control)
    {
        Mean[0] += ControlSrc[Control * ControlStrideInDwords + 0];
        Mean[1] += ControlSrc[Control * ControlStrideInDwords + 1];
        Mean[2] += ControlSrc[Control * ControlStrideInDwords + 2];
    }}
    {for(int i = 0; i < 3; i++)
    {
        Mean[i] /= NumControls;
    }}


    // Compute the covariance matrix
    real32 Covariance[3][3] = {
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 }
    };
    {for(int Control = 0; Control < NumControls; ++Control)
    {
        real32 const* CurControl = ControlSrc + Control * ControlStrideInDwords;

        // Symmetric, so we're doing too much work here, but eh.
        {for(int i = 0; i < 3; i++)
        {
            {for(int j = 0; j < 3; j++)
            {
                Covariance[i][j] += (CurControl[i] - Mean[i]) * (CurControl[j] - Mean[j]);
            }}
        }}
    }}
    {for(int i = 0; i < 3; i++)
    {
        {for(int j = 0; j < 3; j++)
        {
            Covariance[i][j] /= NumControls;
        }}
    }}

    //--- TODO: power method is Lame lame lame!  Replace with a proper
    // eigensystem solver
    // Extract the principal eigenvector
    real32 Power[3] = { 1, 2, 3 };
    {for(int Iter = 0; Iter < 25; Iter++)
    {
        VectorTransform3(Power, (real32 const*)Covariance);
        NormalizeOrZero3(Power);
    }}

    if (RescaleResults)
    {
        real32 const MeanDot = InnerProduct3(Mean, Power);

        real32 MinDot = GetReal32AlmostInfinity();
        real32 MaxDot = -GetReal32AlmostInfinity();
        {for(int Control = 0; Control < NumControls; ++Control)
        {
            real32 const* CurControl = ControlSrc + Control * ControlStrideInDwords;
            real32 DotProd = InnerProduct3(CurControl, Power);

            if (DotProd < MinDot)
                MinDot = DotProd;
            if (DotProd > MaxDot)
                MaxDot = DotProd;
        }}

        if (MinDot == MaxDot)
        {
            // All the points are the same along the axis of maximal
            // variance?  That's not really what we expect.  Just select
            // something sensible.
            {for(int i = 0; i < 3; i++)
            {
                Offsets[i] = ControlSrc[i];
                Scales[i]  = 1.0f;
            }}
        }
        else
        {
            // Ok, we have an honest to goodness line here.  Since power
            // is normalized at this point, we can obtain the correct
            // Scales, by multiplying by (MaxDot - MinDot).  Note that the
            // correct offset is along the line implied by the direction
            // of Power from the Mean, at the same "dot level" as the
            // minimal point.
            {for(int i = 0; i < 3; i++)
            {
                Scales[i]  = Power[i] * (MaxDot - MinDot);
                Offsets[i] = Mean[i] - Power[i] * (MeanDot - MinDot);
            }}
        }
    }
    else
    {
        // We only care about the line direction.  Just return the normalized result
        Copy32(3, Power, Scales);
        Offsets[0] = Offsets[1] = Offsets[2] = 0.0f;
    }
}

// static void
// ComputeBestRotationAxis(int32x const ControlCount,
//                      real32 const *SourceQuaternions,
//                      real32 const *InvBaseTransform,
//                      real32* RotationAxis)
// {
//  real32 MaxLen = -1.0f;
//  {for(int32x Sample = 0; Sample < ControlCount; Sample++)
//  {
//      real32 const *CurSample = SourceQuaternions + (Sample * 4);
//      real32 temp[4];
//      Copy32(4, CurSample, temp);
//      if (InvBaseTransform != NULL)
//      {
//          QuaternionMultiply4(temp, temp, InvBaseTransform);
//      }

//      real32 AxisLength = VectorLength3(temp);
//      if (AxisLength > MaxLen)
//      {
//          Copy32(3, temp, RotationAxis);
//          MaxLen = NormalizeOrZero3(RotationAxis);
//      }
//  }}

//  // The 'sign' of the axis is ambiguous, but we need a consistent
//  // interpretation.  Choose the axis such that the first component
//  // over 0.1 is positive.  (0.1 is arbitrary.)
//  {for (int i = 0; i < 3; i++)
//  {
//      if (IntrinsicAbsoluteValue(RotationAxis[i]) > 0.1)
//      {
//          if (RotationAxis[i] < 0.0f)
//          {
//              RotationAxis[0] = -RotationAxis[0];
//              RotationAxis[1] = -RotationAxis[1];
//              RotationAxis[2] = -RotationAxis[2];
//          }
//          return;
//      }
//  }}

//  // This can happen if the curve reducer isn't able to recognize
//  // the curve as constant.  Essentially, all the keys are the
//  // same, so it doesn't matter what we choose here.
//  RotationAxis[0] = 1.0f;
//  RotationAxis[1] = 0.0f;
//  RotationAxis[2] = 0.0f;
// }

// static inline void
// ExtractQuantizedVector(uint16 const *Quantized,
//                     real32 *Unquantized,
//                     int32x Dimension)
// {
//  {for(int Dim = 0; Dim < Dimension; ++Dim)
//  {
//      Unquantized[Dim] = Quantized[Dim] * (2.0f / UInt16Maximum) - 1.0f;
//  }}
// }

// static void
// QuantizeVector(real32 const *Unquantized,
//             uint16 *Quantized,
//             int32x Dimension)
// {
//  {for(int Dim = 0; Dim < Dimension; ++Dim)
//  {
//      Assert(Unquantized[Dim] >= -1.0f && Unquantized[Dim] <= 1.0f);

//      int32x ScaledValue = RoundReal32ToInt32((Unquantized[Dim] + 1.0f) * (UInt16Maximum / 2.0f));
//      Assert(ScaledValue >= 0 && ScaledValue <= UInt16Maximum);

//      Quantized[Dim] = (uint16)ScaledValue;
//  }}
// }


// And now the functions for all the types. Mostly just wrappers for other functions, changing the names on the way.


// A truly absurd number of redirections to get the C preprocessor to
// do the right thing...
#define CURVE_DATA_TYPE1(name) curve_data_##name
#define CURVE_DATA_TYPE_NAME1(name) CurveData##name##Type
#define CURVE_GET_DIMENSION1(name) CurveGetDimensionUnchecked##name
#define CURVE_GET_KNOT_COUNT1(name) CurveGetKnotCount##name
#define CURVE_FIND_KNOT1(name) CurveFindKnot##name
#define CURVE_FIND_CLOSE_KNOT1(name) CurveFindCloseKnot##name
#define CURVE_EXTRACT_KNOT_VALUES1(name) CurveExtractKnotValues##name
#define CURVE_TRANSFORM_SAMPLES1(name) CurveTransformSamples##name
#define CURVE_NOTE_SAMPLE_TRANSFORM1(name) CurveNoteSampleTransform##name
#define CURVE_SET_ALL_KNOT_VALUES1(name) CurveSetAllKnotValues##name
#define CURVE_AGGR_CURVE_DATA1(name) CurveAggrCurveData##name
#define CURVE_COPY_CURVE1(name) CurveCopyCurve##name
#define CURVE_SCALE_OFFSET_SWIZZLE1(name) CurveScaleOffsetSwizzle##name

#define CURVE_DATA_TYPE2(name) CURVE_DATA_TYPE1(name)
#define CURVE_DATA_TYPE_NAME2(name) CURVE_DATA_TYPE_NAME1(name)
#define CURVE_DATA_TYPE CURVE_DATA_TYPE2(LOWER_NAME)
#define CURVE_DATA_TYPE_NAME CURVE_DATA_TYPE_NAME2(MIXED_NAME)

#define CURVE_GET_DIMENSION(name) CURVE_GET_DIMENSION1(name)
#define CURVE_GET_KNOT_COUNT(name) CURVE_GET_KNOT_COUNT1(name)
#define CURVE_FIND_KNOT(name) CURVE_FIND_KNOT1(name)
#define CURVE_FIND_CLOSE_KNOT(name) CURVE_FIND_CLOSE_KNOT1(name)
#define CURVE_EXTRACT_KNOT_VALUES(name) CURVE_EXTRACT_KNOT_VALUES1(name)
#define CURVE_TRANSFORM_SAMPLES(name) CURVE_TRANSFORM_SAMPLES1(name)
#define CURVE_NOTE_SAMPLE_TRANSFORM(name) CURVE_NOTE_SAMPLE_TRANSFORM1(name)
#define CURVE_SET_ALL_KNOT_VALUES(name) CURVE_SET_ALL_KNOT_VALUES1(name)
#define CURVE_AGGR_CURVE_DATA(name) CURVE_AGGR_CURVE_DATA1(name)
#define CURVE_COPY_CURVE(name) CURVE_COPY_CURVE1(name)
#define CURVE_SCALE_OFFSET_SWIZZLE(name) CURVE_SCALE_OFFSET_SWIZZLE1(name)


// Handy function to check that the data alignment/padding worked correctly.
#define CURVE_CHECK_HEADER Assert ( ((void*)&(CurveData.CurveDataHeader) == (void*)&(CurveData) ) && ( CurveTypeTable[CurveData.CurveDataHeader.Format].TypeDefinition == CURVE_DATA_TYPE_NAME2(MIXED_NAME)) )


#define IMPL_CURVE_GET_DIMENSION(MIXED_NAME)                                            \
    int32x GRANNY                                                                       \
    CURVE_GET_DIMENSION(MIXED_NAME)(curve2 const& Curve)                                \
    {                                                                                   \
        PARANOID_CURVE_CHECK ( Curve );                                                 \
        CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;  \
        CURVE_CHECK_HEADER;                                                             \
        return ( FIND_DIMENSION );                                                      \
    }

#define IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME)                                           \
    int32x GRANNY                                                                       \
    CURVE_GET_KNOT_COUNT(MIXED_NAME)(curve2 const& Curve)                               \
    {                                                                                   \
        PARANOID_CURVE_CHECK ( Curve );                                                 \
        CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;  \
        CURVE_CHECK_HEADER;                                                             \
        int32x CurveDimension = FIND_DIMENSION;                                         \
        CurveDimension = CurveDimension;  /* quiet, compiler! */                        \
        return ( FIND_KNOT_COUNT );                                                     \
    }

#define IMPL_CURVE_FIND_KNOT_NULL(MIXED_NAME)                                           \
    int32x GRANNY                                                                       \
    CURVE_FIND_KNOT(MIXED_NAME) ( curve2 const &Curve, real32 t )                       \
    {                                                                                   \
        PARANOID_CURVE_CHECK ( Curve );                                                 \
        CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;  \
        CURVE_CHECK_HEADER;                                                             \
        return ( 0 );                                                                   \
    }

#define IMPL_CURVE_FIND_CLOSE_KNOT_NULL(MIXED_NAME)                                             \
    int32x GRANNY                                                                               \
    CURVE_FIND_CLOSE_KNOT(MIXED_NAME) ( curve2 const &Curve, real32 t, int32x StartingIndex )   \
    {                                                                                           \
        PARANOID_CURVE_CHECK ( Curve );                                                         \
        CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;          \
        CURVE_CHECK_HEADER;                                                                     \
        return ( 0 );                                                                           \
    }

// Notes for the below function:
//
// @Assert (ScaledT < 1.0f + (real32)UInt16Maximum
//   FindKnot finds the knot that's _ahead_ of where you asked.  So if you have
//   to round ScaledT, then round it _down_. It you round it _up_, you might
//   actually hit the knot, in which case you'll actually get the next knot
//   returned.  e.g. knots at 2, 3, 4. ScaledT is 2.9, so you need to round down
//   to 2 so that the resulting knot is the one at 3.
//
// @if (KnotIndex == CurveKnotCount - 1)
//   FindKnot may have wanted to give you CurveKnotCount, but had to clamp it
//   (calling code gets confused if you do.  In which case, all bets are off and
//   the below may not be true!
#define IMPL_CURVE_FIND_KNOT_Quantized(MIXED_NAME, Bits)                                        \
    int32x GRANNY                                                                               \
    CURVE_FIND_KNOT(MIXED_NAME) ( curve2 const &Curve, real32 t )                               \
    {                                                                                           \
        const   int32x MaxVal = UInt ## Bits ## Maximum;                                        \
        typedef uint ## Bits knot_type;                                                         \
                                                                                                \
        PARANOID_CURVE_CHECK ( Curve );                                                         \
        CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;          \
        CURVE_CHECK_HEADER;                                                                     \
                                                                                                \
        int32x CurveDimension = FIND_DIMENSION; CurveDimension = CurveDimension;                \
        int32x CurveKnotCount = FIND_KNOT_COUNT;                                                \
        real32 OneOverKnotScale;                                                                \
        EXTRACT_ONE_OVER_KNOT_SCALE;                                                            \
        real32 ScaledT = t * OneOverKnotScale;                                                  \
        Assert ( ScaledT >= 0.0f );                                                             \
        Assert ( ScaledT < 1.0f + (real32)MaxVal );                                             \
        int32x QuantisedT = FloorReal32ToInt32( ScaledT );                                      \
        Assert ( QuantisedT >= 0 );                                                             \
        Assert ( QuantisedT <= MaxVal );                                                        \
        int32x KnotIndex = FindKnotUint ## Bits ( CurveKnotCount,                               \
                                                  GET_KNOT_POINTER, (knot_type)QuantisedT );    \
        if ( KnotIndex == CurveKnotCount - 1 )                                                  \
        {                                                                                       \
        }                                                                                       \
        else                                                                                    \
        {                                                                                       \
            if ( KnotIndex < CurveKnotCount )                                                   \
            {                                                                                   \
                Assert ( CurveData.KnotsControls[KnotIndex] > QuantisedT );                     \
            }                                                                                   \
            if ( KnotIndex > 0 )                                                                \
            {                                                                                   \
                Assert ( CurveData.KnotsControls[KnotIndex-1] <= QuantisedT );                  \
            }                                                                                   \
        }                                                                                       \
        return KnotIndex;                                                                       \
    }

#define IMPL_CURVE_FIND_CLOSE_KNOT_Quantized(MIXED_NAME, Bits)                                      \
    int32x GRANNY                                                                                   \
    CURVE_FIND_CLOSE_KNOT(MIXED_NAME) ( curve2 const &Curve, real32 t, int32x StartingIndex )       \
    {                                                                                               \
        const   int32x MaxVal = UInt ## Bits ## Maximum;                                            \
        typedef uint ## Bits knot_type;                                                             \
                                                                                                    \
        PARANOID_CURVE_CHECK ( Curve );                                                             \
        CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;              \
        CURVE_CHECK_HEADER;                                                                         \
                                                                                                    \
        int32x CurveDimension = FIND_DIMENSION; CurveDimension = CurveDimension;                    \
        int32x CurveKnotCount = FIND_KNOT_COUNT;                                                    \
        real32 OneOverKnotScale;                                                                    \
        EXTRACT_ONE_OVER_KNOT_SCALE;                                                                \
        real32 ScaledT = t * OneOverKnotScale;                                                      \
        Assert ( ScaledT >= 0.0f );                                                                 \
        Assert ( ScaledT < 1.0f + (real32)MaxVal );                                                 \
        int32x QuantisedT = FloorReal32ToInt32( ScaledT );                                          \
        Assert ( QuantisedT >= 0 );                                                                 \
        Assert ( QuantisedT <= MaxVal );                                                            \
        int32x KnotIndex = FindCloseKnotUint ## Bits ( CurveKnotCount, GET_KNOT_POINTER, (knot_type)QuantisedT, StartingIndex ); \
        Assert ( CurveData.KnotsControls[KnotIndex] > QuantisedT );                                 \
        return KnotIndex;                                                                           \
    }

#define IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME)                           \
    void GRANNY                                                                 \
    CURVE_TRANSFORM_SAMPLES(MIXED_NAME)(int32x SampleCount, int32x Dimension,   \
                                        real32 *Samples)                        \
    {                                                                           \
        /* nothing */                                                           \
    }

#define IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME)                           \
    void GRANNY                                                                     \
    CURVE_NOTE_SAMPLE_TRANSFORM(MIXED_NAME)(curve2 &Curve,                          \
                                            int32x SampleCount, int32x Dimension,   \
                                            real32 const *OriginalSamples)          \
    {                                                                               \
        /* nothing */                                                               \
    }

#if 0
#define IMPL_CURVE_TRANSFORM_SAMPLES_HINGE(MIXED_NAME)                                      \
    void GRANNY                                                                             \
    CURVE_TRANSFORM_SAMPLES(MIXED_NAME)(int32x SampleCount,                                 \
                                        int32x Dimension,                                   \
                                        real32 *Samples)                                    \
    {                                                                                       \
        Assert(Dimension == 4);                                                             \
                                                                                            \
        /* Copy out the base transformation first (same as below!) */                       \
        real32 InverseFirstControl[4];                                                      \
        {                                                                                   \
            real32 BaseQuat[4];                                                             \
            Copy32(4, Samples, BaseQuat);                                                   \
            Normalize4(BaseQuat);                                                           \
                                                                                            \
            /* Encode, and reextract the quaternion */                                      \
            uint16 Quant[4];                                                                \
            QuantizeVector(BaseQuat, Quant, 4);                                             \
            ExtractQuantizedVector(Quant, BaseQuat, 4);                                     \
                                                                                            \
            /* Inverse is the conjugate */                                                  \
            Conjugate4(InverseFirstControl, BaseQuat);                                      \
        }                                                                                   \
                                                                                            \
        real32 RotAxis[3];                                                                  \
        ComputeBestRotationAxis(SampleCount, Samples, InverseFirstControl, RotAxis);        \
                                                                                            \
        {for(int32x Sample = 0; Sample < SampleCount; Sample++)                             \
        {                                                                                   \
            real32 *CurSample = Samples + (Sample * Dimension);                             \
            QuaternionMultiply4(CurSample, CurSample, InverseFirstControl);                 \
                                                                                            \
            real32 CosThetaOver2 = CurSample[3];                                            \
            real32 SinThetaOver2 = InnerProduct3(CurSample, RotAxis);                       \
            real32 ThetaOver2    = (real32)IntrinsicATan2(SinThetaOver2, CosThetaOver2);    \
                                                                                            \
            CurSample[0] = 0.0f;                                                            \
            CurSample[1] = 0.0f;                                                            \
            CurSample[2] = 0.0f;                                                            \
            CurSample[3] = ThetaOver2 * 2.0f;                                               \
        }}                                                                                  \
    }

#define IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_HINGE(MIXED_NAME)                          \
    void GRANNY                                                                     \
    CURVE_NOTE_SAMPLE_TRANSFORM(MIXED_NAME)(curve2 &Curve,                          \
                                            int32x SampleCount, int32x Dimension,   \
                                            real32 const *OriginalSamples)          \
    {                                                                               \
        Assert(Dimension == 4);                                                     \
        PARANOID_CURVE_CHECK ( Curve );                                             \
        CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;    \
        CURVE_CHECK_HEADER;                                                         \
        /* Copy out the base transformation first (same as above!) */               \
        real32 BaseQuat[4];                                                         \
        Copy32(4, OriginalSamples, BaseQuat);                                       \
        Normalize4(BaseQuat);                                                       \
                                                                                    \
        /* Encode the transform */                                                  \
        QuantizeVector(BaseQuat, CurveData.BaseQuaternion, 4);                      \
                                                                                    \
        /* Extract the rotation axis */                                             \
        Conjugate4(BaseQuat);                                                       \
        real32 RotAxis[3];                                                          \
        ComputeBestRotationAxis(SampleCount, OriginalSamples, BaseQuat, RotAxis);   \
        QuantizeVector(RotAxis, CurveData.RotationAxis, 3);                         \
    }
#endif // 0



//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(DaKeyframes32f);

#define MIXED_NAME DaKeyframes32f
#define LOWER_NAME da_keyframes32f

#define FIND_DIMENSION (CurveData.Dimension)
#define FIND_KNOT_COUNT (CurveData.ControlCount / CurveData.Dimension)

IMPL_CURVE_GET_DIMENSION(MIXED_NAME);
IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME);
IMPL_CURVE_FIND_KNOT_NULL(MIXED_NAME);
IMPL_CURVE_FIND_CLOSE_KNOT_NULL(MIXED_NAME);
IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);

void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;

    // This doesn't return knot values (coz there are none).
    // it just returns "control" values, i.e. the keyframes.
    Assert ( KnotIndex >= 0 );
    Assert ( KnotIndex + KnotCount <= CurveKnotCount );
    // Caller shouldn't even be asking for knot values,
    // but it's expensive to prevent certain wrapper routs from asking.
    //Assert ( KnotResults == NULL );
    if ( ControlResults != NULL )
    {
        Copy32 ( KnotCount * CurveDimension, CurveData.Controls + KnotIndex * CurveDimension, ControlResults );
    }
}

void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME) ( curve2 &Curve,
                                        int32x KnotCount, int32x Dimension,
                                        real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    // This ignores the knot values (coz there are none).
    // it just sets the "control" values, i.e. the keyframes.
    CurveData.Dimension = (int16)Dimension;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( KnotCount == CurveKnotCount );
    // Caller probably shouldn't even be supplying knot values.
    Assert ( KnotSrc == NULL );

    Copy32 ( KnotCount * Dimension, ControlSrc, CurveData.Controls );
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    AggrAllocPtr ( Allocator, LocalCurveData );
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->KnotCount * Builder->Dimension, ControlCount, Controls );
}


void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    CurveData.Dimension = CurveDataSource.Dimension;
    Assert ( CurveData.ControlCount == CurveDataSource.ControlCount );
    Copy32 ( CurveData.ControlCount, CurveDataSource.Controls, CurveData.Controls );
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( Dimension == CurveDimension );

    Assert ( CurveDimension <= MaximumBSplineDimension );
    real32 TempControls[MaximumBSplineDimension];

    real32 *Control = CurveData.Controls;
    {for ( int32x ControlNum = 0; ControlNum < CurveKnotCount; ControlNum++ )
    {
        Copy32 ( CurveDimension, Control, TempControls );
        {for ( int32x CurDimension = 0; CurDimension < CurveDimension; CurDimension++ )
        {
            Assert ( ( Swizzles[CurDimension] >= 0 ) && ( Swizzles[CurDimension] < CurveDimension ) );
            Control[CurDimension] = TempControls[Swizzles[CurDimension]] * Scales[CurDimension] + Offsets[CurDimension];
        }}
        Control += CurveDimension;
    }}
}


#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE


//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(DaIdentity);

#define MIXED_NAME DaIdentity
#define LOWER_NAME da_identity

#define FIND_DIMENSION (CurveData.Dimension)
#define FIND_KNOT_COUNT (0)

IMPL_CURVE_GET_DIMENSION(MIXED_NAME);
IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME);
IMPL_CURVE_FIND_KNOT_NULL(MIXED_NAME);
IMPL_CURVE_FIND_CLOSE_KNOT_NULL(MIXED_NAME);
IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);

void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    int32x CurveDimension = FIND_DIMENSION;
    Assert ( KnotIndex == 0 );
    Assert ( ( KnotCount == 0 ) || ( KnotCount == 1 ) );
    if ( ( KnotCount == 1 ) && ( KnotResults != NULL ) )
    {
        KnotResults[0] = 0.0f;
    }
    Assert ( IdentityVector != NULL );
    if ( ControlResults != NULL )
    {
        Copy32 ( CurveDimension, IdentityVector, ControlResults );
    }
}

void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME) ( curve2 &Curve,
                                        int32x KnotCount, int32x Dimension,
                                        real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    CurveData.Dimension = (int16)Dimension;
    Assert ( KnotCount == 0 );
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    Assert ( Builder->KnotCount == 0 );
    AggrAllocPtr ( Allocator, LocalCurveData );
}

void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    CurveData.Dimension = CurveDataSource.Dimension;
    // Nothing else to do!
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    InvalidCodePath ( "Never call this on a DaIdentity" );
}

#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE

//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(DaConstant32f);

#define MIXED_NAME DaConstant32f
#define LOWER_NAME da_constant32f

#define FIND_DIMENSION (CurveData.ControlCount)
#define FIND_KNOT_COUNT (1)

IMPL_CURVE_GET_DIMENSION(MIXED_NAME);
IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME);
IMPL_CURVE_FIND_KNOT_NULL(MIXED_NAME);
IMPL_CURVE_FIND_CLOSE_KNOT_NULL(MIXED_NAME);
IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);

void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    int32x CurveDimension = FIND_DIMENSION;
    Assert ( KnotIndex == 0 );
    Assert ( KnotCount == 1 );
    if ( KnotResults != NULL )
    {
        KnotResults[0] = 0.0f;
    }
    if ( ControlResults != NULL )
    {
        Copy32 ( CurveDimension, CurveData.Controls, ControlResults );
    }
}

void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME) ( curve2 &Curve,
                                        int32x KnotCount, int32x Dimension,
                                        real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( Dimension == CurveDimension );
    Assert ( KnotCount == CurveKnotCount );
    Assert ( KnotSrc[0] == 0.0f );
    Copy32 ( CurveDimension, ControlSrc, CurveData.Controls );
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    Assert ( Builder->KnotCount == 1 );
    AggrAllocPtr ( Allocator, LocalCurveData );
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->Dimension, ControlCount, Controls );
}

void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    Assert ( CurveData.ControlCount == CurveDataSource.ControlCount );
    Copy32 ( CurveData.ControlCount, CurveDataSource.Controls, CurveData.Controls );
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    int32x CurveDimension = FIND_DIMENSION;
    Assert ( Dimension == CurveDimension );

    Assert ( CurveDimension <= MaximumBSplineDimension );
    real32 TempControls[MaximumBSplineDimension];

    Copy32 ( CurveDimension, CurveData.Controls, TempControls );
    {for ( int32x CurDimension = 0; CurDimension < CurveDimension; CurDimension++ )
    {
        Assert ( ( Swizzles[CurDimension] >= 0 ) && ( Swizzles[CurDimension] < CurveDimension ) );
        CurveData.Controls[CurDimension] = TempControls[Swizzles[CurDimension]] * Scales[CurDimension] + Offsets[CurDimension];
    }}
}

#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE

//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(D3Constant32f);

#define MIXED_NAME D3Constant32f
#define LOWER_NAME d3_constant32f

#define FIND_DIMENSION (3)
#define FIND_KNOT_COUNT (1)

IMPL_CURVE_GET_DIMENSION(MIXED_NAME);
IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME);
IMPL_CURVE_FIND_KNOT_NULL(MIXED_NAME);
IMPL_CURVE_FIND_CLOSE_KNOT_NULL(MIXED_NAME);
IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);

void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    int32x CurveDimension = FIND_DIMENSION;
    Assert ( KnotIndex == 0 );
    Assert ( KnotCount == 1 );
    if ( KnotResults != NULL )
    {
        KnotResults[0] = 0.0f;
    }
    if ( ControlResults != NULL )
    {
        Copy32 ( CurveDimension, CurveData.Controls, ControlResults );
    }
}

void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME) ( curve2 &Curve,
                                        int32x KnotCount, int32x Dimension,
                                        real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( Dimension == CurveDimension );
    Assert ( KnotCount == CurveKnotCount );
    Assert ( KnotSrc[0] == 0.0f );
    Copy32 ( CurveDimension, ControlSrc, CurveData.Controls );
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    Assert ( Builder->KnotCount == 1 );
    AggrAllocPtr ( Allocator, LocalCurveData );
}

void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    Copy32 ( 3, CurveDataSource.Controls, CurveData.Controls );
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    int32x CurveDimension = FIND_DIMENSION;
    Assert ( Dimension == CurveDimension );

    Assert ( CurveDimension <= MaximumBSplineDimension );
    real32 TempControls[MaximumBSplineDimension];

    Copy32 ( CurveDimension, CurveData.Controls, TempControls );
    {for ( int32x CurDimension = 0; CurDimension < CurveDimension; CurDimension++ )
    {
        Assert ( ( Swizzles[CurDimension] >= 0 ) && ( Swizzles[CurDimension] < CurveDimension ) );
        CurveData.Controls[CurDimension] = TempControls[Swizzles[CurDimension]] * Scales[CurDimension] + Offsets[CurDimension];
    }}
}

#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE


//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(D4Constant32f);

#define MIXED_NAME D4Constant32f
#define LOWER_NAME d4_constant32f

#define FIND_DIMENSION (4)
#define FIND_KNOT_COUNT (1)

IMPL_CURVE_GET_DIMENSION(MIXED_NAME);
IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME);
IMPL_CURVE_FIND_KNOT_NULL(MIXED_NAME);
IMPL_CURVE_FIND_CLOSE_KNOT_NULL(MIXED_NAME);
IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);

void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    int32x CurveDimension = FIND_DIMENSION;
    Assert ( KnotIndex == 0 );
    Assert ( KnotCount == 1 );
    if ( KnotResults != NULL )
    {
        KnotResults[0] = 0.0f;
    }
    if ( ControlResults != NULL )
    {
        Copy32 ( CurveDimension, CurveData.Controls, ControlResults );
    }
}

void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME) ( curve2 &Curve,
                                        int32x KnotCount, int32x Dimension,
                                        real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( Dimension == CurveDimension );
    Assert ( KnotCount == CurveKnotCount );
    Assert ( KnotSrc[0] == 0.0f );
    Copy32 ( CurveDimension, ControlSrc, CurveData.Controls );
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    Assert ( Builder->KnotCount == 1 );
    AggrAllocPtr ( Allocator, LocalCurveData );
}

void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    Copy32 ( 4, CurveDataSource.Controls, CurveData.Controls );
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    int32x CurveDimension = FIND_DIMENSION;
    Assert ( Dimension == CurveDimension );

    Assert ( CurveDimension <= MaximumBSplineDimension );
    real32 TempControls[MaximumBSplineDimension];

    Copy32 ( CurveDimension, CurveData.Controls, TempControls );
    {for ( int32x CurDimension = 0; CurDimension < CurveDimension; CurDimension++ )
    {
        Assert ( ( Swizzles[CurDimension] >= 0 ) && ( Swizzles[CurDimension] < CurveDimension ) );
        CurveData.Controls[CurDimension] = TempControls[Swizzles[CurDimension]] * Scales[CurDimension] + Offsets[CurDimension];
    }}
}

#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE


//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(DaK32fC32f);

#define MIXED_NAME DaK32fC32f
#define LOWER_NAME da_k32f_c32f

#define FIND_DIMENSION (CurveData.ControlCount / CurveData.KnotCount)
#define FIND_KNOT_COUNT (CurveData.KnotCount)
#define GET_KNOT_POINTER (CurveData.Knots)

IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);
IMPL_CURVE_GET_DIMENSION(MIXED_NAME);


int32x GRANNY
CURVE_GET_KNOT_COUNT(MIXED_NAME)(curve2 const& Curve)
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    return ( FIND_KNOT_COUNT );
}

int32x GRANNY
CURVE_FIND_KNOT(MIXED_NAME) ( curve2 const &Curve, real32 t )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    return ( FindKnot ( FIND_KNOT_COUNT, GET_KNOT_POINTER, t ) );
}

int32x GRANNY
CURVE_FIND_CLOSE_KNOT(MIXED_NAME) ( curve2 const &Curve, real32 t, int32x StartingIndex )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    return ( FindCloseKnot ( FIND_KNOT_COUNT, GET_KNOT_POINTER, t, StartingIndex ) );
}

void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( KnotIndex >= 0 );
    Assert ( KnotIndex + KnotCount <= CurveKnotCount );
    if ( KnotResults != NULL )
    {
        Copy32 ( KnotCount                 , CurveData.Knots    + KnotIndex                 , KnotResults );
    }
    if ( ControlResults != NULL )
    {
        Copy32 ( KnotCount * CurveDimension, CurveData.Controls + KnotIndex * CurveDimension, ControlResults );
    }
}

void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME) ( curve2 &Curve,
                                        int32x KnotCount, int32x Dimension,
                                        real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( Dimension == CurveDimension );
    Assert ( KnotCount == CurveKnotCount );
    // Easy.
    Copy32 ( KnotCount            , KnotSrc,    CurveData.Knots );
    Copy32 ( KnotCount * Dimension, ControlSrc, CurveData.Controls );
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    // Knot count of 0 means identity, 1 means constant. These are only valid for DaK32fC32f!
    // All other curve types must have KnotCount > 1. If you want identity or constant,
    // use DaIdentity and DXConstant!
    Assert ( Builder->KnotCount >= 0 );
    AggrAllocPtr ( Allocator, LocalCurveData );
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->KnotCount, KnotCount, Knots );
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->KnotCount * Builder->Dimension, ControlCount, Controls );
}

void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    Assert ( CurveData.KnotCount == CurveDataSource.KnotCount );
    Copy32 ( CurveData.KnotCount, CurveDataSource.Knots, CurveData.Knots );
    Assert ( CurveData.ControlCount == CurveDataSource.ControlCount );
    Copy32 ( CurveData.ControlCount, CurveDataSource.Controls, CurveData.Controls );
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( Dimension == CurveDimension );

    Assert ( CurveDimension <= MaximumBSplineDimension );
    real32 TempControls[MaximumBSplineDimension];

    real32 *Control = CurveData.Controls;
    {for ( int32x ControlNum = 0; ControlNum < CurveKnotCount; ControlNum++ )
    {
        Copy32 ( CurveDimension, Control, TempControls );
        {for ( int32x CurDimension = 0; CurDimension < CurveDimension; CurDimension++ )
        {
            Assert ( ( Swizzles[CurDimension] >= 0 ) && ( Swizzles[CurDimension] < CurveDimension ) );
            Control[CurDimension] = TempControls[Swizzles[CurDimension]] * Scales[CurDimension] + Offsets[CurDimension];
        }}
        Control += CurveDimension;
    }}
}

#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE


//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(DaK16uC16u);

#define MIXED_NAME DaK16uC16u
#define LOWER_NAME da_k16u_c16u

// KnotControlCount = KnotCount + ControlCount
// ControlCount = KnotCount * Dimension
// Dimension = ControlScaleOffsetSize / 2
#define FIND_DIMENSION (CurveData.ControlScaleOffsetCount / 2)
#define FIND_KNOT_COUNT (CurveData.KnotControlCount / ( CurveDimension + 1 ))
// Only the top half of the IEEE float OneOverKnotScale is stored!
#define EXTRACT_ONE_OVER_KNOT_SCALE *(uint32*)(&OneOverKnotScale) = (((uint32)CurveData.OneOverKnotScaleTrunc)<<16)
#define GET_KNOT_POINTER (CurveData.KnotsControls)


IMPL_CURVE_GET_DIMENSION(MIXED_NAME);
IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME);
IMPL_CURVE_FIND_KNOT_Quantized(MIXED_NAME, 16);
IMPL_CURVE_FIND_CLOSE_KNOT_Quantized(MIXED_NAME, 16);
IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);

void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( KnotIndex >= 0 );
    Assert ( KnotIndex + KnotCount <= CurveKnotCount );
    real32 OneOverKnotScale;
    EXTRACT_ONE_OVER_KNOT_SCALE;
    uint16 *CurKnot    = CurveData.KnotsControls + KnotIndex;
    // Skip the knots, then find the right control.
    uint16 *CurControl = CurveData.KnotsControls + CurveKnotCount + KnotIndex * CurveDimension;
    real32 KnotScale = 1.0f / OneOverKnotScale;
    const real32 *ControlScales = CurveData.ControlScaleOffsets;
    const real32 *ControlOffsets = CurveData.ControlScaleOffsets + CurveDimension;
    if ( KnotResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            *KnotResults++ = KnotScale * (real32)( *CurKnot++ );
        }}
    }
    if ( ControlResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            {for ( int32 Dim = 0; Dim < CurveDimension; Dim++ )
            {
                *ControlResults++ = ControlOffsets[Dim] + ControlScales[Dim] * (real32)( *CurControl++ );
            }}
        }}
    }
}

void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME) ( curve2 &Curve,
                                        int32x KnotCount, int32x Dimension,
                                        real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( Dimension == CurveDimension );
    Assert ( KnotCount == CurveKnotCount );
    uint16 *FirstKnot    = CurveData.KnotsControls;
    // Skip the knots.
    uint16 *FirstControl = CurveData.KnotsControls + CurveKnotCount;

    real32 KnotMax = KnotSrc[CurveKnotCount-1];

    CompileAssert ( sizeof(*CurveData.KnotsControls) == 2 );
    real32 OneOverKnotScale = (real32)UInt16Maximum / KnotMax;
    CurveData.OneOverKnotScaleTrunc = (uint16)((*(uint32*)(&OneOverKnotScale))>>16);
    // And then re-extract it, coz it will have changed slightly.
    EXTRACT_ONE_OVER_KNOT_SCALE;
    Assert((real32)UInt16Maximum / KnotMax >= OneOverKnotScale);
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        FirstKnot[Count] = (uint16)RoundReal32ToInt32 ( KnotSrc[Count] * OneOverKnotScale );
    }}

    // Now scan the controls to find the min and max in each dimension.
    // Going to store min and max in ControlScaleOffsets - saves
    // allocating a new array.
    real32 *ControlScales = CurveData.ControlScaleOffsets;
    real32 *ControlOffsets = CurveData.ControlScaleOffsets + Dimension;
    {for ( int32 Dim = 0; Dim < Dimension; Dim++ )
    {
        ControlScales[Dim] = ControlSrc[Dim];
        ControlOffsets[Dim] = ControlSrc[Dim];
    }}

    real32 const *CurControlSrc = ControlSrc;
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        {for ( int32 Dim = 0; Dim < Dimension; Dim++ )
        {
            real32 ControlValue = *CurControlSrc++;
            if ( ControlScales[Dim] > ControlValue )
            {
                ControlScales[Dim] = ControlValue;
            }
            if ( ControlOffsets[Dim] < ControlValue )
            {
                ControlOffsets[Dim] = ControlValue;
            }
        }}
    }}

    // Figure out scale & offset.
    {for ( int32 Dim = 0; Dim < Dimension; Dim++ )
    {
        real32 Min = ControlScales[Dim];
        real32 Max = ControlOffsets[Dim];
        CompileAssert ( sizeof(*CurveData.KnotsControls) == 2 );
        ControlScales[Dim] = ( Max - Min ) / (real32)UInt16Maximum;
        ControlOffsets[Dim] = Min;

        if ( ControlScales[Dim] == 0.0f )
        {
            // Divide-by-zero is not fun.
            ControlScales[Dim] = 1.0f;
        }
    }}

    // And now write the actual control data.
    uint16 *CurControl = FirstControl;
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        {for ( int32 Dim = 0; Dim < Dimension; Dim++ )
        {
            real32 ControlValue = *ControlSrc++;
            real32 ScaledValue = ( ControlValue - ControlOffsets[Dim] ) / ControlScales[Dim];
            // In cases of very small ranges, this can go out of the 0-65535 range because of precision problems.
            if ( ScaledValue < 0.0f )
            {
                Assert ( ScaledValue > -1.0f );
                ScaledValue = 0.0f;
            }
            else if ( ScaledValue > 65535.0f )
            {
                Assert ( ScaledValue < 65536.0f );
                ScaledValue = 65535.0f;
            }
            *CurControl++ = (uint16)RoundReal32ToInt32 ( ScaledValue );
        }}
    }}
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    // Knot count of 0 should be DaIdentity, knot count of 1 should be DXConstant
    Assert ( Builder->KnotCount > 1 );
    AggrAllocPtr ( Allocator, LocalCurveData );
    // An array of the scales and offsets for the controls.
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->Dimension * 2,
                              ControlScaleOffsetCount, ControlScaleOffsets );
    // An array of both knots and controls (knots first, then controls).
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->KnotCount * ( Builder->Dimension + 1 ),
                              KnotControlCount, KnotsControls );
}

void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    CurveData.OneOverKnotScaleTrunc = CurveDataSource.OneOverKnotScaleTrunc;
    Assert ( CurveData.ControlScaleOffsetCount == CurveDataSource.ControlScaleOffsetCount );
    Copy32 ( CurveData.ControlScaleOffsetCount, CurveDataSource.ControlScaleOffsets, CurveData.ControlScaleOffsets );
    Assert ( CurveData.KnotControlCount == CurveDataSource.KnotControlCount );
    Copy ( CurveData.KnotControlCount * sizeof ( *CurveData.KnotsControls ), CurveDataSource.KnotsControls, CurveData.KnotsControls );
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( Dimension == CurveDimension );

    Assert ( CurveDimension <= MaximumBSplineDimension );
    uint16 TempControls[MaximumBSplineDimension];
    real32 TempScales[MaximumBSplineDimension];
    real32 TempOffsets[MaximumBSplineDimension];

    // Swizzle and adjust the scale+offset.
    // First half of ControlScaleOffsets are Scales, second half are Offsets

    // So the math goes that when an integer-format control is read, you do:
    // real_value = scale * int_value + offset
    // So now we want to scale and offset the result, so:
    // new_value = this_scale * real_value + this_offset
    //           = this_scale * ( scale * int_value + offset ) + this_offset
    //           = this_scale * scale * int_value + this_scale * offset + this_offset
    // And we need to find new_scale and new_offset so that:
    // new_value = new_scale * int_value + new_offset
    // Thus:
    // new_scale = this_scale * scale
    // new_offset = this_scale * offset + this_offset
    real32 *ControlScales = CurveData.ControlScaleOffsets;
    real32 *ControlOffsets = CurveData.ControlScaleOffsets + CurveDimension;
    Copy32 ( CurveDimension, ControlScales,  TempScales );
    Copy32 ( CurveDimension, ControlOffsets, TempOffsets );
    {for ( int32x CurDimension = 0; CurDimension < CurveDimension; CurDimension++ )
    {
        int32x Swizzle = Swizzles[CurDimension];
        Assert ( ( Swizzle >= 0 ) && ( Swizzle < CurveDimension ) );
        real32 OldScale  = TempScales [Swizzle];
        real32 OldOffset = TempOffsets[Swizzle];
        ControlScales[CurDimension] = Scales[CurDimension] * OldScale;
        ControlOffsets[CurDimension] = Scales[CurDimension] * OldOffset + Offsets[CurDimension];
    }}

    // Now swizzle the control data.
    // First part of KnotsControls is Knots, last part is Controls.
    uint16 *Control = CurveData.KnotsControls + CurveKnotCount;
    {for ( int32x ControlNum = 0; ControlNum < CurveKnotCount; ControlNum++ )
    {
        Copy ( CurveDimension * sizeof (Control[0]), Control, TempControls );
        {for ( int32x CurDimension = 0; CurDimension < CurveDimension; CurDimension++ )
        {
            // Scale and offsets happen above, so this just does swizzles.
            Control[CurDimension] = TempControls[Swizzles[CurDimension]];
        }}
        Control += CurveDimension;
    }}
}

#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE

//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(DaK8uC8u);

#define MIXED_NAME DaK8uC8u
#define LOWER_NAME da_k8u_c8u

// KnotControlCount = KnotCount + ControlCount
// ControlCount = KnotCount * Dimension
// Dimension = ControlScaleOffsetSize / 2
#define FIND_DIMENSION (CurveData.ControlScaleOffsetCount / 2)
#define FIND_KNOT_COUNT (CurveData.KnotControlCount / ( CurveDimension + 1 ))
// Only the top half of the IEEE float OneOverKnotScale is stored!
#define EXTRACT_ONE_OVER_KNOT_SCALE *(uint32*)(&OneOverKnotScale) = (((uint32)CurveData.OneOverKnotScaleTrunc)<<16)
#define GET_KNOT_POINTER (CurveData.KnotsControls)

IMPL_CURVE_GET_DIMENSION(MIXED_NAME);
IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME);
IMPL_CURVE_FIND_KNOT_Quantized(MIXED_NAME, 8);
IMPL_CURVE_FIND_CLOSE_KNOT_Quantized(MIXED_NAME, 8);
IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);

void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( KnotIndex >= 0 );
    Assert ( KnotIndex + KnotCount <= CurveKnotCount );
    uint8 *CurKnot    = CurveData.KnotsControls + KnotIndex;
    // Skip the knots, then find the right control.
    uint8 *CurControl = CurveData.KnotsControls + CurveKnotCount + KnotIndex * CurveDimension;
    real32 OneOverKnotScale;
    EXTRACT_ONE_OVER_KNOT_SCALE;
    real32 KnotScale = 1.0f / OneOverKnotScale;
    const real32 *ControlScales = CurveData.ControlScaleOffsets;
    const real32 *ControlOffsets = CurveData.ControlScaleOffsets + CurveDimension;
    if ( KnotResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            *KnotResults++ = KnotScale * (real32)( *CurKnot++ );
        }}
    }
    if ( ControlResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            {for ( int32 Dim = 0; Dim < CurveDimension; Dim++ )
            {
                *ControlResults++ = ControlOffsets[Dim] + ControlScales[Dim] * (real32)( *CurControl++ );
            }}
        }}
    }
}

void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME) ( curve2 &Curve,
                                        int32x KnotCount, int32x Dimension,
                                        real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( Dimension == CurveDimension );
    Assert ( KnotCount == CurveKnotCount );
    uint8 *FirstKnot    = CurveData.KnotsControls;
    // Skip the knots.
    uint8 *FirstControl = CurveData.KnotsControls + CurveKnotCount;

    real32 KnotMax = KnotSrc[CurveKnotCount-1];

    CompileAssert ( sizeof(*CurveData.KnotsControls) == 1 );
    real32 OneOverKnotScale = (real32)UInt8Maximum / KnotMax;
    CurveData.OneOverKnotScaleTrunc = (uint16)((*(uint32*)(&OneOverKnotScale))>>16);
    // And then re-extract it, coz it will have changed slightly.
    EXTRACT_ONE_OVER_KNOT_SCALE;
    Assert((real32)UInt8Maximum / KnotMax >= OneOverKnotScale);
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        FirstKnot[Count] = (uint8)RoundReal32ToInt32 ( KnotSrc[Count] * OneOverKnotScale );
    }}

    // Now scan the controls to find the min and max in each dimension.
    // Going to store min and max in ControlScaleOffsets - saves
    // allocating a new array.
    real32 *ControlScales = CurveData.ControlScaleOffsets;
    real32 *ControlOffsets = CurveData.ControlScaleOffsets + Dimension;
    {for ( int32 Dim = 0; Dim < Dimension; Dim++ )
    {
        ControlScales[Dim] = ControlSrc[Dim];
        ControlOffsets[Dim] = ControlSrc[Dim];
    }}

    real32 const *CurControlSrc = ControlSrc;
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        {for ( int32 Dim = 0; Dim < Dimension; Dim++ )
        {
            real32 ControlValue = *CurControlSrc++;
            if ( ControlScales[Dim] > ControlValue )
            {
                ControlScales[Dim] = ControlValue;
            }
            if ( ControlOffsets[Dim] < ControlValue )
            {
                ControlOffsets[Dim] = ControlValue;
            }
        }}
    }}

    // Figure out scale & offset.
    {for ( int32 Dim = 0; Dim < Dimension; Dim++ )
    {
        real32 Min = ControlScales[Dim];
        real32 Max = ControlOffsets[Dim];
        CompileAssert ( sizeof(*CurveData.KnotsControls) == 1 );
        ControlScales[Dim] = ( Max - Min ) / (real32)UInt8Maximum;
        ControlOffsets[Dim] = Min;

        if ( ControlScales[Dim] == 0.0f )
        {
            // Divide-by-zero is not fun.
            ControlScales[Dim] = 1.0f;
        }
    }}

    // And now write the actual control data.
    uint8 *CurControl = FirstControl;
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        {for ( int32 Dim = 0; Dim < Dimension; Dim++ )
        {
            real32 ControlValue = *ControlSrc++;
            real32 ScaledValue = ( ControlValue - ControlOffsets[Dim] ) / ControlScales[Dim];
            // In cases of very small ranges, this can go out of the 0-255 range because of precision problems.
            if ( ScaledValue < 0.0f )
            {
                Assert ( ScaledValue > -1.0f );
                ScaledValue = 0.0f;
            }
            else if ( ScaledValue > 255.0f )
            {
                Assert ( ScaledValue < 256.0f );
                ScaledValue = 255.0f;
            }
            *CurControl++ = (uint8)RoundReal32ToInt32 ( ScaledValue );
        }}
    }}
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    // Knot count of 0 should be DaIdentity, knot count of 1 should be DXConstant
    Assert ( Builder->KnotCount > 1 );
    AggrAllocPtr ( Allocator, LocalCurveData );
    // An array of the scales and offsets for the controls.
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->Dimension * 2,
                              ControlScaleOffsetCount, ControlScaleOffsets );
    // An array of both knots and controls (knots first, then controls).
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->KnotCount * ( Builder->Dimension + 1 ),
                              KnotControlCount, KnotsControls );
}

void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    CurveData.OneOverKnotScaleTrunc = CurveDataSource.OneOverKnotScaleTrunc;
    Assert ( CurveData.ControlScaleOffsetCount == CurveDataSource.ControlScaleOffsetCount );
    Copy32 ( CurveData.ControlScaleOffsetCount, CurveDataSource.ControlScaleOffsets, CurveData.ControlScaleOffsets );
    Assert ( CurveData.KnotControlCount == CurveDataSource.KnotControlCount );
    Copy ( CurveData.KnotControlCount * sizeof ( *CurveData.KnotsControls ), CurveDataSource.KnotsControls, CurveData.KnotsControls );
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( Dimension == CurveDimension );

    Assert ( CurveDimension <= MaximumBSplineDimension );
    uint8 TempControls[MaximumBSplineDimension];
    real32 TempScales[MaximumBSplineDimension];
    real32 TempOffsets[MaximumBSplineDimension];

    // Swizzle and adjust the scale+offset.
    // First half of ControlScaleOffsets are Scales, second half are Offsets

    // So the math goes that when an integer-format control is read, you do:
    // real_value = scale * int_value + offset
    // So now we want to scale and offset the result, so:
    // new_value = this_scale * real_value + this_offset
    //           = this_scale * ( scale * int_value + offset ) + this_offset
    //           = this_scale * scale * int_value + this_scale * offset + this_offset
    // And we need to find new_scale and new_offset so that:
    // new_value = new_scale * int_value + new_offset
    // Thus:
    // new_scale = this_scale * scale
    // new_offset = this_scale * offset + this_offset
    real32 *ControlScales = CurveData.ControlScaleOffsets;
    real32 *ControlOffsets = CurveData.ControlScaleOffsets + CurveDimension;
    Copy32 ( CurveDimension, ControlScales,  TempScales );
    Copy32 ( CurveDimension, ControlOffsets, TempOffsets );
    {for ( int32x CurDimension = 0; CurDimension < CurveDimension; CurDimension++ )
    {
        int32x Swizzle = Swizzles[CurDimension];
        Assert ( ( Swizzle >= 0 ) && ( Swizzle < CurveDimension ) );
        real32 OldScale  = TempScales [Swizzle];
        real32 OldOffset = TempOffsets[Swizzle];
        ControlScales[CurDimension] = Scales[CurDimension] * OldScale;
        ControlOffsets[CurDimension] = Scales[CurDimension] * OldOffset + Offsets[CurDimension];
    }}

    // Now swizzle the control data.
    // First part of KnotsControls is Knots, last part is Controls.
    uint8 *Control = CurveData.KnotsControls + CurveKnotCount;
    {for ( int32x ControlNum = 0; ControlNum < CurveKnotCount; ControlNum++ )
    {
        Copy ( CurveDimension * sizeof (Control[0]), Control, TempControls );
        {for ( int32x CurDimension = 0; CurDimension < CurveDimension; CurDimension++ )
        {
            // Scale and offsets happen above, so this just does swizzles.
            Control[CurDimension] = TempControls[Swizzles[CurDimension]];
        }}
        Control += CurveDimension;
    }}
}


#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE


//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(D4nK16uC15u);

#define MIXED_NAME D4nK16uC15u
#define LOWER_NAME d4n_k16u_c15u

// KnotControlCount = KnotCount + ControlCount
// ControlCount = KnotCount * 3
// Dimension = 4
#define FIND_DIMENSION (4)
#define FIND_KNOT_COUNT (CurveData.KnotControlCount >> 2)

#define EXTRACT_ONE_OVER_KNOT_SCALE OneOverKnotScale = CurveData.OneOverKnotScale
#define GET_KNOT_POINTER (CurveData.KnotsControls)


IMPL_CURVE_GET_DIMENSION(MIXED_NAME);
IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME);
IMPL_CURVE_FIND_KNOT_Quantized(MIXED_NAME, 16);
IMPL_CURVE_FIND_CLOSE_KNOT_Quantized(MIXED_NAME, 16);
IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);

void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( KnotIndex >= 0 );
    Assert ( KnotIndex + KnotCount <= CurveKnotCount );
    const uint16 *CurKnot    = CurveData.KnotsControls + KnotIndex;
    // Skip the knots, then find the right control.
    const uint16 *CurControl = CurveData.KnotsControls + CurveKnotCount + KnotIndex * 3;
    real32 KnotScale = 1.0f / CurveData.OneOverKnotScale;
    // Extract the scales and offsets.
    real32 ControlScales[4];
    real32 ControlOffsets[4];
    uint16 ScaleOffsetTableEntry = CurveData.ScaleOffsetTableEntries;
    {for ( int32x Dim = 0; Dim < 4; Dim++ )
    {
        int32x TableIndex = ScaleOffsetTableEntry & 0xf;
        ScaleOffsetTableEntry >>= 4;
        ControlScales [Dim] = QUATERNION_SCALE_ENTRY(TableIndex, 0) * (1.0f / ((real32)((1<<15)-1)));
        ControlOffsets[Dim] = QUATERNION_SCALE_ENTRY(TableIndex, 1);
    }}

    if ( KnotResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            *KnotResults++ = KnotScale * (real32)( *CurKnot++ );
        }}
    }
    if ( ControlResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            // Extract the three uint16s.
            uint16 Data[3];
            Data[0] = CurControl[0];
            Data[1] = CurControl[1];
            Data[2] = CurControl[2];
            // The upper bits are the special ones.
            // The first is the sign of the missing component.
            bool MissingComponentIsNegative = ( Data[0] & 0x8000 ) != 0;
            // The others are the index of the missing component.
            int32x MissingComponentIndex = ( ( Data[1] >> 14 ) & 0x2 ) | ( Data[2] >> 15 );
            // Swizzle, scale and offset the others in (remembering to mask off the top bits).
            int32x DstComp = MissingComponentIndex;
            real32 SummedSq = 0.0f;
            {for ( int32x SrcComp = 0; SrcComp < 3; SrcComp++ )
            {
                DstComp++;
                DstComp &= 0x3;
                real32 Result = ControlOffsets[DstComp] + ControlScales[DstComp] * (real32)( Data[SrcComp] & 0x7fff );
                Assert ( AbsoluteValue ( Result ) < 0.71f );
                SummedSq += Result * Result;
                ControlResults[DstComp] = Result;
            }}
            // Now compute the missing member.
            real32 MissingResult = SquareRoot ( 1.0f - SummedSq );
            Assert ( MissingResult > 0.4999f );
            if ( MissingComponentIsNegative )
            {
                MissingResult = -MissingResult;
            }
            ControlResults[MissingComponentIndex] = MissingResult;

            ControlResults += 4;
            CurControl += 3;
        }}
    }
}



void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME) ( curve2 &Curve,
                                        int32x KnotCount, int32x Dimension,
                                        real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( Dimension == CurveDimension );
    Assert ( KnotCount == CurveKnotCount );
    uint16 *FirstKnot    = CurveData.KnotsControls;
    // Skip the knots.
    uint16 *FirstControl = CurveData.KnotsControls + CurveKnotCount;

    real32 KnotMax = KnotSrc[CurveKnotCount-1];

    CompileAssert ( sizeof(*CurveData.KnotsControls) == 2 );
    CurveData.OneOverKnotScale = (real32)UInt16Maximum / KnotMax;
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        int32x KnotValue = RoundReal32ToInt32 ( KnotSrc[Count] * CurveData.OneOverKnotScale );
        Assert ( KnotValue >= 0 );
        Assert ( KnotValue < (1<<16) );
        FirstKnot[Count] = (uint16)KnotValue;
    }}

    // Now scan the controls to find the min and max in each dimension.
    real32 ControlMin[4];
    real32 ControlMax[4];
    {for ( int32 Dim = 0; Dim < 4; Dim++ )
    {
        // Init to extreme values (we know quaternions will be normalised).
        ControlMin[Dim] = 2.0f;
        ControlMax[Dim] = -2.0f;
    }}


    // Scan the data and for each quat, normalise it, then
    // find out which component has the largest magnitude.
    // That's the one we're going to throw away.
    // Find the min and max of the others (NOT the one being binned).
    // I'm going to store the decision about which component to
    // throw away in the tail-end of the KnotsControls array -
    // again, no point in allocating a new one.
    uint16 *DiscardedControlIndex = ( CurveData.KnotsControls + CurveData.KnotControlCount - KnotCount );
    real32 const *CurControlSrc = ControlSrc;
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        // Copy and normalise.
        real32 Control[4];
        VectorEquals4 ( Control, CurControlSrc );

        // The solver occasionally turns out 0 quaternions.  Turn those into identity
        // transforms.  If that causes errors, the compressor will handle throwing out the
        // result.
        if (NormalizeOrZero4 ( Control ) == 0.0f)
        {
            Control[3] = 1.0f;
        }

        int32x Discard = 0;
        {for ( int32 Dim = 1; Dim < 4; Dim++ )
        {
            if ( AbsoluteValue ( Control[Dim] ) > AbsoluteValue ( Control[Discard] ) )
            {
                Discard = Dim;
            }
        }}
        // In a normalised quat, the smallest the largest value can be is if all four values are 0.5.
        Assert ( AbsoluteValue ( Control[Discard] ) > 0.4999f );
        *DiscardedControlIndex++ = (uint16)Discard;

        {for ( int32 Dim = 0; Dim < 4; Dim++ )
        {
            real32 ControlValue = Control[Dim];
            // Do not check the range of discarded values.
            if ( Dim != Discard )
            {
                if ( ControlMin[Dim] > ControlValue )
                {
                    ControlMin[Dim] = ControlValue;
                    Assert ( ControlMin[Dim] >= -0.708f );
                }
                if ( ControlMax[Dim] < ControlValue )
                {
                    ControlMax[Dim] = ControlValue;
                    Assert ( ControlMax[Dim] <=  0.708f );
                }
            }
        }}
        CurControlSrc += 4;
    }}



    // Now find which of the possible table entries this range would have fitted in.
    CurveData.ScaleOffsetTableEntries = 0;
    real32 ControlScales[4];
    real32 ControlOffsets[4];
    {for ( int32 Dim = 0; Dim < 4; Dim++ )
    {
        int32x ScaleOffsetTableEntry = -1;
        if ( ( ControlMin[Dim] == 2.0f ) && ( ControlMax[Dim] == -2.0f ) )
        {
            // Unused component.
            ControlScales[Dim] = 1.0f;
            ControlOffsets[Dim] = 0.0f;
        }
        else
        {
            Assert ( ControlMin[Dim] <= ControlMax[Dim] );
            // Scan from bottom upwards, to find the smallest ranges first (more precision).
            {for ( int32x TableEntry = 7; TableEntry >= 0; TableEntry-- )
            {
                real32 Min = QUATERNION_SCALE_ENTRY(TableEntry, 1);
                real32 Max = QUATERNION_SCALE_ENTRY(TableEntry, 0) + Min;
                if ( ( Min <= ControlMin[Dim] ) && ( Max >= ControlMax[Dim] ) )
                {
                    // This will fit.
                    ScaleOffsetTableEntry = TableEntry;
                    break;
                }
            }}
            if ( ScaleOffsetTableEntry < 0 )
            {
                // This can happen because of floating-point imprecision. Ah well.
                // InvalidCodePath ( "Could not find suitable scale-offset table" );
                ScaleOffsetTableEntry = 0;
            }
            CurveData.ScaleOffsetTableEntries |= ( ScaleOffsetTableEntry << ( Dim * 4 ) );
            ControlScales[Dim] = QUATERNION_SCALE_ENTRY(ScaleOffsetTableEntry, 0) / ((real32)((1<<15)-1));
            ControlOffsets[Dim] = QUATERNION_SCALE_ENTRY(ScaleOffsetTableEntry, 1);
        }
    }}


    // And now write the actual control data.
    // The three uint16s are the three controls that were not discarded,
    // and their top bits are replaced by the index of the discarded component,
    // and its sign.
    uint16 *CurControl = FirstControl;
    CurControlSrc = ControlSrc;
    DiscardedControlIndex = ( CurveData.KnotsControls + CurveData.KnotControlCount - KnotCount );
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        // Copy and normalise.
        real32 Control[4];
        VectorEquals4 ( Control, CurControlSrc );

        // The solver occasionally turns out 0 quaternions.  Turn those into identity
        // transforms.  If that causes errors, the compressor will handle throwing out the
        // result.
        if (NormalizeOrZero4 ( Control ) == 0.0f)
        {
            Control[3] = 1.0f;
        }

        int32x DiscardedComponent = *DiscardedControlIndex++;
        Assert ( AbsoluteValue ( Control[DiscardedComponent] ) >= 0.4999f );

        // Scale the components.
        uint16 UintControl[4];
        {for ( int32 Dim = 0; Dim < 4; Dim++ )
        {
            if ( Dim != DiscardedComponent )
            {
                real32 ScaledValue = ( Control[Dim] - ControlOffsets[Dim] ) / ControlScales[Dim];
                // In cases of very small ranges, this can go out of the 0-32767 range because of precision problems.
                if ( ScaledValue < 0.0f )
                {
                    Assert ( ScaledValue > -1.0f );
                    ScaledValue = 0.0f;
                }
                else if ( ScaledValue > 32767.0f )
                {
                    Assert ( ScaledValue < 32768.0f );
                    ScaledValue = 32767.0f;
                }
                UintControl[Dim] = (uint16)RoundReal32ToInt32 ( ScaledValue );
            }
        }}

        // Now copy the non-discarded ones.
        int32x SrcDim = DiscardedComponent;
        {for ( int32x DstDim = 0; DstDim < 3; DstDim++ )
        {
            SrcDim++;
            SrcDim &= 0x3;
            // We're about to mangle the top bit...
            Assert ( ( UintControl[SrcDim] & 0x8000 ) == 0 );
            CurControl[DstDim] = UintControl[SrcDim];
        }}

        // And now pack in the sign and discard data.
        if ( Control[DiscardedComponent] < 0.0f )
        {
            CurControl[0] |= 0x8000;
        }
        CurControl[1] |= ( DiscardedComponent & 0x2 ) << 14;
        CurControl[2] |= ( DiscardedComponent & 0x1 ) << 15;

        CurControlSrc += 4;
        CurControl += 3;
    }}
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    // Knot count of 0 should be DaIdentity, knot count of 1 should be DXConstant
    Assert ( Builder->KnotCount > 1 );
    Assert ( Builder->Dimension == 4 );
    AggrAllocPtr ( Allocator, LocalCurveData );
    // An array of both knots and controls (knots first, then controls).
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->KnotCount * 4,
                              KnotControlCount, KnotsControls );
}

void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    CurveData.OneOverKnotScale = CurveDataSource.OneOverKnotScale;
    CurveData.ScaleOffsetTableEntries = CurveDataSource.ScaleOffsetTableEntries;
    Assert ( CurveData.KnotControlCount == CurveDataSource.KnotControlCount );
    Copy ( CurveData.KnotControlCount * sizeof ( *CurveData.KnotsControls ), CurveDataSource.KnotsControls, CurveData.KnotsControls );
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( Dimension == CurveDimension );

    // Since this is quaternion data, it makes no sense to try to offset the data.
    Assert ( Offsets[0] == 0.0f );
    Assert ( Offsets[1] == 0.0f );
    Assert ( Offsets[2] == 0.0f );
    // Also, scales must be either +1 or -1
    Assert ( AbsoluteValue ( Scales[0] ) == 1.0f );
    Assert ( AbsoluteValue ( Scales[1] ) == 1.0f );
    Assert ( AbsoluteValue ( Scales[2] ) == 1.0f );
    // And it's a quat, so W should be unaffected.
    Assert ( Offsets[3] == 0.0f );
    Assert ( Scales[3] == 1.0f );
    Assert ( Swizzles[3] == 3 );

    uint16 ScaleOffsetTableCurrent = CurveData.ScaleOffsetTableEntries;
    CurveData.ScaleOffsetTableEntries = 0;
    {for ( int32x CurDimension = 0; CurDimension < 4; CurDimension++ )
    {
        int32x Swizzle = Swizzles[CurDimension];
        int32x Entry = ( ScaleOffsetTableCurrent >> ( 4 * Swizzle ) ) & 0xf;
        if ( Scales[CurDimension] < 0.0f )
        {
            Assert ( Scales[CurDimension] == -1.0f );
            // Flip the top bit, to get it in the other half of the table,
            // which is the negative of the other half.
            Entry ^= 0x8;
        }
        else
        {
            Assert ( Scales[CurDimension] == 1.0f );
        }
        CurveData.ScaleOffsetTableEntries |= ( Entry << ( CurDimension * 4 ) );
    }}

    // Now swizzle the control data.
    // First part of KnotsControls is Knots, last part is Controls.
    uint16 *CurControl = CurveData.KnotsControls + CurveKnotCount;
    {for ( int32x ControlNum = 0; ControlNum < CurveKnotCount; ControlNum++ )
    {
        // Extract the three uint16s to the correct components.
        uint16 Data[3];
        Data[0] = CurControl[0];
        Data[1] = CurControl[1];
        Data[2] = CurControl[2];
        // The upper bits are the special ones.
        // The first is the sign of the missing component.
        bool MissingComponentIsNegative = ( Data[0] & 0x8000 ) != 0;
        // The others are the index of the missing component.
        int32x MissingComponentIndex = ( ( Data[1] >> 14 ) & 0x2 ) | ( Data[2] >> 15 );
        uint16 TempControls[4];
        int32x DstComp = MissingComponentIndex;
        {for ( int32x SrcComp = 0; SrcComp < 3; SrcComp++ )
        {
            DstComp++;
            DstComp &= 0x3;
            TempControls[DstComp] = (uint16)( Data[SrcComp] & 0x7fff );
        }}

        // OK, now swizzle.
        {for ( int32x Comp = 0; Comp < 4; Comp++ )
        {
            if ( Swizzles[Comp] == MissingComponentIndex )
            {
                MissingComponentIndex = Comp;
                break;
            }
        }}
        if ( Scales[MissingComponentIndex] < 0.0f )
        {
            Assert ( Scales[MissingComponentIndex] == -1.0f );
            MissingComponentIsNegative = !MissingComponentIsNegative;
        }
        else
        {
            Assert ( Scales[MissingComponentIndex] == 1.0f );
        }

        uint16 SwizzledControls[4];
        {for ( int32x Comp = 0; Comp < 4; Comp++ )
        {
            SwizzledControls[Comp] = TempControls[Swizzles[Comp]];
        }}

        // Now write back.
        int32x SrcComp = MissingComponentIndex;
        {for ( int32x DstComp = 0; DstComp < 3; DstComp++ )
        {
            SrcComp++;
            SrcComp &= 0x3;
            Assert ( ( SwizzledControls[SrcComp] & 0x8000 ) == 0 );
            CurControl[DstComp] = SwizzledControls[SrcComp];
        }}

        // And add back in the three magic bits.
        if ( MissingComponentIsNegative )
        {
            CurControl[0] |= 0x8000;
        }
        CurControl[1] |= ( MissingComponentIndex & 0x2 ) << 14;
        CurControl[2] |= ( MissingComponentIndex & 0x1 ) << 15;

        // Phew!

        CurControl += 3;
    }}
}

#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE


//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(D4nK8uC7u);
// NOTE - these routines are almost identical to D4nK16uC15u, but with the types and some ranges tweaked.

#define MIXED_NAME D4nK8uC7u
#define LOWER_NAME d4n_k8u_c7u

// KnotControlCount = KnotCount + ControlCount
// ControlCount = KnotCount * 3
// Dimension = 4
#define FIND_DIMENSION (4)
#define FIND_KNOT_COUNT (CurveData.KnotControlCount >> 2)
#define GET_KNOT_POINTER (CurveData.KnotsControls)

#define EXTRACT_ONE_OVER_KNOT_SCALE OneOverKnotScale = CurveData.OneOverKnotScale


IMPL_CURVE_GET_DIMENSION(MIXED_NAME);
IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME);
IMPL_CURVE_FIND_KNOT_Quantized(MIXED_NAME, 8);
IMPL_CURVE_FIND_CLOSE_KNOT_Quantized(MIXED_NAME, 8);
IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);

// Xenon has a custom version of this command in the platform file.
#if !PLATFORM_XENON && !PLATFORM_PS3

void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( KnotIndex >= 0 );
    Assert ( KnotIndex + KnotCount <= CurveKnotCount );
    const uint8 *CurKnot    = CurveData.KnotsControls + KnotIndex;
    // Skip the knots, then find the right control.
    const uint8 *CurControl = CurveData.KnotsControls + CurveKnotCount + KnotIndex * 3;
    real32 KnotScale = 1.0f / CurveData.OneOverKnotScale;
    // Extract the scales and offsets.
    real32 ControlScales[4];
    real32 ControlOffsets[4];
    uint16 ScaleOffsetTableEntry = CurveData.ScaleOffsetTableEntries;
    {for ( int32x Dim = 0; Dim < 4; Dim++ )
    {
        int32x TableIndex = ScaleOffsetTableEntry & 0xf;
        ScaleOffsetTableEntry >>= 4;
        ControlScales [Dim] = QUATERNION_SCALE_ENTRY(TableIndex, 0) * (1.0f / ((real32)((1<<7)-1)));
        ControlOffsets[Dim] = QUATERNION_SCALE_ENTRY(TableIndex, 1);
    }}

    if ( KnotResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            *KnotResults++ = KnotScale * (real32)( *CurKnot++ );
        }}
    }
    if ( ControlResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            // Extract the three uint8s.
            uint8 Data[3];
            Data[0] = CurControl[0];
            Data[1] = CurControl[1];
            Data[2] = CurControl[2];
            // The upper bits are the special ones.
            // The first is the sign of the missing component.
            bool MissingComponentIsNegative = ( Data[0] & 0x80 ) != 0;
            // The others are the index of the missing component.
            int32x MissingComponentIndex = ( ( Data[1] >> 6 ) & 0x2 ) | ( Data[2] >> 7 );
            // Swizzle, scale and offset the others in (remembering to mask off the top bits).
            int32x DstComp = MissingComponentIndex;
            real32 SummedSq = 0.0f;
            {for ( int32x SrcComp = 0; SrcComp < 3; SrcComp++ )
            {
                DstComp++;
                DstComp &= 0x3;
                real32 Result = ControlOffsets[DstComp] + ControlScales[DstComp] * (real32)( Data[SrcComp] & 0x7f );
                Assert ( AbsoluteValue ( Result ) < 0.71f );
                SummedSq += Result * Result;
                ControlResults[DstComp] = Result;
            }}
            // Now compute the missing member.
            real32 MissingResult = SquareRoot ( 1.0f - SummedSq );
            Assert ( MissingResult > 0.49f );
            if ( MissingComponentIsNegative )
            {
                MissingResult = -MissingResult;
            }
            ControlResults[MissingComponentIndex] = MissingResult;

            ControlResults += 4;
            CurControl += 3;
        }}
    }
}

#endif


void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME) ( curve2 &Curve,
                                        int32x KnotCount, int32x Dimension,
                                        real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( Dimension == CurveDimension );
    Assert ( KnotCount == CurveKnotCount );
    uint8 *FirstKnot    = CurveData.KnotsControls;
    // Skip the knots.
    uint8 *FirstControl = CurveData.KnotsControls + CurveKnotCount;

    real32 KnotMax = KnotSrc[CurveKnotCount-1];

    CompileAssert ( sizeof(*CurveData.KnotsControls) == 1 );
    CurveData.OneOverKnotScale = (real32)UInt8Maximum / KnotMax;
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        int32x KnotValue = RoundReal32ToInt32 ( KnotSrc[Count] * CurveData.OneOverKnotScale );
        Assert ( KnotValue >= 0 );
        Assert ( KnotValue < (1<<8) );
        FirstKnot[Count] = (uint8)KnotValue;
    }}

    // Now scan the controls to find the min and max in each dimension.
    real32 ControlMin[4];
    real32 ControlMax[4];
    {for ( int32 Dim = 0; Dim < 4; Dim++ )
    {
        // Init to extreme values (we know quaternions will be normalised).
        ControlMin[Dim] = 2.0f;
        ControlMax[Dim] = -2.0f;
    }}


    // Scan the data and for each quat, normalise it, then
    // find out which component has the largest magnitude.
    // That's the one we're going to throw away.
    // Find the min and max of the others (NOT the one being binned).
    // I'm going to store the decision about which component to
    // throw away in the tail-end of the KnotsControls array -
    // again, no point in allocating a new one.
    uint8 *DiscardedControlIndex = ( CurveData.KnotsControls + CurveData.KnotControlCount - KnotCount );
    real32 const *CurControlSrc = ControlSrc;
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        // Copy and normalise.
        real32 Control[4];
        VectorEquals4 ( Control, CurControlSrc );

        // The solver occasionally turns out 0 quaternions.  Turn those into identity
        // transforms.  If that causes errors, the compressor will handle throwing out the
        // result.
        if (NormalizeOrZero4 ( Control ) == 0.0f)
        {
            Control[3] = 1.0f;
        }

        int32x Discard = 0;
        {for ( int32 Dim = 1; Dim < 4; Dim++ )
        {
            if ( AbsoluteValue ( Control[Dim] ) > AbsoluteValue ( Control[Discard] ) )
            {
                Discard = Dim;
            }
        }}
        // In a normalised quat, the smallest the largest value can be is if all four values are 0.5.
        Assert ( AbsoluteValue ( Control[Discard] ) > 0.4999f );
        *DiscardedControlIndex++ = (uint8)Discard;

        {for ( int32 Dim = 0; Dim < 4; Dim++ )
        {
            real32 ControlValue = Control[Dim];
            // Do not check the range of discarded values.
            if ( Dim != Discard )
            {
                if ( ControlMin[Dim] > ControlValue )
                {
                    ControlMin[Dim] = ControlValue;
                    Assert ( ControlMin[Dim] >= -0.708f );
                }
                if ( ControlMax[Dim] < ControlValue )
                {
                    ControlMax[Dim] = ControlValue;
                    Assert ( ControlMax[Dim] <=  0.708f );
                }
            }
        }}
        CurControlSrc += 4;
    }}



    // Now find which of the possible table entries this range would have fitted in.
    CurveData.ScaleOffsetTableEntries = 0;
    real32 ControlScales[4];
    real32 ControlOffsets[4];
    {for ( int32 Dim = 0; Dim < 4; Dim++ )
    {
        int32x ScaleOffsetTableEntry = -1;
        if ( ( ControlMin[Dim] == 2.0f ) && ( ControlMax[Dim] == -2.0f ) )
        {
            // Unused component.
            ControlScales[Dim] = 1.0f;
            ControlOffsets[Dim] = 0.0f;
        }
        else
        {
            Assert ( ControlMin[Dim] <= ControlMax[Dim] );
            // Scan from bottom upwards, to find the smallest ranges first (more precision).
            {for ( int32x TableEntry = 7; TableEntry >= 0; TableEntry-- )
            {
                real32 Min = QUATERNION_SCALE_ENTRY(TableEntry, 1);
                real32 Max = QUATERNION_SCALE_ENTRY(TableEntry, 0) + Min;
                if ( ( Min <= ControlMin[Dim] ) && ( Max >= ControlMax[Dim] ) )
                {
                    // This will fit.
                    ScaleOffsetTableEntry = TableEntry;
                    break;
                }
            }}
            if ( ScaleOffsetTableEntry < 0 )
            {
                // This can happen because of floating-point imprecision. Ah well.
                // InvalidCodePath ( "Could not find suitable scale-offset table" );
                ScaleOffsetTableEntry = 0;
            }
            CurveData.ScaleOffsetTableEntries |= ( ScaleOffsetTableEntry << ( Dim * 4 ) );
            ControlScales[Dim]  = QUATERNION_SCALE_ENTRY(ScaleOffsetTableEntry, 0) / ((real32)((1<<7)-1));
            ControlOffsets[Dim] = QUATERNION_SCALE_ENTRY(ScaleOffsetTableEntry, 1);
        }
    }}


    // And now write the actual control data.
    // The three uint8s are the three controls that were not discarded,
    // and their top bits are replaced by the index of the discarded component,
    // and its sign.
    uint8 *CurControl = FirstControl;
    CurControlSrc = ControlSrc;
    DiscardedControlIndex = ( CurveData.KnotsControls + CurveData.KnotControlCount - KnotCount );
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        // Copy and normalise.
        real32 Control[4];
        VectorEquals4 ( Control, CurControlSrc );

        // The solver occasionally turns out 0 quaternions.  Turn those into identity
        // transforms.  If that causes errors, the compressor will handle throwing out the
        // result.
        if (NormalizeOrZero4 ( Control ) == 0.0f)
        {
            Control[3] = 1.0f;
        }

        int32x DiscardedComponent = *DiscardedControlIndex++;
        Assert ( AbsoluteValue ( Control[DiscardedComponent] ) >= 0.4999f );

        // Scale the components.
        uint8 UintControl[4];
        {for ( int32 Dim = 0; Dim < 4; Dim++ )
        {
            if ( Dim != DiscardedComponent )
            {
                real32 ScaledValue = ( Control[Dim] - ControlOffsets[Dim] ) / ControlScales[Dim];
                // In cases of very small ranges, this can go out of the 0-127 range because of precision problems.
                if ( ScaledValue < 0.0f )
                {
                    Assert ( ScaledValue > -1.0f );
                    ScaledValue = 0.0f;
                }
                else if ( ScaledValue > 127.0f )
                {
                    Assert ( ScaledValue < 128.0f );
                    ScaledValue = 127.0f;
                }
                UintControl[Dim] = (uint8)RoundReal32ToInt32 ( ScaledValue );
            }
        }}

        // Now copy the non-discarded ones.
        int32x SrcDim = DiscardedComponent;
        {for ( int32x DstDim = 0; DstDim < 3; DstDim++ )
        {
            SrcDim++;
            SrcDim &= 0x3;
            // We're about to mangle the top bit...
            Assert ( ( UintControl[SrcDim] & 0x80 ) == 0 );
            CurControl[DstDim] = UintControl[SrcDim];
        }}

        // And now pack in the sign and discard data.
        if ( Control[DiscardedComponent] < 0.0f )
        {
            CurControl[0] |= 0x80;
        }
        CurControl[1] |= ( DiscardedComponent & 0x2 ) << 6;
        CurControl[2] |= ( DiscardedComponent & 0x1 ) << 7;

        CurControlSrc += 4;
        CurControl += 3;
    }}
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    // Knot count of 0 should be DaIdentity, knot count of 1 should be DXConstant
    Assert ( Builder->KnotCount > 1 );
    Assert ( Builder->Dimension == 4 );
    AggrAllocPtr ( Allocator, LocalCurveData );
    // An array of both knots and controls (knots first, then controls).
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->KnotCount * 4,
                              KnotControlCount, KnotsControls );
}

void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    CurveData.OneOverKnotScale = CurveDataSource.OneOverKnotScale;
    CurveData.ScaleOffsetTableEntries = CurveDataSource.ScaleOffsetTableEntries;
    Assert ( CurveData.KnotControlCount == CurveDataSource.KnotControlCount );
    Copy ( CurveData.KnotControlCount * sizeof ( *CurveData.KnotsControls ), CurveDataSource.KnotsControls, CurveData.KnotsControls );
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( Dimension == CurveDimension );

    // Since this is quaternion data, it makes no sense to try to offset the data.
    Assert ( Offsets[0] == 0.0f );
    Assert ( Offsets[1] == 0.0f );
    Assert ( Offsets[2] == 0.0f );
    // Also, scales must be either +1 or -1
    Assert ( AbsoluteValue ( Scales[0] ) == 1.0f );
    Assert ( AbsoluteValue ( Scales[1] ) == 1.0f );
    Assert ( AbsoluteValue ( Scales[2] ) == 1.0f );
    // And it's a quat, so W should be unaffected.
    Assert ( Offsets[3] == 0.0f );
    Assert ( Scales[3] == 1.0f );
    Assert ( Swizzles[3] == 3 );

    uint16 ScaleOffsetTableCurrent = CurveData.ScaleOffsetTableEntries;
    CurveData.ScaleOffsetTableEntries = 0;
    {for ( int32x CurDimension = 0; CurDimension < 4; CurDimension++ )
    {
        int32x Swizzle = Swizzles[CurDimension];
        int32x Entry = ( ScaleOffsetTableCurrent >> ( 4 * Swizzle ) ) & 0xf;
        if ( Scales[CurDimension] < 0.0f )
        {
            Assert ( Scales[CurDimension] == -1.0f );
            // Flip the top bit, to get it in the other half of the table,
            // which is the negative of the other half.
            Entry ^= 0x8;
        }
        else
        {
            Assert ( Scales[CurDimension] == 1.0f );
        }
        CurveData.ScaleOffsetTableEntries |= ( Entry << ( CurDimension * 4 ) );
    }}

    // Now swizzle the control data.
    // First part of KnotsControls is Knots, last part is Controls.
    uint8 *CurControl = CurveData.KnotsControls + CurveKnotCount;
    {for ( int32x ControlNum = 0; ControlNum < CurveKnotCount; ControlNum++ )
    {
        // Extract the three uint8s to the correct components.
        uint8 Data[3];
        Data[0] = CurControl[0];
        Data[1] = CurControl[1];
        Data[2] = CurControl[2];
        // The upper bits are the special ones.
        // The first is the sign of the missing component.
        bool MissingComponentIsNegative = ( Data[0] & 0x80 ) != 0;
        // The others are the index of the missing component.
        int32x MissingComponentIndex = ( ( Data[1] >> 6 ) & 0x2 ) | ( Data[2] >> 7 );
        uint8 TempControls[4];
        int32x DstComp = MissingComponentIndex;
        {for ( int32x SrcComp = 0; SrcComp < 3; SrcComp++ )
        {
            DstComp++;
            DstComp &= 0x3;
            TempControls[DstComp] = (uint8)( Data[SrcComp] & 0x7f );
        }}

        // OK, now swizzle.
        {for ( int32x Comp = 0; Comp < 4; Comp++ )
        {
            if ( Swizzles[Comp] == MissingComponentIndex )
            {
                MissingComponentIndex = Comp;
                break;
            }
        }}
        if ( Scales[MissingComponentIndex] < 0.0f )
        {
            Assert ( Scales[MissingComponentIndex] == -1.0f );
            MissingComponentIsNegative = !MissingComponentIsNegative;
        }
        else
        {
            Assert ( Scales[MissingComponentIndex] == 1.0f );
        }

        uint8 SwizzledControls[4];
        {for ( int32x Comp = 0; Comp < 4; Comp++ )
        {
            SwizzledControls[Comp] = TempControls[Swizzles[Comp]];
        }}

        // Now write back.
        int32x SrcComp = MissingComponentIndex;
        {for ( int32x DstComp = 0; DstComp < 3; DstComp++ )
        {
            SrcComp++;
            SrcComp &= 0x3;
            Assert ( ( SwizzledControls[SrcComp] & 0x80 ) == 0 );
            CurControl[DstComp] = SwizzledControls[SrcComp];
        }}

        // And add back in the three magic bits.
        if ( MissingComponentIsNegative )
        {
            CurControl[0] |= 0x80;
        }
        CurControl[1] |= ( MissingComponentIndex & 0x2 ) << 6;
        CurControl[2] |= ( MissingComponentIndex & 0x1 ) << 7;

        // Phew!

        CurControl += 3;
    }}
}

#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE



//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(D3K16uC16u);

#define MIXED_NAME D3K16uC16u
#define LOWER_NAME d3_k16u_c16u

// KnotControlCount = KnotCount + ControlCount
// ControlCount = KnotCount * Dimension
// Dimension = 3
#define FIND_DIMENSION (3)
#define FIND_KNOT_COUNT (CurveData.KnotControlCount >>2)
// Only the top half of the IEEE float OneOverKnotScale is stored!
#define EXTRACT_ONE_OVER_KNOT_SCALE *(uint32*)(&OneOverKnotScale) = (((uint32)CurveData.OneOverKnotScaleTrunc)<<16)
#define GET_KNOT_POINTER (CurveData.KnotsControls)

IMPL_CURVE_GET_DIMENSION(MIXED_NAME);
IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME);
IMPL_CURVE_FIND_KNOT_Quantized(MIXED_NAME, 16);
IMPL_CURVE_FIND_CLOSE_KNOT_Quantized(MIXED_NAME, 16);
IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);

void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    real32 OneOverKnotScale;
    EXTRACT_ONE_OVER_KNOT_SCALE;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( KnotIndex >= 0 );
    Assert ( KnotIndex + KnotCount <= CurveKnotCount );
    const uint16 *CurKnot    = CurveData.KnotsControls + KnotIndex;
    // Skip the knots, then find the right control.
    const uint16 *CurControl = CurveData.KnotsControls + CurveKnotCount + KnotIndex * CurveDimension;
    real32 KnotScale = 1.0f / OneOverKnotScale;
    const real32 *ControlScales = CurveData.ControlScales;
    const real32 *ControlOffsets = CurveData.ControlOffsets;
    if ( KnotResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            *KnotResults++ = KnotScale * (real32)( *CurKnot++ );
        }}
    }
    if ( ControlResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            {for ( int32 Dim = 0; Dim < CurveDimension; Dim++ )
            {
                *ControlResults++ = ControlOffsets[Dim] + ControlScales[Dim] * (real32)( *CurControl++ );
            }}
        }}
    }
}

void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME) ( curve2 &Curve,
                                        int32x KnotCount, int32x Dimension,
                                        real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( Dimension == CurveDimension );
    Assert ( KnotCount == CurveKnotCount );
    uint16 *FirstKnot    = CurveData.KnotsControls;
    // Skip the knots.
    uint16 *FirstControl = CurveData.KnotsControls + CurveKnotCount;

    real32 KnotMax = KnotSrc[CurveKnotCount-1];

    CompileAssert ( sizeof(*CurveData.KnotsControls) == 2 );
    real32 OneOverKnotScale = (real32)UInt16Maximum / KnotMax;
    CurveData.OneOverKnotScaleTrunc = (uint16)( (*(uint32*)(&OneOverKnotScale))>>16 );
    // And then re-extract it, coz it will have changed slightly.
    EXTRACT_ONE_OVER_KNOT_SCALE;
    Assert((real32)UInt16Maximum / KnotMax >= OneOverKnotScale);
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        FirstKnot[Count] = (uint16)RoundReal32ToInt32 ( KnotSrc[Count] * OneOverKnotScale );
    }}

    // Now scan the controls to find the min and max in each dimension.
    // Going to store min and max in ControlScaleOffsets - saves
    // allocating a new array.
    real32 *ControlScales = CurveData.ControlScales;
    real32 *ControlOffsets = CurveData.ControlOffsets;
    {for ( int32 Dim = 0; Dim < CurveDimension; Dim++ )
    {
        ControlScales[Dim] = ControlSrc[Dim];
        ControlOffsets[Dim] = ControlSrc[Dim];
    }}

    real32 const *CurControlSrc = ControlSrc;
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        {for ( int32 Dim = 0; Dim < CurveDimension; Dim++ )
        {
            real32 ControlValue = *CurControlSrc++;
            if ( ControlScales[Dim] > ControlValue )
            {
                ControlScales[Dim] = ControlValue;
            }
            if ( ControlOffsets[Dim] < ControlValue )
            {
                ControlOffsets[Dim] = ControlValue;
            }
        }}
    }}

    // Figure out scale & offset.
    {for ( int32 Dim = 0; Dim < CurveDimension; Dim++ )
    {
        real32 Min = ControlScales[Dim];
        real32 Max = ControlOffsets[Dim];
        CompileAssert ( sizeof(*CurveData.KnotsControls) == 2 );
        ControlScales[Dim] = ( Max - Min ) / (real32)UInt16Maximum;
        ControlOffsets[Dim] = Min;

        if ( ControlScales[Dim] == 0.0f )
        {
            // Divide-by-zero is not fun.
            ControlScales[Dim] = 1.0f;
        }
    }}

    // And now write the actual control data.
    uint16 *CurControl = FirstControl;
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        {for ( int32 Dim = 0; Dim < CurveDimension; Dim++ )
        {
            real32 ControlValue = *ControlSrc++;
            real32 ScaledValue = ( ControlValue - ControlOffsets[Dim] ) / ControlScales[Dim];
            // In cases of very small ranges, this can go out of the 0-65535 range because of precision problems.
            if ( ScaledValue < 0.0f )
            {
                Assert ( ScaledValue > -1.0f );
                ScaledValue = 0.0f;
            }
            else if ( ScaledValue > 65535.0f )
            {
                Assert ( ScaledValue < 65536.0f );
                ScaledValue = 65535.0f;
            }
            *CurControl++ = (uint16)RoundReal32ToInt32 ( ScaledValue );
        }}
    }}
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    // Knot count of 0 should be DaIdentity, knot count of 1 should be DXConstant
    Assert ( Builder->KnotCount > 1 );
    Assert ( Builder->Dimension == 3 );
    AggrAllocPtr ( Allocator, LocalCurveData );
    // An array of both knots and controls (knots first, then controls).
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->KnotCount * ( 3 + 1 ),
                              KnotControlCount, KnotsControls );
}

void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    CurveData.OneOverKnotScaleTrunc = CurveDataSource.OneOverKnotScaleTrunc;
    Copy32 ( 3, CurveDataSource.ControlScales, CurveData.ControlScales );
    Copy32 ( 3, CurveDataSource.ControlOffsets, CurveData.ControlOffsets );
    Assert ( CurveData.KnotControlCount == CurveDataSource.KnotControlCount );
    Copy ( CurveData.KnotControlCount * sizeof ( *CurveData.KnotsControls ), CurveDataSource.KnotsControls, CurveData.KnotsControls );
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( Dimension == CurveDimension );

    Assert ( CurveDimension <= MaximumBSplineDimension );
    uint16 TempControls[MaximumBSplineDimension];
    real32 TempScales[MaximumBSplineDimension];
    real32 TempOffsets[MaximumBSplineDimension];

    // Swizzle and adjust the scale+offset.
    // First half of ControlScaleOffsets are Scales, second half are Offsets

    // So the math goes that when an integer-format control is read, you do:
    // real_value = scale * int_value + offset
    // So now we want to scale and offset the result, so:
    // new_value = this_scale * real_value + this_offset
    //           = this_scale * ( scale * int_value + offset ) + this_offset
    //           = this_scale * scale * int_value + this_scale * offset + this_offset
    // And we need to find new_scale and new_offset so that:
    // new_value = new_scale * int_value + new_offset
    // Thus:
    // new_scale = this_scale * scale
    // new_offset = this_scale * offset + this_offset
    real32 *ControlScales = CurveData.ControlScales;
    real32 *ControlOffsets = CurveData.ControlOffsets;
    Copy32 ( CurveDimension, ControlScales,  TempScales );
    Copy32 ( CurveDimension, ControlOffsets, TempOffsets );
    {for ( int32x CurDimension = 0; CurDimension < CurveDimension; CurDimension++ )
    {
        int32x Swizzle = Swizzles[CurDimension];
        Assert ( ( Swizzle >= 0 ) && ( Swizzle < CurveDimension ) );
        real32 OldScale  = TempScales [Swizzle];
        real32 OldOffset = TempOffsets[Swizzle];
        ControlScales[CurDimension] = Scales[CurDimension] * OldScale;
        ControlOffsets[CurDimension] = Scales[CurDimension] * OldOffset + Offsets[CurDimension];
    }}

    // Now swizzle the control data.
    // First part of KnotsControls is Knots, last part is Controls.
    uint16 *Control = CurveData.KnotsControls + CurveKnotCount;
    {for ( int32x ControlNum = 0; ControlNum < CurveKnotCount; ControlNum++ )
    {
        Copy ( CurveDimension * sizeof (Control[0]), Control, TempControls );
        {for ( int32x CurDimension = 0; CurDimension < CurveDimension; CurDimension++ )
        {
            // Scale and offsets happen above, so this just does swizzles.
            Control[CurDimension] = TempControls[Swizzles[CurDimension]];
        }}
        Control += CurveDimension;
    }}
}

#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE


//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(D3K8uC8u);

#define MIXED_NAME D3K8uC8u
#define LOWER_NAME d3_k8u_c8u

// KnotControlCount = KnotCount + ControlCount
// ControlCount = KnotCount * Dimension
// Dimension = 3
#define FIND_DIMENSION (3)
#define FIND_KNOT_COUNT (CurveData.KnotControlCount >> 2)
// Only the top half of the IEEE float OneOverKnotScale is stored!
#define EXTRACT_ONE_OVER_KNOT_SCALE *(uint32*)(&OneOverKnotScale) = (((uint32)CurveData.OneOverKnotScaleTrunc)<<16)
#define GET_KNOT_POINTER (CurveData.KnotsControls)

IMPL_CURVE_GET_DIMENSION(MIXED_NAME);
IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME);
IMPL_CURVE_FIND_KNOT_Quantized(MIXED_NAME, 8);
IMPL_CURVE_FIND_CLOSE_KNOT_Quantized(MIXED_NAME, 8);
IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);

void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( KnotIndex >= 0 );
    Assert ( KnotIndex + KnotCount <= CurveKnotCount );
    uint8 *CurKnot    = CurveData.KnotsControls + KnotIndex;
    // Skip the knots, then find the right control.
    uint8 *CurControl = CurveData.KnotsControls + CurveKnotCount + KnotIndex * CurveDimension;
    real32 OneOverKnotScale;
    EXTRACT_ONE_OVER_KNOT_SCALE;
    real32 KnotScale = 1.0f / OneOverKnotScale;
    const real32 *ControlScales = CurveData.ControlScales;
    const real32 *ControlOffsets = CurveData.ControlOffsets;
    if ( KnotResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            *KnotResults++ = KnotScale * (real32)( *CurKnot++ );
        }}
    }
    if ( ControlResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            {for ( int32 Dim = 0; Dim < CurveDimension; Dim++ )
            {
                *ControlResults++ = ControlOffsets[Dim] + ControlScales[Dim] * (real32)( *CurControl++ );
            }}
        }}
    }
}

void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME) ( curve2 &Curve,
                                        int32x KnotCount, int32x Dimension,
                                        real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( Dimension == CurveDimension );
    Assert ( KnotCount == CurveKnotCount );
    uint8 *FirstKnot    = CurveData.KnotsControls;
    // Skip the knots.
    uint8 *FirstControl = CurveData.KnotsControls + CurveKnotCount;

    real32 KnotMax = KnotSrc[CurveKnotCount-1];

    CompileAssert ( sizeof(*CurveData.KnotsControls) == 1 );
    real32 OneOverKnotScale = (real32)UInt8Maximum / KnotMax;
    CurveData.OneOverKnotScaleTrunc = (uint16)( (*(uint32*)(&OneOverKnotScale))>>16 );
    // And then re-extract it, coz it will have changed slightly.
    EXTRACT_ONE_OVER_KNOT_SCALE;
    Assert((real32)UInt8Maximum / KnotMax >= OneOverKnotScale);
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        FirstKnot[Count] = (uint8)RoundReal32ToInt32 ( KnotSrc[Count] * OneOverKnotScale );
    }}

    // Now scan the controls to find the min and max in each dimension.
    // Going to store min and max in ControlScaleOffsets - saves
    // allocating a new array.
    real32 *ControlScales = CurveData.ControlScales;
    real32 *ControlOffsets = CurveData.ControlOffsets;
    {for ( int32 Dim = 0; Dim < CurveDimension; Dim++ )
    {
        ControlScales[Dim] = ControlSrc[Dim];
        ControlOffsets[Dim] = ControlSrc[Dim];
    }}

    real32 const *CurControlSrc = ControlSrc;
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        {for ( int32 Dim = 0; Dim < CurveDimension; Dim++ )
        {
            real32 ControlValue = *CurControlSrc++;
            if ( ControlScales[Dim] > ControlValue )
            {
                ControlScales[Dim] = ControlValue;
            }
            if ( ControlOffsets[Dim] < ControlValue )
            {
                ControlOffsets[Dim] = ControlValue;
            }
        }}
    }}

    // Figure out scale & offset.
    {for ( int32 Dim = 0; Dim < CurveDimension; Dim++ )
    {
        real32 Min = ControlScales[Dim];
        real32 Max = ControlOffsets[Dim];
        CompileAssert ( sizeof(*CurveData.KnotsControls) == 1 );
        ControlScales[Dim] = ( Max - Min ) / (real32)UInt8Maximum;
        ControlOffsets[Dim] = Min;

        if ( ControlScales[Dim] == 0.0f )
        {
            // Divide-by-zero is not fun.
            ControlScales[Dim] = 1.0f;
        }
    }}

    // And now write the actual control data.
    uint8 *CurControl = FirstControl;
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        {for ( int32 Dim = 0; Dim < CurveDimension; Dim++ )
        {
            real32 ControlValue = *ControlSrc++;
            real32 ScaledValue = ( ControlValue - ControlOffsets[Dim] ) / ControlScales[Dim];
            // In cases of very small ranges, this can go out of the 0-255 range because of precision problems.
            if ( ScaledValue < 0.0f )
            {
                Assert ( ScaledValue > -1.0f );
                ScaledValue = 0.0f;
            }
            else if ( ScaledValue > 255.0f )
            {
                Assert ( ScaledValue < 256.0f );
                ScaledValue = 255.0f;
            }
            *CurControl++ = (uint8)RoundReal32ToInt32 ( ScaledValue );
        }}
    }}
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    // Knot count of 0 should be DaIdentity, knot count of 1 should be DXConstant
    Assert ( Builder->KnotCount > 1 );
    Assert ( Builder->Dimension == 3 );
    AggrAllocPtr ( Allocator, LocalCurveData );
    // An array of both knots and controls (knots first, then controls).
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->KnotCount * ( 3 + 1 ),
                              KnotControlCount, KnotsControls );
}

void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    CurveData.OneOverKnotScaleTrunc = CurveDataSource.OneOverKnotScaleTrunc;
    Copy32 ( 3, CurveDataSource.ControlScales, CurveData.ControlScales );
    Copy32 ( 3, CurveDataSource.ControlOffsets, CurveData.ControlOffsets );
    Assert ( CurveData.KnotControlCount == CurveDataSource.KnotControlCount );
    Copy ( CurveData.KnotControlCount * sizeof ( *CurveData.KnotsControls ), CurveDataSource.KnotsControls, CurveData.KnotsControls );
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( Dimension == CurveDimension );

    Assert ( CurveDimension <= MaximumBSplineDimension );
    uint8 TempControls[MaximumBSplineDimension];
    real32 TempScales[MaximumBSplineDimension];
    real32 TempOffsets[MaximumBSplineDimension];

    // Swizzle and adjust the scale+offset.
    // First half of ControlScaleOffsets are Scales, second half are Offsets

    // So the math goes that when an integer-format control is read, you do:
    // real_value = scale * int_value + offset
    // So now we want to scale and offset the result, so:
    // new_value = this_scale * real_value + this_offset
    //           = this_scale * ( scale * int_value + offset ) + this_offset
    //           = this_scale * scale * int_value + this_scale * offset + this_offset
    // And we need to find new_scale and new_offset so that:
    // new_value = new_scale * int_value + new_offset
    // Thus:
    // new_scale = this_scale * scale
    // new_offset = this_scale * offset + this_offset
    real32 *ControlScales = CurveData.ControlScales;
    real32 *ControlOffsets = CurveData.ControlOffsets;
    Copy32 ( CurveDimension, ControlScales,  TempScales );
    Copy32 ( CurveDimension, ControlOffsets, TempOffsets );
    {for ( int32x CurDimension = 0; CurDimension < CurveDimension; CurDimension++ )
    {
        int32x Swizzle = Swizzles[CurDimension];
        Assert ( ( Swizzle >= 0 ) && ( Swizzle < CurveDimension ) );
        real32 OldScale  = TempScales [Swizzle];
        real32 OldOffset = TempOffsets[Swizzle];
        ControlScales[CurDimension] = Scales[CurDimension] * OldScale;
        ControlOffsets[CurDimension] = Scales[CurDimension] * OldOffset + Offsets[CurDimension];
    }}

    // Now swizzle the control data.
    // First part of KnotsControls is Knots, last part is Controls.
    uint8 *Control = CurveData.KnotsControls + CurveKnotCount;
    {for ( int32x ControlNum = 0; ControlNum < CurveKnotCount; ControlNum++ )
    {
        Copy ( CurveDimension * sizeof (Control[0]), Control, TempControls );
        {for ( int32x CurDimension = 0; CurDimension < CurveDimension; CurDimension++ )
        {
            // Scale and offsets happen above, so this just does swizzles.
            Control[CurDimension] = TempControls[Swizzles[CurDimension]];
        }}
        Control += CurveDimension;
    }}
}


#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE


//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(D9I1K16uC16u);

#define MIXED_NAME D9I1K16uC16u
#define LOWER_NAME d9i1_k16u_c16u

// KnotControlCount = KnotCount + ControlCount
// ControlCount = KnotCount
// Dimension = 9              FAKE!
#define FIND_DIMENSION (9)
#define FIND_KNOT_COUNT (CurveData.KnotControlCount / 2 )
// Only the top half of the IEEE float OneOverKnotScale is stored!
#define EXTRACT_ONE_OVER_KNOT_SCALE *(uint32*)(&OneOverKnotScale) = (((uint32)CurveData.OneOverKnotScaleTrunc)<<16)
#define GET_KNOT_POINTER (CurveData.KnotsControls)

IMPL_CURVE_GET_DIMENSION(MIXED_NAME);
IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME);
IMPL_CURVE_FIND_KNOT_Quantized(MIXED_NAME, 16);
IMPL_CURVE_FIND_CLOSE_KNOT_Quantized(MIXED_NAME, 16);
IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);

void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveKnotCount = FIND_KNOT_COUNT;
    int32x ControlIndex   = KnotIndex;          // we know this to be the case in d1_

    Assert ( KnotIndex >= 0 );
    Assert ( KnotIndex + KnotCount <= CurveKnotCount );
    real32 OneOverKnotScale;
    EXTRACT_ONE_OVER_KNOT_SCALE;
    uint16 *CurKnot    = CurveData.KnotsControls + KnotIndex;
    // Skip the knots, then find the right control.
    uint16 *CurControl = (CurveData.KnotsControls + CurveKnotCount) + ControlIndex;
    real32 KnotScale   = 1.0f / OneOverKnotScale;
    if ( KnotResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            *KnotResults++ = KnotScale * (real32)( *CurKnot++ );
        }}
    }
    if ( ControlResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            // NB: We're outputting a 3x3 here.  Since we're only storing a uniform
            //  scale factor, this works out as:
            real32 uniformScale = CurveData.ControlOffset + CurveData.ControlScale * (real32)( *CurControl++ );

            ControlResults[0] = uniformScale;
            ControlResults[1] = 0;
            ControlResults[2] = 0;
            ControlResults[3] = 0;
            ControlResults[4] = uniformScale;
            ControlResults[5] = 0;
            ControlResults[6] = 0;
            ControlResults[7] = 0;
            ControlResults[8] = uniformScale;
            ControlResults += 9;
        }}
    }
}

void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME) ( curve2 &Curve,
                                        int32x KnotCount, int32x Dimension,
                                        real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( Dimension == CurveDimension );
    Assert ( KnotCount == CurveKnotCount );
    uint16 *FirstKnot    = CurveData.KnotsControls;
    // Skip the knots.
    uint16 *FirstControl = CurveData.KnotsControls + CurveKnotCount;

    real32 KnotMax = KnotSrc[CurveKnotCount-1];

    CompileAssert ( sizeof(*CurveData.KnotsControls) == 2 );
    real32 OneOverKnotScale = (real32)UInt16Maximum / KnotMax;
    CurveData.OneOverKnotScaleTrunc = (uint16)((*(uint32*)(&OneOverKnotScale))>>16);
    // And then re-extract it, coz it will have changed slightly.
    EXTRACT_ONE_OVER_KNOT_SCALE;
    Assert((real32)UInt16Maximum / KnotMax >= OneOverKnotScale);
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        FirstKnot[Count] = (uint16)RoundReal32ToInt32 ( KnotSrc[Count] * OneOverKnotScale );
    }}

    // Now scan the controls to find the min and max
    real32 minControl = ControlSrc[0];
    real32 maxControl = ControlSrc[0];
    {
        real32 const *CurControlSrc = ControlSrc;
        for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            real32 ControlValue = CurControlSrc[0];
            CurControlSrc += 9;

            if (minControl > ControlValue)
            {
                minControl = ControlValue;
            }
            if (maxControl < ControlValue)
            {
                maxControl = ControlValue;
            }
        }
    }

    // Figure out scale & offset.
    CompileAssert ( sizeof(*CurveData.KnotsControls) == 2 );
    Assert ( minControl <= maxControl );
    CurveData.ControlScale  = ( maxControl - minControl ) / (real32)UInt16Maximum;
    CurveData.ControlOffset = minControl;
    if ( CurveData.ControlScale == 0.0f )
    {
        // Divide-by-zero is not fun.
        CurveData.ControlScale = 1.0f;
    }

    // And now write the actual control data.
    {
        uint16 *CurControl = FirstControl;
        real32 const *CurControlSrc = ControlSrc;
        for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            real32 ControlValue = CurControlSrc[0];
            real32 ScaledValue = ( ControlValue - CurveData.ControlOffset ) / CurveData.ControlScale;

            // In cases of very small ranges, this can go out of the 0-65535 range because of precision problems.
            if ( ScaledValue < 0.0f )
            {
                Assert ( ScaledValue > -1.0f );
                ScaledValue = 0.0f;
            }
            else if ( ScaledValue > 65535.0f )
            {
                Assert ( ScaledValue < 65536.0f );
                ScaledValue = 65535.0f;
            }

            int32x resultControl = RoundReal32ToInt32 ( ScaledValue );
            Assert ( resultControl >= 0 && resultControl <= UInt16Maximum );

            CurControl[0] = (uint16)resultControl;

            CurControlSrc += 9;
            CurControl    += 1;
        }
    }
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    // Knot count of 0 should be DaIdentity, knot count of 1 should be DXConstant
    Assert ( Builder->KnotCount > 1 );
    AggrAllocPtr ( Allocator, LocalCurveData );
    // An array of both knots and controls (knots first, then controls).
    // NB: Not Builder->Dimension!  1 + 1 is the correct allocation adjustment
    Assert( Builder->Dimension == 9 );
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->KnotCount * ( 1 + 1 ),
                              KnotControlCount, KnotsControls );
}

void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    CurveData.OneOverKnotScaleTrunc = CurveDataSource.OneOverKnotScaleTrunc;
    CurveData.ControlScale  = CurveDataSource.ControlScale;
    CurveData.ControlOffset = CurveDataSource.ControlOffset;
    Assert ( CurveData.KnotControlCount == CurveDataSource.KnotControlCount );
    Copy ( CurveData.KnotControlCount * sizeof ( *CurveData.KnotsControls ), CurveDataSource.KnotsControls, CurveData.KnotsControls );
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( Dimension == FIND_DIMENSION );
    Assert ( Dimension == 9 );

    // Swizzle and adjust the scale+offset.
    // First half of ControlScaleOffsets are Scales, second half are Offsets

    // So the math goes that when an integer-format control is read, you do:
    // real_value = scale * int_value + offset
    // So now we want to scale and offset the result, so:
    // new_value = this_scale * real_value + this_offset
    //           = this_scale * ( scale * int_value + offset ) + this_offset
    //           = this_scale * scale * int_value + this_scale * offset + this_offset
    // And we need to find new_scale and new_offset so that:
    // new_value = new_scale * int_value + new_offset
    // Thus:
    // new_scale = this_scale * scale
    // new_offset = this_scale * offset + this_offset
    real32 TempScale  = CurveData.ControlScale;
    real32 TempOffset = CurveData.ControlOffset;
    {
        Assert ( Swizzles[0] == 0 );
        real32 OldScale  = TempScale;
        real32 OldOffset = TempOffset;
        CurveData.ControlScale  = Scales[0] * OldScale;
        CurveData.ControlOffset = Scales[0] * OldOffset + Offsets[0];
    }

    // Now swizzle the control data.
    // No, wait, don't.  Can't swizzle 1d data.
}

#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE


//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(D9I3K16uC16u);

#define MIXED_NAME D9I3K16uC16u
#define LOWER_NAME d9i3_k16u_c16u

// KnotControlCount = KnotCount + ControlCount
// ControlCount = KnotCount * 3
// Dimension = 9              FAKE!
#define FIND_DIMENSION (9)
#define FIND_KNOT_COUNT (CurveData.KnotControlCount / 4 )
// Only the top half of the IEEE float OneOverKnotScale is stored!
#define EXTRACT_ONE_OVER_KNOT_SCALE *(uint32*)(&OneOverKnotScale) = (((uint32)CurveData.OneOverKnotScaleTrunc)<<16)
#define GET_KNOT_POINTER (CurveData.KnotsControls)

IMPL_CURVE_GET_DIMENSION(MIXED_NAME);
IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME);
IMPL_CURVE_FIND_KNOT_Quantized(MIXED_NAME, 16);
IMPL_CURVE_FIND_CLOSE_KNOT_Quantized(MIXED_NAME, 16);
IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);

void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    const int32x ImplDimCount = 3;
    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( KnotIndex >= 0 );
    Assert ( KnotIndex + KnotCount <= CurveKnotCount );
    real32 OneOverKnotScale;
    EXTRACT_ONE_OVER_KNOT_SCALE;
    uint16 *CurKnot    = CurveData.KnotsControls + KnotIndex;
    // Skip the knots, then find the right control.
    uint16 *CurControl = (CurveData.KnotsControls + CurveKnotCount) + KnotIndex * ImplDimCount;
    real32 KnotScale   = 1.0f / OneOverKnotScale;
    if ( KnotResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            *KnotResults++ = KnotScale * (real32)( *CurKnot++ );
        }}
    }
    if ( ControlResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            // NB: We're outputting a 3x3 matrix here.  Since we're only storing a scale
            // vector, this works out as:
            ControlResults[0] = CurveData.ControlOffsets[0] + CurveData.ControlScales[0] * (real32)( *CurControl++ );
            ControlResults[1] = 0;
            ControlResults[2] = 0;
            ControlResults[3] = 0;
            ControlResults[4] = CurveData.ControlOffsets[1] + CurveData.ControlScales[1] * (real32)( *CurControl++ );
            ControlResults[5] = 0;
            ControlResults[6] = 0;
            ControlResults[7] = 0;
            ControlResults[8] = CurveData.ControlOffsets[2] + CurveData.ControlScales[2] * (real32)( *CurControl++ );
            ControlResults += 9;
        }}
    }
}

// TODO: Factor out, using curvedimension/fakedimension
void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME) ( curve2 &Curve,
                                        int32x KnotCount, int32x Dimension,
                                        real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    const int32x ImplDimCount           = 3;
    const int32x DimRemap[ImplDimCount] = { 0, 4, 8 };

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( Dimension == CurveDimension );
    Assert ( KnotCount == CurveKnotCount );
    uint16 *FirstKnot    = CurveData.KnotsControls;
    // Skip the knots.
    uint16 *FirstControl = CurveData.KnotsControls + CurveKnotCount;

    real32 KnotMax = KnotSrc[CurveKnotCount-1];

    CompileAssert ( sizeof(*CurveData.KnotsControls) == 2 );
    real32 OneOverKnotScale = (real32)UInt16Maximum / KnotMax;
    CurveData.OneOverKnotScaleTrunc = (uint16)((*(uint32*)(&OneOverKnotScale))>>16);
    // And then re-extract it, coz it will have changed slightly.
    EXTRACT_ONE_OVER_KNOT_SCALE;
    Assert((real32)UInt16Maximum / KnotMax >= OneOverKnotScale);
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        FirstKnot[Count] = (uint16)RoundReal32ToInt32 ( KnotSrc[Count] * OneOverKnotScale );
    }}

    // Now scan the controls to find the min and max
    real32 minControl[3] = { ControlSrc[DimRemap[0]], ControlSrc[DimRemap[1]], ControlSrc[DimRemap[2]] };
    real32 maxControl[3] = { ControlSrc[DimRemap[0]], ControlSrc[DimRemap[1]], ControlSrc[DimRemap[2]] };
    {
        real32 const *CurControlSrc = ControlSrc;
        for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            {for(int32x i = 0; i < ImplDimCount; i++)
            {
                real32 ControlValue = CurControlSrc[DimRemap[i]];

                if (minControl[i] > ControlValue)
                {
                    minControl[i] = ControlValue;
                }
                if (maxControl[i] < ControlValue)
                {
                    maxControl[i] = ControlValue;
                }
            }}

            CurControlSrc += CurveDimension;
        }
    }

    // Figure out scale & offset.
    CompileAssert ( sizeof(*CurveData.KnotsControls) == 2 );
    {for(int32x i = 0; i < ImplDimCount; i++)
    {
        Assert ( minControl[i] <= maxControl[i] );
        CurveData.ControlScales[i]  = ( maxControl[i] - minControl[i] ) / (real32)UInt16Maximum;
        CurveData.ControlOffsets[i] = minControl[i];
        if ( CurveData.ControlScales[i] == 0.0f )
        {
            // Divide-by-zero is not fun.
            CurveData.ControlScales[i] = 1.0f;
        }
    }}

    // And now write the actual control data.
    {
        uint16 *CurControl = FirstControl;
        real32 const *CurControlSrc = ControlSrc;
        for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            {for(int32x i = 0; i < ImplDimCount; i++)
            {
                real32 ControlValue = CurControlSrc[DimRemap[i]];
                real32 ScaledValue = ( ControlValue - CurveData.ControlOffsets[i] ) / CurveData.ControlScales[i];

                // In cases of very small ranges, this can go out of the 0-65535 range because of precision problems.
                if ( ScaledValue < 0.0f )
                {
                    Assert ( ScaledValue > -1.0f );
                    ScaledValue = 0.0f;
                }
                else if ( ScaledValue > 65535.0f )
                {
                    Assert ( ScaledValue < 65536.0f );
                    ScaledValue = 65535.0f;
                }

                int32x resultControl = RoundReal32ToInt32 ( ScaledValue );
                Assert ( resultControl >= 0 && resultControl <= UInt16Maximum );
                CurControl[i] = (uint16)resultControl;
            }}

            CurControlSrc += CurveDimension;
            CurControl    += ImplDimCount;
        }
    }
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    // Knot count of 0 should be DaIdentity, knot count of 1 should be DXConstant
    Assert ( Builder->KnotCount > 1 );
    AggrAllocPtr ( Allocator, LocalCurveData );
    // An array of both knots and controls (knots first, then controls).

    // NB: Not Builder->Dimension!  1 + 3 is the correct allocation adjustment
    Assert( Builder->Dimension == 9 );
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->KnotCount * ( 1 + 3 ),
                              KnotControlCount, KnotsControls );
}

void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    CurveData.OneOverKnotScaleTrunc = CurveDataSource.OneOverKnotScaleTrunc;
    Copy32 ( 3, CurveDataSource.ControlScales, CurveData.ControlScales );
    Copy32 ( 3, CurveDataSource.ControlOffsets, CurveData.ControlOffsets );
    Assert ( CurveData.KnotControlCount == CurveDataSource.KnotControlCount );
    Copy ( CurveData.KnotControlCount * sizeof ( *CurveData.KnotsControls ), CurveDataSource.KnotsControls, CurveData.KnotsControls );
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    // Note!The trick to be aware of here is that we're only looking
    //  at the diagonal entries of this transform.
    // TODO: Should we assert that the swizzle is valid here?  User
    //  might transform a file by into a wacky space, which could
    //  invalidate our uniform scale assumption for this curve type.

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( Dimension == CurveDimension );

    const int32x ImplDimCount = 3;
    const int32x DimRemap[ImplDimCount] = { 0, 4, 8 };

    Assert ( CurveDimension <= MaximumBSplineDimension );
    uint16 TempControls[MaximumBSplineDimension];
    real32 TempScales[MaximumBSplineDimension];
    real32 TempOffsets[MaximumBSplineDimension];

    // Swizzle and adjust the scale+offset.
    // First half of ControlScaleOffsets are Scales, second half are Offsets

    // So the math goes that when an integer-format control is read, you do:
    // real_value = scale * int_value + offset
    // So now we want to scale and offset the result, so:
    // new_value = this_scale * real_value + this_offset
    //           = this_scale * ( scale * int_value + offset ) + this_offset
    //           = this_scale * scale * int_value + this_scale * offset + this_offset
    // And we need to find new_scale and new_offset so that:
    // new_value = new_scale * int_value + new_offset
    // Thus:
    // new_scale = this_scale * scale
    // new_offset = this_scale * offset + this_offset
    real32* ControlScales  = CurveData.ControlScales;
    real32* ControlOffsets = CurveData.ControlOffsets;
    {for(int32x i = 0; i < ImplDimCount; i++)
    {
        TempScales[DimRemap[i]]  = ControlScales[i];
        TempOffsets[DimRemap[i]] = ControlOffsets[i];
    }}
    {for ( int32x CurImplDimension = 0; CurImplDimension < ImplDimCount; CurImplDimension++ )
    {
        int32x RealDimension = DimRemap[CurImplDimension];
        int32x Swizzle       = Swizzles[RealDimension];

        Assert ( ( Swizzle >= 0 ) && ( Swizzle < CurveDimension ) );
        real32 OldScale  = TempScales [Swizzle];
        real32 OldOffset = TempOffsets[Swizzle];
        ControlScales[CurImplDimension]  = Scales[RealDimension] * OldScale;
        ControlOffsets[CurImplDimension] = Scales[RealDimension] * OldOffset + Offsets[RealDimension];
    }}

    // Now swizzle the control data.
    // First part of KnotsControls is Knots, last part is Controls.
    uint16 *Control = CurveData.KnotsControls + CurveKnotCount;
    {for ( int32x ControlNum = 0; ControlNum < CurveKnotCount; ControlNum++ )
    {
        // We know we shouldn't access anything but these variables (see note
        //  above on validating swizzle input)
        TempControls[DimRemap[0]] = Control[0];
        TempControls[DimRemap[1]] = Control[1];
        TempControls[DimRemap[2]] = Control[2];
        {for ( int32x CurImplDimension = 0; CurImplDimension < ImplDimCount; CurImplDimension++ )
        {
            // Scale and offsets happen above, so this just does swizzles.
            Control[CurImplDimension] = TempControls[Swizzles[DimRemap[CurImplDimension]]];
        }}
        Control += ImplDimCount;
    }}
}

#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE


//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(D9I1K8uC8u);

#define MIXED_NAME D9I1K8uC8u
#define LOWER_NAME d9i1_k8u_c8u

// KnotControlCount = KnotCount + ControlCount
// ControlCount = KnotCount
// Dimension = 9              FAKE!
#define FIND_DIMENSION (9)
#define FIND_KNOT_COUNT (CurveData.KnotControlCount / 2 )
// Only the top half of the IEEE float OneOverKnotScale is stored!
#define EXTRACT_ONE_OVER_KNOT_SCALE *(uint32*)(&OneOverKnotScale) = (((uint32)CurveData.OneOverKnotScaleTrunc)<<16)
#define GET_KNOT_POINTER (CurveData.KnotsControls)

IMPL_CURVE_GET_DIMENSION(MIXED_NAME);
IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME);
IMPL_CURVE_FIND_KNOT_Quantized(MIXED_NAME, 8);
IMPL_CURVE_FIND_CLOSE_KNOT_Quantized(MIXED_NAME, 8);
IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);

void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveKnotCount = FIND_KNOT_COUNT;
    int32x ControlIndex   = KnotIndex;          // we know this to be the case in d1_

    Assert ( KnotIndex >= 0 );
    Assert ( KnotIndex + KnotCount <= CurveKnotCount );
    real32 OneOverKnotScale;
    EXTRACT_ONE_OVER_KNOT_SCALE;
    uint8 *CurKnot    = CurveData.KnotsControls + KnotIndex;
    // Skip the knots, then find the right control.
    uint8 *CurControl = (CurveData.KnotsControls + CurveKnotCount) + ControlIndex;
    real32 KnotScale   = 1.0f / OneOverKnotScale;
    if ( KnotResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            *KnotResults++ = KnotScale * (real32)( *CurKnot++ );
        }}
    }
    if ( ControlResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            // NB: We're outputting a 3x3 here.  Since we're only storing a uniform
            //  scale factor, this works out as:
            real32 uniformScale = CurveData.ControlOffset + CurveData.ControlScale * (real32)( *CurControl++ );

            ControlResults[0] = uniformScale;
            ControlResults[1] = 0;
            ControlResults[2] = 0;
            ControlResults[3] = 0;
            ControlResults[4] = uniformScale;
            ControlResults[5] = 0;
            ControlResults[6] = 0;
            ControlResults[7] = 0;
            ControlResults[8] = uniformScale;
            ControlResults += 9;
        }}
    }
}

void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME) ( curve2 &Curve,
                                        int32x KnotCount, int32x Dimension,
                                        real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( Dimension == CurveDimension );
    Assert ( KnotCount == CurveKnotCount );
    uint8 *FirstKnot    = CurveData.KnotsControls;
    // Skip the knots.
    uint8 *FirstControl = CurveData.KnotsControls + CurveKnotCount;

    real32 KnotMax = KnotSrc[CurveKnotCount-1];

    CompileAssert ( sizeof(*CurveData.KnotsControls) == 1 );
    real32 OneOverKnotScale = (real32)UInt8Maximum / KnotMax;
    CurveData.OneOverKnotScaleTrunc = (uint16)((*(uint32*)(&OneOverKnotScale))>>16);
    // And then re-extract it, coz it will have changed slightly.
    EXTRACT_ONE_OVER_KNOT_SCALE;
    Assert((real32)UInt8Maximum / KnotMax >= OneOverKnotScale);
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        FirstKnot[Count] = (uint8)RoundReal32ToInt32 ( KnotSrc[Count] * OneOverKnotScale );
    }}

    // Now scan the controls to find the min and max
    real32 minControl = ControlSrc[0];
    real32 maxControl = ControlSrc[0];
    {
        real32 const *CurControlSrc = ControlSrc;
        for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            real32 ControlValue = CurControlSrc[0];
            CurControlSrc += 9;

            if (minControl > ControlValue)
            {
                minControl = ControlValue;
            }
            if (maxControl < ControlValue)
            {
                maxControl = ControlValue;
            }
        }
    }

    // Figure out scale & offset.
    CompileAssert ( sizeof(*CurveData.KnotsControls) == 1 );
    Assert ( minControl <= maxControl );
    CurveData.ControlScale  = ( maxControl - minControl ) / (real32)UInt8Maximum;
    CurveData.ControlOffset = minControl;
    if ( CurveData.ControlScale == 0.0f )
    {
        // Divide-by-zero is not fun.
        CurveData.ControlScale = 1.0f;
    }

    // And now write the actual control data.
    {
        uint8 *CurControl = FirstControl;
        real32 const *CurControlSrc = ControlSrc;
        for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            real32 ControlValue = CurControlSrc[0];
            real32 ScaledValue = ( ControlValue - CurveData.ControlOffset ) / CurveData.ControlScale;

            // In cases of very small ranges, this can go out of the 0-65535 range because of precision problems.
            if ( ScaledValue < 0.0f )
            {
                Assert ( ScaledValue > -1.0f );
                ScaledValue = 0.0f;
            }
            else if ( ScaledValue > 65535.0f )
            {
                Assert ( ScaledValue < 65536.0f );
                ScaledValue = 65535.0f;
            }

            int32x resultControl = RoundReal32ToInt32 ( ScaledValue );
            Assert ( resultControl >= 0 && resultControl <= UInt8Maximum );

            CurControl[0] = (uint8)resultControl;

            CurControlSrc += 9;
            CurControl    += 1;
        }
    }
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    // Knot count of 0 should be DaIdentity, knot count of 1 should be DXConstant
    Assert ( Builder->KnotCount > 1 );
    AggrAllocPtr ( Allocator, LocalCurveData );
    // An array of both knots and controls (knots first, then controls).
    // NB: Not Builder->Dimension!  1 + 1 is the correct allocation adjustment
    Assert( Builder->Dimension == 9 );
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->KnotCount * ( 1 + 1 ),
                              KnotControlCount, KnotsControls );
}

void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    CurveData.OneOverKnotScaleTrunc = CurveDataSource.OneOverKnotScaleTrunc;
    CurveData.ControlScale  = CurveDataSource.ControlScale;
    CurveData.ControlOffset = CurveDataSource.ControlOffset;
    Assert ( CurveData.KnotControlCount == CurveDataSource.KnotControlCount );
    Copy ( CurveData.KnotControlCount * sizeof ( *CurveData.KnotsControls ), CurveDataSource.KnotsControls, CurveData.KnotsControls );
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( Dimension == FIND_DIMENSION );
    Assert ( Dimension == 9 );

    // Swizzle and adjust the scale+offset.
    // First half of ControlScaleOffsets are Scales, second half are Offsets

    // So the math goes that when an integer-format control is read, you do:
    // real_value = scale * int_value + offset
    // So now we want to scale and offset the result, so:
    // new_value = this_scale * real_value + this_offset
    //           = this_scale * ( scale * int_value + offset ) + this_offset
    //           = this_scale * scale * int_value + this_scale * offset + this_offset
    // And we need to find new_scale and new_offset so that:
    // new_value = new_scale * int_value + new_offset
    // Thus:
    // new_scale = this_scale * scale
    // new_offset = this_scale * offset + this_offset
    real32 TempScale  = CurveData.ControlScale;
    real32 TempOffset = CurveData.ControlOffset;
    {
        Assert ( Swizzles[0] == 0 );
        real32 OldScale  = TempScale;
        real32 OldOffset = TempOffset;
        CurveData.ControlScale  = Scales[0] * OldScale;
        CurveData.ControlOffset = Scales[0] * OldOffset + Offsets[0];
    }

    // Now swizzle the control data.
    // No, wait, don't.  Can't swizzle 1d data.
}

#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE


//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(D9I3K8uC8u);

#define MIXED_NAME D9I3K8uC8u
#define LOWER_NAME d9i3_k8u_c8u

// KnotControlCount = KnotCount + ControlCount
// ControlCount = KnotCount * 3
// Dimension = 9              FAKE!
#define FIND_DIMENSION (9)
#define FIND_KNOT_COUNT (CurveData.KnotControlCount / 4 )
// Only the top half of the IEEE float OneOverKnotScale is stored!
#define EXTRACT_ONE_OVER_KNOT_SCALE *(uint32*)(&OneOverKnotScale) = (((uint32)CurveData.OneOverKnotScaleTrunc)<<16)
#define GET_KNOT_POINTER (CurveData.KnotsControls)

IMPL_CURVE_GET_DIMENSION(MIXED_NAME);
IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME);
IMPL_CURVE_FIND_KNOT_Quantized(MIXED_NAME, 8);
IMPL_CURVE_FIND_CLOSE_KNOT_Quantized(MIXED_NAME, 8);
IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);

void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    const int32x ImplDimCount = 3;
    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( KnotIndex >= 0 );
    Assert ( KnotIndex + KnotCount <= CurveKnotCount );
    real32 OneOverKnotScale;
    EXTRACT_ONE_OVER_KNOT_SCALE;
    uint8 *CurKnot    = CurveData.KnotsControls + KnotIndex;
    // Skip the knots, then find the right control.
    uint8 *CurControl = (CurveData.KnotsControls + CurveKnotCount) + KnotIndex * ImplDimCount;
    real32 KnotScale   = 1.0f / OneOverKnotScale;
    if ( KnotResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            *KnotResults++ = KnotScale * (real32)( *CurKnot++ );
        }}
    }
    if ( ControlResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            // NB: We're outputting a 3x3 matrix here.  Since we're only storing a scale
            // vector, this works out as:
            ControlResults[0] = CurveData.ControlOffsets[0] + CurveData.ControlScales[0] * (real32)( *CurControl++ );
            ControlResults[1] = 0;
            ControlResults[2] = 0;
            ControlResults[3] = 0;
            ControlResults[4] = CurveData.ControlOffsets[1] + CurveData.ControlScales[1] * (real32)( *CurControl++ );
            ControlResults[5] = 0;
            ControlResults[6] = 0;
            ControlResults[7] = 0;
            ControlResults[8] = CurveData.ControlOffsets[2] + CurveData.ControlScales[2] * (real32)( *CurControl++ );
            ControlResults += 9;
        }}
    }
}

// TODO: Factor out, using curvedimension/fakedimension
void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME) ( curve2 &Curve,
                                        int32x KnotCount, int32x Dimension,
                                        real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    const int32x ImplDimCount           = 3;
    const int32x DimRemap[ImplDimCount] = { 0, 4, 8 };

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( Dimension == CurveDimension );
    Assert ( KnotCount == CurveKnotCount );
    uint8 *FirstKnot    = CurveData.KnotsControls;
    // Skip the knots.
    uint8 *FirstControl = CurveData.KnotsControls + CurveKnotCount;

    real32 KnotMax = KnotSrc[CurveKnotCount-1];

    CompileAssert ( sizeof(*CurveData.KnotsControls) == 1 );
    real32 OneOverKnotScale = (real32)UInt8Maximum / KnotMax;
    CurveData.OneOverKnotScaleTrunc = (uint16)((*(uint32*)(&OneOverKnotScale))>>16);
    // And then re-extract it, coz it will have changed slightly.
    EXTRACT_ONE_OVER_KNOT_SCALE;
    Assert((real32)UInt8Maximum / KnotMax >= OneOverKnotScale);
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        FirstKnot[Count] = (uint8)RoundReal32ToInt32 ( KnotSrc[Count] * OneOverKnotScale );
    }}

    // Now scan the controls to find the min and max
    real32 minControl[3] = { ControlSrc[DimRemap[0]], ControlSrc[DimRemap[1]], ControlSrc[DimRemap[2]] };
    real32 maxControl[3] = { ControlSrc[DimRemap[0]], ControlSrc[DimRemap[1]], ControlSrc[DimRemap[2]] };
    {
        real32 const *CurControlSrc = ControlSrc;
        for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            {for(int32x i = 0; i < ImplDimCount; i++)
            {
                real32 ControlValue = CurControlSrc[DimRemap[i]];

                if (minControl[i] > ControlValue)
                {
                    minControl[i] = ControlValue;
                }
                if (maxControl[i] < ControlValue)
                {
                    maxControl[i] = ControlValue;
                }
            }}

            CurControlSrc += CurveDimension;
        }
    }

    // Figure out scale & offset.
    CompileAssert ( sizeof(*CurveData.KnotsControls) == 1 );
    {for(int32x i = 0; i < ImplDimCount; i++)
    {
        Assert ( minControl[i] <= maxControl[i] );
        CurveData.ControlScales[i]  = ( maxControl[i] - minControl[i] ) / (real32)UInt8Maximum;
        CurveData.ControlOffsets[i] = minControl[i];
        if ( CurveData.ControlScales[i] == 0.0f )
        {
            // Divide-by-zero is not fun.
            CurveData.ControlScales[i] = 1.0f;
        }
    }}

    // And now write the actual control data.
    {
        uint8 *CurControl = FirstControl;
        real32 const *CurControlSrc = ControlSrc;
        for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            {for(int32x i = 0; i < ImplDimCount; i++)
            {
                real32 ControlValue = CurControlSrc[DimRemap[i]];
                real32 ScaledValue = ( ControlValue - CurveData.ControlOffsets[i] ) / CurveData.ControlScales[i];

                // In cases of very small ranges, this can go out of the 0-65535 range because of precision problems.
                if ( ScaledValue < 0.0f )
                {
                    Assert ( ScaledValue > -1.0f );
                    ScaledValue = 0.0f;
                }
                else if ( ScaledValue > 65535.0f )
                {
                    Assert ( ScaledValue < 65536.0f );
                    ScaledValue = 65535.0f;
                }

                int32x resultControl = RoundReal32ToInt32 ( ScaledValue );
                Assert ( resultControl >= 0 && resultControl <= UInt8Maximum );
                CurControl[i] = (uint8)resultControl;
            }}

            CurControlSrc += CurveDimension;
            CurControl    += ImplDimCount;
        }
    }
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    // Knot count of 0 should be DaIdentity, knot count of 1 should be DXConstant
    Assert ( Builder->KnotCount > 1 );
    AggrAllocPtr ( Allocator, LocalCurveData );
    // An array of both knots and controls (knots first, then controls).

    // NB: Not Builder->Dimension!  1 + 3 is the correct allocation adjustment
    Assert( Builder->Dimension == 9 );
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->KnotCount * ( 1 + 3 ),
                              KnotControlCount, KnotsControls );
}

void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    CurveData.OneOverKnotScaleTrunc = CurveDataSource.OneOverKnotScaleTrunc;
    Copy32 ( 3, CurveDataSource.ControlScales, CurveData.ControlScales );
    Copy32 ( 3, CurveDataSource.ControlOffsets, CurveData.ControlOffsets );
    Assert ( CurveData.KnotControlCount == CurveDataSource.KnotControlCount );
    Copy ( CurveData.KnotControlCount * sizeof ( *CurveData.KnotsControls ), CurveDataSource.KnotsControls, CurveData.KnotsControls );
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    // Note!The trick to be aware of here is that we're only looking
    //  at the diagonal entries of this transform.
    // TODO: Should we assert that the swizzle is valid here?  User
    //  might transform a file by into a wacky space, which could
    //  invalidate our uniform scale assumption for this curve type.

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( Dimension == CurveDimension );

    const int32x ImplDimCount = 3;
    const int32x DimRemap[ImplDimCount] = { 0, 4, 8 };

    Assert ( CurveDimension <= MaximumBSplineDimension );
    uint8 TempControls[MaximumBSplineDimension];
    real32 TempScales[MaximumBSplineDimension];
    real32 TempOffsets[MaximumBSplineDimension];

    // Swizzle and adjust the scale+offset.
    // First half of ControlScaleOffsets are Scales, second half are Offsets

    // So the math goes that when an integer-format control is read, you do:
    // real_value = scale * int_value + offset
    // So now we want to scale and offset the result, so:
    // new_value = this_scale * real_value + this_offset
    //           = this_scale * ( scale * int_value + offset ) + this_offset
    //           = this_scale * scale * int_value + this_scale * offset + this_offset
    // And we need to find new_scale and new_offset so that:
    // new_value = new_scale * int_value + new_offset
    // Thus:
    // new_scale = this_scale * scale
    // new_offset = this_scale * offset + this_offset
    real32* ControlScales  = CurveData.ControlScales;
    real32* ControlOffsets = CurveData.ControlOffsets;
    {for(int32x i = 0; i < ImplDimCount; i++)
    {
        TempScales[DimRemap[i]]  = ControlScales[i];
        TempOffsets[DimRemap[i]] = ControlOffsets[i];
    }}
    {for ( int32x CurImplDimension = 0; CurImplDimension < ImplDimCount; CurImplDimension++ )
    {
        int32x RealDimension = DimRemap[CurImplDimension];
        int32x Swizzle       = Swizzles[RealDimension];

        Assert ( ( Swizzle >= 0 ) && ( Swizzle < CurveDimension ) );
        real32 OldScale  = TempScales [Swizzle];
        real32 OldOffset = TempOffsets[Swizzle];
        ControlScales[CurImplDimension]  = Scales[RealDimension] * OldScale;
        ControlOffsets[CurImplDimension] = Scales[RealDimension] * OldOffset + Offsets[RealDimension];
    }}

    // Now swizzle the control data.
    // First part of KnotsControls is Knots, last part is Controls.
    uint8 *Control = CurveData.KnotsControls + CurveKnotCount;
    {for ( int32x ControlNum = 0; ControlNum < CurveKnotCount; ControlNum++ )
    {
        // We know we shouldn't access anything but these variables (see note
        //  above on validating swizzle input)
        TempControls[DimRemap[0]] = Control[0];
        TempControls[DimRemap[1]] = Control[1];
        TempControls[DimRemap[2]] = Control[2];
        {for ( int32x CurImplDimension = 0; CurImplDimension < ImplDimCount; CurImplDimension++ )
        {
            // Scale and offsets happen above, so this just does swizzles.
            Control[CurImplDimension] = TempControls[Swizzles[DimRemap[CurImplDimension]]];
        }}
        Control += ImplDimCount;
    }}
}

#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE


//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(D3I1K32fC32f);

#define MIXED_NAME D3I1K32fC32f
#define LOWER_NAME d3i1_k32f_c32f

// KnotControlCount = KnotCount + ControlCount
// ControlCount = KnotCount
// Dimension = 3              FAKE!
#define FIND_DIMENSION (3)
#define FIND_KNOT_COUNT (CurveData.KnotControlCount / 2 )
#define GET_KNOT_POINTER (CurveData.KnotsControls)

IMPL_CURVE_GET_DIMENSION(MIXED_NAME);
IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME);
IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);

int32x GRANNY
CURVE_FIND_KNOT(MIXED_NAME) ( curve2 const &Curve, real32 t )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    return ( FindKnot ( FIND_KNOT_COUNT, GET_KNOT_POINTER, t ) );
}

int32x GRANNY
CURVE_FIND_CLOSE_KNOT(MIXED_NAME) ( curve2 const &Curve, real32 t, int32x StartingIndex )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    return ( FindCloseKnot ( FIND_KNOT_COUNT, GET_KNOT_POINTER, t, StartingIndex ) );
}


void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( KnotIndex >= 0 );
    Assert ( KnotIndex + KnotCount <= CurveKnotCount );
    real32 *CurKnot    = CurveData.KnotsControls + KnotIndex;

    // Skip the knots, then find the right control.
    real32 *CurControl = (CurveData.KnotsControls + CurveKnotCount) + KnotIndex;

    if ( KnotResults != NULL )
    {
        Copy32(KnotCount, CurKnot, KnotResults);
    }
    if ( ControlResults != NULL )
    {
        //--- todo: aliasing check
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            // NB: We're outputting a 3 vector here.  Since we're only
            // storing a parameter, this works out as:
            real32 const CurrParameter = *CurControl++;
            ControlResults[0] = CurveData.ControlOffsets[0] + CurveData.ControlScales[0] * CurrParameter;
            ControlResults[1] = CurveData.ControlOffsets[1] + CurveData.ControlScales[1] * CurrParameter;
            ControlResults[2] = CurveData.ControlOffsets[2] + CurveData.ControlScales[2] * CurrParameter;
            ControlResults += 3;
        }}
    }
}

void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME)(curve2 &Curve,
                                      int32x KnotCount, int32x Dimension,
                                      real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( Dimension == CurveDimension );
    Assert ( KnotCount == CurveKnotCount );

    real32 *FirstKnot    = CurveData.KnotsControls;

    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        FirstKnot[Count] = KnotSrc[Count];
    }}

    // Let's extract the LSQ line for these points...
    ExtractPCALine3Dim(KnotCount, ControlSrc, 3,
                       CurveData.ControlOffsets, CurveData.ControlScales,
                       true);

    // And fit the points to the line as the controls.
    real32 *FirstControl = CurveData.KnotsControls + CurveKnotCount;
    real32 const ScaleLengthSq = VectorLength3(CurveData.ControlScales) * VectorLength3(CurveData.ControlScales);
    Assert(ScaleLengthSq != 0.0f);
    {for (int32x Index = 0; Index < KnotCount; Index++)
    {
        real32 const *CurSrcControl = ControlSrc + Index*3;

        real32 OffsetPoint[3];
        VectorSubtract3(OffsetPoint, CurSrcControl, CurveData.ControlOffsets);

        real32 const tVal = InnerProduct3(OffsetPoint, CurveData.ControlScales) / ScaleLengthSq;
        FirstControl[Index] = tVal;
    }}
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    // Knot count of 0 should be DaIdentity, knot count of 1 should be DXConstant
    Assert ( Builder->KnotCount > 1 );
    AggrAllocPtr ( Allocator, LocalCurveData );
    // An array of both knots and controls (knots first, then controls).

    // NB: Not Builder->Dimension!  1 + 1 is the correct allocation adjustment
    Assert( Builder->Dimension == 3 );
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->KnotCount * ( 1 + 1 ),
                              KnotControlCount, KnotsControls );
}

void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    Copy32 ( 3, CurveDataSource.ControlScales, CurveData.ControlScales );
    Copy32 ( 3, CurveDataSource.ControlOffsets, CurveData.ControlOffsets );
    Assert ( CurveData.KnotControlCount == CurveDataSource.KnotControlCount );
    Copy ( CurveData.KnotControlCount * sizeof ( *CurveData.KnotsControls ), CurveDataSource.KnotsControls, CurveData.KnotsControls );
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert(Dimension == 3);

    // This is actually quite nice.  We can just twiddle the offsets
    // and scale members, and then no problems.  The linear controls
    // still work perfectly.
    real32 TempScales[3];
    real32 TempOffsets[3];
    Copy32(3, CurveData.ControlScales, TempScales);
    Copy32(3, CurveData.ControlOffsets, TempOffsets);

    {for(int CurDim = 0; CurDim < 3; CurDim++)
    {
        CurveData.ControlScales[CurDim]  = TempScales[Swizzles[CurDim]] * Scales[CurDim];
        CurveData.ControlOffsets[CurDim] = TempOffsets[Swizzles[CurDim]] * Scales[CurDim] + Offsets[CurDim];
    }}
}

#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE


//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(D3I1K16uC16u);

#define MIXED_NAME D3I1K16uC16u
#define LOWER_NAME d3i1_k16u_c16u

// KnotControlCount = KnotCount + ControlCount
// ControlCount = KnotCount
// Dimension = 3              FAKE!
#define FIND_DIMENSION (3)
#define FIND_KNOT_COUNT (CurveData.KnotControlCount / 2 )
#define EXTRACT_ONE_OVER_KNOT_SCALE *(uint32*)(&OneOverKnotScale) = (((uint32)CurveData.OneOverKnotScaleTrunc)<<16)
#define GET_KNOT_POINTER (CurveData.KnotsControls)

IMPL_CURVE_GET_DIMENSION(MIXED_NAME);
IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME);
IMPL_CURVE_FIND_KNOT_Quantized(MIXED_NAME, 16);
IMPL_CURVE_FIND_CLOSE_KNOT_Quantized(MIXED_NAME, 16);
IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);

void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( KnotIndex >= 0 );
    Assert ( KnotIndex + KnotCount <= CurveKnotCount );
    real32 OneOverKnotScale;
    EXTRACT_ONE_OVER_KNOT_SCALE;
    uint16 *CurKnot   = CurveData.KnotsControls + KnotIndex;
    real32 KnotScale  = 1.0f / OneOverKnotScale;
    if ( KnotResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            *KnotResults++ = KnotScale * (real32)( *CurKnot++ );
        }}
    }

    // Skip the knots, then find the right control.
    uint16 *CurControl = (CurveData.KnotsControls + CurveKnotCount) + KnotIndex;
    if ( ControlResults != NULL )
    {
        //--- todo: aliasing check
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            // NB: We're outputting a 3 vector here.  Since we're only
            // storing a parameter, this works out as:
            uint16 const CurrParameter = *CurControl++;
            ControlResults[0] = CurveData.ControlOffsets[0] + CurveData.ControlScales[0] * CurrParameter;
            ControlResults[1] = CurveData.ControlOffsets[1] + CurveData.ControlScales[1] * CurrParameter;
            ControlResults[2] = CurveData.ControlOffsets[2] + CurveData.ControlScales[2] * CurrParameter;
            ControlResults += 3;
        }}
    }
}

void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME)(curve2 &Curve,
                                      int32x KnotCount, int32x Dimension,
                                      real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( Dimension == CurveDimension );
    Assert ( KnotCount == CurveKnotCount );
    CompileAssert ( sizeof(*CurveData.KnotsControls) == 2 );

    real32 KnotMax = KnotSrc[CurveKnotCount-1];
    uint16 *FirstKnot = CurveData.KnotsControls;

    real32 OneOverKnotScale = (real32)UInt16Maximum / KnotMax;
    CurveData.OneOverKnotScaleTrunc = (uint16)((*(uint32*)(&OneOverKnotScale))>>16);
    // And then re-extract it, coz it will have changed slightly.
    EXTRACT_ONE_OVER_KNOT_SCALE;
    Assert((real32)UInt16Maximum / KnotMax >= OneOverKnotScale);
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        FirstKnot[Count] = (uint16)RoundReal32ToInt32 ( KnotSrc[Count] * OneOverKnotScale );
    }}

    // Let's extract the LSQ line for these points...
    ExtractPCALine3Dim(KnotCount, ControlSrc, 3,
                       CurveData.ControlOffsets, CurveData.ControlScales,
                       true);

    // And fit the points to the line as the controls.
    uint16 *FirstControl = CurveData.KnotsControls + CurveKnotCount;
    real32 const ScaleLengthSq = VectorLength3(CurveData.ControlScales) * VectorLength3(CurveData.ControlScales);
    Assert(ScaleLengthSq != 0.0f);
    {for (int32x Index = 0; Index < KnotCount; Index++)
    {
        real32 const *CurSrcControl = ControlSrc + Index*3;

        real32 OffsetPoint[3];
        VectorSubtract3(OffsetPoint, CurSrcControl, CurveData.ControlOffsets);

        real32 const tVal = InnerProduct3(OffsetPoint, CurveData.ControlScales) / ScaleLengthSq;

        // Clamp to make sure this isn't out of range
        real32 ScaledValue = tVal * UInt16Maximum;
        if ( ScaledValue < 0.0f )
        {
            // loosened from 1, precision can slip a past 1.0f for very tiny vectors
            // OK, dammit, the math is correct, but for very small Scale vectors,
            //  this goes wrong by a small epsilon that is too annoying to guess.
            // Assert ( ScaledValue > -SlipValue );
            ScaledValue = 0.0f;
        }
        else if ( ScaledValue > real32(UInt16Maximum) )
        {
            // see above
            //Assert ( ScaledValue < real32(UInt16Maximum + SlipValue));
            ScaledValue = real32(UInt16Maximum);
        }

        int32x resultControl = RoundReal32ToInt32 ( ScaledValue );
        Assert ( resultControl >= 0 && resultControl <= UInt16Maximum );
        FirstControl[Index] = (uint16)resultControl;
    }}

    // We've scaled the controls by 65535, to save on multiplications
    // on decompression, scale the line equation accordingly.
    {for(int Dim = 0; Dim < 3; Dim++)
    {
        CurveData.ControlScales[Dim] /= UInt16Maximum;
    }}
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    // Knot count of 0 should be DaIdentity, knot count of 1 should be DXConstant
    Assert ( Builder->KnotCount > 1 );
    AggrAllocPtr ( Allocator, LocalCurveData );
    // An array of both knots and controls (knots first, then controls).

    // NB: Not Builder->Dimension!  1 + 1 is the correct allocation adjustment
    Assert( Builder->Dimension == 3 );
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->KnotCount * ( 1 + 1 ),
                              KnotControlCount, KnotsControls );
}

void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    CurveData.OneOverKnotScaleTrunc = CurveDataSource.OneOverKnotScaleTrunc;
    Copy32 ( 3, CurveDataSource.ControlScales, CurveData.ControlScales );
    Copy32 ( 3, CurveDataSource.ControlOffsets, CurveData.ControlOffsets );
    Assert ( CurveData.KnotControlCount == CurveDataSource.KnotControlCount );
    Copy ( CurveData.KnotControlCount * sizeof ( *CurveData.KnotsControls ), CurveDataSource.KnotsControls, CurveData.KnotsControls );
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert(Dimension == 3);

    // This is actually quite nice.  We can just twiddle the offsets
    // and scale members, and then no problems.  The linear controls
    // still work perfectly.
    real32 TempScales[3];
    real32 TempOffsets[3];
    Copy32(3, CurveData.ControlScales, TempScales);
    Copy32(3, CurveData.ControlOffsets, TempOffsets);

    {for(int CurDim = 0; CurDim < 3; CurDim++)
    {
        CurveData.ControlScales[CurDim]  = TempScales[Swizzles[CurDim]] * Scales[CurDim];
        CurveData.ControlOffsets[CurDim] = TempOffsets[Swizzles[CurDim]] * Scales[CurDim] + Offsets[CurDim];
    }}
}

#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE


//----------------------------------------------------------------------------------
//DECLARE_FUNCTIONS(D3I1K8uC8u);

#define MIXED_NAME D3I1K8uC8u
#define LOWER_NAME d3i1_k8u_c8u

// KnotControlCount = KnotCount + ControlCount
// ControlCount = KnotCount
// Dimension = 3              FAKE!
#define FIND_DIMENSION (3)
#define FIND_KNOT_COUNT (CurveData.KnotControlCount / 2 )
#define EXTRACT_ONE_OVER_KNOT_SCALE *(uint32*)(&OneOverKnotScale) = (((uint32)CurveData.OneOverKnotScaleTrunc)<<16)
#define GET_KNOT_POINTER (CurveData.KnotsControls)

IMPL_CURVE_GET_DIMENSION(MIXED_NAME);
IMPL_CURVE_GET_KNOT_COUNT(MIXED_NAME);
IMPL_CURVE_FIND_KNOT_Quantized(MIXED_NAME, 8);
IMPL_CURVE_FIND_CLOSE_KNOT_Quantized(MIXED_NAME, 8);
IMPL_CURVE_TRANSFORM_SAMPLES_NULL(MIXED_NAME);
IMPL_CURVE_NOTE_SAMPLE_TRANSFORM_NULL(MIXED_NAME);

void GRANNY
CURVE_EXTRACT_KNOT_VALUES(MIXED_NAME) ( curve2 const &Curve,
                                        int32x KnotIndex,
                                        int32x KnotCount,
                                        real32* NOALIAS KnotResults,
                                        real32* NOALIAS ControlResults,
                                        real32 const* NOALIAS IdentityVector )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE const &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveKnotCount = FIND_KNOT_COUNT;

    Assert ( KnotIndex >= 0 );
    Assert ( KnotIndex + KnotCount <= CurveKnotCount );
    real32 OneOverKnotScale;
    EXTRACT_ONE_OVER_KNOT_SCALE;
    uint8 *CurKnot   = CurveData.KnotsControls + KnotIndex;
    real32 KnotScale  = 1.0f / OneOverKnotScale;
    if ( KnotResults != NULL )
    {
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            *KnotResults++ = KnotScale * (real32)( *CurKnot++ );
        }}
    }

    // Skip the knots, then find the right control.
    uint8 *CurControl = (CurveData.KnotsControls + CurveKnotCount) + KnotIndex;
    if ( ControlResults != NULL )
    {
        //--- todo: aliasing check
        {for ( int32x Count = 0; Count < KnotCount; Count++ )
        {
            // NB: We're outputting a 3 vector here.  Since we're only
            // storing a parameter, this works out as:
            uint8 const CurrParameter = *CurControl++;
            ControlResults[0] = CurveData.ControlOffsets[0] + CurveData.ControlScales[0] * CurrParameter;
            ControlResults[1] = CurveData.ControlOffsets[1] + CurveData.ControlScales[1] * CurrParameter;
            ControlResults[2] = CurveData.ControlOffsets[2] + CurveData.ControlScales[2] * CurrParameter;
            ControlResults += 3;
        }}
    }
}

void GRANNY
CURVE_SET_ALL_KNOT_VALUES(MIXED_NAME)(curve2 &Curve,
                                      int32x KnotCount, int32x Dimension,
                                      real32 const *KnotSrc, real32 const *ControlSrc )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;

    int32x CurveDimension = FIND_DIMENSION;
    int32x CurveKnotCount = FIND_KNOT_COUNT;
    Assert ( Dimension == CurveDimension );
    Assert ( KnotCount == CurveKnotCount );
    CompileAssert ( sizeof(*CurveData.KnotsControls) == 1 );

    real32 KnotMax = KnotSrc[CurveKnotCount-1];
    uint8 *FirstKnot = CurveData.KnotsControls;

    real32 OneOverKnotScale = (real32)UInt8Maximum / KnotMax;
    CurveData.OneOverKnotScaleTrunc = (uint16)((*(uint32*)(&OneOverKnotScale))>>16);
    // And then re-extract it, coz it will have changed slightly.
    EXTRACT_ONE_OVER_KNOT_SCALE;
    Assert((real32)UInt8Maximum / KnotMax >= OneOverKnotScale);
    {for ( int32x Count = 0; Count < KnotCount; Count++ )
    {
        FirstKnot[Count] = (uint8)RoundReal32ToInt32 ( KnotSrc[Count] * OneOverKnotScale );
    }}

    // Let's extract the LSQ line for these points...
    ExtractPCALine3Dim(KnotCount, ControlSrc, 3,
                       CurveData.ControlOffsets, CurveData.ControlScales,
                       true);

    // And fit the points to the line as the controls.
    uint8 *FirstControl = CurveData.KnotsControls + CurveKnotCount;
    real32 const ScaleLengthSq = VectorLength3(CurveData.ControlScales) * VectorLength3(CurveData.ControlScales);
    Assert(ScaleLengthSq != 0.0f);
    {for (int32x Index = 0; Index < KnotCount; Index++)
    {
        real32 const *CurSrcControl = ControlSrc + Index*3;

        real32 OffsetPoint[3];
        VectorSubtract3(OffsetPoint, CurSrcControl, CurveData.ControlOffsets);

        real32 const tVal = InnerProduct3(OffsetPoint, CurveData.ControlScales) / ScaleLengthSq;

        // Clamp to make sure this isn't out of range
        real32 ScaledValue = tVal * UInt8Maximum;

        const float SlipValue = 2.0f;
        if ( ScaledValue < 0.0f )
        {
            // loosened from 1, precision can slip a past 1.0f for very tiny vectors
            Assert ( ScaledValue > -SlipValue );
            ScaledValue = 0.0f;
        }
        else if ( ScaledValue > real32(UInt8Maximum) )
        {
            // loosened from 1, precision can slip a past 1.0f for very tiny vectors
            Assert ( ScaledValue < real32(UInt8Maximum + SlipValue));
            ScaledValue = real32(UInt8Maximum);
        }

        int32x resultControl = RoundReal32ToInt32 ( ScaledValue );
        Assert ( resultControl >= 0 && resultControl <= UInt8Maximum );
        FirstControl[Index] = (uint8)resultControl;
    }}

    // We've scaled the controls by UInt8Maximum, so to save on
    // multiplications on decompression, scale the line equation
    // accordingly.
    {for(int Dim = 0; Dim < 3; Dim++)
    {
        CurveData.ControlScales[Dim] /= UInt8Maximum;
    }}
}

void GRANNY
CURVE_AGGR_CURVE_DATA(MIXED_NAME) ( aggr_allocator &Allocator, curve_builder const *Builder, void *&CurveData )
{
    CURVE_DATA_TYPE *&LocalCurveData = (CURVE_DATA_TYPE *&)CurveData;
    // Knot count of 0 should be DaIdentity, knot count of 1 should be DXConstant
    Assert ( Builder->KnotCount > 1 );
    AggrAllocPtr ( Allocator, LocalCurveData );
    // An array of both knots and controls (knots first, then controls).

    // NB: Not Builder->Dimension!  1 + 1 is the correct allocation adjustment
    Assert( Builder->Dimension == 3 );
    AggrAllocOffsetArrayPtr ( Allocator, LocalCurveData, Builder->KnotCount * ( 1 + 1 ),
                              KnotControlCount, KnotsControls );
}

void GRANNY
CURVE_COPY_CURVE(MIXED_NAME) ( curve2 &Curve, curve2 const &Source )
{
    PARANOID_CURVE_CHECK ( Curve );
    Assert ( CurveFormatIsInitializedCorrectly ( Source, true ) );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_DATA_TYPE const &CurveDataSource = *(CURVE_DATA_TYPE const *) Source.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert ( CurveData.CurveDataHeader.Format == CurveDataSource.CurveDataHeader.Format );
    Assert ( CurveData.CurveDataHeader.Degree == CurveDataSource.CurveDataHeader.Degree );
    CurveData.OneOverKnotScaleTrunc = CurveDataSource.OneOverKnotScaleTrunc;
    Copy32 ( 3, CurveDataSource.ControlScales, CurveData.ControlScales );
    Copy32 ( 3, CurveDataSource.ControlOffsets, CurveData.ControlOffsets );
    Assert ( CurveData.KnotControlCount == CurveDataSource.KnotControlCount );
    Copy ( CurveData.KnotControlCount * sizeof ( *CurveData.KnotsControls ), CurveDataSource.KnotsControls, CurveData.KnotsControls );
}

void GRANNY
CURVE_SCALE_OFFSET_SWIZZLE(MIXED_NAME) ( curve2 &Curve, int32x Dimension, real32 const *Scales, real32 const *Offsets, int32x const *Swizzles )
{
    PARANOID_CURVE_CHECK ( Curve );
    CURVE_DATA_TYPE &CurveData = *(CURVE_DATA_TYPE*) Curve.CurveData.Object;
    CURVE_CHECK_HEADER;
    Assert(Dimension == 3);

    // This is actually quite nice.  We can just twiddle the offsets
    // and scale members, and then no problems.  The linear controls
    // still work perfectly.
    real32 TempScales[3];
    real32 TempOffsets[3];
    Copy32(3, CurveData.ControlScales, TempScales);
    Copy32(3, CurveData.ControlOffsets, TempOffsets);

    {for(int CurDim = 0; CurDim < 3; CurDim++)
    {
        CurveData.ControlScales[CurDim]  = TempScales[Swizzles[CurDim]] * Scales[CurDim];
        CurveData.ControlOffsets[CurDim] = TempOffsets[Swizzles[CurDim]] * Scales[CurDim] + Offsets[CurDim];
    }}
}

#undef MIXED_NAME
#undef LOWER_NAME
#undef FIND_DIMENSION
#undef FIND_KNOT_COUNT
#undef GET_KNOT_POINTER
#undef EXTRACT_ONE_OVER_KNOT_SCALE

// nb: removed hinge orientation curve types, can be found in db. 2007/11/27

#undef CURVE_DATA_TYPE1
#undef CURVE_DATA_TYPE_NAME1
#undef CURVE_GET_DIMENSION1
#undef CURVE_GET_KNOT_COUNT1
#undef CURVE_FIND_KNOT1
#undef CURVE_FIND_CLOSE_KNOT1
#undef CURVE_EXTRACT_KNOT_VALUES1
#undef CURVE_SET_ALL_KNOT_VALUES1
#undef CURVE_AGGR_CURVE_DATA1

#undef CURVE_DATA_TYPE2
#undef CURVE_DATA_TYPE_NAME2
#undef CURVE_DATA_TYPE
#undef CURVE_DATA_TYPE_NAME

#undef CURVE_GET_DIMENSION
#undef CURVE_GET_KNOT_COUNT
#undef CURVE_FIND_KNOT
#undef CURVE_FIND_CLOSE_KNOT
#undef CURVE_EXTRACT_KNOT_VALUES
#undef CURVE_SET_ALL_KNOT_VALUES
#undef CURVE_AGGR_CURVE_DATA
#undef CURVE_CHECK_HEADER



BEGIN_GRANNY_NAMESPACE;

real32 CurveIdentityPosition[3]    = { 0.0f, 0.0f, 0.0f };
real32 CurveIdentityNormal[3]      = { 0.0f, 0.0f, 1.0f };
real32 CurveIdentityOrientation[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
real32 CurveIdentityScaleShear[9]  = { 1.0f, 0.0f, 0.0f,
                                       0.0f, 1.0f, 0.0f,
                                       0.0f, 0.0f, 1.0f };
real32 CurveIdentityScale[3]       = { 1.0f, 1.0f, 1.0f };
real32 CurveIdentityShear[6]       = { 0.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 0.0f };

END_GRANNY_NAMESPACE;





