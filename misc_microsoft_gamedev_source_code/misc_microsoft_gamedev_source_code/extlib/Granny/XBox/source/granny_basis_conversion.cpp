// ========================================================================
// $File: //jeffr/granny/rt/granny_basis_conversion.cpp $
// $DateTime: 2007/07/23 15:42:28 $
// $Change: 15572 $
// $Revision: #17 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_BASIS_CONVERSION_H)
#include "granny_basis_conversion.h"
#endif

#if !defined(GRANNY_FILE_INFO_H)
#include "granny_file_info.h"
#endif

#if !defined(GRANNY_MESH_H)
#include "granny_mesh.h"
#endif

#if !defined(GRANNY_SKELETON_H)
#include "granny_skeleton.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
#endif

#if !defined(GRANNY_ART_TOOL_INFO_H)
#include "granny_art_tool_info.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_VERTEX_DATA_H)
#include "granny_vertex_data.h"
#endif

#if !defined(GRANNY_TRI_TOPOLOGY_H)
#include "granny_tri_topology.h"
#endif

#if !defined(GRANNY_MODEL_H)
#include "granny_model.h"
#endif

#if !defined(GRANNY_ANIMATION_H)
#include "granny_animation.h"
#endif

#if !defined(GRANNY_TRACK_GROUP_H)
#include "granny_track_group.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_TRACK_GROUP_BUILDER_H)
#include "granny_track_group_builder.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode MathLogMessage



USING_GRANNY_NAMESPACE;

bool GRANNY
ComputeBasisConversion(file_info const &FileInfo,
                       real32 DesiredUnitsPerMeter,
                       real32 const *DesiredOrigin3,
                       real32 const *DesiredRight3,
                       real32 const *DesiredUp3,
                       real32 const *DesiredBack3,
                       real32 *ResultAffine3,
                       real32 *ResultLinear3x3,
                       real32 *ResultInverseLinear3x3)
{
    CheckPointerNotNull(FileInfo.ArtToolInfo, return false);

    art_tool_info const &ArtToolInfo = *FileInfo.ArtToolInfo;

    triple DesiredAxisSystem[3];
    MatrixColumns3x3(DesiredAxisSystem,
                     DesiredRight3,
                     DesiredUp3,
                     DesiredBack3);

    triple SourceAxisSystem[3];
    MatrixColumns3x3(SourceAxisSystem,
                     ArtToolInfo.RightVector,
                     ArtToolInfo.UpVector,
                     ArtToolInfo.BackVector);
    MatrixTranspose3x3(SourceAxisSystem);

    MatrixMultiply3x3(ResultLinear3x3, (real32 *)DesiredAxisSystem,
                      (real32 *)SourceAxisSystem);
    MatrixScale3x3(ResultLinear3x3,
                   DesiredUnitsPerMeter / ArtToolInfo.UnitsPerMeter);

    MatrixInvert3x3(ResultInverseLinear3x3, ResultLinear3x3);

    VectorSubtract3(ResultAffine3, DesiredOrigin3, ArtToolInfo.Origin);

    return true;
}



