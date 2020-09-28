// ========================================================================
// $File: //jeffr/granny/rt/granny_generic_deformers.cpp $
// $DateTime: 2007/07/23 15:42:28 $
// $Change: 15572 $
// $Revision: #11 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_GENERIC_DEFORMERS_H)
#include "granny_generic_deformers.h"
#endif

#if !defined(GRANNY_DEFORMERS_H)
#include "granny_deformers.h"
#endif

#if !defined(GRANNY_VERTEX_DATA_H)
#include "granny_vertex_data.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

#undef SubsystemCode
#define SubsystemCode DeformerLogMessage

#define DEFINE_CODE 1

#define _Stringize(Name) #Name
#define Stringize(Name) _Stringize(Name)

#define COPY_T2 \
        Dest->UV[0] = Source->UV[0]; \
        Dest->UV[1] = Source->UV[1];

#define INIT_P3 Dest->Position[0] = Dest->Position[1] = Dest->Position[2] = 0.0f;
#define INIT_N3 Dest->Normal[0] = Dest->Normal[1] = Dest->Normal[2] = 0.0f;
#define INIT_G3 Dest->Tangent[0] = Dest->Tangent[1] = Dest->Tangent[2] = 0.0f;
#define INIT_B3 Dest->Binormal[0] = Dest->Binormal[1] = Dest->Binormal[2] = 0.0f;

#define DEF_WEIGHTS matrix_4x4 const *Transform; real32 W;

#define LOAD_WEIGHT_I(WeightIndex) \
    Transform = &Transforms[TransformTable[Source->BoneIndices[WeightIndex]]]; \
    W = (real32)(Source->BoneWeights[WeightIndex]) / 255.0f;

#define LOAD_WEIGHT_D(WeightIndex) \
    Transform = &Transforms[Source->BoneIndices[WeightIndex]]; \
    W = (real32)(Source->BoneWeights[WeightIndex]) / 255.0f;

#define LOAD_WEIGHT_D4(WeightIndex) \
    Transform = &Transforms[(Source->BoneIndices >> \
                            (WeightIndex * 8)) & 0xFF]; \
    W = (real32)((Source->BoneWeights >> (WeightIndex * 8)) & 0xFF) / 255.0f;

#define LOAD_P3                                 \
    real32 const Px = Source->Position[0];      \
    real32 const Py = Source->Position[1];      \
    real32 const Pz = Source->Position[2]

#define LOAD_N3                                 \
    real32 const Nx = Source->Normal[0];        \
    real32 const Ny = Source->Normal[1];        \
    real32 const Nz = Source->Normal[2]

#define LOAD_G3                                 \
    real32 const Gx = Source->Tangent[0];       \
    real32 const Gy = Source->Tangent[1];       \
    real32 const Gz = Source->Tangent[2]

#define LOAD_B3                                 \
    real32 const Bx = Source->Binormal[0];      \
    real32 const By = Source->Binormal[1];      \
    real32 const Bz = Source->Binormal[2]

#define LOAD_T2                                 \
    real32 const Tu = Source->UV[0];            \
    real32 const Tv = Source->UV[1]

// Only used for transformations...
#define STORE_P3                                \
    Dest->Position[0] = Px;                     \
    Dest->Position[1] = Py;                     \
    Dest->Position[2] = Pz

#define STORE_N3                                \
    Dest->Normal[0] = Nx;                       \
    Dest->Normal[1] = Ny;                       \
    Dest->Normal[2] = Nz

#define STORE_G3                                \
    Dest->Tangent[0] = Gx;                      \
    Dest->Tangent[1] = Gy;                      \
    Dest->Tangent[2] = Gz

#define STORE_T2                                \
    Dest->UV[0] = Tu;                           \
    Dest->UV[1] = Tv



#define SUM_N3                                                              \
    Dest->Normal[0] += W * (Nx*(*Transform)[0][0] + Ny*(*Transform)[1][0] + \
                            Nz*(*Transform)[2][0]);                         \
    Dest->Normal[1] += W * (Nx*(*Transform)[0][1] + Ny*(*Transform)[1][1] + \
                            Nz*(*Transform)[2][1]);                         \
    Dest->Normal[2] += W * (Nx*(*Transform)[0][2] + Ny*(*Transform)[1][2] + \
                            Nz*(*Transform)[2][2]);

