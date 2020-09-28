// ========================================================================
// $File: //jeffr/granny/rt/granny_vertex_data.cpp $
// $DateTime: 2007/08/03 14:25:06 $
// $Change: 15659 $
// $Revision: #26 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_VERTEX_DATA_H)
#include "granny_vertex_data.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_STRING_FORMATTING_H)
#include "granny_string_formatting.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif

#if !defined(GRANNY_DATA_TYPE_CONVERSION_H)
#include "granny_data_type_conversion.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CURVE_H)
#include "granny_curve.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode VertexLayoutLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition VertexAnnotationSetType[] =
{
    {StringMember, "Name"},
    {ReferenceToVariantArrayMember, "VertexAnnotations"},
    {Int32Member, "IndicesMapFromVertexToAnnotation"},
    {ReferenceToArrayMember, "VertexAnnotationIndices", Int32Type},

    {EndMember},
};

data_type_definition VertexDataType[] =
{
    {ReferenceToVariantArrayMember, "Vertices"},
    {ReferenceToArrayMember, "VertexComponentNames", StringType},
    {ReferenceToArrayMember, "VertexAnnotationSets", VertexAnnotationSetType},

    {EndMember},
};

data_type_definition P3VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},

    {EndMember},
};

data_type_definition PT32VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {Real32Member, VertexTextureCoordinatesName "0", 0, 2},

    {EndMember},
};

data_type_definition PN33VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {Real32Member, VertexNormalName, 0, 3},

    {EndMember},
};

data_type_definition PNG333VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},

    {EndMember},
};

data_type_definition PNGT3332VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},
    {Real32Member, VertexTextureCoordinatesName "0", 0, 2},

    {EndMember},
};

data_type_definition PNTG3323VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTextureCoordinatesName "0", 0, 2},
    {Real32Member, VertexTangentName, 0, 3},

    {EndMember},
};

data_type_definition PNGB3333VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},
    {Real32Member, VertexBinormalName, 0, 3},

    {EndMember},
};

data_type_definition PNGBX33333VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},
    {Real32Member, VertexBinormalName, 0, 3},
    {Real32Member, VertexTangentBinormalCrossName, 0, 3},

    {EndMember},
};

data_type_definition PNT332VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTextureCoordinatesName "0", 0, 2},

    {EndMember},
};

data_type_definition PNGBT33332VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},
    {Real32Member, VertexBinormalName, 0, 3},
    {Real32Member, VertexTextureCoordinatesName "0", 0, 2},

    {EndMember},
};

data_type_definition PNT333VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTextureCoordinatesName "0", 0, 3},

    {EndMember},
};

data_type_definition PNGBT33333VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},
    {Real32Member, VertexBinormalName, 0, 3},
    {Real32Member, VertexTextureCoordinatesName "0", 0, 3},

    {EndMember},
};

data_type_definition PWN313VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {UInt32Member, VertexBoneIndicesName, 0, 1},
    {Real32Member, VertexNormalName, 0, 3},

    {EndMember},
};

data_type_definition PWNG3133VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {UInt32Member, VertexBoneIndicesName, 0, 1},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},

    {EndMember},
};

data_type_definition PWNGT31332VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {UInt32Member, VertexBoneIndicesName, 0, 1},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},
    {Real32Member, VertexTextureCoordinatesName "0", 0, 3},

    {EndMember},
};

data_type_definition PWNGB31333VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {UInt32Member, VertexBoneIndicesName, 0, 1},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},
    {Real32Member, VertexBinormalName, 0, 3},

    {EndMember},
};

data_type_definition PWNT3132VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {UInt32Member, VertexBoneIndicesName, 0, 1},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTextureCoordinatesName "0", 0, 2},

    {EndMember},
};

data_type_definition PWNGBT313332VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {UInt32Member, VertexBoneIndicesName, 0, 1},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},
    {Real32Member, VertexBinormalName, 0, 3},
    {Real32Member, VertexTextureCoordinatesName "0", 0, 2},

    {EndMember},
};

data_type_definition PWN323VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {NormalUInt8Member, VertexBoneWeightsName, 0, 2},
    {UInt8Member, VertexBoneIndicesName, 0, 2},
    {Real32Member, VertexNormalName, 0, 3},

    {EndMember},
};

