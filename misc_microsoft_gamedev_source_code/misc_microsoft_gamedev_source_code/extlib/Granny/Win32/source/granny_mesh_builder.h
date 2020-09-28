#if !defined(GRANNY_MESH_BUILDER_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_mesh_builder.h $
// $DateTime: 2006/10/19 16:35:09 $
// $Change: 13632 $
// $Revision: #13 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif

#if !defined(GRANNY_VERTEX_DATA_H)
#include "granny_vertex_data.h"
#endif

#if !defined(GRANNY_STACK_ALLOCATOR_H)
#include "granny_stack_allocator.h"
#endif


BEGIN_GRANNY_NAMESPACE EXPGROUP(MeshBuilderGroup);

struct vertex_data;
struct tri_topology;
struct vertex_layout;
struct data_type_definition;
struct buffered_vertex_prefix;
struct builder_edge;

EXPTYPE struct mesh_builder;
struct mesh_builder
{
    // Tolerances
    real32 NormalTolerance;
    real32 TangentTolerance;
    real32 BinormalTolerance;
    real32 TangentBinormalCrossTolerance;
    real32 TangentMergingMinCosine;
    real32 ChannelTolerance[MaximumChannelCount];

    // Vertices
    data_type_definition const *VertexType;
    int32x VertexSize;
    int32x ChannelCount;
    int32x MaximumVertexCount;

    int32x VertexCount;
    stack_allocator VertexStack;                       // Per vertex

    stack_allocator NextCoincidentVertStack;           // Per vertex
    stack_allocator FromTriangleStack;                 // Per vertex

    triple Point;

    int32x BoneCount;
    real32 *BoneWeights;

    int32x WeightCount;
    vertex_weight_arrays WeightArrays;

    // Triangles
    stack_allocator VertexIndexStack;                  // 3 per triangle
    stack_allocator NextInMaterialStack;               // Per triangle

    int32x MaterialCount;
    int32x UsedMaterialCount;
    int32x *FirstMaterialTriangle;

    stack_allocator EdgeStack;
    int32x *EdgeHashHeads;

    int32x NextUnusedTriangle;

    int32x MaterialIndex;
    int32x VertexIndex[3];

    data_type_definition *BufferedVertexType;
    data_type_definition *ChannelTypes;
    int32x BufferedPrefixSize;
    int32x BufferedChannelSize;
    int32x BufferedVertexSize;
    buffered_vertex_prefix *BufferedVertex[3];
    buffered_vertex_prefix *ComparisonVertex;

    // This is not BufferedVertexType, it is just VertexType
    void *TruncatedVertex;

    // Scratch space for the tangent-space generator, if employed
    stack_allocator TriangleTangentStack;              // Per triangle
    stack_allocator VertexTangentStack;                // Per vertex

    // The tool's names for the vertex components.
    // Copied straight into the generated vertex_data
    int32 VertexComponentNameCount;
    char **VertexComponentNames;
};


// Constructing meshes is made simple by the mesh_builder.  You call
// BeginMesh() with a set of parameters predicting the size and layout
// of the mesh.  The mesh_builder will allocate all the required store
// at this time (possibly overallocating in areas where it cannot
// accurately forecast the size).  You can then submit data to the
// mesh builder, and when you are done, EndMesh() will finish
// constructing all mesh attributes, free the mesh_builder, and return
// a properly constructed mesh.
EXPAPI GS_SAFE mesh_builder *BeginMesh(int32x VertexCount, int32x TriangleCount,
                                       int32x MaterialCount, int32x BoneCount,
                                       data_type_definition const *VertexType);
EXPAPI GS_PARAM void GenerateTangentSpaceFromUVs(mesh_builder &Builder);
EXPAPI GS_PARAM bool EndMesh(mesh_builder *Builder,
                             vertex_data *&VertexData,
                             tri_topology *&Topology);

// During construction, you can get the vertex count and maps between
// vertices and triangles.  These only exist between BeginMesh() and
// EndMesh(), so you must copy them if you want to keep them.
EXPAPI GS_READ int32x GetResultingVertexCount(mesh_builder const &Builder);

EXPAPI GS_READ void SerializeResultingCoincidentVertexMap(mesh_builder const &Builder, int32 *Dest);
EXPAPI GS_READ void SerializeResultingVertexToTriangleMap(mesh_builder const &Builder, int32 *Dest);
EXPAPI GS_READ void SerializeResultingVertices(mesh_builder const &Builder, void *Destination);