#define SUM_G3                                                                  \
    Dest->Tangent[0] += W * (Gx*(*Transform)[0][0] + Gy*(*Transform)[1][0] +    \
                             Gz*(*Transform)[2][0]);                            \
    Dest->Tangent[1] += W * (Gx*(*Transform)[0][1] + Gy*(*Transform)[1][1] +    \
                             Gz*(*Transform)[2][1]);                            \
    Dest->Tangent[2] += W * (Gx*(*Transform)[0][2] + Gy*(*Transform)[1][2] +    \
                             Gz*(*Transform)[2][2]);

#define SUM_B3                                                                  \
    Dest->Binormal[0] += W * (Bx*(*Transform)[0][0] + By*(*Transform)[1][0] +   \
                              Bz*(*Transform)[2][0]);                           \
    Dest->Binormal[1] += W * (Bx*(*Transform)[0][1] + By*(*Transform)[1][1] +   \
                              Bz*(*Transform)[2][1]);                           \
    Dest->Binormal[2] += W * (Bx*(*Transform)[0][2] + By*(*Transform)[1][2] +   \
                              Bz*(*Transform)[2][2]);

#define SUM_P3                                                                  \
    Dest->Position[0] += W * (Px*(*Transform)[0][0] + Py*(*Transform)[1][0] +   \
                              Pz*(*Transform)[2][0] +    (*Transform)[3][0]);   \
    Dest->Position[1] += W * (Px*(*Transform)[0][1] + Py*(*Transform)[1][1] +   \
                              Pz*(*Transform)[2][1] +    (*Transform)[3][1]);   \
    Dest->Position[2] += W * (Px*(*Transform)[0][2] + Py*(*Transform)[1][2] +   \
                              Pz*(*Transform)[2][2] +    (*Transform)[3][2]);

#define TRANSFORM_N3(I)                                                         \
    Dest->Normal[0] = (Nx*(Transforms[I])[0][0] + Ny*(Transforms[I])[1][0] +    \
                       Nz*(Transforms[I])[2][0]);                               \
    Dest->Normal[1] = (Nx*(Transforms[I])[0][1] + Ny*(Transforms[I])[1][1] +    \
                       Nz*(Transforms[I])[2][1]);                               \
    Dest->Normal[2] = (Nx*(Transforms[I])[0][2] + Ny*(Transforms[I])[1][2] +    \
                       Nz*(Transforms[I])[2][2]);

#define TRANSFORM_G3(I)                                                         \
    Dest->Tangent[0] = (Gx*(Transforms[I])[0][0] + Gy*(Transforms[I])[1][0] +   \
                        Gz*(Transforms[I])[2][0]);                              \
    Dest->Tangent[1] = (Gx*(Transforms[I])[0][1] + Gy*(Transforms[I])[1][1] +   \
                        Gz*(Transforms[I])[2][1]);                              \
    Dest->Tangent[2] = (Gx*(Transforms[I])[0][2] + Gy*(Transforms[I])[1][2] +   \
                        Gz*(Transforms[I])[2][2]);

#define TRANSFORM_B3(I)                                                         \
    Dest->Binormal[0] = (Bx*(Transforms[I])[0][0] + By*(Transforms[I])[1][0] +  \
                         Bz*(Transforms[I])[2][0]);                             \
    Dest->Binormal[1] = (Bx*(Transforms[I])[0][1] + By*(Transforms[I])[1][1] +  \
                         Bz*(Transforms[I])[2][1]);                             \
    Dest->Binormal[2] = (Bx*(Transforms[I])[0][2] + By*(Transforms[I])[1][2] +  \
                         Bz*(Transforms[I])[2][2]);

#define TRANSFORM_P3(I)                                                         \
    Dest->Position[0] = (Px*(Transforms[I])[0][0] + Py*(Transforms[I])[1][0] +  \
                         Pz*(Transforms[I])[2][0] +    (Transforms[I])[3][0]);  \
    Dest->Position[1] = (Px*(Transforms[I])[0][1] + Py*(Transforms[I])[1][1] +  \
                         Pz*(Transforms[I])[2][1] +    (Transforms[I])[3][1]);  \
    Dest->Position[2] = (Px*(Transforms[I])[0][2] + Py*(Transforms[I])[1][2] +  \
                         Pz*(Transforms[I])[2][2] +    (Transforms[I])[3][2]);