data_type_definition PWNG3233VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {NormalUInt8Member, VertexBoneWeightsName, 0, 2},
    {UInt8Member, VertexBoneIndicesName, 0, 2},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},

    {EndMember},
};

data_type_definition PWNGT32332VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {NormalUInt8Member, VertexBoneWeightsName, 0, 2},
    {UInt8Member, VertexBoneIndicesName, 0, 2},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},
    {Real32Member, VertexTextureCoordinatesName "0", 0, 2},

    {EndMember},
};

data_type_definition PWNGB32333VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {NormalUInt8Member, VertexBoneWeightsName, 0, 2},
    {UInt8Member, VertexBoneIndicesName, 0, 2},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},
    {Real32Member, VertexBinormalName, 0, 3},

    {EndMember},
};

data_type_definition PWNT3232VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {NormalUInt8Member, VertexBoneWeightsName, 0, 2},
    {UInt8Member, VertexBoneIndicesName, 0, 2},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTextureCoordinatesName "0", 0, 2},

    {EndMember},
};

data_type_definition PWNGBT323332VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {NormalUInt8Member, VertexBoneWeightsName, 0, 2},
    {UInt8Member, VertexBoneIndicesName, 0, 2},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},
    {Real32Member, VertexBinormalName, 0, 3},
    {Real32Member, VertexTextureCoordinatesName "0", 0, 2},

    {EndMember},
};

data_type_definition PWN343VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {NormalUInt8Member, VertexBoneWeightsName, 0, 4},
    {UInt8Member, VertexBoneIndicesName, 0, 4},
    {Real32Member, VertexNormalName, 0, 3},

    {EndMember},
};

data_type_definition PWNG3433VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {NormalUInt8Member, VertexBoneWeightsName, 0, 4},
    {UInt8Member, VertexBoneIndicesName, 0, 4},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},

    {EndMember},
};

data_type_definition PWNGT34332VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {NormalUInt8Member, VertexBoneWeightsName, 0, 4},
    {UInt8Member, VertexBoneIndicesName, 0, 4},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},
    {Real32Member, VertexTextureCoordinatesName "0", 0, 2},

    {EndMember},
};

data_type_definition PWNGB34333VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {NormalUInt8Member, VertexBoneWeightsName, 0, 4},
    {UInt8Member, VertexBoneIndicesName, 0, 4},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},
    {Real32Member, VertexBinormalName, 0, 3},

    {EndMember},
};

data_type_definition PWNT3432VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {NormalUInt8Member, VertexBoneWeightsName, 0, 4},
    {UInt8Member, VertexBoneIndicesName, 0, 4},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTextureCoordinatesName "0", 0, 2},

    {EndMember},
};

data_type_definition PWNGBT343332VertexType[] =
{
    {Real32Member, VertexPositionName, 0, 3},
    {NormalUInt8Member, VertexBoneWeightsName, 0, 4},
    {UInt8Member, VertexBoneIndicesName, 0, 4},
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},
    {Real32Member, VertexBinormalName, 0, 3},
    {Real32Member, VertexTextureCoordinatesName "0", 0, 2},

    {EndMember},
};

data_type_definition VertexWeightArraysType[] =
{
    {Real32Member, VertexBoneWeightsName, 0, MaximumWeightCount},
    {UInt32Member, VertexBoneIndicesName, 0, MaximumWeightCount},
    {EndMember},
};

struct copy_entry
{
    int32x SourceOffset;
    int32x CopySize;
    int32x Advance;
};

END_GRANNY_NAMESPACE;

