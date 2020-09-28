// ========================================================================
// $File: //jeffr/granny/rt/granny_tangent_frame.cpp $
// $DateTime: 2007/07/23 15:42:28 $
// $Change: 15572 $
// $Revision: #15 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TANGENT_FRAME_H)
#include "granny_tangent_frame.h"
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

#if !defined(GRANNY_MESH_BUILDER_H)
#include "granny_mesh_builder.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_TRI_TOPOLOGY_H)
#include "granny_tri_topology.h"
#endif

#if !defined(GRANNY_VERTEX_DATA_H)
#include "granny_vertex_data.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode ExporterLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition GBX333VertexType[] =
{
    {Real32Member, VertexTangentName, 0, 3},
    {Real32Member, VertexBinormalName, 0, 3},
    {Real32Member, VertexTangentBinormalCrossName, 0, 3},

    {EndMember},
};

END_GRANNY_NAMESPACE;

void GRANNY
FrameSubtract3(tangent_frame &Result,
               tangent_frame const &A, tangent_frame const &B)
{
    VectorSubtract3(Result.U, A.U, B.U);
    VectorSubtract3(Result.V, A.V, B.V);
    VectorSubtract3(Result.N, A.N, B.N);
}

void GRANNY
FrameScale3(tangent_frame &Result, real32 Scalar)
{
    VectorScale3(Result.U, Scalar);
    VectorScale3(Result.V, Scalar);
    VectorScale3(Result.N, Scalar);
}

void GRANNY
FrameScale3(tangent_frame &Result,
            real32 Scalar, tangent_frame const &A)
{
    VectorScale3(Result.U, Scalar, A.U);
    VectorScale3(Result.V, Scalar, A.V);
    VectorScale3(Result.N, Scalar, A.N);
}

void GRANNY
ScaleFrameAdd3(tangent_frame &Result,
               real32 Scalar, tangent_frame const &B)
{
    ScaleVectorAdd3(Result.U, Scalar, B.U);
    ScaleVectorAdd3(Result.V, Scalar, B.V);
    ScaleVectorAdd3(Result.N, Scalar, B.N);
}

void GRANNY
ScaleFrameAdd3(tangent_frame &Result,
               tangent_frame const &A,
               real32 Scalar, tangent_frame const &B)
{
    ScaleVectorAdd3(Result.U, A.U, Scalar, B.U);
    ScaleVectorAdd3(Result.V, A.V, Scalar, B.V);
    ScaleVectorAdd3(Result.N, A.N, Scalar, B.N);
}

void GRANNY
FrameAdd3(tangent_frame &Result, tangent_frame const &A)
{
    VectorAdd3(Result.U, A.U);
    VectorAdd3(Result.V, A.V);
    VectorAdd3(Result.N, A.N);
}

static void
ResetVertexTangentNormals(mesh_builder& Builder)
{
    int32x const VertexCount = GetStackUnitCount(Builder.VertexStack);

    {for(int32x VertexIndex = 0;
         VertexIndex < VertexCount;
         ++VertexIndex)
    {
        pnt332_vertex Vertex;

        GetSingleVertex(GetVertexTypeDefinition(Builder),
                        GetStackUnit(Builder.VertexStack, VertexIndex), 0,
                        PNT332VertexType, &Vertex);

        tangent_frame &Tangent = STACK_UNIT_AS(Builder.VertexTangentStack, VertexIndex, tangent_frame);
        VectorEquals3(Tangent.N, Vertex.Normal);
    }}
}

static void
NormalizeVertexTangents(mesh_builder& Builder)
{
    int32x const VertexCount = GetStackUnitCount(Builder.VertexStack);

    {for(int32x VertexIndex = 0;
         VertexIndex < VertexCount;
         ++VertexIndex)
    {
        tangent_frame &Tangent = STACK_UNIT_AS(Builder.VertexTangentStack, VertexIndex, tangent_frame);

        NormalizeOrZero3(Tangent.U);
        NormalizeOrZero3(Tangent.V);
        NormalizeOrZero3(Tangent.N);
    }}
}