#define COPY_AND_ADVANCE(SInit, DInit, CInit, S_Stride, D_Stride)   \
    {                                                               \
        uint32 const *S = (uint32 const *)((SInit) + 1);            \
        uint32 *D = (uint32 *)((DInit) + 1);                        \
        int32x C = (CInit);                                         \
        while(C--) *D++ = *S++;                                     \
        (uint8 const *&)SInit = (uint8 const *&)SInit + S_Stride;   \
        (uint8 *&)DInit = (uint8*)DInit + D_Stride;                 \
    } typedef int RequireSemiColon

#define DEF_BLEND_VARIABLES
    matrix_4x4 SummedMatrix;
    matrix_4x4 const *Transform;
    real32 W;

#define TRANSFORM_POINT(Point) \
    Dest->Point[0] = (Source->Point[0]*(SummedMatrix)[0][0] + Source->Point[1]*(SummedMatrix)[1][0] + \
                      Source->Point[2]*(SummedMatrix)[2][0] +    (SummedMatrix)[3][0]); \
    Dest->Point[1] = (Source->Point[0]*(SummedMatrix)[0][1] + Source->Point[1]*(SummedMatrix)[1][1] + \
                      Source->Point[2]*(SummedMatrix)[2][1] +    (SummedMatrix)[3][1]); \
    Dest->Point[2] = (Source->Point[0]*(SummedMatrix)[0][2] + Source->Point[1]*(SummedMatrix)[1][2] + \
                      Source->Point[2]*(SummedMatrix)[2][2] +    (SummedMatrix)[3][2]);
#define TRANSFORM_VECTOR(Vector) \
    Dest->Vector[0] = (Source->Vector[0]*(SummedMatrix)[0][0] + Source->Vector[1]*(SummedMatrix)[1][0] + \
                      Source->Vector[2]*(SummedMatrix)[2][0]); \
    Dest->Vector[1] = (Source->Vector[0]*(SummedMatrix)[0][1] + Source->Vector[1]*(SummedMatrix)[1][1] + \
                      Source->Vector[2]*(SummedMatrix)[2][1]); \
    Dest->Vector[2] = (Source->Vector[0]*(SummedMatrix)[0][2] + Source->Vector[1]*(SummedMatrix)[1][2] + \
                      Source->Vector[2]*(SummedMatrix)[2][2]);

#define SUM_MATRIX \
    SummedMatrix[0][0] += W*(*Transform)[0][0]; \
    SummedMatrix[0][1] += W*(*Transform)[0][1]; \
    SummedMatrix[0][2] += W*(*Transform)[0][2]; \
    SummedMatrix[0][3] += W*(*Transform)[0][3]; \
    SummedMatrix[1][0] += W*(*Transform)[1][0]; \
    SummedMatrix[1][1] += W*(*Transform)[1][1]; \
    SummedMatrix[1][2] += W*(*Transform)[1][2]; \
    SummedMatrix[1][3] += W*(*Transform)[1][3]; \
    SummedMatrix[2][0] += W*(*Transform)[2][0]; \
    SummedMatrix[2][1] += W*(*Transform)[2][1]; \
    SummedMatrix[2][2] += W*(*Transform)[2][2]; \
    SummedMatrix[2][3] += W*(*Transform)[2][3]; \
    SummedMatrix[3][0] += W*(*Transform)[3][0]; \
    SummedMatrix[3][1] += W*(*Transform)[3][1]; \
    SummedMatrix[3][2] += W*(*Transform)[3][2]; \
    SummedMatrix[3][3] += W*(*Transform)[3][3];

