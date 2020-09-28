// ========================================================================
// $File: //jeffr/granny/rt/granny_mesh_builder.cpp $
// $DateTime: 2007/07/23 15:42:28 $
// $Change: 15572 $
// $Revision: #26 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_MESH_BUILDER_H)
#include "granny_mesh_builder.h"
#endif

#if !defined(GRANNY_TRI_TOPOLOGY_H)
#include "granny_tri_topology.h"
#endif

#if !defined(GRANNY_DATA_TYPE_CONVERSION_H)
#include "granny_data_type_conversion.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_TANGENT_FRAME_H)
#include "granny_tangent_frame.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode VertexLayoutLogMessage

USING_GRANNY_NAMESPACE;
BEGIN_GRANNY_NAMESPACE;

struct buffered_vertex_prefix
{
    real32 Normal[3];
    real32 Tangent[3];
    real32 Binormal[3];
    real32 TangentBinormalCross[3];
};
data_type_definition BufferedVertexPrefixType[] =
{
    {Real32Member, VertexNormalName, 0, 3},
    {Real32Member, VertexTangentName, 0, 3},
    {Real32Member, VertexBinormalName, 0, 3},
    {Real32Member, VertexTangentBinormalCrossName, 0, 3},

    {EndMember},
};

int32 const Terminator = -1;

struct builder_edge
{
    int32x V0, V1;
    uint32 FromTriangleAndEdge;
    int32x HashNext;
    int32x NextCoincident;
};

// 256k hash entries = 1 meg of hash table space.
int32x const EdgeHashTableSize = 256 << 10;

END_GRANNY_NAMESPACE;

struct buffered_vertex_channel
{
    real32 Channel[MaximumChannelWidth];
};
static data_type_definition BufferedVertexChannelType[] =
{
    {Real32Member, "Channel", 0, MaximumChannelWidth},
    {EndMember},
};

static void
InitializeTriangleBuffer(mesh_builder &Builder)
{
    {for(int32x Edge = 0;
         Edge < 3;
         ++Edge)
    {
        Builder.MaterialIndex = 0;
        Builder.VertexIndex[Edge] = 0;
        MakeEmptyDataTypeObject(Builder.BufferedVertexType,
                                Builder.BufferedVertex[Edge]);
    }}
}

static void
InitializeVertexBuffer(mesh_builder &Builder)
{
    VectorSet3(Builder.Point, 0, 0, 0);
    SetReal32(Builder.BoneCount, 0, Builder.BoneWeights);
}

static void
InitializeBufferedVertexType(mesh_builder &Builder)
{
    data_type_definition const *Source = BufferedVertexPrefixType;
    data_type_definition *Dest = Builder.BufferedVertexType;
    while(Source->Type != EndMember)
    {
        *Dest++ = *Source++;
    }

    Builder.ChannelTypes = Dest;

    Source = Builder.VertexType;
    while(Source->Type != EndMember)
    {
        if(!IsSpatialVertexMember(Source->Name))
        {
            *Dest = *Source;
            Dest->Type = Real32Member;
            Dest->ArrayWidth = MaximumChannelWidth;
            ++Dest;
        }

        Source++;
    }

    Dest->Type = EndMember;
}


// 0 = one big alloc.
// 1 = lots of little allocs

// The problem this solves is that if it's a really big mesh,
// the single block can grow to >.5Gb. Which would be just about fine,
// except in a very fragmented system with not much memory left
// (which Max and Maya are), you can simply fail to find a
// contiguous block that size. So if we break it up a bit
// into lots of discrete allocations, it will hopefully be easier
// to fit them all in. Of course, breaking it up into lots of
// smaller blocks increases fragmentation itself! C'est la vie...
#define BEGIN_MESH_USES_LOTS_OF_ALLOCS 1


// Logging info for Ensemble.
#define LOG_ALLOCATION_SIZES 0


void DeallocateSafeBuilder ( mesh_builder *Builder );