static void
OrthonormalizeVertexTangents(mesh_builder& Builder)
{
    int32x const VertexCount = GetStackUnitCount(Builder.VertexStack);

    {for(int32x VertexIndex = 0;
         VertexIndex < VertexCount;
         ++VertexIndex)
    {
        tangent_frame &Tangent = STACK_UNIT_AS(Builder.VertexTangentStack, VertexIndex, tangent_frame);

        triple Test;
        VectorCrossProduct3(Test, Tangent.U, Tangent.V);
        bool Reversed = (InnerProduct3(Test, Tangent.N) < 0.0f);

        if(Reversed)
        {
            VectorCrossProduct3(Tangent.V, Tangent.N, Tangent.U);
            VectorCrossProduct3(Tangent.U, Tangent.V, Tangent.N);
        }
        else
        {
            VectorCrossProduct3(Tangent.V, Tangent.U, Tangent.N);
            VectorCrossProduct3(Tangent.U, Tangent.N, Tangent.V);
        }

        NormalizeOrZero3(Tangent.U);
        NormalizeOrZero3(Tangent.V);
        NormalizeOrZero3(Tangent.N);
    }}
}

static bool
CreateBaseVertexTangents(mesh_builder& Builder)
{
    CheckCondition(GetStackUnitCount(Builder.VertexTangentStack) == 0, return false);

    tangent_frame ZeroTangent;
    VectorZero3(ZeroTangent.U);
    VectorZero3(ZeroTangent.V);
    VectorZero3(ZeroTangent.N);

    int32x FirstIndex;
    if (MultipleNewStackUnits(Builder.VertexTangentStack,
                              GetStackUnitCount(Builder.VertexStack),
                              FirstIndex, &ZeroTangent) == false)
    {
        return false;
    }
    Assert(FirstIndex == 0);

    return true;
}

static void
ScaleAddVectorDirectional(real32 *Dest, real32 C, real32 const *Source)
{
    if(InnerProduct3(Dest, Source) >= 0.0f)
    {
        ScaleVectorAdd3(Dest, C, Source);
    }
    else
    {
        ScaleVectorAdd3(Dest, -C, Source);
    }
}

static void
ScaleAddFrameDirectional(tangent_frame &Frame0,
                         real32 C1, tangent_frame const &Frame1)
{
    ScaleAddVectorDirectional(Frame0.U, C1, Frame1.U);
    ScaleAddVectorDirectional(Frame0.V, C1, Frame1.V);
    ScaleAddVectorDirectional(Frame0.N, C1, Frame1.N);
}

static bool
TangentsAreOppositeWinding(tangent_frame const &A,
                           tangent_frame const &B)
{
    triple ACross;
    triple BCross;

    VectorCrossProduct3(ACross, A.U, A.V);
    VectorCrossProduct3(BCross, B.U, B.V);

    real32 ADot = InnerProduct3(ACross, A.N);
    real32 BDot = InnerProduct3(BCross, B.N);

    if ( ( ADot == 0.0f ) || ( BDot == 0.0f ) )
    {
        // Almost always means that the tangent-space info for one
        // of the verts has not been set up yet (i.e. you just called
        // CreateBaseVertexTangents). In which case, the two are not _opposite_,
        // because one or more of them is zero.
        return false;
    }

    return(Sign(ADot) != Sign(BDot));
}

static bool
TangentsAreCloseEnough(tangent_frame const &A,
                       tangent_frame const &B,
                       real32 MinCosine)
{
    real32 MinProduct = Minimum(AbsoluteValue(InnerProduct3(A.U, B.U)),
                                AbsoluteValue(InnerProduct3(A.V, B.V)));
    return(MinProduct > MinCosine);
}

static bool
TangentsAreCloseEnoughWithZeroCheck(tangent_frame const &A,
                                    tangent_frame const &B,
                                    real32 MinCosine)
{
    triple NormAU; VectorEquals3(NormAU, A.U);
    triple NormAV; VectorEquals3(NormAV, A.V);
    triple NormBU; VectorEquals3(NormBU, B.U);
    triple NormBV; VectorEquals3(NormBV, B.V);

    if (NormalizeOrZero3(NormAU) > 0.0f && NormalizeOrZero3(NormBU) >= 0.0f)
    {
        real32 Product = InnerProduct3(NormAU, NormBU);
        if (Product <= MinCosine)
            return false;
    }

    if (NormalizeOrZero3(NormAV) > 0.0f && NormalizeOrZero3(NormBV) >= 0.0f)
    {
        real32 Product = InnerProduct3(NormAV, NormBV);
        if (Product <= MinCosine)
            return false;
    }

    return true;
}