#define SET_MATRIX \
    SummedMatrix[0][0] = W*(*Transform)[0][0]; \
    SummedMatrix[0][1] = W*(*Transform)[0][1]; \
    SummedMatrix[0][2] = W*(*Transform)[0][2]; \
    SummedMatrix[0][3] = W*(*Transform)[0][3]; \
    SummedMatrix[1][0] = W*(*Transform)[1][0]; \
    SummedMatrix[1][1] = W*(*Transform)[1][1]; \
    SummedMatrix[1][2] = W*(*Transform)[1][2]; \
    SummedMatrix[1][3] = W*(*Transform)[1][3]; \
    SummedMatrix[2][0] = W*(*Transform)[2][0]; \
    SummedMatrix[2][1] = W*(*Transform)[2][1]; \
    SummedMatrix[2][2] = W*(*Transform)[2][2]; \
    SummedMatrix[2][3] = W*(*Transform)[2][3]; \
    SummedMatrix[3][0] = W*(*Transform)[3][0]; \
    SummedMatrix[3][1] = W*(*Transform)[3][1]; \
    SummedMatrix[3][2] = W*(*Transform)[3][2]; \
    SummedMatrix[3][3] = W*(*Transform)[3][3];

//
//
//

#define USE_SUMMED_MATRICES 1

void GRANNY
DeformPWN313I(int32x Count, void const *SourceInit, void *DestInit,
              int32x const *TransformTable, matrix_4x4 const *Transforms,
              int32x CopySize, int32x SourceStride, int32x DestStride)
{
    COUNT_BLOCK("DeformPWN313I");

    pwn313_vertex const *Source = (pwn313_vertex const *)SourceInit;
    pn33_vertex *Dest = (pn33_vertex *)DestInit;

    while(Count--)
    {
        LOAD_P3; LOAD_N3;
        TRANSFORM_P3(TransformTable[Source->BoneIndex]);
        TRANSFORM_N3(TransformTable[Source->BoneIndex]);

        COPY_AND_ADVANCE(Source, Dest, CopySize, SourceStride, DestStride);
    }
}

void GRANNY
DeformPWN313D(int32x Count, void const *SourceInit, void *DestInit,
              matrix_4x4 const *Transforms,
              int32x CopySize, int32x SourceStride, int32x DestStride)
{
    COUNT_BLOCK("DeformPWN313D");

    pwn313_vertex const *Source = (pwn313_vertex const *)SourceInit;
    pn33_vertex *Dest = (pn33_vertex *)DestInit;

    while(Count--)
    {
        LOAD_P3; LOAD_N3;
        TRANSFORM_P3(Source->BoneIndex);
        TRANSFORM_N3(Source->BoneIndex);

        COPY_AND_ADVANCE(Source, Dest, CopySize, SourceStride, DestStride);
    }
}

void GRANNY
DeformPWNGB31333I(int32x Count, void const *SourceInit, void *DestInit,
                  int32x const *TransformTable, matrix_4x4 const *Transforms,
                  int32x CopySize, int32x SourceStride, int32x DestStride)
{
    COUNT_BLOCK("DeformPWNGB31333I");

    pwngb31333_vertex const *Source = (pwngb31333_vertex const *)SourceInit;
    pngb3333_vertex *Dest = (pngb3333_vertex *)DestInit;

    while(Count--)
    {
        LOAD_P3; LOAD_N3; LOAD_G3; LOAD_B3;
        TRANSFORM_P3(TransformTable[Source->BoneIndex]);
        TRANSFORM_N3(TransformTable[Source->BoneIndex]);
        TRANSFORM_G3(TransformTable[Source->BoneIndex]);
        TRANSFORM_B3(TransformTable[Source->BoneIndex]);

        COPY_AND_ADVANCE(Source, Dest, CopySize, SourceStride, DestStride);
    }
}

void GRANNY
DeformPWNGB31333D(int32x Count, void const *SourceInit, void *DestInit,
                  matrix_4x4 const *Transforms,
                  int32x CopySize, int32x SourceStride, int32x DestStride)
{
    COUNT_BLOCK("DeformPWNGB31333D");

    pwngb31333_vertex const *Source = (pwngb31333_vertex const *)SourceInit;
    pngb3333_vertex *Dest = (pngb3333_vertex *)DestInit;

    while(Count--)
    {
        LOAD_P3; LOAD_N3; LOAD_G3; LOAD_B3;
        TRANSFORM_P3(Source->BoneIndex);
        TRANSFORM_N3(Source->BoneIndex);
        TRANSFORM_G3(Source->BoneIndex);
        TRANSFORM_B3(Source->BoneIndex);

        COPY_AND_ADVANCE(Source, Dest, CopySize, SourceStride, DestStride);
    }
}

