#if !defined(GRANNY_TRI_TOPOLOGY_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_tri_topology.h $
// $DateTime: 2006/10/16 15:44:40 $
// $Change: 13593 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(TopologyGroup);

#define TopologyNullIndex 0xFFFFFFFF EXPMACRO

EXPTYPE struct tri_material_group
{
    int32 MaterialIndex;
    int32 TriFirst;
    int32 TriCount;
};
EXPCONST EXPGROUP(tri_material_group) extern data_type_definition
    TriMaterialGroupType[];

EXPTYPE struct tri_annotation_set
{
    // The name of this annotation set
    char const *Name;

    // Annotation per triangle
    data_type_definition *TriAnnotationType;
    int32 TriAnnotationCount;
    uint8 *TriAnnotations;

    // Indexing (if this is 0, then it's direct-mapped)
    // Some indices may be -1, which means they do not have an entry in TriAnnotations
    // If IndicesMapFromTriToAnnotation is false, then
    //   TriAnnotationIndexCount==TriAnnotationCount,
    //   and for each member in TriAnnotations, there is an entry
    //   in TriAnnotationIndices that says which mesh Tri it is for.
    // If IndicesMapFromTriToAnnotation is true, then
    //   TriAnnotationIndexCount==TriCount,
    //   and for each Tri there is an entry in TriAnnotationIndices
    //   that says which entry in TriAnnotations is applied to it,
    //   or if the index is -1, there is no annotation of this type.
    int32 IndicesMapFromTriToAnnotation;

    int32   TriAnnotationIndexCount;
    int32  *TriAnnotationIndices;
};
EXPCONST EXPGROUP(tri_annotation_set) extern data_type_definition
    TriAnnotationSetType[];

EXPTYPE struct tri_topology
{
    // tri_material_group array
    int32               GroupCount;
    tri_material_group *Groups;

    // Vertex index array
    int32   IndexCount;
    int32  *Indices;

    int32   Index16Count;
    uint16 *Indices16;

    // A mapping from each vertex to its next coincident vertex (circular)
    int32  VertexToVertexCount;
    int32 *VertexToVertexMap;

    // A mapping from each vertex to a face that used it
    int32  VertexToTriangleCount;
    int32 *VertexToTriangleMap;

    // A mapping from each triangle's side to its neighboring triangle's side
    int32   SideToNeighborCount;
    uint32 *SideToNeighborMap;

    // A mapping from each triangle to the bones by which it is affected
    int32  BonesForTriangleCount;
    int32 *BonesForTriangle;

    int32  TriangleToBoneCount;
    int32 *TriangleToBoneIndices;

    // Annotation per triangle
    int32               TriAnnotationSetCount;
    tri_annotation_set *TriAnnotationSets;
};
EXPCONST EXPGROUP(tri_topology) extern data_type_definition TriTopologyType[];

EXPAPI GS_PARAM void InvertTriTopologyWinding(tri_topology &Topology);
EXPAPI GS_PARAM void RemapTopologyMaterials(tri_topology &Topology,
                                            int32x RemapCount, int32x *RemapTable);

EXPAPI GS_PARAM void ConvertIndices(int32x IndexCount,
                                    int32x FromBytesPerIndex, void const *FromIndices,
                                    int32x ToBytesPerIndex, void *ToIndices);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_TRI_TOPOLOGY_H
#endif
