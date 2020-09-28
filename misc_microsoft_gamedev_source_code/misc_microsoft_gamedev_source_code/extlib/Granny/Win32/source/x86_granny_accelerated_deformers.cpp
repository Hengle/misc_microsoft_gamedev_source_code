// ========================================================================
// $File: //jeffr/granny/rt/x86/x86_granny_accelerated_deformers.cpp $
// $DateTime: 2007/04/25 14:34:11 $
// $Change: 14851 $
// $Revision: #11 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#include <xmmintrin.h>

#if !defined(GRANNY_ACCELERATED_DEFORMERS_H)
#include "granny_accelerated_deformers.h"
#endif

#if !defined(GRANNY_VERTEX_DATA_H)
#include "granny_vertex_data.h"
#endif

#if !defined(GRANNY_DEFORMERS_H)
#include "granny_deformers.h"
#endif

#if !defined(GRANNY_GENERIC_DEFORMERS_H)
#include "granny_generic_deformers.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(X86_GRANNY_CPU_QUERIES_H)
#include "x86_granny_cpu_queries.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

#define DEFINE_CODE 1

#define _Stringize(Name) #Name
#define Stringize(Name) _Stringize(Name)

/*
 * Note: the matrix_4x4 Transforms are assumed to be 16 byte aligned for all the
 *  routines below. You'll crash rather quickly if they aren't.
 */

/*
 * 16 byte aligned structure to help hold intermediate results and
 *  our transform pointers.
 */
typedef struct _SSEABUF
{
    __m128 DestPosXYZ;
    __m128 DestNormXYZ;

    matrix_4x4 const *Transforms;
    int32x const *TransformTable;
} SSEABUF;

// Weight conversion multiplier.
static const __m128 OneOver255 = { 1.0f/255.0f, 1.0f/255.0f, 1.0f/255.0f, 1.0f/255.0f };

/*
 * Include the direct versions of our sse helper routines.
 */
#define DIRECT_ROUTINES
#include "x86_granny_accelerated_deformers_custom.inl"

/*
 * Include the indirect versions of our sse helper routines.
 */
#undef DIRECT_ROUTINES
#include "x86_granny_accelerated_deformers_custom.inl"

#define LogSSEPunt(T, S, D)                                         \
{                                                                   \
    static bool Once = false;                                       \
    if(Once)                                                        \
    {                                                               \
        Log3(ErrorLogMessage, DeformerLogMessage,                   \
             "SSE routine bypassed because the following pointers " \
             "were not aligned: %s%s%s  (this message will only "   \
             "appear once)",                                        \
             IS_ALIGNED_16(T) ? "" : "MatrixBuffer4x4 ",            \
             IS_ALIGNED_16(S) ? "" : "SourceVertices ",             \
             IS_ALIGNED_16(D) ? "" : "DestVertices ");              \
                                                                    \
        Once = false;                                               \
    }                                                               \
}

//=========================================================================
// SseDeformPWNT3132I
//
// DestInit must be 16 byte aligned.
//=========================================================================
void
SseDeformPWNT3132I(int32x Count, void const *SourceInit, void *DestInit,
                   int32x const *TransformTable, matrix_4x4 const *Transforms,
                   int32x TailCopy32Count, int32x SourceStride, int32x DestStride)
{
    Assert(TailCopy32Count == 0);

    if(IS_ALIGNED_16(Transforms) && IS_ALIGNED_16(DestInit))
    {
        COUNT_BLOCK("SseDeformPWNT3132I");

        pwnt3132_vertex const *Source = (pwnt3132_vertex const *)SourceInit;
        pnt332_vertex *Dest = (pnt332_vertex *)DestInit;
        SSEABUF abuf;

        abuf.Transforms = Transforms;
        abuf.TransformTable = TransformTable;

        // Call helper routine
        SseDeformPWNT3132HelperI(Count >> 1, Source, Dest, &abuf);

        if(Count & 0x1)
        {
            // Handle odd vert.
            DeformPWN313I(1, Source + Count - 1,
                          Dest + Count - 1,
                          TransformTable, Transforms, 2, SourceStride, DestStride);
        }
    }
    else
    {
        LogSSEPunt(Transforms, 0, DestInit);
        DeformPWN313I(Count, SourceInit, DestInit,
                      TransformTable, Transforms, 2,
                      SourceStride, DestStride);
    }
}