//
//
//

#define BONE_COUNT 0
#include "granny_generic_deformer_wrapper.inl"

#define BONE_COUNT 1
#include "granny_generic_deformer_wrapper.inl"

#define BONE_COUNT 2
#include "granny_generic_deformer_wrapper.inl"

#if 0
#define BONE_COUNT 3
#include "granny_generic_deformer_wrapper.inl"
#endif

#define BONE_COUNT 4
#include "granny_generic_deformer_wrapper.inl"

//
//
//

void GRANNY
DeformPN33I(int32x Count, void const *SourceInit, void *DestInit,
            int32x const *TransformTable, matrix_4x4 const *Transforms,
            int32x CopySize, int32x SourceStride, int32x DestStride)
{
    COUNT_BLOCK("DeformPN33I");

    pn33_vertex const *Source = (pn33_vertex const *)SourceInit;
    pn33_vertex *Dest = (pn33_vertex *)DestInit;

    while(Count--)
    {
        LOAD_P3; LOAD_N3;
        TRANSFORM_P3(TransformTable[0]);
        TRANSFORM_N3(TransformTable[0]);

        COPY_AND_ADVANCE(Source, Dest, CopySize, SourceStride, DestStride);
    }
}

void GRANNY
DeformPN33D(int32x Count, void const *SourceInit, void *DestInit,
            matrix_4x4 const *Transforms, int32x CopySize, int32x SourceStride, int32x DestStride)
{
    COUNT_BLOCK("DeformPN33D");

    pn33_vertex const *Source = (pn33_vertex const *)SourceInit;
    pn33_vertex *Dest = (pn33_vertex *)DestInit;

    while(Count--)
    {
        LOAD_P3; LOAD_N3;
        TRANSFORM_P3(0);
        TRANSFORM_N3(0);

        COPY_AND_ADVANCE(Source, Dest, CopySize, SourceStride, DestStride);
    }
}

void GRANNY
DeformPNGB3333I(int32x Count, void const *SourceInit, void *DestInit,
                int32x const *TransformTable, matrix_4x4 const *Transforms,
                int32x CopySize, int32x SourceStride, int32x DestStride)
{
    COUNT_BLOCK("DeformPNGB3333I");

    pngb3333_vertex const *Source = (pngb3333_vertex const *)SourceInit;
    pngb3333_vertex *Dest = (pngb3333_vertex *)DestInit;

    while(Count--)
    {
        LOAD_P3; LOAD_N3; LOAD_G3; LOAD_B3;
        TRANSFORM_P3(TransformTable[0]);
        TRANSFORM_N3(TransformTable[0]);
        TRANSFORM_G3(TransformTable[0]);
        TRANSFORM_B3(TransformTable[0]);

        COPY_AND_ADVANCE(Source, Dest, CopySize, SourceStride, DestStride);
    }
}

void GRANNY
DeformPNGB3333D(int32x Count, void const *SourceInit, void *DestInit,
                matrix_4x4 const *Transforms, int32x CopySize, int32x SourceStride, int32x DestStride)
{
    COUNT_BLOCK("DeformPNGB3333D");

    pngb3333_vertex const *Source = (pngb3333_vertex const *)SourceInit;
    pngb3333_vertex *Dest = (pngb3333_vertex *)DestInit;

    while(Count--)
    {
        LOAD_P3; LOAD_N3; LOAD_G3; LOAD_B3;
        TRANSFORM_P3(0);
        TRANSFORM_N3(0);
        TRANSFORM_G3(0);
        TRANSFORM_B3(0);

        COPY_AND_ADVANCE(Source, Dest, CopySize, SourceStride, DestStride);
    }
}

