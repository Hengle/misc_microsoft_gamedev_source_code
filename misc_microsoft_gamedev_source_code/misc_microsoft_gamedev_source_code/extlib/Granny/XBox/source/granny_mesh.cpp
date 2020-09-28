// ========================================================================
// $File: //jeffr/granny/rt/granny_mesh.cpp $
// $DateTime: 2007/07/23 15:42:28 $
// $Change: 15572 $
// $Revision: #9 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_MESH_H)
#include "granny_mesh.h"
#endif

#if !defined(GRANNY_MATERIAL_H)
#include "granny_material.h"
#endif

#if !defined(GRANNY_VERTEX_DATA_H)
#include "granny_vertex_data.h"
#endif

#if !defined(GRANNY_TRI_TOPOLOGY_H)
#include "granny_tri_topology.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode MeshLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition BoneBindingType[] =
{
    {StringMember, "BoneName"},
    {Real32Member, "OBBMin", 0, 3},
    {Real32Member, "OBBMax", 0, 3},
    {ReferenceToArrayMember, "TriangleIndices", Int32Type},
    {EndMember},
};

data_type_definition MaterialBindingType[] =
{
    {ReferenceMember, "Material", MaterialType},
    {EndMember},
};

data_type_definition MorphTargetType[] =
{
    {StringMember, "ScalarName"},
    {ReferenceMember, "VertexData", VertexDataType},
    {Int32Member, "DataIsDeltas"},
    {EndMember},
};

data_type_definition MeshType[] =
{
    {StringMember, "Name"},

    {ReferenceMember, "PrimaryVertexData", VertexDataType},
    {ReferenceToArrayMember, "MorphTargets", MorphTargetType},

    {ReferenceMember, "PrimaryTopology", TriTopologyType},

    {ReferenceToArrayMember, "MaterialBindings", MaterialBindingType},
    {ReferenceToArrayMember, "BoneBindings", BoneBindingType},

    {VariantReferenceMember, "ExtendedData"},

    {EndMember},
};

END_GRANNY_NAMESPACE;

int32x GRANNY
GetMeshMorphTargetCount(mesh const &Mesh)
{
    return(Mesh.MorphTargetCount);
}

int32x GRANNY
GetMeshTriangleGroupCount(mesh const &Mesh)
{
    int32x GroupCount = 0;
    if(Mesh.PrimaryTopology)
    {
        GroupCount = Mesh.PrimaryTopology->GroupCount;
    }

    return(GroupCount);
}

tri_material_group *GRANNY
GetMeshTriangleGroups(mesh const &Mesh)
{
    tri_material_group *Groups = 0;
    if(Mesh.PrimaryTopology)
    {
        Groups = Mesh.PrimaryTopology->Groups;
    }

    return(Groups);
}

data_type_definition *GRANNY
GetMeshVertexType(mesh const &Mesh)
{
    data_type_definition *VertexType = 0;
    if(Mesh.PrimaryVertexData)
    {
        VertexType = Mesh.PrimaryVertexData->VertexType;
    }

    return(VertexType);
}

data_type_definition *GRANNY
GetMeshMorphVertexType(mesh const &Mesh, int32x MorphTargetIndex)
{
    CheckCountedInt32(MorphTargetIndex, Mesh.MorphTargetCount, return(0));

    morph_target &MorphTarget = Mesh.MorphTargets[MorphTargetIndex];

    data_type_definition *VertexType = 0;
    if(MorphTarget.VertexData)
    {
        VertexType = MorphTarget.VertexData->VertexType;
    }

    return(VertexType);
}

int32x GRANNY
GetMeshVertexCount(mesh const &Mesh)
{
    int32x VertexCount = 0;
    if(Mesh.PrimaryVertexData)
    {
        VertexCount = Mesh.PrimaryVertexData->VertexCount;
    }

    return(VertexCount);
}

int32x GRANNY
GetMeshMorphVertexCount(mesh const &Mesh, int32x MorphTargetIndex)
{
    CheckCountedInt32(MorphTargetIndex, Mesh.MorphTargetCount, return(0));

    morph_target &MorphTarget = Mesh.MorphTargets[MorphTargetIndex];

    int32x VertexCount = 0;
    if(MorphTarget.VertexData)
    {
        VertexCount = MorphTarget.VertexData->VertexCount;
    }

    return(VertexCount);
}

void GRANNY
CopyMeshVertices(mesh const &Mesh, data_type_definition const *VertexType,
                 void *DestVertices)
{
    void *SourceVertices = GetMeshVertices(Mesh);
    if(SourceVertices)
    {
        ConvertVertexLayouts(GetMeshVertexCount(Mesh),
                             GetMeshVertexType(Mesh),
                             SourceVertices,
                             VertexType, DestVertices);
    }
    else
    {
        Log1(ErrorLogMessage, MeshLogMessage,
             "Mesh \"%s\" has no vertices.", Mesh.Name);
    }
}