void GRANNY
TransformMesh(mesh &Mesh,
              real32 const *Affine3,
              real32 const *Linear3x3,
              real32 const *InverseLinear3x3,
              real32 AffineTolerance,
              real32 LinearTolerance,
              uint32x Flags)
{
    if(Mesh.PrimaryVertexData)
    {
        TransformVertices(Mesh.PrimaryVertexData->VertexCount,
                          Mesh.PrimaryVertexData->VertexType,
                          Mesh.PrimaryVertexData->Vertices,
                          Affine3, Linear3x3, InverseLinear3x3,
                          Flags & RenormalizeNormals,
                          false);

        // Look for VertexMorphCurves and transform them too.
        {for ( int32x VertexAnnotationNum = 0; VertexAnnotationNum < Mesh.PrimaryVertexData->VertexAnnotationSetCount; VertexAnnotationNum++ )
        {
            vertex_annotation_set &Set = Mesh.PrimaryVertexData->VertexAnnotationSets[VertexAnnotationNum];
            if ( StringsAreEqual ( Set.Name, VertexMorphCurvePrefix VertexPositionName ) )
            {
                Assert ( DataTypesAreEqual ( Set.VertexAnnotationType, Curve2Type ) );
                curve2 *Curve = (curve2 *)Set.VertexAnnotations;
                {for ( int32x VertNum = 0; VertNum < Set.VertexAnnotationCount; VertNum++ )
                {
                    Assert ( 3 == CurveGetDimensionUnchecked ( *Curve ) );
                    if(!CurveIsIdentity ( *Curve ))
                    {
                        TransformCurveVec3(Affine3, Linear3x3, AffineTolerance, LinearTolerance, Curve );
                    }
                    ++Curve;
                }}
            }
            else if (
                StringsAreEqual ( Set.Name, VertexMorphCurvePrefix VertexNormalName ) ||
                StringsAreEqual ( Set.Name, VertexMorphCurvePrefix VertexTangentBinormalCrossName )
                )
            {
                Assert ( DataTypesAreEqual ( Set.VertexAnnotationType, Curve2Type ) );
                curve2 *Curve = (curve2 *)Set.VertexAnnotations;
                {for ( int32x VertNum = 0; VertNum < Set.VertexAnnotationCount; VertNum++ )
                {
                    // These are multiplied by A^-T because they are normals.
                    real32 LocalTransform[9];
                    MatrixEquals3x3Transpose ( LocalTransform, InverseLinear3x3 );
                    if(Flags & RenormalizeNormals)
                    {
                        // This normalize will deal with equal scaling in the matrix,
                        // but not shears or unequal scaling.
                        // TODO: make this work with shears and unequal scales?
                        NormalizeOrZero3(LocalTransform+0);
                        NormalizeOrZero3(LocalTransform+3);
                        NormalizeOrZero3(LocalTransform+6);
                    }
                    Assert ( 3 == CurveGetDimensionUnchecked ( *Curve ) );
                    if(!CurveIsIdentity ( *Curve ))
                    {
                        TransformCurveVec3(GlobalZeroVector, LocalTransform, 1.0f, LinearTolerance, Curve );
                    }
                    ++Curve;
                }}
            }
            else if (
                StringsAreEqual ( Set.Name, VertexMorphCurvePrefix VertexTangentName ) ||
                StringsAreEqual ( Set.Name, VertexMorphCurvePrefix VertexBinormalName )
                )
            {
                Assert ( DataTypesAreEqual ( Set.VertexAnnotationType, Curve2Type ) );
                curve2 *Curve = (curve2 *)Set.VertexAnnotations;
                {for ( int32x VertNum = 0; VertNum < Set.VertexAnnotationCount; VertNum++ )
                {
                    // These are multiplied by A because they are normal vectors.
                    real32 LocalTransform[9];
                    MatrixEquals3x3 ( LocalTransform, Linear3x3 );
                    if(Flags & RenormalizeNormals)
                    {
                        // This normalize will deal with equal scaling in the matrix,
                        // but not shears or unequal scaling.
                        // TODO: make this work with shears and unequal scales?
                        NormalizeOrZero3(LocalTransform+0);
                        NormalizeOrZero3(LocalTransform+3);
                        NormalizeOrZero3(LocalTransform+6);
                    }
                    Assert ( 3 == CurveGetDimensionUnchecked ( *Curve ) );
                    if(!CurveIsIdentity ( *Curve ))
                    {
                        TransformCurveVec3(GlobalZeroVector, LocalTransform, 1.0f, LinearTolerance, Curve );
                    }
                    ++Curve;
                }}
            }

        }}
    }

    {for(int32x MorphTargetIndex = 0;
         MorphTargetIndex < Mesh.MorphTargetCount;
         ++MorphTargetIndex)
    {
        morph_target& MorphTarget = Mesh.MorphTargets[MorphTargetIndex];

        vertex_data *VertexData =
            Mesh.MorphTargets[MorphTargetIndex].VertexData;
        if (VertexData)
        {
            if (!MorphTarget.DataIsDeltas)
            {
                TransformVertices(VertexData->VertexCount,
                                  VertexData->VertexType,
                                  VertexData->Vertices,
                                  Affine3, Linear3x3, InverseLinear3x3,
                                  Flags & RenormalizeNormals,
                                  false);
            }
            else
            {
                TransformVertices(VertexData->VertexCount,
                                  VertexData->VertexType,
                                  VertexData->Vertices,
                                  Affine3, Linear3x3, InverseLinear3x3,
                                  false, true);
            }
        }
    }}

    if(Flags & ReorderTriangleIndices)
    {
        if(Mesh.PrimaryTopology && (MatrixDeterminant3x3(Linear3x3) < 0.0f))
        {
            InvertTriTopologyWinding(*Mesh.PrimaryTopology);
        }
    }

    {for(int32x BoneBindingIndex = 0;
         BoneBindingIndex < Mesh.BoneBindingCount;
         ++BoneBindingIndex)
    {
        bone_binding &Binding = Mesh.BoneBindings[BoneBindingIndex];
        TransformBoundingBox(Affine3, Linear3x3,
                             Binding.OBBMin, Binding.OBBMax);
    }}
}

void GRANNY
TransformSkeleton(skeleton &Skeleton,
                  real32 const *Affine3,
                  real32 const *Linear3x3,
                  real32 const *InverseLinear3x3,
                  real32 AffineTolerance,
                  real32 LinearTolerance,
                  uint32x Flags)
{
    {for(int32x BoneIndex = 0;
         BoneIndex < Skeleton.BoneCount;
         ++BoneIndex)
    {
        SimilarityTransform(Skeleton.Bones[BoneIndex].LocalTransform,
                            Affine3, Linear3x3, InverseLinear3x3);
        InPlaceSimilarityTransform4x3(
            Affine3, Linear3x3, InverseLinear3x3,
            (real32 *)Skeleton.Bones[BoneIndex].InverseWorld4x4);
    }}
}

