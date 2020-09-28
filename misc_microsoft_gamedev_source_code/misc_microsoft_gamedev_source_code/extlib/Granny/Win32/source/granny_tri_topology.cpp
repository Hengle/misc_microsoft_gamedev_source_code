// ========================================================================
// $File: //jeffr/granny/rt/granny_tri_topology.cpp $
// $DateTime: 2007/07/23 15:42:28 $
// $Change: 15572 $
// $Revision: #11 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TRI_TOPOLOGY_H)
#include "granny_tri_topology.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode MeshLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition TriMaterialGroupType[] =
{
    {Int32Member, "MaterialIndex"},
    {Int32Member, "TriFirst"},
    {Int32Member, "TriCount"},

    {EndMember},
};

data_type_definition TriAnnotationSetType[] =
{
    {StringMember, "Name"},

    {ReferenceToVariantArrayMember, "TriAnnotations"},
    {Int32Member, "IndicesMapFromTriToAnnotation"},
    {ReferenceToArrayMember, "TriAnnotationIndices", Int32Type},

    {EndMember},
};

data_type_definition TriTopologyType[] =
{
    {ReferenceToArrayMember, "Groups", TriMaterialGroupType},
    {ReferenceToArrayMember, "Indices", Int32Type},
    {ReferenceToArrayMember, "Indices16", Int16Type},
    {ReferenceToArrayMember, "VertexToVertexMap", Int32Type},
    {ReferenceToArrayMember, "VertexToTriangleMap", Int32Type},
    {ReferenceToArrayMember, "SideToNeighborMap", Int32Type},

    {ReferenceToArrayMember, "BonesForTriangle", Int32Type},
    {ReferenceToArrayMember, "TriangleToBoneIndices", Int32Type},

    {ReferenceToArrayMember, "TriAnnotationSets", TriAnnotationSetType},

    {EndMember},
};

END_GRANNY_NAMESPACE;

static void
InvertNeighbor(uint32 &Neighbor)
{
    uint32 Index = Neighbor & 3;
    // Although we swap vertices 0 and 2, this causes us to have to swap _edges_ 0 and 1.
    // Check it out on a bit of paper if you don't believe me!
    if(Index == 1)
    {
        Index = 0;
    }
    else if(Index == 0)
    {
        Index = 1;
    }

    Neighbor = (Neighbor & ~3) | (Index);
}

void GRANNY
InvertTriTopologyWinding(tri_topology &Topology)
{
    {
        int32x IndexCount = Topology.IndexCount;
        int32 *Index = Topology.Indices;
        while(IndexCount >= 3)
        {
            uint32 const Temp = *Index;
            *Index = *(Index + 2);
            *(Index + 2) = Temp;

            Index += 3;
            IndexCount -= 3;
        }
    }

    {
        int32x IndexCount = Topology.Index16Count;
        uint16 *Index = Topology.Indices16;
        while(IndexCount >= 3)
        {
            uint16 const Temp = *Index;
            *Index = *(Index + 2);
            *(Index + 2) = Temp;

            Index += 3;
            IndexCount -= 3;
        }
    }

    {
        int32x NeighborCount = Topology.SideToNeighborCount;
        uint32 *Neighbor = Topology.SideToNeighborMap;
        while(NeighborCount >= 3)
        {
        // Although we swap vertices 0 and 2, this causes us to have to swap _edges_ 0 and 1.
        // Check it out on a bit of paper if you don't believe me!
            uint32 const Temp = *Neighbor;
            *Neighbor = *(Neighbor + 1);
            *(Neighbor + 1) = Temp;

            InvertNeighbor(*Neighbor++);
            InvertNeighbor(*Neighbor++);
            InvertNeighbor(*Neighbor++);

            NeighborCount -= 3;
        }
    }
}

void GRANNY
RemapTopologyMaterials(tri_topology &Topology,
                       int32x RemapCount, int32x *RemapTable)
{
    int32x GroupCount = Topology.GroupCount;
    tri_material_group *Group = Topology.Groups;
    while(GroupCount--)
    {
        if(Group->MaterialIndex < RemapCount)
        {
            Group->MaterialIndex = RemapTable[Group->MaterialIndex];
        }
        else
        {
            Log1(WarningLogMessage, MeshLogMessage,
                 "tri_topology material group had out-of-range "
                 "index %d during remapping", Group->MaterialIndex);
        }

        ++Group;
    }
}

void GRANNY
ConvertIndices(int32x IndexCount,
               int32x FromBytesPerIndex, void const *FromIndices,
               int32x ToBytesPerIndex, void *ToIndices)
{
#define COPY_INDICES(from_type, to_type)                    \
    from_type const *From = (from_type const *)FromIndices; \
    to_type *To = (to_type *)ToIndices;                     \
    while(IndexCount--)                                     \
    {                                                       \
        *To++ = (to_type)*From++;                           \
    }

#define COPY_INDICES_DIRECT                                         \
    Assert(FromBytesPerIndex == ToBytesPerIndex);                   \
    Copy(IndexCount * FromBytesPerIndex, FromIndices, ToIndices);

    switch(FromBytesPerIndex)
    {
        case 1:
        {
            switch(ToBytesPerIndex)
            {
                case 1:
                {
                    COPY_INDICES_DIRECT;
                } break;

                case 2:
                {
                    COPY_INDICES(uint8, uint16);
                } break;

                case 4:
                {
                    COPY_INDICES(uint8, uint32);
                } break;

                default:
                {
                    Log1(ErrorLogMessage, MeshLogMessage,
                         "Illegal value for ToBytesPerIndex (%d)",
                         ToBytesPerIndex);
                } break;
            }
        } break;

        case 2:
        {
            switch(ToBytesPerIndex)
            {
                case 1:
                {
                    COPY_INDICES(uint16, uint8);
                } break;

                case 2:
                {
                    COPY_INDICES_DIRECT;
                } break;

                case 4:
                {
                    COPY_INDICES(uint16, uint32);
                } break;

                default:
                {
                    Log1(ErrorLogMessage, MeshLogMessage,
                         "Illegal value for ToBytesPerIndex (%d)",
                         ToBytesPerIndex);
                } break;
            }
        } break;

        case 4:
        {
            switch(ToBytesPerIndex)
            {
                case 1:
                {
                    COPY_INDICES(uint32, uint8);
                } break;

                case 2:
                {
                    COPY_INDICES(uint32, uint16);
                } break;

                case 4:
                {
                    COPY_INDICES_DIRECT;
                } break;

                default:
                {
                    Log1(ErrorLogMessage, MeshLogMessage,
                         "Illegal value for ToBytesPerIndex (%d)",
                         ToBytesPerIndex);
                } break;
            }
        } break;

        default:
        {
            Log1(ErrorLogMessage, MeshLogMessage,
                 "Illegal value for FromBytesPerIndex (%d)",
                 FromBytesPerIndex);
        } break;
    }
}