//=========================================================================
// SseDeformPWNT3132D
//
// DestInit must be 16 byte aligned.
//=========================================================================
void
SseDeformPWNT3132D(int32x Count, void const *SourceInit, void *DestInit,
                   matrix_4x4 const *Transforms,
                   int32x TailCopy32Count, int32x SourceStride, int32x DestStride)
{
    Assert(TailCopy32Count == 0);

    if(IS_ALIGNED_16(Transforms) && IS_ALIGNED_16(DestInit))
    {
        COUNT_BLOCK("SseDeformPWNT3132D");

        pwnt3132_vertex const *Source = (pwnt3132_vertex const *)SourceInit;
        pnt332_vertex *Dest = (pnt332_vertex *)DestInit;
        SSEABUF abuf;

        abuf.Transforms = Transforms;

        SseDeformPWNT3132HelperD(Count >> 1, Source, Dest, &abuf);

        if(Count & 0x1)
        {
            // Handle odd vert.
            DeformPWN313D(1, Source + Count - 1,
                          Dest + Count - 1,
                          Transforms, 2,
                          SourceStride, DestStride);
        }
    }
    else
    {
        LogSSEPunt(Transforms, 0, DestInit);
        DeformPWN313D(Count, SourceInit, DestInit, Transforms, 2,
                      SourceStride, DestStride);
    }
}

//=========================================================================
// SseDeformPWNT3232I
//
// DestInit must be 16 byte aligned.
//=========================================================================
void
SseDeformPWNT3232I(int32x Count, void const *SourceInit, void *DestInit,
                   int32x const *TransformTable, matrix_4x4 const *Transforms,
                   int32x TailCopy32Count, int32x SourceStride, int32x DestStride)
{
    Assert(TailCopy32Count == 0);

    if(IS_ALIGNED_16(Transforms) && IS_ALIGNED_16(DestInit))
    {
        COUNT_BLOCK("SseDeformPWNT3232I");

        pwnt3232_vertex const *Source = (pwnt3232_vertex const *)SourceInit;
        pnt332_vertex *Dest = (pnt332_vertex *)DestInit;
        SSEABUF abuf;

        abuf.Transforms = Transforms;
        abuf.TransformTable = TransformTable;

        SseDeformPWNT3232HelperI(Count, Source, Dest, &abuf);
    }
    else
    {
        LogSSEPunt(Transforms, 0, DestInit);
        GeneratedDeformPWN323I(Count, SourceInit, DestInit,
                               TransformTable, Transforms, 2,
                               SourceStride, DestStride);
    }
}

//=========================================================================
// SseDeformPWNT3232D
//
// DestInit must be 16 byte aligned.
//=========================================================================
void
SseDeformPWNT3232D(int32x Count, void const *SourceInit, void *DestInit,
                   matrix_4x4 const *Transforms,
                   int32x TailCopy32Count, int32x SourceStride, int32x DestStride)
{
    Assert(TailCopy32Count == 0);

    if(IS_ALIGNED_16(Transforms) && IS_ALIGNED_16(DestInit))
    {
        COUNT_BLOCK("SseDeformPWNT3232D");

        pwnt3232_vertex const *Source = (pwnt3232_vertex const *)SourceInit;
        pnt332_vertex *Dest = (pnt332_vertex *)DestInit;
        SSEABUF abuf;

        abuf.Transforms = Transforms;

        SseDeformPWNT3232HelperD(Count, Source, Dest, &abuf);
    }
    else
    {
        LogSSEPunt(Transforms, 0, DestInit);
        GeneratedDeformPWN323D(Count, SourceInit, DestInit, Transforms, 2,
                               SourceStride, DestStride);
    }
}