static void
LockSeamTangents(mesh_builder& Builder)
{
    int32x const VertexCount = GetStackUnitCount(Builder.VertexStack);

    tangent_frame *TempFrames = AllocateArray(VertexCount, tangent_frame);
    {for(int32x Frame = 0; Frame < VertexCount; ++Frame)
    {
        VectorZero3(TempFrames[Frame].U);
        VectorZero3(TempFrames[Frame].V);
        VectorZero3(TempFrames[Frame].N);
    }}

    {for(int32x VertexIndex = 0;
         VertexIndex < VertexCount;
         ++VertexIndex)
    {
        int32x BaseVertexIndex = VertexIndex;
        pnt332_vertex BaseVertex;
        GetSingleVertex(GetVertexTypeDefinition(Builder),
                        GetStackUnit(Builder.VertexStack, VertexIndex), 0,
                        PNT332VertexType, &BaseVertex);

        int32x CurrentVertexIndex = BaseVertexIndex;
        do
        {
            pnt332_vertex CurrentVertex;
            GetSingleVertex(GetVertexTypeDefinition(Builder),
                            GetStackUnit(Builder.VertexStack, CurrentVertexIndex), 0,
                            PNT332VertexType, &CurrentVertex);

            tangent_frame const &CurrentFrame =
                STACK_UNIT_AS(Builder.VertexTangentStack, CurrentVertexIndex, tangent_frame);
            tangent_frame &ResultFrame = TempFrames[CurrentVertexIndex];

            int32x AccumVertexIndex = CurrentVertexIndex;
            do
            {
                pnt332_vertex AccumVertex;
                GetSingleVertex(GetVertexTypeDefinition(Builder),
                                GetStackUnit(Builder.VertexStack, AccumVertexIndex), 0,
                                PNT332VertexType, &AccumVertex);

                tangent_frame const &AccumFrame =
                    STACK_UNIT_AS(Builder.VertexTangentStack, AccumVertexIndex, tangent_frame);

                if(AreEqual(3, CurrentVertex.Normal,
                            AccumVertex.Normal, 0.00001f) &&
                   TangentsAreCloseEnough(CurrentFrame, AccumFrame, Builder.TangentMergingMinCosine))
                {
                    ScaleAddFrameDirectional(ResultFrame, 1.0f, AccumFrame);
                }

                AccumVertexIndex = STACK_UNIT_AS(Builder.NextCoincidentVertStack, AccumVertexIndex, int32);
            } while((AccumVertexIndex != -1) &&
                    (AccumVertexIndex != CurrentVertexIndex));

            CurrentVertexIndex = STACK_UNIT_AS(Builder.NextCoincidentVertStack, CurrentVertexIndex, int32);
        } while((CurrentVertexIndex != -1) &&
                (CurrentVertexIndex != BaseVertexIndex));
    }}

    {for(int32x Frame = 0; Frame < VertexCount; ++Frame)
    {
        STACK_UNIT_AS(Builder.VertexTangentStack, Frame, tangent_frame) =
            TempFrames[Frame];
    }}

    Deallocate(TempFrames);
}