void GRANNY
CopyMeshMorphVertices(mesh const &Mesh, int32x MorphTargetIndex,
                      data_type_definition const *VertexType,
                      void *DestVertices)
{
    void *SourceVertices = GetMeshMorphVertices(Mesh, MorphTargetIndex);
    if(SourceVertices)
    {
        ConvertVertexLayouts(GetMeshMorphVertexCount(Mesh, MorphTargetIndex),
                             GetMeshMorphVertexType(Mesh, MorphTargetIndex),
                             SourceVertices,
                             VertexType, DestVertices);
    }
    else
    {
        Log1(ErrorLogMessage, MeshLogMessage,
             "Mesh \"%s\" has no vertices.", Mesh.Name);
    }
}

void *GRANNY
GetMeshVertices(mesh const &Mesh)
{
    void *Vertices = 0;
    if(Mesh.PrimaryVertexData)
    {
        Vertices = Mesh.PrimaryVertexData->Vertices;
    }

    return(Vertices);
}

void *GRANNY
GetMeshMorphVertices(mesh const &Mesh, int32x MorphTargetIndex)
{
    CheckCountedInt32(MorphTargetIndex, Mesh.MorphTargetCount, return(0));

    morph_target &MorphTarget = Mesh.MorphTargets[MorphTargetIndex];

    void *Vertices = 0;
    if(MorphTarget.VertexData)
    {
        Vertices = MorphTarget.VertexData->Vertices;
    }

    return(Vertices);
}

bool GRANNY
MeshIsRigid(mesh const &Mesh)
{
    return(Mesh.BoneBindingCount <= 1);
}

int32x GRANNY
GetMeshIndexCount(mesh const &Mesh)
{
    int32x IndexCount = 0;

    if(Mesh.PrimaryTopology)
    {
        IndexCount = Mesh.PrimaryTopology->IndexCount;
        if(!IndexCount)
        {
            IndexCount = Mesh.PrimaryTopology->Index16Count;
        }
    }

    return(IndexCount);
}

int32x GRANNY
GetMeshTriangleCount(mesh const &Mesh)
{
    return(GetMeshIndexCount(Mesh) / 3);
}

int32x GRANNY
GetMeshBytesPerIndex(mesh const &Mesh)
{
    int32x BytesPerIndex = 0;
    if(Mesh.PrimaryTopology)
    {
        if(Mesh.PrimaryTopology->IndexCount)
        {
            BytesPerIndex = 4;
        }
        else if(Mesh.PrimaryTopology->Index16Count)
        {
            BytesPerIndex = 2;
        }
    }

    return(BytesPerIndex);
}

void *GRANNY
GetMeshIndices(mesh const &Mesh)
{
    void *Indices = 0;
    if(Mesh.PrimaryTopology)
    {
        if(Mesh.PrimaryTopology->IndexCount)
        {
            Indices = Mesh.PrimaryTopology->Indices;
        }
        else
        {
            Indices = Mesh.PrimaryTopology->Indices16;
        }
    }

    return(Indices);
}

void GRANNY
CopyMeshIndices(mesh const &Mesh, int32x BytesPerIndex, void *DestIndices)
{
    void *Indices = GetMeshIndices(Mesh);
    if(Indices)
    {
        ConvertIndices(GetMeshIndexCount(Mesh),
                       GetMeshBytesPerIndex(Mesh),
                       Indices, BytesPerIndex, DestIndices);
    }
    else
    {
        Log1(ErrorLogMessage, MeshLogMessage,
             "Mesh \"%s\" has no indices.", Mesh.Name);
    }
}

void GRANNY
TransformBoundingBox(real32 const *Affine3, real32 const *Linear3x3,
                     real32 *OBBMin, real32 *OBBMax)
{
    triple NewMin, NewMax;
    NewMin[0] = NewMin[1] = NewMin[2] = Real32Maximum;
    NewMax[0] = NewMax[1] = NewMax[2] = -Real32Maximum;

    {for(int32x Z = 0; Z < 2; ++Z)
    {
        {for(int32x Y = 0; Y < 2; ++Y)
        {
            {for(int32x X = 0; X < 2; ++X)
            {
                triple Point = {(X == 0) ? OBBMin[0] : OBBMax[0],
                                (Y == 0) ? OBBMin[1] : OBBMax[1],
                                (Z == 0) ? OBBMin[2] : OBBMax[2]};

                VectorTransform3(Point, Linear3x3);
                VectorAdd3(Point, Affine3);

                {for(int32x Element = 0;
                     Element < 3;
                     ++Element)
                {
                    NewMin[Element] = Minimum(NewMin[Element],
                                              Point[Element]);
                    NewMax[Element] = Maximum(NewMax[Element],
                                              Point[Element]);
                }}
            }}
        }}
    }}

    VectorEquals3(OBBMin, NewMin);
    VectorEquals3(OBBMax, NewMax);
}
