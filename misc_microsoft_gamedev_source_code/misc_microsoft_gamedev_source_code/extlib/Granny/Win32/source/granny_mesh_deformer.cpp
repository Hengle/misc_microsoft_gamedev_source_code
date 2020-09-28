// ========================================================================
// $File: //jeffr/granny/rt/granny_mesh_deformer.cpp $
// $DateTime: 2007/10/16 16:53:12 $
// $Change: 16306 $
// $Revision: #8 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_MESH_DEFORMER_H)
#include "granny_mesh_deformer.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

mesh_deformer *GRANNY
NewMeshDeformer(data_type_definition const *InputVertexLayout,
                data_type_definition const *OutputVertexLayout,
                deformation_type DeformationType,
                deformer_tail_flags TailFlag)
{
    mesh_deformer *Deformer = 0;

    bone_deformer BoneDeformer;
    bone_deformer_parameters Parameters;
    if(FindBoneDeformerFor(InputVertexLayout, OutputVertexLayout,
                           DeformationType, TailFlag == AllowUncopiedTail,
                           BoneDeformer, Parameters))
    {
        Deformer = Allocate(mesh_deformer);
        if(Deformer)
        {
            Deformer->BoneDeformer = BoneDeformer;
            Deformer->Parameters = Parameters;
        }
    }

    return(Deformer);
}

void GRANNY
FreeMeshDeformer(mesh_deformer *Deformer)
{
    Deallocate(Deformer);
}

void GRANNY
DeformVertices(mesh_deformer const &Deformer,
               int32x const *TransformIndices,
               real32 const *TransformBuffer,
               int32x VertexCount,
               void const *SourceVertices,
               void *DestVertices)
{
    COUNT_BLOCK("DeformVertices");

    // NOTE: We punt early on 0 vertices, because some of the optimized
    // deformers do pre-reading which will fault if we don't.
    if(VertexCount)
    {
        if(TransformIndices)
        {
            Deformer.BoneDeformer.IndirectedBoneDeformer(
                VertexCount, SourceVertices, DestVertices,
                TransformIndices, (matrix_4x4 const *)TransformBuffer,
                Deformer.Parameters.TailCopy32Count,
                Deformer.Parameters.SourceVertexSize,
                Deformer.Parameters.DestVertexSize);
        }
        else
        {
            Deformer.BoneDeformer.DirectBoneDeformer(
                VertexCount, SourceVertices,
                DestVertices, (matrix_4x4 const *)TransformBuffer,
                Deformer.Parameters.TailCopy32Count,
                Deformer.Parameters.SourceVertexSize,
                Deformer.Parameters.DestVertexSize);
        }

        if (Deformer.BoneDeformer.PostDeformTransform)
        {
            Deformer.BoneDeformer.PostDeformTransform(VertexCount,
                                                      DestVertices,
                                                      Deformer.Parameters.DestVertexSize);
        }
    }
}