static bool
CreateConversionPlan(data_type_definition const* ToType,
                     data_type_definition const* FromType,
                     copy_entry* Plan, int32x MaxEntries,
                     int32x* NumEntries,
                     bool* PlainCopy)
{
    Assert(ToType);
    Assert(FromType);
    Assert(Plan);
    Assert(MaxEntries > 0);
    Assert(NumEntries);
    Assert(PlainCopy);

    int32x TotalSize = GetTotalObjectSize(ToType);

    *PlainCopy = TotalSize == GetTotalObjectSize(FromType);
    int32x CurrentEntry = 0;
    int32x CurrentPos   = 0;
    while (ToType->Type != EndMember && CurrentEntry < MaxEntries)
    {
        switch (ToType->Type)
        {
            // We can't handle these, but they're unlikely to show up in a vertex
            // Todo: we could handle the non-references if the data types are equal...
            case InlineMember:
            case ReferenceMember:
            case ReferenceToArrayMember:
            case ArrayOfReferencesMember:
            case VariantReferenceMember:
            case UnsupportedMemberType_Remove:
            case ReferenceToVariantArrayMember:
            case TransformMember:
            case EmptyReferenceMember:
			{
				*PlainCopy = false;
                return false;
			}

            default: break;
        }

        int32x DestMemberSize = GetMemberTypeSize(*ToType);
        variant Result;
        if (FindMatchingMember(FromType, NULL, ToType->Name, &Result))
        {
            if (ToType->Type == Result.Type->Type)
            {
                int32x SourceMemberSize = GetMemberTypeSize(*Result.Type);
                Plan[CurrentEntry].SourceOffset = (int32x)(intaddrx)Result.Object;
                Plan[CurrentEntry].CopySize = DestMemberSize > SourceMemberSize ? SourceMemberSize : DestMemberSize;
                Plan[CurrentEntry].Advance  = DestMemberSize;

                if (Plan[CurrentEntry].SourceOffset != CurrentPos ||
                    SourceMemberSize != DestMemberSize)
                {
                    *PlainCopy = false;
                }

                CurrentPos += DestMemberSize;
            }
            else
            {
                // Conversion.  We can't handle that in a simple copy plan
                return false;
            }
        }
        else
        {
            // Advancement entry only
            Plan[CurrentEntry].SourceOffset = 0;
            Plan[CurrentEntry].CopySize     = 0;
            Plan[CurrentEntry].Advance      = DestMemberSize;

            *PlainCopy = false;
            CurrentPos += DestMemberSize;
        }

        ++CurrentEntry;
        ++ToType;
    }

    if (CurrentPos != TotalSize)
    {
        if (CurrentEntry == MaxEntries)
            return false;

        Plan[CurrentEntry].SourceOffset = 0;
        Plan[CurrentEntry].CopySize     = 0;
        Plan[CurrentEntry].Advance      = TotalSize - CurrentPos;
        *PlainCopy = false;
        ++CurrentEntry;
    }

    if (CurrentEntry == MaxEntries && ToType->Type != EndMember)
    {
        // Ran out of entries, or some other unhandlable condition
        return false;
    }

    *NumEntries = CurrentEntry;
    return true;
}

static void
ExecuteConversionPlan(uint8* DestVertices, int32x const DestStride,
                      uint8 const* SourceVertices, int32x const SourceStride,
                      int32x const VertexCount,
                      copy_entry const* Plan,
                      int32x const NumPlanEntries)
{
    {for(int32x Vertex = 0; Vertex < VertexCount; ++Vertex)
    {
        uint8* DestVert   = DestVertices + Vertex * DestStride;
        uint8 const* SourceVert = SourceVertices + Vertex * SourceStride;

        {for(int32x EntryIdx = 0; EntryIdx < NumPlanEntries; ++EntryIdx)
        {
            copy_entry const& Entry = Plan[EntryIdx];

            if (Entry.CopySize != 0)
                Copy(Entry.CopySize, SourceVert + Entry.SourceOffset, DestVert);

            DestVert += Entry.Advance;
        }}
    }}
}