static bool
ComputeTriangleTangents(mesh_builder& Builder)
{
    CheckCondition(GetStackUnitCount(Builder.TriangleTangentStack) == 0, return false);
    int32x const TriangleCount = Builder.NextUnusedTriangle;

    int32x FirstIndex;
    if (MultipleNewStackUnits(Builder.TriangleTangentStack, TriangleCount, FirstIndex, NULL) == false)
    {
        // Serious error
        Log0(ErrorLogMessage, ExporterLogMessage, "Unable to allocate the appropriate number of tangent frames");
        return false;
    }
    Assert(FirstIndex == 0);
    Assert(Builder.NextUnusedTriangle == GetStackUnitCount(Builder.TriangleTangentStack));


    {for(int32x TriangleIndex = 0;
         TriangleIndex < TriangleCount;
         ++TriangleIndex)
    {
        pnt332_vertex Vertex[3];
        {for(int32x Idx = 0; Idx < 3; ++Idx)
        {
            int32 TriangleVertexIndex =
                STACK_UNIT_AS(Builder.VertexIndexStack, 3*TriangleIndex + Idx, int32);

            GetSingleVertex(GetVertexTypeDefinition(Builder),
                            GetStackUnit(Builder.VertexStack, TriangleVertexIndex), 0,
                            PNT332VertexType, &Vertex[Idx]);
        }}

        tangent_frame &Tangent = STACK_UNIT_AS(Builder.TriangleTangentStack, TriangleIndex, tangent_frame);

        triple EdgeA;
        VectorSubtract3(EdgeA, Vertex[1].Position, Vertex[0].Position);
        real32 au = Vertex[1].UV[0] - Vertex[0].UV[0];
        real32 av = Vertex[1].UV[1] - Vertex[0].UV[1];

        triple EdgeB;
        VectorSubtract3(EdgeB, Vertex[2].Position, Vertex[0].Position);
        real32 bu = Vertex[2].UV[0] - Vertex[0].UV[0];
        real32 bv = Vertex[2].UV[1] - Vertex[0].UV[1];

        VectorCrossProduct3(Tangent.N, EdgeA, EdgeB);
        NormalizeOrZero3(Tangent.N);

        real32 Determinant = au*bv - av*bu;
        real32 InvDeterminant = 1.0f / Determinant;

        ScaleVectorPlusScaleVector3(Tangent.U,
                                    -bv * InvDeterminant, EdgeA,
                                    av * InvDeterminant, EdgeB);
        ScaleVectorPlusScaleVector3(Tangent.V,
                                    -bu * InvDeterminant, EdgeA,
                                    au * InvDeterminant, EdgeB);

        NormalizeOrZero3(Tangent.U);
        NormalizeOrZero3(Tangent.V);
    }}

    return true;
}


#if DEBUG
#define TOLERANCE_PARANOIA 1
#else
#define TOLERANCE_PARANOIA 0
#endif


#if TOLERANCE_PARANOIA
static bool
VerticesAreEqual( void const *Vert1, void const *Vert2, int32x SizeOfVertices, pnt332_vertex const &A, pnt332_vertex const &B )
#else
static bool
VerticesAreEqual( void const *Vert1, void const *Vert2, int32x SizeOfVertices )
#endif
{
    if ( Compare ( SizeOfVertices, Vert1, Vert2 ) )
    {
        return true;
    }
    else
    {
        // If they're different, I'm going to be paranoid and check that they weren't only very slightly different.
#if TOLERANCE_PARANOIA
        if (
            AreEqual(3, A.Position, B.Position, 0.00001f) &&
            AreEqual(3, A.Normal, B.Normal, 0.00001f) &&
            AreEqual(2, A.UV, B.UV, 0.00001f) )
        {
            // I assumed that if they weren't bitwise-correct, that I didn't need
            // to check to a tolerance, because the only thing ComputeVertexTangents
            // does is copy vertex data around, never does computation with it.
            // So check this is correct.
            Assert ( Compare ( sizeof(real32)*3, A.Position, B.Position ) );
            Assert ( Compare ( sizeof(real32)*3, A.Normal, B.Normal ) );
            Assert ( Compare ( sizeof(real32)*2, A.UV, B.UV ) );
            // Note that for example these verts might still differ in the second texture coord,
            // or by a colour or something. So just because we got here doesn't necessarily mean they match.
        }
#endif

        return false;
    }
}