//=========================================================================
// SseDeformPWNT3432I
//
// DestInit must be 16 byte aligned.
//=========================================================================
void
SseDeformPWNT3432I(int32x Count, void const *SourceInit, void *DestInit,
                   int32x const *TransformTable, matrix_4x4 const *Transforms,
                   int32x TailCopy32Count, int32x SourceStride, int32x DestStride)
{
    Assert(TailCopy32Count == 0);

    if(IS_ALIGNED_16(Transforms) && IS_ALIGNED_16(DestInit))
    {
        COUNT_BLOCK("SseDeformPWNT3432I");

        pwnt3432_vertex const *Source = (pwnt3432_vertex const *)SourceInit;
        pnt332_vertex *Dest = (pnt332_vertex *)DestInit;
        SSEABUF abuf;

        abuf.Transforms = Transforms;
        abuf.TransformTable = TransformTable;

        SseDeformPWNT3432HelperI(Count, Source, Dest, &abuf);
    }
    else
    {
        LogSSEPunt(Transforms, 0, DestInit);
        GeneratedDeformPWN343I(Count, SourceInit, DestInit,
                               TransformTable, Transforms, 2,
                               SourceStride, DestStride);
    }
}

//=========================================================================
// SseDeformPWNT3432D
//
// DestInit must be 16 byte aligned.
//=========================================================================
void
SseDeformPWNT3432D(int32x Count, void const *SourceInit, void *DestInit,
                   matrix_4x4 const *Transforms,
                   int32x TailCopy32Count, int32x SourceStride, int32x DestStride)
{
    Assert(TailCopy32Count == 0);

    if(IS_ALIGNED_16(Transforms) && IS_ALIGNED_16(DestInit))
    {
        COUNT_BLOCK("SseDeformPWNT3432D");

        pwnt3432_vertex const *Source = (pwnt3432_vertex const *)SourceInit;
        pnt332_vertex *Dest = (pnt332_vertex *)DestInit;
        SSEABUF abuf;

        abuf.Transforms = Transforms;

        SseDeformPWNT3432HelperD(Count, Source, Dest, &abuf);
    }
    else
    {
        LogSSEPunt(Transforms, 0, DestInit);
        GeneratedDeformPWN343D(Count, SourceInit, DestInit, Transforms, 2,
                               SourceStride, DestStride);
    }
}

//
//
//

//
// Output: Trans0 * w0 + Trans1 * w1
//      (in xmm4, xmm5, xmm6, xmm7)
//
// Scratch regs: xmm0, xmm1
//
__inline void SseSetup1XMMMatrix(
    matrix_4x4 const *Trans0)
{
    __asm
    {
        mov ecx, [Trans0]
        movaps xmm4, [ecx]
        movaps xmm5, [ecx+16]
        movaps xmm6, [ecx+32]
        movaps xmm7, [ecx+48]
    }
}

__inline void SseSetup2BoneWeightsXMMMatrix(
    uint8 const *BoneWeights,
    matrix_4x4 const *Trans0,
    matrix_4x4 const *Trans1)
{
    __asm
    {
        mov ecx, [BoneWeights]

        movzx eax, byte ptr [ecx]       // w0
        cvtsi2ss xmm4, eax
        mulss xmm4, [OneOver255]
        shufps xmm4, xmm4, 0            // w0|w0|w0|w0

        movzx eax, byte ptr [ecx + 1]   // w1
        cvtsi2ss xmm0, eax
        mulss xmm0, [OneOver255]
        shufps xmm0, xmm0, 0            // w1|w1|w1|w1

        movaps xmm5, xmm4               // w0|w0|w0|w0
        movaps xmm6, xmm4               // w0|w0|w0|w0
        movaps xmm7, xmm4               // w0|w0|w0|w0

        mov ecx, [Trans0]
        mulps xmm4, [ecx]               // w0 * Trans0
        mulps xmm5, [ecx+16]
        mulps xmm6, [ecx+32]
        mulps xmm7, [ecx+48]

        mov ecx, [Trans1]
        movaps xmm1, xmm0               // w1|w1|w1|w1
        mulps xmm1, [ecx]               // w1 * Trans1
        addps xmm4, xmm1

        movaps xmm1, xmm0               // w1|w1|w1|w1
        mulps xmm1, [ecx+16]
        addps xmm5, xmm1

        movaps xmm1, xmm0               // w1|w1|w1|w1
        mulps xmm1, [ecx+32]
        addps xmm6, xmm1

        mulps xmm0, [ecx+48]
        addps xmm7, xmm0
    }
}

