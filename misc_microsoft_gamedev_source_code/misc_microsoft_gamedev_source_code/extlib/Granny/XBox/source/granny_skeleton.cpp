// ========================================================================
// $File: //jeffr/granny/rt/granny_skeleton.cpp $
// $DateTime: 2007/08/17 17:38:16 $
// $Change: 15766 $
// $Revision: #14 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_SKELETON_H)
#include "granny_skeleton.h"
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

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

// For FindAllowedErrorNumbers
#if !defined(GRANNY_CONTROLLED_ANIMATION_H)
#include "granny_controlled_animation.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode SkeletonLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition BoneType[] =
{
    {StringMember, "Name"},

    {Int32Member, "ParentIndex"},
    {TransformMember, "Transform"},
    {Real32Member, "InverseWorldTransform", 0, 16},
    {Real32Member, "LODError"},

    {VariantReferenceMember, "ExtendedData"},

    {EndMember},
};

data_type_definition SkeletonType[] =
{
    {StringMember, "Name"},

    {ReferenceToArrayMember, "Bones", BoneType},

    {Int32Member, "LODType"},

    {EndMember},
};

END_GRANNY_NAMESPACE;

inline transform &
GetTransform(int32x TransformStride, transform const *Transforms,
             int32x Index)
{
    return(*(transform *)(((uint8 *)Transforms) +
                          (TransformStride*Index)));
}

inline int32x
GetParent(int32x ParentStride, int32 const *Parents, int32x Index)
{
    return(*(int32 *)(((uint8 *)Parents) + (ParentStride*Index)));
}

void GRANNY
BuildSkeletonRelativeTransform(int32x SourceTransformStride,
                               transform const *SourceTransforms,
                               int32x SourceParentStride,
                               int32 const *SourceParents,
                               int32x BoneIndex,
                               transform &Result)
{
    // Do we have parents?
    if(SourceParents[BoneIndex] == NoParentBone)
    {
        // We have no parents, so copy the result directly
        Copy(SizeOf(Result), &GetTransform(SourceTransformStride,
                                           SourceTransforms, BoneIndex),
             &Result);
    }
    else
    {
        // We have at least one parent, so fill the result with the first
        // concatenation, then loop from there.
        int32x ParentIndex = GetParent(SourceParentStride,
                                       SourceParents, BoneIndex);
        Multiply(Result,
                 GetTransform(SourceTransformStride,
                              SourceTransforms,
                              ParentIndex),
                 GetTransform(SourceTransformStride,
                              SourceTransforms,
                              BoneIndex));
        BoneIndex = ParentIndex;

        while((ParentIndex = GetParent(SourceParentStride,
                                       SourceParents, BoneIndex))
              != NoParentBone)
        {
            PreMultiplyBy(Result,
                          GetTransform(SourceTransformStride,
                                       SourceTransforms, ParentIndex));
            BoneIndex = ParentIndex;
        }
    }
}

void GRANNY
BuildSkeletonRelativeTransforms(int32x SourceTransformStride,
                                transform const *SourceTransforms,
                                int32x SourceParentStride,
                                int32 const *SourceParents,
                                int32x Count,
                                int32x ResultStride,
                                transform *Results)
{
    int32 const *Parent = SourceParents;
    transform *Dest = Results;

    if(SourceTransforms == Dest)
    {
        // This is the in-buffer transform path, which is generally
        // more efficient since we don't have to do any copying.

        while(Count--)
        {
            if(*Parent != NoParentBone)
            {
                // We have parents, so multiply by our parents _result_
                // transform (thereby saving us the work of multiplying
                // out our entire chain).
                PreMultiplyBy(*Dest, Results[*Parent]);
            }

            Parent = (int32 const *)((uint8 const *)Parent + SourceParentStride);
            Dest = (transform *)((uint8 *)Dest + ResultStride);
        }
    }
    else
    {
        // This is the buffer-to-buffer transform path
        transform const *Source = SourceTransforms;

        while(Count--)
        {
            if(*Parent == NoParentBone)
            {
                // We have no parents, so copy the result directly
                Copy(SizeOf(*Dest), Source, Dest);
            }
            else
            {
                // We have parents, so multiply by our parents _result_
                // transform (thereby saving us the work of multiplying
                // out our entire chain).
                Multiply(*Dest, GetTransform(ResultStride, Results, *Parent),
                         *Source);
            }

            Source = (transform const *)((uint8 *)Source + SourceTransformStride);
            Parent = (int32 const *)((uint8 const *)Parent + SourceParentStride);
            Dest = (transform *)((uint8 *)Dest + ResultStride);
        }
    }
}

bool GRANNY
FindBoneByName(skeleton const *Skeleton, char const *BoneName, int32x &BoneIndex)
{
    COUNT_BLOCK("FindBoneByName");

    if(Skeleton)
    {
        {for(BoneIndex = 0;
             BoneIndex < Skeleton->BoneCount;
             ++BoneIndex)
        {
            if (StringsAreEqualOrCallback(BoneName, Skeleton->Bones[BoneIndex].Name))
                return true;
        }}
    }

    return(false);
}

bool GRANNY
FindBoneByNameLowercase(skeleton const *Skeleton, char const *BoneName, int32x &BoneIndex)
{
    COUNT_BLOCK("FindBoneByNameLowercase");

    if(Skeleton)
    {
        {for(BoneIndex = 0;
             BoneIndex < Skeleton->BoneCount;
             ++BoneIndex)
        {
            if (StringsAreEqualLowercaseOrCallback(BoneName,
                                                   Skeleton->Bones[BoneIndex].Name))
            {
                return(true);
            }
        }}
    }

    return(false);
}


bool GRANNY
FindBoneByRule(skeleton const* Skeleton,
               char const *ProcessedBoneName,
               char const *BonePattern,
               int32x &BoneIndex)
{
    COUNT_BLOCK("FindBoneByRule");

    char BoneNameBuffer[MaximumBoneNameLength + 1];
    if (Skeleton)
    {
        {for(BoneIndex = 0;
             BoneIndex < Skeleton->BoneCount;
             ++BoneIndex)
        {
            WildCardMatch(Skeleton->Bones[BoneIndex].Name,
                          BonePattern, BoneNameBuffer);
            if (StringsAreEqual(ProcessedBoneName, BoneNameBuffer))
            {
                return true;
            }
        }}
    }

    BoneIndex = -1;
    return false;
}


int GRANNY
GetBoneCountForLOD(skeleton const* Skeleton,
                   real32          AllowedError)
{
    COUNT_BLOCK("GetBoneCountForLOD");
    CheckPointerNotNull(Skeleton != NULL, return 0);

    if (AllowedError == 0.0f)
        return Skeleton->BoneCount;

    real32 ErrorEnd;
    real32 ErrorScaler;
    FindAllowedErrorNumbers(AllowedError, &ErrorEnd, &ErrorScaler);

    if (Skeleton)
    {
        if (Skeleton->LODType == NoSkeletonLOD)
        {
            return Skeleton->BoneCount;
        }

        // Otherwise, we do a search.
        // TODO: replace with binary
        int Count = 0;
        for (; Count < Skeleton->BoneCount; Count++)
        {
            // < to make sure that 0 = 0 doesn't get skipped...
            if (Skeleton->Bones[Count].LODError < ErrorEnd)
                break;
        }

        return Count;
    }
    else
    {
        return 0;
    }
}

