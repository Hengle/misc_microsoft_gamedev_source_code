#if !defined(GRANNY_MESH_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_mesh.h $
// $DateTime: 2007/05/17 21:25:15 $
// $Change: 14972 $
// $Revision: #9 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(MeshGroup);

struct tri_material_group;
struct tri_topology;
struct vertex_data;
struct material;

EXPTYPE struct bone_binding
{
    // Bone identification
    char const *BoneName;

    // Bone-relative OBB
    triple OBBMin;
    triple OBBMax;

    // Triangles touched by this bone, for collision detection
    int32 TriangleCount;
    int32 *TriangleIndices;
};
EXPCONST EXPGROUP(bone_binding) extern data_type_definition BoneBindingType[];

EXPTYPE struct material_binding
{
    material *Material;
};
EXPCONST EXPGROUP(material_binding) extern data_type_definition MaterialBindingType[];

EXPTYPE struct morph_target
{
    char const *ScalarName;
    vertex_data *VertexData;
    int32  DataIsDeltas;
};
EXPCONST EXPGROUP(morph_target) extern data_type_definition MorphTargetType[];

EXPTYPE struct mesh
{
    char const *Name;

    //
    vertex_data *PrimaryVertexData;

    int32 MorphTargetCount;
    morph_target *MorphTargets;

    //
    tri_topology *PrimaryTopology;

    //
    int32 MaterialBindingCount;
    material_binding *MaterialBindings;

    int32 BoneBindingCount;
    bone_binding *BoneBindings;

    //
    variant ExtendedData;
};
EXPCONST EXPGROUP(mesh) extern data_type_definition MeshType[];

EXPAPI GS_READ int32x GetMeshMorphTargetCount(mesh const &Mesh);
EXPAPI GS_READ int32x GetMeshTriangleGroupCount(mesh const &Mesh);
EXPAPI GS_READ tri_material_group *GetMeshTriangleGroups(mesh const &Mesh);
EXPAPI GS_READ data_type_definition *GetMeshVertexType(mesh const &Mesh);
EXPAPI GS_READ data_type_definition *GetMeshMorphVertexType(mesh const &Mesh,
                                                            int32x MorphTargetIndex);
EXPAPI GS_READ int32x GetMeshVertexCount(mesh const &Mesh);
EXPAPI GS_READ int32x GetMeshMorphVertexCount(mesh const &Mesh,
                                              int32x MorphTargetIndex);
EXPAPI GS_PARAM void CopyMeshVertices(mesh const &Mesh,
                                      data_type_definition const *VertexType,
                                      void *DestVertices);
EXPAPI GS_PARAM void CopyMeshMorphVertices(mesh const &Mesh, int32x MorphTargetIndex,
                                           data_type_definition const *VertexType,
                                           void *DestVertices);
EXPAPI GS_READ void *GetMeshVertices(mesh const &Mesh);
EXPAPI GS_READ void *GetMeshMorphVertices(mesh const &Mesh, int32x MorphTargetIndex);
EXPAPI GS_READ bool MeshIsRigid(mesh const &Mesh);
EXPAPI GS_READ int32x GetMeshIndexCount(mesh const &Mesh);
EXPAPI GS_READ int32x GetMeshTriangleCount(mesh const &Mesh);
EXPAPI GS_READ int32x GetMeshBytesPerIndex(mesh const &Mesh);
EXPAPI GS_READ void *GetMeshIndices(mesh const &Mesh);
EXPAPI GS_PARAM void CopyMeshIndices(mesh const &Mesh, int32x BytesPerIndex,
                                     void *DestIndices);

EXPAPI GS_SAFE void TransformBoundingBox(real32 const *Affine3,
                                         real32 const *Linear3x3,
                                         real32 *OBBMin,
                                         real32 *OBBMax);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_MESH_H
#endif