__inline void SseSetup4BoneWeightsXMMMatrix(
    uint8 const *BoneWeights,
    matrix_4x4 const *Trans0,
    matrix_4x4 const *Trans1,
    matrix_4x4 const *Trans2,
    matrix_4x4 const *Trans3)
{
    SseSetup2BoneWeightsXMMMatrix(BoneWeights, Trans0, Trans1);

    __asm
    {
        mov ecx, [BoneWeights]

        movzx eax, byte ptr [ecx + 2]   // w2
        cvtsi2ss xmm0, eax
        mulss xmm0, [OneOver255]
        shufps xmm0, xmm0, 0            // w2|w2|w2|w2

        movzx eax, byte ptr [ecx + 3]   // w3
        cvtsi2ss xmm3, eax
        mulss xmm3, [OneOver255]
        shufps xmm3, xmm3, 0            // w3|w3|w3|w3

        mov ecx, [Trans2]
        movaps xmm1, xmm0               // w2|w2|w2|w2
        mulps xmm1, [ecx]               // w2 * Trans2
        addps xmm4, xmm1

        movaps xmm1, xmm0               // w2|w2|w2|w2
        mulps xmm1, [ecx+16]
        addps xmm5, xmm1

        movaps xmm1, xmm0               // w2|w2|w2|w2
        mulps xmm1, [ecx+32]
        addps xmm6, xmm1

        mulps xmm0, [ecx+48]
        addps xmm7, xmm0

        mov ecx, [Trans3]
        movaps xmm1, xmm3               // w3|w3|w3|w3
        mulps xmm1, [ecx]               // w3 * Trans2
        addps xmm4, xmm1

        movaps xmm1, xmm3               // w3|w3|w3|w3
        mulps xmm1, [ecx+16]
        addps xmm5, xmm1

        movaps xmm1, xmm3               // w3|w3|w3|w3
        mulps xmm1, [ecx+32]
        addps xmm6, xmm1

        mulps xmm3, [ecx+48]
        addps xmm7, xmm3
    }
}

//
// Output: xmm0: Position transformed by xmm4,xmm5,xmm6,xmm7
//
// Scratch regs: xmm3
//
__inline void SseMultiplyPositionByXMMMatrix(float const *Position)
{
    __asm
    {
        // Position
        mov ecx, [Position]

        movss xmm0, [ecx]                               // xx|xx|xx|Px
        shufps xmm0, xmm0, 0                            // Px|Px|Px|Px
        mulps xmm0, xmm4                                // Px|Px|Px|Px * 03|02|01|00
        addps xmm0, xmm7

        movss xmm3, [ecx+4]                             // xx|xx|xx|Py
        shufps xmm3, xmm3, 0                            // Py|Py|Py|Py
        mulps xmm3, xmm5                                // Py|Py|Py|Py * 13|12|11|10
        addps xmm0, xmm3

        movss xmm3, [ecx+8]                             // xx|xx|xx|Pz
        shufps xmm3, xmm3, 0                            // Pz|Pz|Pz|Pz
        mulps xmm3, xmm6                                // Pz|Pz|Pz|Pz * 23|22|21|20
        addps xmm0, xmm3
    }
}

//
// Output: xmm0: Normal transformed by xmm4,xmm5,xmm6
//
// Scratch regs: xmm3
//
__inline void SseMultiplyNormalByXMMMatrix0(float const *Normal)
{
    __asm
    {
        // Normal
        mov ecx, [Normal]

        movss xmm0, [ecx]                               // xx|xx|xx|Nx
        shufps xmm0, xmm0, 0                            // Nx|Nx|Nx|Nx
        mulps xmm0, xmm4

        movss xmm3, [ecx+4]                             // xx|xx|xx|Ny
        shufps xmm3, xmm3, 0                            // Ny|Ny|Ny|Ny
        mulps xmm3, xmm5
        addps xmm0, xmm3

        movss xmm3, [ecx+8]                             // xx|xx|xx|Nz
        shufps xmm3, xmm3, 0                            // Nz|Nz|Nz|Nz
        mulps xmm3, xmm6
        addps xmm0, xmm3
    }
}