static int32x
ComputeVertexTangents(mesh_builder& Builder)
{
    int32x SizeOfVertex = GetTotalObjectSize ( GetVertexTypeDefinition(Builder) );

    int32x const TriangleCount = Builder.NextUnusedTriangle;
    Assert(TriangleCount == GetStackUnitCount(Builder.TriangleTangentStack));

    {for(int32x TriangleIndex = 0;
         TriangleIndex < TriangleCount;
         ++TriangleIndex)
    {
        tangent_frame &TriangleTangent =
            STACK_UNIT_AS(Builder.TriangleTangentStack, TriangleIndex, tangent_frame);

        {for(int32x Side = 0;
             Side < 3;
             ++Side)
        {

            // Look for one of the vertices that share this position, and find
            // one that matches the UV, etc coords (the VerticesAreEqual test)
            // and that has the same tangent-space winding (or none exists for that vertex yet).
            int32x BaseVertexIndexIdx = 3*TriangleIndex + Side;
            int32x BaseVertexIndex    = STACK_UNIT_AS(Builder.VertexIndexStack, 3*TriangleIndex + Side, int32);
#if TOLERANCE_PARANOIA
            pnt332_vertex BaseVertex;
            GetSingleVertex(GetVertexTypeDefinition(Builder),
                            GetStackUnit(Builder.VertexStack, BaseVertexIndex), 0,
                            PNT332VertexType, &BaseVertex);
#endif

            int32x SearchVertexIndex = BaseVertexIndex;
            int32x ChangeBaseVertexIndexTo = -1;
            do
            {
#if TOLERANCE_PARANOIA
                pnt332_vertex TestVertex;
                GetSingleVertex(GetVertexTypeDefinition(Builder),
                                GetStackUnit(Builder.VertexStack, SearchVertexIndex), 0,
                                PNT332VertexType, &TestVertex);
#endif

                void const* CurrentBaseVertex   = GetStackUnit(Builder.VertexStack, BaseVertexIndex);
                void const* CurrentSearchVertex = GetStackUnit(Builder.VertexStack, SearchVertexIndex);
                if (
#if TOLERANCE_PARANOIA
                    VerticesAreEqual ( CurrentBaseVertex, CurrentSearchVertex, SizeOfVertex, BaseVertex, TestVertex )
#else
                    VerticesAreEqual ( CurrentBaseVertex, CurrentSearchVertex, SizeOfVertex )
#endif
                    && !TangentsAreOppositeWinding(STACK_UNIT_AS(Builder.VertexTangentStack, SearchVertexIndex, tangent_frame),
                                                   TriangleTangent))
                {
                    // Vertices match and tangent spaces have same winding.
#if TOLERANCE_PARANOIA
                    BaseVertex = TestVertex;
#endif
                    if ( SearchVertexIndex != BaseVertexIndex )
                    {
                        // The triangle doesn't already point to this vertex. So make it do so.
                        ChangeBaseVertexIndexTo = SearchVertexIndex;
                    }
                    break;
                }

                SearchVertexIndex = STACK_UNIT_AS(Builder.NextCoincidentVertStack, SearchVertexIndex, int32);
            } while(SearchVertexIndex != BaseVertexIndex);


            // Note that we are depending on an implementation detail
            // of the stack allocator here, which is that stacks do
            // NOT change the address of any units when new units are
            // pushed onto them.  Throwing this assert in here mostly
            // to detect when the implementation of the stack changes
            // substantially.
            Assert(Builder.VertexTangentStack.BlockDirectory != NULL);
            tangent_frame *VertexTangent = &STACK_UNIT_AS(Builder.VertexTangentStack, BaseVertexIndex, tangent_frame);

            if ( ChangeBaseVertexIndexTo >= 0 )
            {
                VertexTangent = &STACK_UNIT_AS(Builder.VertexTangentStack, ChangeBaseVertexIndexTo, tangent_frame);
            }

            if(TangentsAreOppositeWinding(*VertexTangent, TriangleTangent) ||
               !TangentsAreCloseEnoughWithZeroCheck(*VertexTangent, TriangleTangent,
                                                    Builder.TangentMergingMinCosine))
            {
                //Assert ( ChangeBaseVertexIndexTo == -1 );
                // Couldn't find a match, and the tangents are
                // opposite winding, so copy this vertex.

                int32x OldVertexIndex = BaseVertexIndex;

                int32x NewVertexIndex;
                {
                    // This is a really unfortunate mess of
                    // implementation details.  Note that we have to
                    // increase the size of all stacks marked with
                    // "per vertex" in the mesh_builder structure.
                    // Assert that the indices wind up the way we
                    // expect...
                    int32x NewTangentIndex;
                    int32x NewTriMapIndex;
                    int32x NewNextVertIndex;
                    if (!NewStackUnit(Builder.VertexStack, NewVertexIndex) ||
                        !NewStackUnit(Builder.VertexTangentStack, NewTangentIndex) ||
                        !NewStackUnit(Builder.FromTriangleStack, NewTriMapIndex) ||
                        !NewStackUnit(Builder.NextCoincidentVertStack, NewNextVertIndex))
                    {
                        // Failure
                        return false;
                    }
                    Assert(NewVertexIndex == NewTangentIndex);
                    Assert(NewVertexIndex == NewTriMapIndex);
                    Assert(NewVertexIndex == NewNextVertIndex);
                }

                // Split the vertex.  This means: copy the vert data,
                // then link into the NextCoincidentVert list
                {
                    Copy(SizeOfVertex,
                         GetStackUnit(Builder.VertexStack, OldVertexIndex),
                         GetStackUnit(Builder.VertexStack, NewVertexIndex));
                    STACK_UNIT_AS(Builder.NextCoincidentVertStack, NewVertexIndex, int32) =
                        STACK_UNIT_AS(Builder.NextCoincidentVertStack, OldVertexIndex, int32);
                    STACK_UNIT_AS(Builder.NextCoincidentVertStack, OldVertexIndex, int32) = NewVertexIndex;
                }

                VertexTangent = &STACK_UNIT_AS(Builder.VertexTangentStack, NewVertexIndex, tangent_frame);
                VectorZero3(VertexTangent->U);
                VectorZero3(VertexTangent->V);
                VectorZero3(VertexTangent->N);
                STACK_UNIT_AS(Builder.FromTriangleStack, NewVertexIndex, int32) = TriangleIndex;

                ChangeBaseVertexIndexTo = NewVertexIndex;
            }


            if ( ChangeBaseVertexIndexTo >= 0 )
            {
                Assert ( BaseVertexIndex != ChangeBaseVertexIndexTo );

                // We remapped this index of this triangle.
                int32x OldVertexIndex = BaseVertexIndex;
                STACK_UNIT_AS(Builder.VertexIndexStack, BaseVertexIndexIdx, int32) = ChangeBaseVertexIndexTo;
                BaseVertexIndex = ChangeBaseVertexIndexTo;

                // This is a map from each vertex to the triangle that caused its creation.
                STACK_UNIT_AS(Builder.FromTriangleStack, BaseVertexIndex, int32) = TriangleIndex;
                if ( STACK_UNIT_AS(Builder.FromTriangleStack, OldVertexIndex, int32) == TriangleIndex )
                {
                    // This triangle is no longer the "provoking" one for the old vertex.
                    // So we need to have a look and find a tri that could be the provoking one.
                    bool Found = false;
                    {for(int32x i = 0;
                         i < TriangleCount * 3;
                         ++i)
                    {
                        if ( STACK_UNIT_AS(Builder.VertexIndexStack, i, int32) == OldVertexIndex )
                        {
                            // This tri uses it.
                            STACK_UNIT_AS(Builder.FromTriangleStack, OldVertexIndex, int32) = i / 3;
                            Found = true;
                            break;
                        }
                    }}

                    if ( !Found )
                    {
                        // So... no triangle uses this vertex? That's not good.
                        // It's not insanely bad, but it is a waste of space.
                        // Hopefully this should now never happen.
                        Assert ( false );
                        STACK_UNIT_AS(Builder.FromTriangleStack, OldVertexIndex, int32) = -1;
                    }
                }
            }


            VectorAdd3(VertexTangent->N, TriangleTangent.N);
            VectorAdd3(VertexTangent->U, TriangleTangent.U);
            VectorAdd3(VertexTangent->V, TriangleTangent.V);
        }}
    }}

    return true;
}


