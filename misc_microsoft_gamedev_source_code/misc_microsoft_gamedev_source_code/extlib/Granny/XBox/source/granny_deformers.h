#if !defined(GRANNY_DEFORMERS_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_deformers.h $
// $DateTime: 2007/04/25 14:34:11 $
// $Change: 14851 $
// $Revision: #8 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(MeshDeformerGroup);

struct bone_deformer_parameters;
struct data_type_definition;

typedef void direct_bone_deformer(int32x Count,
                                  void const *Source, void *Dest,
                                  matrix_4x4 const *TransformTable,
                                  int32x CopySize,
                                  int32x SourceStride, int32x DestStride);
typedef void indirected_bone_deformer(int32x Count,
                                      void const *Source, void *Dest,
                                      int32x const *IndirectionTable,
                                      matrix_4x4 const *TransformTable,
                                      int32x CopySize,
                                      int32x SourceStride, int32x DestStride);

typedef void post_deform_transform(int32x Count,
                                   void *DeformedVerts,
                                   int32x DestStride);


EXPTYPE enum deformation_type
{
    DeformPosition = 1,
    DeformPositionNormal,
    DeformPositionNormalTangent,
    DeformPositionNormalTangentBinormal,
};

struct bone_deformer
{
    deformation_type Type;

    direct_bone_deformer *DirectBoneDeformer;
    indirected_bone_deformer *IndirectedBoneDeformer;
    post_deform_transform *PostDeformTransform;

    data_type_definition *FromLayout;
    data_type_definition *ToLayout;

    bool CanDoTailCopies;
    bool CanIgnoreTailItems;
};

struct bone_deformer_parameters
{
    int32x TailCopy32Count;

    int32x SourceVertexSize;
    int32x DestVertexSize;
};

bool FindBoneDeformerFor(data_type_definition const *SourceLayout,
                         data_type_definition const *DestLayout,
                         deformation_type Type,
                         bool IgnoreMismatchedTail,
                         bone_deformer &Result,
                         bone_deformer_parameters &Parameters);

void AddBoneDeformer(bone_deformer &Deformer);
void AddBoneDeformerTable(int32x Count, bone_deformer *Deformer);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_DEFORMERS_H
#endif