void GRANNY
ConvertVertexLayouts(int32x VertexCount,
                     data_type_definition const *SourceLayoutType,
                     void const *SourceVertices,
                     data_type_definition const *DestLayoutType,
                     void *DestVertices)
{
    int32x SourceStride = GetTotalObjectSize(SourceLayoutType);
    int32x DestStride = GetTotalObjectSize(DestLayoutType);

    int32x const MaxPlanEntries = 32;
    copy_entry Plan[MaxPlanEntries];
    int32x PlanCount = 0;
    bool PlainCopy = false;
    if (CreateConversionPlan(DestLayoutType, SourceLayoutType,
                             Plan, MaxPlanEntries,
                             &PlanCount, &PlainCopy))
    {
        if (!PlainCopy)
        {

            // Fast plan default initialization is always 0.
            SetUInt8(DestStride * VertexCount, 0, DestVertices);
            ExecuteConversionPlan((uint8*)DestVertices, DestStride,
                                  (uint8 const*)SourceVertices, SourceStride,
                                  VertexCount,
                                  Plan, PlanCount);
        }
        else
        {
            // Conversion plain is a simple copy, the types are identical, so just
            // memcpy.
            Assert(DestStride == SourceStride);
            Copy(DestStride * VertexCount, SourceVertices, DestVertices);
        }
    }
    else
    {

        uint8 const *Source = (uint8 const *)SourceVertices;
        uint8 *Dest = (uint8 *)DestVertices;
        while(VertexCount--)
        {
            ConvertSingleObject(SourceLayoutType, Source,
                                DestLayoutType, Dest);

            Source += SourceStride;
            Dest += DestStride;
        }
    }
}


#define ExactOneNormTemplate(Count, Values, OneValue)   \
    do {                                                \
        int32x Total = 0;                               \
                                                        \
        {for(int32x Index = 0;                          \
             Index < Count;                             \
             ++Index)                                   \
        {                                               \
            Total += Values[Index];                     \
        }}                                              \
                                                        \
        int32x Difference = (int32)OneValue - Total;    \
        while(Difference > 0)                           \
        {                                               \
            {for(int32x Index = 0;                      \
                 (Index < Count) && (Difference > 0);   \
                 ++Index)                               \
            {                                           \
                if(Values[Index] < OneValue)            \
                {                                       \
                    ++Values[Index];                    \
                    --Difference;                       \
                }                                       \
            }}                                          \
        }                                               \
        while(Difference < 0)                           \
        {                                               \
            {for(int32x Index = Count - 1;              \
                 (Index >= 0) && (Difference < 0);      \
                 --Index)                               \
            {                                           \
                if(Values[Index] > 0)                   \
                {                                       \
                    --Values[Index];                    \
                    ++Difference;                       \
                }                                       \
            }}                                          \
        }                                               \
    } while (false)


#if DEBUG
#define ExactOneNormTemplateCheck(Count, Values, OneValue)  \
    do {                                                    \
        int32x Total = 0;                                   \
                                                            \
        {for(int32x Index = 0;                              \
             Index < Count;                                 \
             ++Index)                                       \
        {                                                   \
            Total += Values[Index];                         \
        }}                                                  \
        Assert ( Total == (int32)OneValue );                \
    } while (false)

#else
#define ExactOneNormTemplateCheck(Count, Values, OneValue) do {} while (false)
#endif

void GRANNY
EnsureExactOneNorm(data_type_definition const &WeightsType,
                   void *WeightsInit)
{
    int32x WeightsCount = WeightsType.ArrayWidth;
    switch(WeightsType.Type)
    {
        case BinormalInt8Member:
        {
            int8 *Weights = (int8 *)WeightsInit;
            ExactOneNormTemplate(WeightsCount, Weights, Int8Maximum);
            ExactOneNormTemplateCheck(WeightsCount, Weights, Int8Maximum);
        } break;

        case NormalUInt8Member:
        {
            uint8 *Weights = (uint8 *)WeightsInit;
            ExactOneNormTemplate(WeightsCount, Weights, UInt8Maximum);
            ExactOneNormTemplateCheck(WeightsCount, Weights, UInt8Maximum);
        } break;

        case BinormalInt16Member:
        {
            int16 *Weights = (int16 *)WeightsInit;
            ExactOneNormTemplate(WeightsCount, Weights, Int16Maximum);
            ExactOneNormTemplateCheck(WeightsCount, Weights, Int16Maximum);
        } break;

        case NormalUInt16Member:
        {
            uint16 *Weights = (uint16 *)WeightsInit;
            ExactOneNormTemplate(WeightsCount, Weights, UInt16Maximum);
            ExactOneNormTemplateCheck(WeightsCount, Weights, UInt16Maximum);
        } break;

        case Real32Member:
        case Real16Member:
        {
            // Implicitly works - don't need to do anything
        } break;

        default:
        {
            Log1(ErrorLogMessage, MeshLogMessage,
                 "EnsureExactOneNorm called on unsupported "
                 "type %d\n", WeightsType.Type);
        } break;
    }
}