// If you would like to allocate all the memory for the resulting
// mesh, you can use this alternative EndMesh function.
EXPAPI GS_READ int32x GetResultingVertexDataSize(mesh_builder const &Builder);
EXPAPI GS_READ int32x GetResultingTopologySize(mesh_builder const &Builder);
EXPAPI GS_PARAM void EndMeshInPlace(mesh_builder *Builder,
                                    void *VertexDataMemory,
                                    vertex_data *&VertexData,
                                    void *TopologyMemory,
                                    tri_topology *&Topology);

// EndMeshInPlace, split into two phases, the "make me a mesh then" and "free up the builder" phase.
// The point being, you may want to call GetResultingCoincidentVertexMap, etc. after having built the mesh.
void EndProcessingMeshInPlace (mesh_builder *Builder,
                           void *VertexDataMemory,
                           vertex_data *&VertexData,
                           void *TopologyMemory,
                           tri_topology *&Topology);
void ReallyEndMesh(mesh_builder *Builder);


// While submitting vertex attributes, the mesh_builder will attempt
// to determine whether common attributes can be coallesced.  To do so
// it uses parameterizable tolerances for each attribute.  These
// tolerances act as thresholds on the distance between vectors (a
// Euclidean two-norm).
EXPAPI GS_PARAM void SetNormalTolerance(mesh_builder &Builder, real32 Tolerance);
EXPAPI GS_PARAM void SetTangentTolerance(mesh_builder &Builder, real32 Tolerance);
EXPAPI GS_PARAM void SetBinormalTolerance(mesh_builder &Builder, real32 Tolerance);
EXPAPI GS_PARAM void SetTangentBinormalCrossTolerance(mesh_builder &Builder,
                                                      real32 Tolerance);
EXPAPI GS_PARAM void SetTangentMergingTolerance(mesh_builder &Builder, real32 MinCosine);
EXPAPI GS_PARAM void SetChannelTolerance(mesh_builder &Builder, int32x Channel,
                                         real32 Tolerance);

// Note that ChannelCount should match GetVertexComponentCount(VertexType), as should the size of the ChannelNames array.
EXPAPI GS_PARAM void SetVertexChannelComponentNames(mesh_builder &Builder,
                                                    int32x ChannelCount,
                                                    char const **ChannelNames);

// Vertices must be submitted in order.  You set data for the vertex
// via the Point() and AddWeight() calls, and then finish the vertex
// by calling PushVertex().
EXPAPI GS_PARAM void SetPosition(mesh_builder &Builder,
                                 real32 Px, real32 Py, real32 Pz);
EXPAPI GS_PARAM void AddWeight(mesh_builder &Builder, int32x BoneIndex, real32 Weight);
EXPAPI GS_PARAM void PushVertex(mesh_builder &Builder);

// Triangles must be submitted in order.  You set data for the triangle
// via the Normal, Channel, and VertexIndex calls, and then finish the
// triangle by calling PushTriangle().  Note that any data that is not
// specified in the mesh layout will be ignored if submitted here.
EXPAPI GS_PARAM void SetVertexIndex(mesh_builder &Builder, int32x Edge,
                                    int32x Index);
EXPAPI GS_PARAM void SetNormal(mesh_builder &Builder, int32x Edge,
                               real32 Nx, real32 Ny, real32 Nz);
EXPAPI GS_PARAM void SetTangent(mesh_builder &Builder, int32x Edge,
                                real32 Tx, real32 Ty, real32 Tz);
EXPAPI GS_PARAM void SetBinormal(mesh_builder &Builder, int32x Edge,
                                 real32 Bx, real32 By, real32 Bz);
EXPAPI GS_PARAM void SetTangentBinormalCross(mesh_builder &Builder, int32x Edge,
                                             real32 TBCx, real32 TBCy, real32 TBCz);
EXPAPI GS_PARAM int32x GetChannelComponentCount(mesh_builder &Builder, int32x ChannelIndex);
EXPAPI GS_PARAM void SetChannel(mesh_builder &Builder, int32x Edge, int32x Channel,
                                real32 const *Value);
EXPAPI GS_PARAM void SetMaterial(mesh_builder &Builder, int32x MaterialIndex);
EXPAPI GS_PARAM void PushTriangle(mesh_builder &Builder);

EXPAPI GS_READ int32x GetOriginalVertex(mesh_builder &Builder, int32x V);


// Having submitted vertices, but before finishing the builder, we want to do some
// queries on the resulting vertex data.
void *GetVertexData(mesh_builder &Builder, int32x VertexNumber);
int32 GetNextCoincidentVertex(mesh_builder &Builder, int32x VertexIndex);
int32 GetFromTriangleForVertex(mesh_builder &Builder, int32x VertexIndex);

// And yeah, we should know this because we _told_ it in BeginMesh,
// but this makes life easy for the exporters.
data_type_definition const *GetVertexTypeDefinition(mesh_builder &Builder);


END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_MESH_BUILDER_H
#endif