//
// Output: xmm1: Normal transformed by xmm4,xmm5,xmm6
//
// Scratch regs: xmm3
//
__inline void SseMultiplyNormalByXMMMatrix1(float const *Normal)
{
    __asm
    {
        // Normal
        mov ecx, [Normal]

        movss xmm1, [ecx]                               // xx|xx|xx|Nx
        shufps xmm1, xmm1, 0                            // Nx|Nx|Nx|Nx
        mulps xmm1, xmm4

        movss xmm3, [ecx+4]                             // xx|xx|xx|Ny
        shufps xmm3, xmm3, 0                            // Ny|Ny|Ny|Ny
        mulps xmm3, xmm5
        addps xmm1, xmm3

        movss xmm3, [ecx+8]                             // xx|xx|xx|Nz
        shufps xmm3, xmm3, 0                            // Nz|Nz|Nz|Nz
        mulps xmm3, xmm6
        addps xmm1, xmm3
    }
}

#define BONE_COUNT 0
#include "x86_granny_accelerated_deformer_wrapper.inl"

#define BONE_COUNT 1
#include "x86_granny_accelerated_deformer_wrapper.inl"

#define BONE_COUNT 2
#include "x86_granny_accelerated_deformer_wrapper.inl"

#define BONE_COUNT 4
#include "x86_granny_accelerated_deformer_wrapper.inl"

//=========================================================================
// SseDeformPNT332
//
// Source and Dest must be 16 byte aligned.
//=========================================================================
void
SseDeformPNT332(int32x Count, pnt332_vertex const *Source,
                pnt332_vertex *Dest, const float *Trans)
{
    __m128 row_10_12_11_10;
    __m128 row_20_22_21_20;
    __m128 row_0_32_31_30;
    __m128 row_02_01_02_01;
    __m128 row_12_11_12_11;
    __m128 row_22_21_22_21;

    __asm
    {
        mov ecx, dword ptr [Trans]

        movaps xmm4, xmmword ptr [ecx+00]  // 03|02|01|00
        movaps xmm5, xmmword ptr [ecx+16]  // 13|12|11|10
        movaps xmm6, xmmword ptr [ecx+32]  // 23|22|21|20
        movaps xmm1, xmmword ptr [ecx+48]  // 33|32|31|30
        movss xmm7, [ecx+56]               //  0| 0| 0|32

        shufps xmm4, xmm4, 0x24            // 00|02|01|00
        shufps xmm5, xmm5, 0x24            // 10|12|11|10
        shufps xmm6, xmm6, 0x24            // 20|22|21|20

        movaps xmm0, xmm4                  // 00|02|01|00
        movaps [row_10_12_11_10], xmm5
        movaps [row_20_22_21_20], xmm6

        shufps xmm0, xmm0, 0x99            // 02|01|02|01
        shufps xmm5, xmm5, 0x99            // 12|11|12|11
        shufps xmm6, xmm6, 0x99            // 22|21|22|21
        movlhps xmm1, xmm7                 //  0|32|31|30

        movaps [row_02_01_02_01], xmm0
        movaps [row_12_11_12_11], xmm5
        movaps [row_22_21_22_21], xmm6
        movaps [row_0_32_31_30], xmm1

        // Set up count of vertices
        mov edx, Count

        mov ecx, dword ptr [Source]
        mov edi, dword ptr [Dest]

        ALIGN 16

transform_loop:

        prefetchnta [ecx + 64 + 32]
        prefetchnta [ecx + 96 + 32]

        // Nx1, Pz1, Py1, Px1
        movaps xmm0, dword ptr [ecx]    // Nx|Pz|Py|Px
        movups xmm1, dword ptr [ecx+4]  // Ny|Nx|Pz|Py

        shufps xmm0, xmm0, 0xc0         // Nx|Px|Px|Px
        mulps xmm0, xmm4                // Nx*00|Px*02|Px*01|Px*00
        addps xmm0, [row_0_32_31_30]    // xmm0 += row_0_32_31_30

        movups xmm2, dword ptr [ecx+8]  // Nz|Ny|Nx|Pz

        shufps xmm1, xmm1, 0xc0         // Ny|Py|Py|Py
        mulps xmm1, [row_10_12_11_10]   // Ny*10|Py*12|Py*11|Py*10
        addps xmm0, xmm1                // xmm0 += xmm1

        movss xmm1, [ecx+12]            //   0|  0|  0|Nx1

        shufps xmm2, xmm2, 0xc0         // Nz|Pz|Pz|Pz
        mulps xmm2, [row_20_22_21_20]   // Nz*20|Pz*22|Pz*21|Pz*20
        addps xmm0, xmm2                // xmm0 += xmm2

        movss xmm2, [ecx+16]            //   0|  0|  0|Ny1

        // Ny2, Nz2, Ny1, Nz1
        shufps xmm1, [ecx+32], 0xf0     // Nx2|Nx2|Nx1|Nx1
        mulps xmm1, [row_02_01_02_01]   // Nx2*02|Nx2*01|Nx1*02|Nx1*01

        movss xmm3, [ecx+20]            //   0|  0|  0|Nz1

        shufps xmm2, [ecx+48], 0x00     // Ny2|Ny2|Ny1|Ny1
        mulps xmm2, [row_12_11_12_11]   // Ny2*12|Ny2*11|Ny1*12|Ny1*11
        addps xmm1, xmm2                // xmm1 += xmm2

        // Nx2, Pz2, Py2, Px2
        movaps xmm7, dword ptr [ecx+32] // Nx|Pz|Py|Px

        shufps xmm3, [ecx+48], 0x50     // Nz2|Nz2|Nz1|Nz1
        mulps xmm3, [row_22_21_22_21]   // Nz2*22|Nz2*21|Nz1*22|Nz1*21
        addps xmm1, xmm3                // xmm1 += xmm3

        movups xmm2, dword ptr [ecx+36] // Ny|Nx|Pz|Py

        shufps xmm7, xmm7, 0xc0         // Ny|Py|Py|Py
        mulps xmm7, xmm4                // Ny*00|Py*02|Py*01|Py*00
        addps xmm7, [row_0_32_31_30]    // xmm7 += row_0_32_31_30

        movups xmm3, dword ptr [ecx+40] // Nz|Ny|Nx|Pz

        shufps xmm2, xmm2, 0xc0         // Nz|Pz|Pz|Pz
        mulps xmm2, [row_10_12_11_10]   // Nz*10|Pz*12|Pz*11|Pz*10
        addps xmm7, xmm2                // xmm7 += xmm2

        movhlps xmm2, xmm1              // xx|xx|Nz2|Ny2
        movhps xmm1, [ecx + 24]         // Tv|Tu|Nz1|Ny1
        movhps xmm2, [ecx + 56]         // Tv|Tu|Nz1|Ny1

        shufps xmm3, xmm3, 0xc0         // Nz|Pz|Pz|Pz
        mulps xmm3, [row_20_22_21_20]   // Nz*20|Pz*22|Pz*21|Pz*20
        addps xmm7, xmm3                // xmm7 += xmm3

        movntps xmmword ptr [edi], xmm0     // Nx1, Pz1, Py1, Px1
        movntps xmmword ptr [edi+16], xmm1  // Tv,  Tu,  Nz1, Ny1
        movntps xmmword ptr [edi+32], xmm7  // Nx2, Pz2, Py2, Px2
        movntps xmmword ptr [edi+48], xmm2  // Tv,  Tu,  Nz2, Ny2

        add ecx, size pnt332_vertex*2;
        add edi, size pnt332_vertex*2;

        dec edx

        jnz transform_loop
    }
}