void GRANNY
OneNormalizeWeights(int32x VertexCount,
                    data_type_definition const *LayoutType,
                    void *Vertices)
{
    uint8 *VertexPointer = (uint8 *)Vertices;
    int32x VertexSize = GetTotalObjectSize(LayoutType);

    variant Weights;
    FindMatchingMember(LayoutType, 0,
                       VertexBoneWeightsName,
                       &Weights);
    if(Weights.Type)
    {
        int32x WeightCount = Weights.Type->ArrayWidth;

        vertex_weight_arrays WeightList;
        while(VertexCount--)
        {
            ConvertSingleObject(LayoutType, VertexPointer,
                                VertexWeightArraysType, &WeightList);

            real32 TotalWeight = 0.0f;
            {for(int32x WeightIndex = 0;
                 WeightIndex < WeightCount;
                 ++WeightIndex)
            {
                TotalWeight += WeightList.BoneWeights[WeightIndex];
            }}

            if(TotalWeight != 0.0f)
            {
                real32 const Normalization = 1.0f / TotalWeight;
                {for(int32x WeightIndex = 0;
                     WeightIndex < WeightCount;
                     ++WeightIndex)
                {
                    WeightList.BoneWeights[WeightIndex] *= Normalization;
                }}
            }

            // Ensure that if the weight is 0, the index is the same as the first index.
            // This prevents illegal access, and is also more cache efficient if we
            // unconditionally load the matrix
            {for(int32x WeightIndex = 1;
                 WeightIndex < WeightCount;
                 ++WeightIndex)
            {
                if (WeightList.BoneWeights[WeightIndex] == 0.0f)
                    WeightList.BoneIndices[WeightIndex] = WeightList.BoneIndices[0];
            }}

            MergeSingleObject(VertexWeightArraysType, &WeightList,
                              LayoutType, VertexPointer);
            EnsureExactOneNorm(*Weights.Type,
                               VertexPointer + (intaddrx)Weights.Object);

            VertexPointer += VertexSize;
        }
    }
}

void GRANNY
TransformVertices(int32x VertexCount,
                  data_type_definition const *LayoutType, void *Vertices,
                  real32 const *Affine3, real32 const *Linear3x3,
                  real32 const *InverseLinear3x3,
                  bool Renormalize, bool TreatAsDeltas)
{
    // Transform all the vertices
    uint8 *Vertex = (uint8 *)Vertices;
    int32x VertexSize = GetTotalObjectSize(LayoutType);

    pngbx33333_vertex Unpacked;
    {for(int32x VertexIndex = 0;
         VertexIndex < VertexCount;
         ++VertexIndex)
    {
        ConvertSingleObject(LayoutType, Vertex,
                            PNGBX33333VertexType, &Unpacked);

        // These are multiplied by A because they are regular vectors (unless they are
        // morph deltas)
        VectorTransform3(Unpacked.Position, Linear3x3);
        if (!TreatAsDeltas)
        {
            VectorAdd3(Unpacked.Position, Affine3);
        }

        VectorTransform3(Unpacked.Tangent, Linear3x3);
        VectorTransform3(Unpacked.Binormal, Linear3x3);

        // These are mulitplied by A^-T because they are normals, again, unless they are
        // OS deltas against real normals from a morph target
        if (!TreatAsDeltas)
        {
            TransposeVectorTransform3(Unpacked.TangentBinormalCross,
                                      InverseLinear3x3);
            TransposeVectorTransform3(Unpacked.Normal,
                                      InverseLinear3x3);
        }
        else
        {
            TransposeVectorTransform3(Unpacked.TangentBinormalCross,
                                      Linear3x3);
            TransposeVectorTransform3(Unpacked.Normal,
                                      Linear3x3);
        }

        if(Renormalize)
        {
            NormalizeOrZero3(Unpacked.Tangent);
            NormalizeOrZero3(Unpacked.Binormal);
            NormalizeOrZero3(Unpacked.TangentBinormalCross);
            NormalizeOrZero3(Unpacked.Normal);
        }

        MergeSingleObject(PNGBX33333VertexType, &Unpacked,
                          LayoutType, Vertex);

        Vertex += VertexSize;
    }}
}