static void
Convert_PNGT3332_PNTG3323(int32x Count,
                          void *DeformedVerts,
                          int32x Stride)
{
    CheckPointerNotNull(DeformedVerts, return);

    uint8* CurrVertBase = (uint8*)DeformedVerts;
    {for(int32x Vert = 0; Vert < Count; ++Vert)
    {
        pngt3332_vertex* Source = (pngt3332_vertex*)CurrVertBase;
        LOAD_P3; LOAD_N3; LOAD_G3; LOAD_T2;

        pntg3323_vertex* Dest = (pntg3323_vertex*)CurrVertBase;
        STORE_P3; STORE_N3; STORE_T2; STORE_G3;

        CurrVertBase += Stride;
    }}
}


void GRANNY
AddGenericDeformers(void)
{
    bone_deformer GenericDeformers[] = {
        {DeformPositionNormal,
         DeformPWN313D, DeformPWN313I, NULL,
         PWN313VertexType, PN33VertexType, true, true},

        {DeformPositionNormalTangent,
         GeneratedDeformPWNG3133D, GeneratedDeformPWNG3133I, NULL,
         PWNG3133VertexType, PNG333VertexType, true, true},

        {DeformPositionNormalTangentBinormal,
         DeformPWNGB31333D, DeformPWNGB31333I, NULL,
         PWNGB31333VertexType, PNGB3333VertexType, true, true},


        {DeformPositionNormal,
         GeneratedDeformPWN323D, GeneratedDeformPWN323I, NULL,
         PWN323VertexType, PN33VertexType, true, true},

        {DeformPositionNormalTangent,
         GeneratedDeformPWNG3233D, GeneratedDeformPWNG3233I, NULL,
         PWNG3233VertexType, PNG333VertexType, true, true},

        {DeformPositionNormalTangentBinormal,
         GeneratedDeformPWNGB32333D, GeneratedDeformPWNGB32333I, NULL,
         PWNGB32333VertexType, PNGB3333VertexType, true, true},


        {DeformPositionNormal,
         GeneratedDeformPWN343D, GeneratedDeformPWN343I, NULL,
         PWN343VertexType, PN33VertexType, true, true},

        {DeformPositionNormalTangent,
         GeneratedDeformPWNG3433D, GeneratedDeformPWNG3433I, NULL,
         PWNG3433VertexType, PNG333VertexType, true, true},

        {DeformPositionNormalTangentBinormal,
         GeneratedDeformPWNGB34333D, GeneratedDeformPWNGB34333I, NULL,
         PWNGB34333VertexType, PNGB3333VertexType, true, true},


        {DeformPositionNormal,
         DeformPN33D, DeformPN33I, NULL,
         PN33VertexType, PN33VertexType, true, true},

        {DeformPositionNormalTangent,
         GeneratedDeformPNG333D, GeneratedDeformPNG333I, NULL,
         PNG333VertexType, PNG333VertexType, true, true},

        {DeformPositionNormalTangentBinormal,
         DeformPNGB3333D, DeformPNGB3333I, NULL,
         PNGB3333VertexType, PNGB3333VertexType, true, true},
    };

    AddBoneDeformerTable(ArrayLength(GenericDeformers), GenericDeformers);

    // Add a few deformers that require post deformation transforms.
    // TODO: make more general if more are needed
    {
        data_type_definition* BaseVertexFormats[] = {
            PWNGT31332VertexType,
            PWNGT32332VertexType,
            PWNGT34332VertexType,
        };

        {for(int32x BaseFmt = 0; BaseFmt < ArrayLength(BaseVertexFormats); ++BaseFmt)
        {
            bone_deformer BaseDeformer;
            bone_deformer_parameters BaseDeformerParam;
            if (FindBoneDeformerFor(BaseVertexFormats[BaseFmt], PNGT3332VertexType,
                                    DeformPositionNormalTangent,
                                    false,
                                    BaseDeformer,
                                    BaseDeformerParam))
            {
                Assert(BaseDeformer.PostDeformTransform == NULL);

                BaseDeformer.ToLayout = PNTG3323VertexType;
                BaseDeformer.PostDeformTransform = &Convert_PNGT3332_PNTG3323;
                BaseDeformer.CanDoTailCopies = false;
                BaseDeformer.CanIgnoreTailItems = false;
                AddBoneDeformer(BaseDeformer);
            }
        }}
    }
}