//=========================================================================
// SseDeformPNT332I
//
// SourceInit and DestInit must be 16 byte aligned.
//=========================================================================
void
SseDeformPNT332I(int32x Count, void const *SourceInit, void *DestInit,
                 int32x const *TransformTable, matrix_4x4 const *Transforms,
                 int32x TailCopy32Count, int32x SourceStride, int32x DestStride)
{
    Assert(TailCopy32Count == 0);

    if(IS_ALIGNED_16(Transforms) &&
       IS_ALIGNED_16(SourceInit) && IS_ALIGNED_16(DestInit))
    {
        pnt332_vertex const *Source = (pnt332_vertex const *)SourceInit;
        pnt332_vertex *Dest = (pnt332_vertex *)DestInit;

        int32x I = TransformTable[0];
        const float *Trans = (Transforms[I])[0];

        SseDeformPNT332(Count >> 1, Source, Dest, Trans);

        if(Count & 0x1)
        {
            // Handle odd vert.
            DeformPN33I(1, Source + Count - 1,
                        Dest + Count - 1,
                        TransformTable, Transforms, 2,
                        SourceStride, DestStride);
        }
    }
    else
    {
        LogSSEPunt(Transforms, SourceInit, DestInit);
        DeformPN33I(Count, SourceInit, DestInit,
                    TransformTable, Transforms, 2,
                    SourceStride, DestStride);
    }
}