mesh_builder *GRANNY
BeginMesh(int32x VertexCount, int32x TriangleCount, int32x MaterialCount,
          int32x BoneCount, data_type_definition const *VertexType)
{
    Assert(MaterialCount > 0);

    int32x const WeightCount = GetVertexBoneCount(VertexType);
    // A channel is a colour or texture coordinate (or something like that).
    // Position, normal, tangent and weights are not channels.
    int32x const ChannelCount = GetVertexChannelCount(VertexType);

    // With double-sided stuff, the tri count can potentially double.
    int32x MaximumTriangleCount = TriangleCount * 2;

    int32x const MaximumEdgeCount = MaximumTriangleCount * 3;

    // Except that when computing tangent spaces,
    // It can start with a bunch of pre-replicated vertices,
    // but then create a whole bunch _more_.
    // So my suspicion is that the max is actually
    // EdgeCount + VertexCount*2
    // But I'm just going to play it safe and double the lot.
    //int32x MaximumVertexCount = EdgeCount + VertexCount;
    // (although remember VertexCount*2 for double-sided possibilities).
    int32x MaximumVertexCount = ( MaximumEdgeCount + VertexCount * 2 ) * 2;

    int32x const VertexSize = GetTotalObjectSize(VertexType);

    // Sorry for the hackery...
    // We need to use lots of smaller allocs because the big one can
    // easily fail when Max/Maya has fragmented memory.
#if BEGIN_MESH_USES_LOTS_OF_ALLOCS

    bool FailedToAllocate = false;

#if LOG_ALLOCATION_SIZES

#define MySpecialAggrAllocOffsetSizePtr(Allocator, OwnerPointer,Size,Member)                        \
    {                                                                                               \
        void *TempPointer = AllocateSize ( Size );                                                  \
        *((void**)&(OwnerPointer->Member)) = TempPointer;                                           \
        Log2(NoteLogMessage,ExporterLogMessage,"Allocate %i bytes for " #Member " got address 0x%x", Size, TempPointer ); \
        FailedToAllocate |= ( TempPointer == NULL );                                                \
    } typedef int __RequireSemiColon


#define MySpecialAggrAllocOffsetArrayPtr(Allocator, OwnerPointer,Count,CountMember,PtrMember)       \
    {                                                                                               \
        void *TempPointer = AllocateSize ( Count * sizeof(OwnerPointer->PtrMember[0]) );            \
        *((void**)&(OwnerPointer->PtrMember)) = TempPointer;                                        \
        OwnerPointer->CountMember = Count;                                                          \
        Log2(NoteLogMessage,ExporterLogMessage,"Allocate %i bytes for " #PtrMember " got address 0x%x", Count * sizeof(OwnerPointer->PtrMember[0]), TempPointer ); \
        FailedToAllocate |= ( TempPointer == NULL );                                                \
    } typedef int __RequireSemiColon

#else

#define MySpecialAggrAllocOffsetSizePtr(Allocator, OwnerPointer,Size,Member)    \
    {                                                                           \
        void *TempPointer = AllocateSize ( Size );                              \
        *((void**)&(OwnerPointer->Member)) = TempPointer;                       \
        FailedToAllocate |= ( TempPointer == NULL );                            \
    } typedef int __RequireSemiColon


#define MySpecialAggrAllocOffsetArrayPtr(Allocator, OwnerPointer,Count,CountMember,PtrMember)   \
    {                                                                                           \
        void *TempPointer = AllocateSize ( Count * sizeof(OwnerPointer->PtrMember[0]) );        \
        *((void**)&(OwnerPointer->PtrMember)) = TempPointer;                                    \
        OwnerPointer->CountMember = Count;                                                      \
        FailedToAllocate |= ( TempPointer == NULL );                                            \
    } typedef int __RequireSemiColon

#define MySpecialAggrAllocOffsetCountlessArrayPtr(Allocator, OwnerPointer,Count, PtrMember) \
    {                                                                                       \
        void *TempPointer = AllocateSize ( Count * sizeof(OwnerPointer->PtrMember[0]) );    \
        *((void**)&(OwnerPointer->PtrMember)) = TempPointer;                                \
        FailedToAllocate |= ( TempPointer == NULL );                                        \
    } typedef int __RequireSemiColon

#endif

    // We'll be doing lots of small allocs, so terminate the aggr
    // right away.  The defines above morph the calls into
    // straightforward AllocateSize calls
    mesh_builder *Builder;
    {
        aggr_allocator Allocator;
        InitializeAggrAlloc(Allocator);

        AggrAllocPtr(Allocator, Builder);
        EndAggrAlloc(Allocator);
    }

#else

#define MySpecialAggrAllocOffsetSizePtr AggrAllocOffsetSizePtr
#define MySpecialAggrAllocOffsetArrayPtr AggrAllocOffsetArrayPtr
#define MySpecialAggrAllocOffsetCountlessArrayPtr AggrAllocOffsetCountlessArrayPtr

    // Normal AggrAlloc style.
    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    mesh_builder *Builder;
    AggrAllocPtr(Allocator, Builder);

#endif

    int32x const GeneralBlockSize = 1 << 13;  // 8k

    MySpecialAggrAllocOffsetArrayPtr(Allocator, Builder, BoneCount,BoneCount, BoneWeights);
    MySpecialAggrAllocOffsetArrayPtr(Allocator, Builder, MaterialCount, MaterialCount, FirstMaterialTriangle);
    MySpecialAggrAllocOffsetCountlessArrayPtr(Allocator, Builder, EdgeHashTableSize, EdgeHashHeads);

    int32x BufferedPrefixSize = GetTotalObjectSize(BufferedVertexPrefixType);
    int32x BufferedChannelSize = GetTotalObjectSize(BufferedVertexChannelType);
    int32x BufferedVertexSize = (BufferedPrefixSize +
                                 ChannelCount * BufferedChannelSize);
    MySpecialAggrAllocOffsetSizePtr(Allocator, Builder, BufferedVertexSize, BufferedVertex[0]);
    MySpecialAggrAllocOffsetSizePtr(Allocator, Builder, BufferedVertexSize, BufferedVertex[1]);
    MySpecialAggrAllocOffsetSizePtr(Allocator, Builder, BufferedVertexSize, BufferedVertex[2]);

    MySpecialAggrAllocOffsetSizePtr(Allocator, Builder, VertexSize, TruncatedVertex);
    MySpecialAggrAllocOffsetSizePtr(Allocator, Builder, BufferedVertexSize, ComparisonVertex);

    int32x BufferedVertexTypeSize =
        (GetTotalTypeSize(BufferedVertexPrefixType) +
         ChannelCount*SizeOf(data_type_definition));
    MySpecialAggrAllocOffsetSizePtr(Allocator, Builder,
                                    BufferedVertexTypeSize,
                                    BufferedVertexType);

    int32x ThisVertexComponentCount = GetVertexComponentCount(VertexType);
    MySpecialAggrAllocOffsetArrayPtr ( Allocator, Builder, ThisVertexComponentCount,
                                       VertexComponentNameCount, VertexComponentNames );

#if BEGIN_MESH_USES_LOTS_OF_ALLOCS

    if ( FailedToAllocate )
    {
        // At least one of the allocates failed. Free everything.
        DeallocateSafeBuilder(Builder);
    }

#else

    EndAggrAlloc(Allocator);

#endif

    StackInitializeWithDirectory(Builder->VertexStack, VertexSize, GeneralBlockSize, MaximumVertexCount);
    StackInitializeWithDirectory(Builder->NextCoincidentVertStack, SizeOf(int32), GeneralBlockSize, MaximumVertexCount);
    StackInitializeWithDirectory(Builder->FromTriangleStack, SizeOf(int32), GeneralBlockSize, MaximumVertexCount);
    StackInitializeWithDirectory(Builder->NextInMaterialStack, SizeOf(int32), GeneralBlockSize, MaximumTriangleCount);
    StackInitializeWithDirectory(Builder->VertexIndexStack, SizeOf(int32), GeneralBlockSize, (3*MaximumTriangleCount));
    StackInitializeWithDirectory(Builder->TriangleTangentStack, SizeOf(tangent_frame), GeneralBlockSize, MaximumTriangleCount);
    StackInitializeWithDirectory(Builder->VertexTangentStack,   SizeOf(tangent_frame), GeneralBlockSize, MaximumVertexCount);
    StackInitializeWithDirectory(Builder->EdgeStack, SizeOf(builder_edge), GeneralBlockSize, MaximumEdgeCount);

    if ( Builder )
    {
        Builder->NormalTolerance = 0.001f;
        Builder->TangentTolerance = 0.001f;
        Builder->BinormalTolerance = 0.001f;
        Builder->TangentBinormalCrossTolerance = 0.001f;
        // By default this is 0.25f (75 degrees) because tangent spaces can
        // vary wildly but still need merging. But for some meshes, artists will
        // need to raise this tolerance to stop tangent spaces merging when they shouldn't.
        Builder->TangentMergingMinCosine = 0.25f;
        SetReal32(MaximumChannelCount, 0.001f, Builder->ChannelTolerance);

        Builder->VertexType = VertexType;
        Builder->VertexSize = VertexSize;
        Builder->ChannelCount = ChannelCount;
        Builder->VertexCount = VertexCount;

        Builder->BoneCount = BoneCount;
        Builder->NextUnusedTriangle = 0;
        Builder->UsedMaterialCount = 0;
        SetInt32(MaterialCount, Terminator,
                 Builder->FirstMaterialTriangle);

        // Initialize the next coincident to the terminator for the
        // base vertices.
        int32x Index;
        MultipleNewStackUnits(Builder->NextCoincidentVertStack, VertexCount, Index, &Terminator);
        Assert(Index == 0);
        MultipleNewStackUnits(Builder->FromTriangleStack, VertexCount, Index, &Terminator);
        Assert(Index == 0);

        SetInt32(EdgeHashTableSize, Terminator, Builder->EdgeHashHeads);

        Builder->WeightCount = WeightCount;

        Builder->BufferedPrefixSize = BufferedPrefixSize;
        Builder->BufferedChannelSize = BufferedChannelSize;
        Builder->BufferedVertexSize = BufferedVertexSize;

        InitializeBufferedVertexType(*Builder);
        InitializeVertexBuffer(*Builder);
        InitializeTriangleBuffer(*Builder);

        {for ( int32x i = 0; i < Builder->VertexComponentNameCount; i++ )
        {
            Builder->VertexComponentNames[i] = NULL;
        }}
    }

    return(Builder);
}


void GRANNY
SetVertexChannelComponentNames(mesh_builder &Builder,
                               int32x ChannelCount,
                               char const **ChannelNames)
{
    CheckCondition ( ChannelCount == GetVertexComponentCount(Builder.VertexType), return );
    {for ( int32x i = 0; i < ChannelCount; i++ )
    {
        Builder.VertexComponentNames[i] = (char*)ChannelNames[i];
    }}
}



int32x GRANNY
GetOriginalVertex(mesh_builder &Builder, int32x V)
{
    while(V >= Builder.VertexCount)
    {
        Assert(V != Terminator);
        V = STACK_UNIT_AS(Builder.NextCoincidentVertStack, V, int32);
    }

    return(V);
}

#if 0   // Not actually used
static bool
VerticesAreCoincident(mesh_builder &Builder, int32x V0, int32x V1)
{
    int32x OV0 = GetOriginalVertex(Builder, V0);
    int32x OV1 = GetOriginalVertex(Builder, V1);

    return(OV0 == OV1);
}
#endif

static uint32
GetEdgeHashValue(int32x &V0, int32x &V1)
{
    // Make sure that V0 is always the lesser for comparison purposes.
    if(V0 > V1)
    {
        int32x Temp = V1;
        V1 = V0;
        V0 = Temp;
    }

    // 7499 is prime, which is probably not strictly necessary, but
    // makes me feel better
    return((7499 * V1) + V0);
}

static void
AddEdge(mesh_builder &Builder, uint32 FromTriangleAndEdge,
        int32x V0, int32x V1)
{
    uint32 const HashValue      = GetEdgeHashValue(V0, V1);
    uint32 const HashTableIndex = HashValue % EdgeHashTableSize;

    int32x CoincidentEdgeIndex = Terminator;
    int32x PreviousEdgeIndex   = Terminator;
    int32x EdgeIndex  = Builder.EdgeHashHeads[HashTableIndex];
    while (EdgeIndex != Terminator)
    {

        builder_edge& Edge = STACK_UNIT_AS(Builder.EdgeStack, EdgeIndex, builder_edge);
        Assert(!(Edge.V0 == V0 && Edge.V1 == V1 && Edge.FromTriangleAndEdge == FromTriangleAndEdge));

        // Did we find the coincident chain we were looking for?
        if (CoincidentEdgeIndex == Terminator &&
            (Edge.V0 == V0 && Edge.V1 == V1))
        {
            CoincidentEdgeIndex = EdgeIndex;

            int32x CoincidentWalk = Edge.NextCoincident;
            while (CoincidentWalk != Terminator)
            {
                CoincidentEdgeIndex = CoincidentWalk;

                builder_edge& CoincidentEdge = STACK_UNIT_AS(Builder.EdgeStack,
                                                             CoincidentWalk, builder_edge);
                Assert(CoincidentEdge.V0 == Edge.V0 &&
                       CoincidentEdge.V1 == Edge.V1 &&
                       CoincidentEdge.FromTriangleAndEdge != Edge.FromTriangleAndEdge);
                CoincidentWalk = CoincidentEdge.NextCoincident;
            }
        }

        PreviousEdgeIndex = EdgeIndex;
        EdgeIndex = Edge.HashNext;
    }

    int32x* NextCoincidentIndex = NULL;
    int32x* NextHashIndex = NULL;
    {
        // Need to start a new hash chain?
        if (PreviousEdgeIndex == Terminator)
        {
            NextHashIndex = &Builder.EdgeHashHeads[HashTableIndex];
        }
        else
        {
            builder_edge& PrevEdge = STACK_UNIT_AS(Builder.EdgeStack, PreviousEdgeIndex, builder_edge);
            NextHashIndex = &PrevEdge.HashNext;
        }

        // Link into the coincident chain?
        if (CoincidentEdgeIndex != Terminator)
        {
            builder_edge& PrevEdge = STACK_UNIT_AS(Builder.EdgeStack, CoincidentEdgeIndex, builder_edge);
            NextCoincidentIndex = &PrevEdge.NextCoincident;
        }
    }

    // Allocate the new edge
    int32x NewEdgeIndex;
    if (NewStackUnit(Builder.EdgeStack, NewEdgeIndex) == false)
    {
        // oh, very, very bad.
        Assert(false);
        return;
    }

    builder_edge& NewEdge = STACK_UNIT_AS(Builder.EdgeStack, NewEdgeIndex, builder_edge);
    NewEdge.FromTriangleAndEdge = FromTriangleAndEdge;
    NewEdge.V0 = V0;
    NewEdge.V1 = V1;
    NewEdge.HashNext = Terminator;
    NewEdge.NextCoincident = Terminator;

    *NextHashIndex = NewEdgeIndex;
    if (NextCoincidentIndex != NULL)
        *NextCoincidentIndex = NewEdgeIndex;
}

static uint32
GetNextEdgeAndTriangle(mesh_builder &Builder, uint32 FromTriangleAndEdge,
                       int32x V0, int32x V1)
{
    uint32 const HashValue      = GetEdgeHashValue(V0, V1);
    uint32 const HashTableIndex = HashValue % EdgeHashTableSize;

    int32x EdgeIndex = Builder.EdgeHashHeads[HashTableIndex];
    while (EdgeIndex != Terminator)
    {
        builder_edge& Edge = STACK_UNIT_AS(Builder.EdgeStack, EdgeIndex, builder_edge);

        if(Edge.V0 == V0 && Edge.V1 == V1)
        {
            int32x CoincidentWalk = EdgeIndex;
            while (CoincidentWalk != Terminator)
            {
                builder_edge& CoincidentEdge = STACK_UNIT_AS(Builder.EdgeStack, CoincidentWalk, builder_edge);
                Assert(CoincidentEdge.V0 == V0 && CoincidentEdge.V1 == V1);

                if (CoincidentEdge.FromTriangleAndEdge == FromTriangleAndEdge)
                    break;

                CoincidentWalk = CoincidentEdge.NextCoincident;
            }
            Assert(CoincidentWalk != Terminator);

            builder_edge& ThisEdge = STACK_UNIT_AS(Builder.EdgeStack, CoincidentWalk, builder_edge);
            if (ThisEdge.NextCoincident == Terminator)
            {
                // Back to the start
                builder_edge& NextEdge = STACK_UNIT_AS(Builder.EdgeStack, EdgeIndex, builder_edge);
                return NextEdge.FromTriangleAndEdge;
            }
            else
            {
                // Back to the start
                builder_edge& NextEdge = STACK_UNIT_AS(Builder.EdgeStack, ThisEdge.NextCoincident, builder_edge);
                return NextEdge.FromTriangleAndEdge;
            }
        }

        EdgeIndex = Edge.HashNext;
    }

    return uint32(Terminator);
}

static void
BuildTopology(mesh_builder &Builder, int32x TriangleCount,
              int32 const *Vertices, uint32 *EdgeDataOut)
{
    int32 const *V = Vertices;
    {for(int32x TriangleIndex = 0;
         TriangleIndex < TriangleCount;
         ++TriangleIndex)
    {
        int32x const TV0 = GetOriginalVertex(Builder, *V++);
        int32x const TV1 = GetOriginalVertex(Builder, *V++);
        int32x const TV2 = GetOriginalVertex(Builder, *V++);

        uint32 Tri = TriangleIndex << 2;
        AddEdge(Builder, Tri | 0, TV0, TV1);
        AddEdge(Builder, Tri | 1, TV1, TV2);
        AddEdge(Builder, Tri | 2, TV2, TV0);
    }}

    V = Vertices;
    {for(int32x TriangleIndex = 0;
         TriangleIndex < TriangleCount;
         ++TriangleIndex)
    {
        int32x const TV0 = GetOriginalVertex(Builder, *V++);
        int32x const TV1 = GetOriginalVertex(Builder, *V++);
        int32x const TV2 = GetOriginalVertex(Builder, *V++);

        uint32 Tri = TriangleIndex << 2;
        *EdgeDataOut++ = GetNextEdgeAndTriangle(Builder, Tri | 0, TV0, TV1);
        *EdgeDataOut++ = GetNextEdgeAndTriangle(Builder, Tri | 1, TV1, TV2);
        *EdgeDataOut++ = GetNextEdgeAndTriangle(Builder, Tri | 2, TV2, TV0);
    }}
}

static int32x
BuildIndices(mesh_builder &Builder, int32x FirstTriangle,
             int32 *IndexDataOut)
{
    int32x Count = 0;

    int32x TriangleIndex = FirstTriangle;
    while(TriangleIndex != Terminator)
    {
        *IndexDataOut++ = STACK_UNIT_AS(Builder.VertexIndexStack, 3*TriangleIndex + 0, int32);
        *IndexDataOut++ = STACK_UNIT_AS(Builder.VertexIndexStack, 3*TriangleIndex + 1, int32);
        *IndexDataOut++ = STACK_UNIT_AS(Builder.VertexIndexStack, 3*TriangleIndex + 2, int32);

        TriangleIndex = STACK_UNIT_AS(Builder.NextInMaterialStack, TriangleIndex, int32);
        ++Count;
    }

    return(Count);
}

static void
AggrVertexData(aggr_allocator &Allocator,
               mesh_builder const &Builder,
               vertex_data *&VertexData,
               char **VertexComponentNameStringSpace)
{
    int32x UsedVertexCount = GetStackUnitCount(Builder.VertexStack);

    AggrAllocPtr(Allocator, VertexData);
    AggrAllocOffsetSizePtr(Allocator, VertexData,
                           GetTotalTypeSize(Builder.VertexType),
                           VertexType);
    AggrAllocOffsetArraySizePtr(Allocator, VertexData,
                                UsedVertexCount, Builder.VertexSize,
                                VertexCount, Vertices);

    // Allocate space for the name string array pointers.
    AggrAllocOffsetArrayPtr(Allocator, VertexData, Builder.VertexComponentNameCount,
                            VertexComponentNameCount, VertexComponentNames );

    // And then the space for the strings themselves. Need to be a little careful with these.
    // The caller will then need to put the string data into here, and point the bits of the VertexComponentNames array at it.
    int32x TotalSizeNeeded = 0;
    {for ( int32x ComponentNum = 0; ComponentNum < Builder.VertexComponentNameCount; ComponentNum++ )
    {
        if ( Builder.VertexComponentNames[ComponentNum] == NULL )
        {
            TotalSizeNeeded += 1;
        }
        else
        {
            TotalSizeNeeded += 1 + StringLength ( Builder.VertexComponentNames[ComponentNum] );
        }
    }}
    Assert ( VertexComponentNameStringSpace != NULL );
    *VertexComponentNameStringSpace = NULL;
    AggrAllocSizePtr ( Allocator, TotalSizeNeeded, *VertexComponentNameStringSpace );
}

static void
AggrTopology(aggr_allocator &Allocator,
             mesh_builder const &Builder,
             tri_topology *&Topology)
{
    int32x UsedVertexCount   = GetStackUnitCount(Builder.VertexStack);
    int32x UsedMaterialCount = Builder.UsedMaterialCount;
    int32x UsedTriangleCount = Builder.NextUnusedTriangle;

    AggrAllocPtr(Allocator, Topology);
    AggrAllocOffsetArrayPtr(Allocator, Topology,
                            UsedMaterialCount,
                            GroupCount, Groups);
    AggrAllocOffsetArrayPtr(Allocator, Topology, UsedTriangleCount*3,
                            IndexCount, Indices);
    AggrAllocOffsetArrayPtr(Allocator, Topology, 0,
                            Index16Count, Indices16);
    AggrAllocOffsetArrayPtr(Allocator, Topology, UsedVertexCount,
                            VertexToVertexCount, VertexToVertexMap);
    AggrAllocOffsetArrayPtr(Allocator, Topology, UsedVertexCount,
                            VertexToTriangleCount, VertexToTriangleMap);
    AggrAllocOffsetArrayPtr(Allocator, Topology, UsedTriangleCount*3,
                            SideToNeighborCount, SideToNeighborMap);
}

void GRANNY
GenerateTangentSpaceFromUVs(mesh_builder &Builder)
{
    int32x TriangleCount = Builder.NextUnusedTriangle;

    // We must zero the tangent data currently inside the
    // vertices, because BuildTangentSpace checks for vertex equality,
    // and if there's bogus data already in there, it will confuse it.
    tangent_frame ZeroTangentFrame;
    VectorZero3 ( ZeroTangentFrame.U );
    VectorZero3 ( ZeroTangentFrame.V );
    VectorZero3 ( ZeroTangentFrame.N );


    int32x const VertexCountBefore = GetStackUnitCount(Builder.VertexStack);
    {for(int32x VertexIndex = 0;
         VertexIndex < VertexCountBefore;
         ++VertexIndex)
    {
        MergeSingleObject(GBX333VertexType,
                          &ZeroTangentFrame,
                          Builder.VertexType,
                          GetStackUnit(Builder.VertexStack, VertexIndex));
    }}


    if (BuildTangentSpace(Builder) == false)
    {
        Log0(ErrorLogMessage, ExporterLogMessage, "Failed to create the tangent space" );

        // Initialize all the tangent frames to a default...
        tangent_frame DefaultTangentFrame = {
            { 1, 0, 0 },
            { 0, 1, 0 },
            { 0, 0, 1 }
        };

        int32x const VertexCountAfter = GetStackUnitCount(Builder.VertexStack);
        {for(int32x VertexIndex = 0;
             VertexIndex < VertexCountAfter;
             ++VertexIndex)
        {
            MergeSingleObject(GBX333VertexType, &DefaultTangentFrame,
                              Builder.VertexType,
                              GetStackUnit(Builder.VertexStack, VertexIndex));
        }}
        return;
    }


    int32x const VertexCountAfter = GetStackUnitCount(Builder.VertexStack);
    {for(int32x VertexIndex = 0;
         VertexIndex < VertexCountAfter;
         ++VertexIndex)
    {
        MergeSingleObject(GBX333VertexType,
                          GetStackUnit(Builder.VertexTangentStack, VertexIndex),
                          Builder.VertexType,
                          GetStackUnit(Builder.VertexStack, VertexIndex));
    }}

#if DEBUG
    // Routine paranoia.
    {for ( int32x VertNum = 0; VertNum < VertexCountAfter; VertNum++ )
    {
        int32x ProvokingTri = STACK_UNIT_AS(Builder.FromTriangleStack, VertNum, int32);
        Assert ( ProvokingTri >= -1 );
        if ( ProvokingTri >= 0 )
        {
            Assert ( ProvokingTri < TriangleCount );
            Assert ( ( STACK_UNIT_AS(Builder.VertexIndexStack, ProvokingTri*3+0, int32) == VertNum ) ||
                     ( STACK_UNIT_AS(Builder.VertexIndexStack, ProvokingTri*3+1, int32) == VertNum ) ||
                     ( STACK_UNIT_AS(Builder.VertexIndexStack, ProvokingTri*3+2, int32) == VertNum ) );
        }
        else
        {
            // Provoking tri of -1 means this is an "orphan" vertex, where there's
            // no triangle that actually uses it.
            // Which is not a great thing, but not insanely bad.
            {for ( int32x i = 0; i < TriangleCount * 3; i++ )
            {
                Assert ( STACK_UNIT_AS(Builder.VertexIndexStack, i, int32) != VertNum );
            }}
        }
    }}
#endif
}

bool GRANNY
EndMesh(mesh_builder *Builder, vertex_data *&VertexData,
        tri_topology *&Topology)
{
    VertexData = 0;
    Topology = 0;

    if(Builder)
    {
        int32x const VertexDataSize = GetResultingVertexDataSize(*Builder);
        int32x const TopologySize   = GetResultingTopologySize(*Builder);

        aggr_allocator Allocator;
        InitializeAggrAlloc(Allocator);

        void *VertexDataMemory = 0;
        void *TopologyMemory = 0;
        AggrAllocSizePtr(Allocator, VertexDataSize, VertexDataMemory);
        AggrAllocSizePtr(Allocator, TopologySize, TopologyMemory);
        EndAggrAlloc(Allocator);

        // Note that even if the allocation fails, we still need to
        // call this with the 0 pointers, because the Builder must be
        // cleaned up!
        EndMeshInPlace(Builder,
                       VertexDataMemory, VertexData,
                       TopologyMemory, Topology);
    }

    return(VertexData && Topology);
}

int32x GRANNY
GetResultingVertexDataSize(mesh_builder const &Builder)
{
    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    vertex_data *VertexData;
    char *Dummy = NULL;
    AggrVertexData(Allocator, Builder, VertexData, &Dummy);

    int32x ResultingSize;
    CheckConvertToInt32(ResultingSize, EndAggrSize(Allocator), return 0);
    return ResultingSize;
}

int32x GRANNY
GetResultingTopologySize(mesh_builder const &Builder)
{
    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    tri_topology *Topology;
    AggrTopology(Allocator, Builder, Topology);

    int32x ResultingSize;
    CheckConvertToInt32(ResultingSize, EndAggrSize(Allocator), return 0);
    return ResultingSize;
}

int32x GRANNY
GetResultingVertexCount(mesh_builder const &Builder)
{
    return GetStackUnitCount(Builder.VertexStack);
}

void GRANNY
SerializeResultingCoincidentVertexMap(mesh_builder const &Builder, int32* Dest)
{
    SerializeStack(Builder.NextCoincidentVertStack, Dest);
}

void GRANNY
SerializeResultingVertexToTriangleMap(mesh_builder const &Builder, int32* Dest)
{
    SerializeStack(Builder.FromTriangleStack, Dest);
}

void GRANNY
SerializeResultingVertices(mesh_builder const &Builder, void* Dest)
{
    SerializeStack(Builder.VertexStack, Dest);
}



void DeallocateSafeBuilder ( mesh_builder *Builder )
{
    if ( Builder != NULL )
    {
        StackCleanUp ( Builder->VertexStack );
        StackCleanUp ( Builder->NextCoincidentVertStack );
        StackCleanUp ( Builder->FromTriangleStack );
        StackCleanUp ( Builder->NextInMaterialStack );
        StackCleanUp ( Builder->VertexIndexStack );
        StackCleanUp ( Builder->TriangleTangentStack );
        StackCleanUp ( Builder->VertexTangentStack );
        StackCleanUp ( Builder->EdgeStack );

        // Deallocate the memory we used to build the mesh
    #if BEGIN_MESH_USES_LOTS_OF_ALLOCS
        DeallocateSafe ( Builder->BoneWeights);

        DeallocateSafe ( Builder->FirstMaterialTriangle);
        DeallocateSafe ( Builder->EdgeHashHeads);

        DeallocateSafe ( Builder->BufferedVertex[0]);
        DeallocateSafe ( Builder->BufferedVertex[1]);
        DeallocateSafe ( Builder->BufferedVertex[2]);

        DeallocateSafe ( Builder->TruncatedVertex);
        DeallocateSafe ( Builder->ComparisonVertex);

        DeallocateSafe ( Builder->BufferedVertexType);

        DeallocateSafe ( Builder->VertexComponentNames );

    #endif

        DeallocateSafe(Builder);
    }
}



void GRANNY
EndProcessingMeshInPlace(mesh_builder *Builder,
                         void *VertexDataMemory,
                         vertex_data *&VertexData,
                         void *TopologyMemory,
                         tri_topology *&Topology)
{
    VertexData = 0;
    Topology = 0;

    if(Builder)
    {
        aggr_allocator Allocator;
        InitializeAggrAlloc(Allocator);

        char *VertexComponentNameStringSpace = NULL;
        AggrVertexData(Allocator, *Builder, VertexData, &VertexComponentNameStringSpace);
        if(EndAggrPlacement(Allocator, VertexDataMemory))
        {
            // Sweet juju.  Reuse this, I happen to know that's ok.
            InitializeAggrAlloc(Allocator);

            AggrTopology(Allocator, *Builder, Topology);
            if(EndAggrPlacement(Allocator, TopologyMemory))
            {
                // Fill in the vertex component names.
                {for ( int32x ComponentNum = 0; ComponentNum < Builder->VertexComponentNameCount; ComponentNum++ )
                {
                    // VertexComponentNameStringSpace is a continuous array that is large enough to hold all the strings.
                    VertexComponentNameStringSpace[0] = '\0';
                    int32x NameStringLength = 0;
                    if ( Builder->VertexComponentNames[ComponentNum] != NULL )
                    {
                        CopyString ( Builder->VertexComponentNames[ComponentNum], VertexComponentNameStringSpace );
                        NameStringLength = StringLength ( Builder->VertexComponentNames[ComponentNum] );
                    }
                    VertexData->VertexComponentNames[ComponentNum] = VertexComponentNameStringSpace;
                    VertexComponentNameStringSpace += 1 + NameStringLength;
                }}

                // NOTE: You cannot use MakeEmptyDataTypeObject here,
                // because the aggregate allocator is filling in pointers.
                VertexData->VertexAnnotationSetCount = 0;
                VertexData->VertexAnnotationSets = 0;

                Topology->TriAnnotationSetCount = 0;
                Topology->TriAnnotationSets = 0;

                Topology->BonesForTriangleCount = 0;
                Topology->BonesForTriangle = 0;
                Topology->TriangleToBoneCount = 0;
                Topology->TriangleToBoneIndices = 0;

                // Copy the vertex data
                Copy(GetTotalTypeSize(Builder->VertexType),
                     Builder->VertexType, VertexData->VertexType);

                Assert(GetStackUnitCount(Builder->VertexStack) == VertexData->VertexCount);
                SerializeStack(Builder->VertexStack, VertexData->Vertices);

                // Copy the primitives
                Assert(Builder->UsedMaterialCount == Topology->GroupCount);

                int32x TriangleCount = 0;
                int32x GroupIndex = 0;
                int32 *V = Topology->Indices;
                {for(int32x MaterialIndex = 0;
                     MaterialIndex < Builder->MaterialCount;
                     ++MaterialIndex)
                {
                    int32x const FirstTriangle =
                        Builder->FirstMaterialTriangle[MaterialIndex];
                    if(FirstTriangle != Terminator)
                    {
                        tri_material_group &Group =
                            Topology->Groups[GroupIndex++];
                        Group.MaterialIndex = MaterialIndex;
                        Group.TriFirst = TriangleCount;
                        Group.TriCount = BuildIndices(*Builder, FirstTriangle,
                                                      &V[TriangleCount * 3]);

                        TriangleCount += Group.TriCount;
                    }
                }}

                // Copy the vertex-to-vertex map
                SerializeStack(Builder->NextCoincidentVertStack, Topology->VertexToVertexMap);

                // Copy the vertex-to-triangle map
                Assert ( Topology->VertexToTriangleCount == GetStackUnitCount(Builder->VertexStack) );
                SerializeStack(Builder->FromTriangleStack, Topology->VertexToTriangleMap);

                // Build the side-to-neighbor map
                Assert(TriangleCount == Builder->NextUnusedTriangle);
                BuildTopology(*Builder, TriangleCount,
                              Topology->Indices,
                              Topology->SideToNeighborMap);
            }
        }

    }
}

void GRANNY
ReallyEndMesh(mesh_builder *Builder)
{
    if(Builder)
    {
        DeallocateSafeBuilder(Builder);
    }
}



void GRANNY
EndMeshInPlace(mesh_builder *Builder,
               void *VertexDataMemory,
               vertex_data *&VertexData,
               void *TopologyMemory,
               tri_topology *&Topology)
{
    EndProcessingMeshInPlace ( Builder, VertexDataMemory, VertexData, TopologyMemory, Topology );
    ReallyEndMesh(Builder);
}



void GRANNY
SetNormalTolerance(mesh_builder &Builder, real32 Tolerance)
{
    Builder.NormalTolerance = Tolerance;
}

void GRANNY
SetTangentTolerance(mesh_builder &Builder, real32 Tolerance)
{
    Builder.TangentTolerance = Tolerance;
}

void GRANNY
SetBinormalTolerance(mesh_builder &Builder, real32 Tolerance)
{
    Builder.BinormalTolerance = Tolerance;
}

void GRANNY
SetTangentBinormalCrossTolerance(mesh_builder &Builder, real32 Tolerance)
{
    Builder.TangentBinormalCrossTolerance = Tolerance;
}

void GRANNY
SetTangentMergingTolerance(mesh_builder &Builder, real32 MinCosine)
{
    Builder.TangentMergingMinCosine = MinCosine;
}

void GRANNY
SetChannelTolerance(mesh_builder &Builder, int32x Channel,
                    real32 Tolerance)
{
    Assert(Channel < Builder.ChannelCount);
    Builder.ChannelTolerance[Channel] = Tolerance;
}

static bool
GetMatch(real32 Tolerance, int32x Count,
         real32 const *Value0, real32 const *Value1)
{
    real32 SquaredDistance = 0;
    while(Count--)
    {
        SquaredDistance += Square(*Value0++ - *Value1++);
    }

    return(SquaredDistance <= (Tolerance*Tolerance));
}

static bool
VertexMatches(mesh_builder &Builder, int32x Side, int32x VertexIndex)
{
    buffered_vertex_prefix *Target = Builder.ComparisonVertex;
    ConvertSingleObject(Builder.VertexType,
                        GetStackUnit(Builder.VertexStack, VertexIndex),
                        Builder.BufferedVertexType, Target);

    buffered_vertex_prefix *Test = Builder.BufferedVertex[Side];

    bool NormalMatch = GetMatch(Builder.NormalTolerance,
                                3, Target->Normal, Test->Normal);
    bool TangentMatch = GetMatch(Builder.TangentTolerance,
                                 3, Target->Tangent, Test->Tangent);
    bool BinormalMatch = GetMatch(Builder.BinormalTolerance,
                                  3, Target->Binormal, Test->Binormal);
    bool TangentBinormalCrossMatch = GetMatch(
        Builder.TangentBinormalCrossTolerance, 3,
        Target->TangentBinormalCross, Test->TangentBinormalCross);

    real32 *TargetChannel = (real32 *)(Target + 1);
    real32 *TestChannel = (real32 *)(Test + 1);

    data_type_definition *Channel = Builder.ChannelTypes;
    int32x ChannelIndex = 0;
    bool ChannelMatch = true;
    while(ChannelMatch && (Channel->Type != EndMember))
    {
        ChannelMatch = GetMatch(
            Builder.ChannelTolerance[ChannelIndex],
            Channel->ArrayWidth, TargetChannel, TestChannel);
        TargetChannel += Channel->ArrayWidth;
        TestChannel += Channel->ArrayWidth;

        ++Channel;
        ++ChannelIndex;
    }

    return(NormalMatch && TangentMatch && BinormalMatch &&
           TangentBinormalCrossMatch && ChannelMatch);
}


static int32x
GetMatchingVertex(mesh_builder &Builder, int32x TriangleIndex, int32x Side)
{
    // Run the vertex through the conversion process to guarantee that
    // all values are properly truncated before matching.
    ConvertSingleObject(Builder.BufferedVertexType,
                        Builder.BufferedVertex[Side],
                        Builder.VertexType,
                        Builder.TruncatedVertex);
    ConvertSingleObject(Builder.VertexType,
                        Builder.TruncatedVertex,
                        Builder.BufferedVertexType,
                        Builder.BufferedVertex[Side]);

    int32x OriginalVertexIndex = Builder.VertexIndex[Side];
    int32x VertexIndex = OriginalVertexIndex;
    int32x PreviousVertexIndex = VertexIndex;
    if(STACK_UNIT_AS(Builder.NextCoincidentVertStack, VertexIndex, int32) != Terminator)
    {
        do
        {
            if(VertexMatches(Builder, Side, VertexIndex))
            {
                return(VertexIndex);
            }

            PreviousVertexIndex = VertexIndex;
            VertexIndex = STACK_UNIT_AS(Builder.NextCoincidentVertStack, VertexIndex, int32);
        } while(VertexIndex != OriginalVertexIndex);

        NewStackUnit(Builder.VertexStack, VertexIndex);
        Assert(VertexIndex >= 0);

        int32x CheckIndex;
        NewStackUnit(Builder.NextCoincidentVertStack, CheckIndex);
        Assert(CheckIndex == VertexIndex);

        NewStackUnit(Builder.FromTriangleStack, CheckIndex);
        Assert(CheckIndex == VertexIndex);
    }

    // If we got here, we didn't find a match, so we'd better make one


    void *OriginalVertex = (uint8*)GetStackUnit(Builder.VertexStack, OriginalVertexIndex);
    void *Vertex = GetStackUnit(Builder.VertexStack, VertexIndex);
    Copy(Builder.VertexSize, OriginalVertex, Vertex);
    MergeSingleObject(Builder.BufferedVertexType,
                      Builder.BufferedVertex[Side],
                      Builder.VertexType,
                      Vertex);

    STACK_UNIT_AS(Builder.NextCoincidentVertStack, PreviousVertexIndex, int32) = VertexIndex;
    STACK_UNIT_AS(Builder.NextCoincidentVertStack, VertexIndex, int32)         = OriginalVertexIndex;
    STACK_UNIT_AS(Builder.FromTriangleStack, VertexIndex, int32)               = TriangleIndex;

    return(VertexIndex);
}

void GRANNY
SetPosition(mesh_builder &Builder, real32 Px, real32 Py, real32 Pz)
{
    VectorSet3(Builder.Point, Px, Py, Pz);
}

void GRANNY
AddWeight(mesh_builder &Builder, int32x BoneIndex, real32 Weight)
{
    Assert(BoneIndex < Builder.BoneCount);
#if 1
    Builder.BoneWeights[BoneIndex] += Weight;
#else
    // This SHOULD mess things up, but it seems hard to find actual
    // art where it does.
    Builder.BoneWeights[BoneIndex] = Weight;
#endif
}

static void
ClipWeights(mesh_builder &Builder)
{
    // Clear the clipper table to an invalid value
    // Number of weights per vertex.
    int32x const WeightCount = Builder.WeightCount;
    // vertex_weight_arrays data for the current vertex.
    real32 *TopWeights = Builder.WeightArrays.BoneWeights;
    uint32 *TopIndices = Builder.WeightArrays.BoneIndices;

    SetUInt32(WeightCount, 0, TopIndices);
    SetReal32(WeightCount, 0, TopWeights);

    // Find the n largest weights
    {for(int32x BoneIndex = 0;
         BoneIndex < Builder.BoneCount;
         ++BoneIndex)
    {
        real32 const BoneWeight = Builder.BoneWeights[BoneIndex];
        {for(int32x WeightIndex = 0;
             WeightIndex < WeightCount;
             ++WeightIndex)
        {
            if(TopWeights[WeightIndex] < BoneWeight)
            {
                {for(int32x CopyIndex = WeightCount - 1;
                     CopyIndex > WeightIndex;
                     --CopyIndex)
                {
                    TopWeights[CopyIndex] = TopWeights[CopyIndex - 1];
                    TopIndices[CopyIndex] = TopIndices[CopyIndex - 1];
                }}

                TopWeights[WeightIndex] = Builder.BoneWeights[BoneIndex];
                TopIndices[WeightIndex] = BoneIndex;

                break;
            }
        }}
    }}

    // Re-normalize the weights
    // TODO: Make this optionally renormalize to the _previous_ total,
    // rather than 1.0f (for non-buggy inputs)?
    real32 WeightSum = 0.0f;
    {for(int32x WeightIndex = 0;
         WeightIndex < WeightCount;
         ++WeightIndex)
    {
        WeightSum += TopWeights[WeightIndex];
    }}

    if(WeightSum > 0.0f)
    {
        real32 const InverseWeightSum = 1.0f / WeightSum;
        {for(int32x WeightIndex = 0;
             WeightIndex < WeightCount;
             ++WeightIndex)
        {
            TopWeights[WeightIndex] *= InverseWeightSum;
        }}
    }
    else
    {
        if(WeightCount)
        {
            Assert(!"Encountered weighted vertex with no weights");
            TopWeights[0] = 1.0f;
        }
    }
}

void GRANNY
PushVertex(mesh_builder &Builder)
{
    Assert(GetStackUnitCount(Builder.VertexStack) < Builder.VertexCount);

    int32x VertexIndex;
    NewStackUnit(Builder.VertexStack, VertexIndex);
    Assert(VertexIndex >= 0);
    uint8 *VertexPointer = (uint8*)GetStackUnit(Builder.VertexStack, VertexIndex);

    ClipWeights(Builder);

    ConvertSingleObject(P3VertexType, Builder.Point,
                        Builder.VertexType, VertexPointer);
    MergeSingleObject(VertexWeightArraysType, &Builder.WeightArrays,
                      Builder.VertexType, VertexPointer);

    variant Weights;
    if(FindMatchingMember(Builder.VertexType, 0,
                          VertexBoneWeightsName,
                          &Weights))
    {
        EnsureExactOneNorm(*Weights.Type,
                           VertexPointer + (intaddrx)Weights.Object);
    }

    InitializeVertexBuffer(Builder);
}

void GRANNY
SetVertexIndex(mesh_builder &Builder, int32x Edge, int32x Index)
{
    Assert(Edge < 3);
    Builder.VertexIndex[Edge] = Index;
}

void GRANNY
SetNormal(mesh_builder &Builder, int32x Edge, real32 Nx, real32 Ny, real32 Nz)
{
    Assert(Edge < 3);
    VectorSet3(Builder.BufferedVertex[Edge]->Normal, Nx, Ny, Nz);
}

void GRANNY
SetTangent(mesh_builder &Builder, int32x Edge, real32 Tx, real32 Ty, real32 Tz)
{
    Assert(Edge < 3);
    VectorSet3(Builder.BufferedVertex[Edge]->Tangent, Tx, Ty, Tz);
}

void GRANNY
SetBinormal(mesh_builder &Builder, int32x Edge, real32 Bx, real32 By, real32 Bz)
{
    Assert(Edge < 3);
    VectorSet3(Builder.BufferedVertex[Edge]->Binormal, Bx, By, Bz);
}

void GRANNY
SetTangentBinormalCross(mesh_builder &Builder, int32x Edge,
                        real32 TBCx, real32 TBCy, real32 TBCz)
{
    Assert(Edge < 3);
    VectorSet3(Builder.BufferedVertex[Edge]->TangentBinormalCross,
               TBCx, TBCy, TBCz);
}

int32x GRANNY
GetChannelComponentCount(mesh_builder &Builder, int32x ChannelIndex)
{
    Assert(ChannelIndex < (int32x)Builder.ChannelCount);
    data_type_definition &ChannelDef = Builder.ChannelTypes[ChannelIndex];
    return ChannelDef.ArrayWidth;
}


void GRANNY
SetChannel(mesh_builder &Builder, int32x Edge, int32x ChannelIndex,
           real32 const *Value)
{
    Assert(Edge < 3);
    Assert(ChannelIndex < (int32x)Builder.ChannelCount);

    real32 *Channel = (real32 *)(Builder.BufferedVertex[Edge] + 1);

    data_type_definition *ChannelDef = Builder.ChannelTypes;
    while(ChannelIndex--)
    {
        Channel += (ChannelDef++)->ArrayWidth;
    }

    int32x ChannelWidth = ChannelDef->ArrayWidth;
    while(ChannelWidth--)
    {
        *Channel++ = *Value++;
    }
}

void GRANNY
SetMaterial(mesh_builder &Builder, int32x MaterialIndex)
{
    Assert((MaterialIndex >= 0) && (MaterialIndex < Builder.MaterialCount));

    Builder.MaterialIndex = MaterialIndex;
}

void GRANNY
PushTriangle(mesh_builder &Builder)
{
    int32x const TriangleIndex = Builder.NextUnusedTriangle++;

    int32x FirstVertIndex;
    MultipleNewStackUnits(Builder.VertexIndexStack, 3, FirstVertIndex, NULL);
    {for(int32x Side = 0;
         Side < 3;
         ++Side)
    {
        STACK_UNIT_AS(Builder.VertexIndexStack, FirstVertIndex + Side, int32) =
            GetMatchingVertex(Builder, TriangleIndex, Side);
    }}


    // Add to the material list...
    {
        int NextMatIndex;
        NewStackUnit(Builder.NextInMaterialStack, NextMatIndex);
        Assert(NextMatIndex == TriangleIndex);

        if (Builder.FirstMaterialTriangle[Builder.MaterialIndex] == Terminator)
        {
            // This is the first time we've seen this material
            ++Builder.UsedMaterialCount;
        }

        STACK_UNIT_AS(Builder.NextInMaterialStack, TriangleIndex, int32) =
            Builder.FirstMaterialTriangle[Builder.MaterialIndex];
        Builder.FirstMaterialTriangle[Builder.MaterialIndex] = TriangleIndex;
    }

    InitializeTriangleBuffer(Builder);
}



data_type_definition const *GRANNY
GetVertexTypeDefinition(mesh_builder &Builder)
{
    return Builder.VertexType;
}

void *GRANNY
GetVertexData(mesh_builder &Builder, int32x VertexNumber)
{
    return GetStackUnit(Builder.VertexStack, VertexNumber);
}

int32 GRANNY
GetNextCoincidentVertex(mesh_builder &Builder, int32x VertexIndex)
{
    return STACK_UNIT_AS(Builder.NextCoincidentVertStack, VertexIndex, int32);
}

int32 GRANNY
GetFromTriangleForVertex(mesh_builder &Builder, int32x VertexIndex)
{
    return STACK_UNIT_AS(Builder.FromTriangleStack, VertexIndex, int32);
}

