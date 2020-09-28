#if !defined(GRANNY_MESH_BINDING_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_mesh_binding.h $
// $DateTime: 2006/10/19 16:35:09 $
// $Change: 13632 $
// $Revision: #6 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(MeshBindingGroup);

struct mesh;
struct skeleton;
struct world_pose;
struct model_instance;

EXPTYPE struct mesh_binding;

EXPAPI GS_READ mesh_binding *NewMeshBinding(mesh const &Mesh,
                                            skeleton const &FromSkeleton,
                                            skeleton const &ToSkeleton);
EXPAPI GS_PARAM void FreeMeshBinding(mesh_binding *Binding);

EXPAPI GS_READ int32x GetResultingMeshBindingSize(mesh const &Mesh,
                                                  skeleton const &FromSkeleton,
                                                  skeleton const &ToSkeleton);
EXPAPI GS_READ mesh_binding *NewMeshBindingInPlace(mesh const &Mesh,
                                                   skeleton const &FromSkeleton,
                                                   skeleton const &ToSkeleton,
                                                   void *Memory);

EXPAPI GS_READ bool MeshBindingIsTransferred(mesh_binding const &Binding);
EXPAPI GS_READ int32x GetMeshBinding4x4ArraySize(mesh_binding const &Binding,
                                                 int32x BoneCount);
EXPAPI GS_READ void BuildMeshBinding4x4Array(mesh_binding const &Binding,
                                             world_pose const &WorldPose,
                                             int32x FirstBoneIndex,
                                             int32x BoneCount,
                                             real32 *Matrix4x4Array);

EXPAPI GS_READ int32x GetMeshBindingBoneCount(mesh_binding const &Binding);
EXPAPI GS_READ int32x const *GetMeshBindingFromBoneIndices(mesh_binding const &Binding);
EXPAPI GS_READ skeleton *GetMeshBindingFromSkeleton(mesh_binding const &Binding);
EXPAPI GS_READ int32x const *GetMeshBindingToBoneIndices(mesh_binding const &Binding);
EXPAPI GS_READ skeleton *GetMeshBindingToSkeleton(mesh_binding const &Binding);
EXPAPI GS_READ mesh *GetMeshBindingSourceMesh(mesh_binding const &Binding);


END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_MESH_BINDING_H
#endif