//=========================================================================
// SseDeformPNT332D
//
// SourceInit and DestInit must be 16 byte aligned.
//=========================================================================
void
SseDeformPNT332D(int32x Count, void const *SourceInit, void *DestInit,
                 matrix_4x4 const *Transforms,
                 int32x TailCopy32Count, int32x SourceStride, int32x DestStride)
{
    Assert(TailCopy32Count == 0);

    if(IS_ALIGNED_16(Transforms) &&
       IS_ALIGNED_16(SourceInit) && IS_ALIGNED_16(DestInit))
    {
        pnt332_vertex const *Source = (pnt332_vertex const *)SourceInit;
        pnt332_vertex *Dest = (pnt332_vertex *)DestInit;
        const float *Trans = (Transforms[0])[0];

        SseDeformPNT332(Count >> 1, Source, Dest, Trans);

        if(Count & 0x1)
        {
            // Handle odd vert.
            DeformPN33D(1, Source + Count - 1,
                        Dest + Count - 1,
                        Transforms, 2,
                        SourceStride, DestStride);
        }
    }
    else
    {
        LogSSEPunt(Transforms, SourceInit, DestInit);
        DeformPN33D(Count, SourceInit, DestInit, Transforms, 2,
                    SourceStride, DestStride);
    }
}

void GRANNY
AddAcceleratedDeformers(void)
{
    if(SSEIsAvailable())
    {
        bone_deformer SSEDeformers[] =
        {
            {DeformPositionNormal,
             SseDeformPWNT3132D, SseDeformPWNT3132I, NULL,
             PWNT3132VertexType, PNT332VertexType, false, false},

            {DeformPositionNormalTangent,
             GeneratedSSEDeformPWNGT31332D, GeneratedSSEDeformPWNGT31332I, NULL,
             PWNGT31332VertexType, PNGT3332VertexType, false, false},

            {DeformPositionNormalTangentBinormal,
             GeneratedSSEDeformPWNGBT313332D, GeneratedSSEDeformPWNGBT313332I, NULL,
             PWNGBT313332VertexType, PNGBT33332VertexType, false, false},


            {DeformPositionNormal,
             SseDeformPWNT3232D, SseDeformPWNT3232I, NULL,
             PWNT3232VertexType, PNT332VertexType, false, false},

            {DeformPositionNormalTangent,
             GeneratedSSEDeformPWNGT32332D, GeneratedSSEDeformPWNGT32332I, NULL,
             PWNGT32332VertexType, PNGT3332VertexType, false, false},

            {DeformPositionNormalTangentBinormal,
             GeneratedSSEDeformPWNGBT323332D, GeneratedSSEDeformPWNGBT323332I, NULL,
             PWNGBT323332VertexType, PNGBT33332VertexType, false, false},


            {DeformPositionNormal,
             SseDeformPWNT3432D, SseDeformPWNT3432I, NULL,
             PWNT3432VertexType, PNT332VertexType, false, false},

            {DeformPositionNormalTangent,
             GeneratedSSEDeformPWNGT34332D, GeneratedSSEDeformPWNGT34332I, NULL,
             PWNGT34332VertexType, PNGT3332VertexType, false, false},

            {DeformPositionNormalTangent,
             GeneratedSSEDeformPWNGBT343332D, GeneratedSSEDeformPWNGBT343332I, NULL,
             PWNGBT343332VertexType, PNGBT33332VertexType, false, false},


            {DeformPositionNormal,
             SseDeformPNT332D, SseDeformPNT332I, NULL,
             PNT332VertexType, PNT332VertexType, false, false},

            {DeformPositionNormalTangent,
             GeneratedSSEDeformPNGT3332D, GeneratedSSEDeformPNGT3332I, NULL,
             PNGT3332VertexType, PNGT3332VertexType, false, false},

            {DeformPositionNormalTangentBinormal,
             GeneratedSSEDeformPNGBT33332D, GeneratedSSEDeformPNGBT33332I, NULL,
             PNGBT33332VertexType, PNGBT33332VertexType, false, false},
        };

        AddBoneDeformerTable(SizeOf(SSEDeformers) /
                             SizeOf(SSEDeformers[0]),
                             SSEDeformers);
    }
}