bool GRANNY
BuildTangentSpace(mesh_builder& Builder)
{
    if (ComputeTriangleTangents(Builder) == false)
        return false;

    if (CreateBaseVertexTangents(Builder) == false ||
        ComputeVertexTangents(Builder) == false)
    {
        // Log the error
        return false;
    }

    ResetVertexTangentNormals(Builder);
    NormalizeVertexTangents(Builder);

    LockSeamTangents(Builder);
    NormalizeVertexTangents(Builder);

//     {for(int32x IterationIndex = 0;
//          IterationIndex < 10;
//          ++IterationIndex)
//     {
//         SmoothVertexTangents(Mesh);
//         ResetVertexTangentNormals(Mesh);
//         NormalizeVertexTangents(Mesh);
//         LockSeamTangents(Mesh);
//     }}

    ResetVertexTangentNormals(Builder);
    OrthonormalizeVertexTangents(Builder);

    return true;
}


// static void
// SplitSharedVertexTangents(loaded_mesh &Mesh,
//                           int32x ThisTriangleIndex,
//                           int32x NeighborTriangleIndex)
// {
//     int32 *ThisTriangleVertexIndex =
//         Mesh.Indices + 3*ThisTriangleIndex;
//     int32 *NeighborTriangleVertexIndex =
//         Mesh.Indices + 3*NeighborTriangleIndex;
//
//     {for(int32x ThisSide = 0;
//          ThisSide < 3;
//          ++ThisSide)
//     {
//         {for(int32x NeighborSide = 0;
//              NeighborSide < 3;
//              ++NeighborSide)
//         {
//             int32 &ThisVertexIndex = ThisTriangleVertexIndex[ThisSide];
//             if(ThisVertexIndex ==
//                NeighborTriangleVertexIndex[NeighborSide])
//             {
//                 ThisVertexIndex = SplitVertex(Mesh, ThisVertexIndex);
//             }
//         }}
//     }}
// }
//
//
// static void
// SmoothVertexTangents(loaded_mesh &Mesh)
// {
//     tangent_frame *TempFrames =
//         AllocateArray(Mesh.VertexCount, tangent_frame);
//     ZeroVertexTangents(Mesh.VertexCount, TempFrames);
//
//     {for(int32x TriangleIndex = 0;
//          TriangleIndex < Mesh.TriangleCount;
//          ++TriangleIndex)
//     {
//         int32 *TriangleVertexIndex = Mesh.Indices + 3*TriangleIndex;
//
//         pnt332_vertex const &Vertex0 = Mesh.Vertices[TriangleVertexIndex[0]];
//         pnt332_vertex const &Vertex1 = Mesh.Vertices[TriangleVertexIndex[1]];
//         pnt332_vertex const &Vertex2 = Mesh.Vertices[TriangleVertexIndex[2]];
//
//         tangent_frame &SourceFrame0 = Mesh.VertexTangentSpace[TriangleVertexIndex[0]];
//         tangent_frame &SourceFrame1 = Mesh.VertexTangentSpace[TriangleVertexIndex[1]];
//         tangent_frame &SourceFrame2 = Mesh.VertexTangentSpace[TriangleVertexIndex[2]];
//
//         tangent_frame &DestFrame0 = TempFrames[TriangleVertexIndex[0]];
//         tangent_frame &DestFrame1 = TempFrames[TriangleVertexIndex[1]];
//         tangent_frame &DestFrame2 = TempFrames[TriangleVertexIndex[2]];
//
//         real32 Coefficient = 0.5f;
//
//         ScaleFrameAdd3(DestFrame0, Coefficient, SourceFrame0);
//         ScaleFrameAdd3(DestFrame0, Coefficient, SourceFrame1);
//         ScaleFrameAdd3(DestFrame0, Coefficient, SourceFrame2);
//
//         ScaleFrameAdd3(DestFrame1, Coefficient, SourceFrame0);
//         ScaleFrameAdd3(DestFrame1, Coefficient, SourceFrame1);
//         ScaleFrameAdd3(DestFrame1, Coefficient, SourceFrame2);
//
//         ScaleFrameAdd3(DestFrame2, Coefficient, SourceFrame0);
//         ScaleFrameAdd3(DestFrame2, Coefficient, SourceFrame1);
//         ScaleFrameAdd3(DestFrame2, Coefficient, SourceFrame2);
//     }}
//
//     CopyArray(Mesh.VertexCount, TempFrames, Mesh.VertexTangentSpace);
//     Deallocate(TempFrames);
// }