void GRANNY
NormalizeVertices(int32x VertexCount,
                  data_type_definition const *LayoutType, void *Vertices)
{
    // Transform all the vertices
    uint8 *Vertex = (uint8 *)Vertices;
    int32x VertexSize = GetTotalObjectSize(LayoutType);

    pngbx33333_vertex Unpacked;
    {for(int32x VertexIndex = 0;
         VertexIndex < VertexCount;
         ++VertexIndex)
    {
        ConvertSingleObject(LayoutType, Vertex,
                            PNGBX33333VertexType, &Unpacked);
        NormalizeOrZero3(Unpacked.Tangent);
        NormalizeOrZero3(Unpacked.Binormal);
        NormalizeOrZero3(Unpacked.TangentBinormalCross);
        NormalizeOrZero3(Unpacked.Normal);

        MergeSingleObject(PNGBX33333VertexType, &Unpacked,
                          LayoutType, Vertex);

        Vertex += VertexSize;
    }}
}

int32x GRANNY
GetVertexTextureCoordinatesName(int32x Index, char *Buffer)
{
    return(ConvertToStringVar(MaximumVertexNameLength, Buffer,
                              "%s%d", VertexTextureCoordinatesName, Index));
}

int32x GRANNY
GetVertexDiffuseColorName(int32x Index, char *Buffer)
{
    return(ConvertToStringVar(MaximumVertexNameLength, Buffer,
                              "%s%d", VertexDiffuseColorName, Index));
}

int32x GRANNY
GetVertexSpecularColorName(int32x Index, char *Buffer)
{
    return(ConvertToStringVar(MaximumVertexNameLength, Buffer,
                              "%s%d", VertexSpecularColorName, Index));
}

bool GRANNY
IsSpatialVertexMember(char const *Name)
{
    return(StringsAreEqual(Name, VertexPositionName) ||
           StringsAreEqual(Name, VertexNormalName) ||
           StringsAreEqual(Name, VertexTangentName) ||
           StringsAreEqual(Name, VertexBinormalName) ||
           StringsAreEqual(Name, VertexTangentBinormalCrossName) ||
           StringsAreEqual(Name, VertexBoneIndicesName) ||
           StringsAreEqual(Name, VertexBoneWeightsName));
}

int32x GRANNY
GetVertexBoneCount(data_type_definition const *VertexType)
{
    variant Weights;
    FindMatchingMember(VertexType, 0,
                       VertexBoneWeightsName,
                       &Weights);
    if(Weights.Type)
    {
        return(Weights.Type->ArrayWidth);
    }
    else
    {
        FindMatchingMember(VertexType, 0,
                           VertexBoneIndicesName,
                           &Weights);
        if(Weights.Type)
        {
            return(Weights.Type->ArrayWidth);
        }
    }

    return(0);
}

int32x GRANNY
GetVertexChannelCount(data_type_definition const *VertexType)
{
    int32x ChannelCount = 0;
    while(VertexType->Type != EndMember)
    {
        if(!IsSpatialVertexMember(VertexType->Name))
        {
            ++ChannelCount;
        }

        ++VertexType;
    }

    return(ChannelCount);
}

void GRANNY
GetSingleVertex(data_type_definition const *SourceType,
                void const *SourceVertices,
                int32x VertexIndex, data_type_definition const *As,
                void *Dest)
{
    int32x VertexSize = GetTotalObjectSize(SourceType);
    ConvertSingleObject(SourceType,
                        ((uint8 *)SourceVertices) + VertexIndex*VertexSize,
                        As, Dest);
}

void GRANNY
GetSingleVertex(vertex_data const &VertexData,
                int32x VertexIndex,
                data_type_definition const *As,
                void *Dest)
{
    CheckPointerNotNull(As, return);
    CheckPointerNotNull(Dest, return);
    CheckCountedInt32(VertexIndex, VertexData.VertexCount,
                      MakeEmptyDataTypeObject(As, Dest); return);

    GetSingleVertex(VertexData.VertexType, VertexData.Vertices,
                    VertexIndex, As, Dest);
}