void GRANNY
TransformModel(model &Model,
               real32 const *Affine3,
               real32 const *Linear3x3,
               real32 const *InverseLinear3x3,
               real32 AffineTolerance,
               real32 LinearTolerance,
               uint32x Flags)
{
    if(Model.Skeleton)
    {
        TransformSkeleton(*Model.Skeleton, Affine3, Linear3x3, InverseLinear3x3,
                            AffineTolerance, LinearTolerance, Flags);
    }

    {for(int32x MeshIndex = 0;
         MeshIndex < Model.MeshBindingCount;
         ++MeshIndex)
    {
        mesh *Mesh = Model.MeshBindings[MeshIndex].Mesh;
        if(Mesh)
        {
            TransformMesh(*Mesh, Affine3, Linear3x3, InverseLinear3x3,
                            AffineTolerance, LinearTolerance, Flags);
        }
    }}

    SimilarityTransform(Model.InitialPlacement,
                        Affine3, Linear3x3, InverseLinear3x3);
}

void GRANNY
TransformAnimation(animation &Animation,
                   real32 const *Affine3,
                   real32 const *Linear3x3,
                   real32 const *InverseLinear3x3,
                   real32 AffineTolerance,
                   real32 LinearTolerance,
                   uint32x Flags)
{
    {for(int32x TrackGroupIndex = 0;
         TrackGroupIndex < Animation.TrackGroupCount;
         ++TrackGroupIndex)
    {
        track_group *TrackGroup = Animation.TrackGroups[TrackGroupIndex];
        if(TrackGroup)
        {
            SimilarityTransformTrackGroup(*TrackGroup, Affine3,
                                          Linear3x3, InverseLinear3x3,
                                          AffineTolerance, LinearTolerance);
        }
    }}
}

void GRANNY
TransformFile(file_info &FileInfo,
              real32 const *Affine3,
              real32 const *Linear3x3,
              real32 const *InverseLinear3x3,
              real32 AffineTolerance,
              real32 LinearTolerance,
              uint32x Flags)
{
    {for(int32x SkeletonIndex = 0;
         SkeletonIndex < FileInfo.SkeletonCount;
         ++SkeletonIndex)
    {
        skeleton *Skeleton = FileInfo.Skeletons[SkeletonIndex];
        if(Skeleton)
        {
            TransformSkeleton(*Skeleton, Affine3, Linear3x3, InverseLinear3x3,
                                AffineTolerance, LinearTolerance, Flags);
        }
    }}

    {for(int32x MeshIndex = 0;
         MeshIndex < FileInfo.MeshCount;
         ++MeshIndex)
    {
        mesh *Mesh = FileInfo.Meshes[MeshIndex];
        if(Mesh)
        {
            TransformMesh(*Mesh, Affine3, Linear3x3, InverseLinear3x3,
                        AffineTolerance, LinearTolerance, Flags);
        }
    }}

    {for(int32x ModelIndex = 0;
         ModelIndex < FileInfo.ModelCount;
         ++ModelIndex)
    {
        model *Model = FileInfo.Models[ModelIndex];
        if(Model)
        {
            SimilarityTransform(Model->InitialPlacement, Affine3, Linear3x3,
                                InverseLinear3x3);
        }
    }}

    {for(int32x AnimationIndex = 0;
         AnimationIndex < FileInfo.AnimationCount;
         ++AnimationIndex)
    {
        animation *Animation = FileInfo.Animations[AnimationIndex];
        if(Animation)
        {
            TransformAnimation(*Animation, Affine3, Linear3x3,
                               InverseLinear3x3, AffineTolerance, LinearTolerance, Flags);
        }
    }}
}

void GRANNY
ResortAllAnimationTrackGroups(animation &Animation)
{
    {for(int32x TrackGroupIndex = 0;
         TrackGroupIndex < Animation.TrackGroupCount;
         ++TrackGroupIndex)
    {
        track_group *TrackGroup = Animation.TrackGroups[TrackGroupIndex];
        if(TrackGroup)
        {
            ResortTrackGroup(*TrackGroup);
        }
    }}
}

void GRANNY
ResortAllFileTrackGroups(file_info &FileInfo)
{
    {for(int32x AnimationIndex = 0;
         AnimationIndex < FileInfo.AnimationCount;
         ++AnimationIndex)
    {
        animation *Animation = FileInfo.Animations[AnimationIndex];
        if(Animation)
        {
            ResortAllAnimationTrackGroups(*Animation);
        }
    }}
}