void GRANNY
SetVertexPosition(data_type_definition const *VertexLayout,
                  void *VertexPointer, real32 const *Position)
{
    MergeSingleObject(P3VertexType, Position, VertexLayout, VertexPointer);
}

#define DECLARE_TEMP_TYPE \
    data_type_definition TempType[] =    \
    {                                           \
        {Real32Member, 0, 0, 0},                \
        {EndMember},                            \
    }

void GRANNY
SetVertexNormal(data_type_definition const *VertexLayout,
                void *VertexPointer, real32 const *Normal)
{
    DECLARE_TEMP_TYPE;
    TempType[0].Name = VertexNormalName;
    TempType[0].ArrayWidth = 3;
    MergeSingleObject(TempType, Normal, VertexLayout, VertexPointer);
}

void GRANNY
SetVertexColor(data_type_definition const *VertexLayout,
               void *VertexPointer, int32x ColorIndex,
               real32 const *Color)
{
    DECLARE_TEMP_TYPE;

    char TempName[MaximumVertexNameLength];
    GetVertexDiffuseColorName(ColorIndex, TempName);
    TempType[0].Name = TempName;
    TempType[0].ArrayWidth = 3;
    MergeSingleObject(TempType, Color, VertexLayout, VertexPointer);
}

void GRANNY
SetVertexUVW(data_type_definition const *VertexLayout,
             void *VertexPointer, int32x UVWIndex,
             real32 const *UVW)
{
    DECLARE_TEMP_TYPE;

    char TempName[MaximumVertexNameLength];
    GetVertexTextureCoordinatesName(UVWIndex, TempName);
    TempType[0].Name = TempName;
    TempType[0].ArrayWidth = 3;
    MergeSingleObject(TempType, UVW, VertexLayout, VertexPointer);
}


int32x GRANNY
GetVertexComponentCount(data_type_definition const *VertexLayout)
{
    int32x MemberCount = 0;
    data_type_definition const *SourceType = VertexLayout;
    while(SourceType && (SourceType->Type != EndMember))
    {
        // We don't count bone weights because they are implicit from having bone indices. Kinda.
        if ( !StringsAreEqual(SourceType->Name, VertexBoneWeightsName) )
        {
            ++MemberCount;
        }
        ++SourceType;
    }
    return MemberCount;
}


int32x GRANNY
GetVertexComponentIndex(char const *ComponentName, data_type_definition const *VertexLayout)
{
    // So this will return the "item number" of a component of a vertex.
    // So for most formats, GetVertexTypeComponentIndex ( VertexPositionName, VertexLayout ) == 0
    // And then for PNGB3333VertexType, GetVertexTypeComponentIndex ( VertexTangentName, VertexLayout ) == 2
    // This allows people to look up the tool's component name in the VertexComponentNames array by, e.g.:
    // "uvMayaName" == VertexComponentNames[GetVertexTypeComponentIndex ( VertexTextureCoordinatesName "0", VertexLayout )]
    // (which is exactly what GetVertexComponentToolName does)

    int32x MemberNumber = 0;
    data_type_definition const *SourceType = VertexLayout;
    while(SourceType && (SourceType->Type != EndMember))
    {
        if(StringsAreEqualLowercaseOrCallback(SourceType->Name, ComponentName))
        {
            return MemberNumber;
        }

        ++SourceType;
        ++MemberNumber;
    }

    // Not found.
    return -1;
}

char const * GRANNY
GetVertexComponentToolName(char const *ComponentName, vertex_data const *VertexData)
{
    int32x Index = GetVertexComponentIndex ( ComponentName, VertexData->VertexType );
    if ( Index < 0 )
    {
        // Doesn't exist.
        return NULL;
    }
    else if ( Index >= VertexData->VertexComponentNameCount )
    {
        // Probably means VertexComponentNameCount is zero, i.e. old format data.
        return NULL;
    }
    else
    {
        return VertexData->VertexComponentNames[Index];
    }
}



