// ========================================================================
// $File: //jeffr/granny/rt/ppc_xenon/ppc_xenon_granny_bone_operations.cpp $
// $DateTime: 2007/10/24 13:14:48 $
// $Change: 16360 $
// $Revision: #9 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_BONE_OPERATIONS_H)
#include "granny_bone_operations.h"
#endif

#if !defined(GRANNY_TRANSFORM_H)
#include "granny_transform.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(XENON_GRANNY_XTL_H)
#include "xenon_granny_xtl.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_SKELETON_H)
#include "granny_skeleton.h"
#endif

#include <xboxmath.h>

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#define USE_ACCELERATED_BONE_OPS

USING_GRANNY_NAMESPACE;

#define DECLARE_UNALIASED(Name, type) type* NOALIAS Name = Name ## Aliased

// =====================================================================
// Factored out code chunks to be combined for different alignments

#define DECLARE_VARS                                        \
    XMVECTOR AR0, AR1, AR2, AR3;                            \
    XMVECTOR BR0, BR1, BR2, BR3;                            \
    XMVECTOR Temp0, Temp1, Temp2;                           \
    XMVECTOR ResultCol0, ResultCol1, ResultCol2, ResultCol3

#define UNALIGNED_LOAD(VecPrefix, Matrix, Temp)     \
    VecPrefix##0     = __lvlx(Matrix, 0);           \
    Temp   = __lvrx(Matrix, 16);                    \
    VecPrefix##0     = __vor(VecPrefix##0, Temp);   \
    VecPrefix##1     = __lvlx(Matrix, 16);          \
    Temp   = __lvrx(Matrix, 32);                    \
    VecPrefix##1     = __vor(VecPrefix##1, Temp);   \
    VecPrefix##2     = __lvlx(Matrix, 32);          \
    Temp   = __lvrx(Matrix, 48);                    \
    VecPrefix##2     = __vor(VecPrefix##2, Temp);   \
    VecPrefix##3     = __lvlx(Matrix, 48);          \
    Temp   = __lvrx(Matrix, 64);                    \
    VecPrefix##3     = __vor(VecPrefix##3, Temp)

#define ALIGNED_LOAD(VecPrefix, Matrix)         \
    VecPrefix##0 = __lvx(Matrix, 0);            \
    VecPrefix##1 = __lvx(Matrix, 16);           \
    VecPrefix##2 = __lvx(Matrix, 32);           \
    VecPrefix##3 = __lvx(Matrix, 48)

#define MATRIX_MUL                                  \
    Temp0 = __vspltw(AR0, 0);                       \
    Temp1 = __vspltw(AR0, 1);                       \
    Temp2 = __vspltw(AR0, 2);                       \
    ResultCol0 = __vmulfp(Temp0,  BR0);             \
    ResultCol0 = __vmaddfp(Temp1, BR1, ResultCol0); \
    ResultCol0 = __vmaddfp(Temp2, BR2, ResultCol0); \
    /* -- Col1 */                                   \
    Temp0 = __vspltw(AR1, 0);                       \
    Temp1 = __vspltw(AR1, 1);                       \
    Temp2 = __vspltw(AR1, 2);                       \
    ResultCol1 = __vmulfp(Temp0, BR0);              \
    ResultCol1 = __vmaddfp(Temp1, BR1, ResultCol1); \
    ResultCol1 = __vmaddfp(Temp2, BR2, ResultCol1); \
    /* -- Col2 */                                   \
    Temp0 = __vspltw(AR2, 0);                       \
    Temp1 = __vspltw(AR2, 1);                       \
    Temp2 = __vspltw(AR2, 2);                       \
    ResultCol2 = __vmulfp(Temp0, BR0);              \
    ResultCol2 = __vmaddfp(Temp1, BR1, ResultCol2); \
    ResultCol2 = __vmaddfp(Temp2, BR2, ResultCol2); \
    /* -- Col3 */                                   \
    Temp0 = __vspltw(AR3, 0);                       \
    Temp1 = __vspltw(AR3, 1);                       \
    Temp2 = __vspltw(AR3, 2);                       \
    ResultCol3 = __vmulfp(Temp0, BR0);              \
    ResultCol3 = __vmaddfp(Temp1, BR1, ResultCol3); \
    ResultCol3 = __vmaddfp(Temp2, BR2, ResultCol3); \
    /* note the extra add here... */                \
    ResultCol3 = __vaddfp(ResultCol3, BR3)

#define ALIGNED_STORE(Result, VecPrefix)        \
    __stvx(VecPrefix##0, Result, 0);            \
    __stvx(VecPrefix##1, Result, 16);           \
    __stvx(VecPrefix##2, Result, 32);           \
    __stvx(VecPrefix##3, Result, 48)



// Partially aligned operands...
void XenonAligned101ColumnMatrix4x3Multiply(real32 * NOALIAS Result,  // aligned
                                            real32 const* NOALIAS A, // unaligned
                                            real32 const* NOALIAS B) // aligned
{
    Assert(IS_ALIGNED_16(B));
    Assert(IS_ALIGNED_16(Result));

    DECLARE_VARS;

    // Load the a/b matrices
    {
        UNALIGNED_LOAD(AR, A, Temp0);
        ALIGNED_LOAD(BR, B);
    }

    MATRIX_MUL;
    ALIGNED_STORE(Result, ResultCol);
}


// Partially aligned operands...
void XenonAligned110ColumnMatrix4x3Multiply(real32 * NOALIAS Result,  // aligned
                                            real32 const* NOALIAS A, // aligned
                                            real32 const* NOALIAS B) // unaligned
{
    Assert(IS_ALIGNED_16(B));
    Assert(IS_ALIGNED_16(Result));

    DECLARE_VARS;

    // Load the a/b matrices
    {
        ALIGNED_LOAD(AR, A);
        UNALIGNED_LOAD(BR, B, Temp0);
    }

    MATRIX_MUL;
    ALIGNED_STORE(Result, ResultCol);
}


void XenonAlignedColumnMatrix4x3Multiply(real32 * NOALIAS Result,
                                         real32 const* NOALIAS A,
                                         real32 const* NOALIAS B)
{
    Assert(IS_ALIGNED_16(A));
    Assert(IS_ALIGNED_16(B));
    Assert(IS_ALIGNED_16(Result));

    Assert(IS_ALIGNED_16(B));
    Assert(IS_ALIGNED_16(Result));

    DECLARE_VARS;

    // Load the a/b matrices
    {
        ALIGNED_LOAD(AR, A);
        ALIGNED_LOAD(BR, B);
    }

    MATRIX_MUL;
    ALIGNED_STORE(Result, ResultCol);
}


// Partially aligned operands, transposed...
void XenonAligned101ColumnMatrix4x3MultiplyTranspose(real32 * NOALIAS Result, // aligned
                                                     real32 const* NOALIAS A, // unaligned
                                                     real32 const* NOALIAS B) // aligned
{
    Assert(IS_ALIGNED_16(B));
    Assert(IS_ALIGNED_16(Result));
    Assert(!StructuresOverlap(*Result, *A));
    Assert(!StructuresOverlap(*Result, *B));

    XMVECTOR AR0, AR1, AR2, AR3;
    XMVECTOR BR0, BR1, BR2, BR3;
    XMVECTOR Temp0, Temp1, Temp2, Temp3;
    XMVECTOR ResultRow0, ResultRow1, ResultRow2;

    ALIGN16(matrix_4x4) TransposeA = {
        A[0],  A[4],  A[8],  A[12],
        A[1],  A[5],  A[9],  A[13],
        A[2],  A[6],  A[10], A[14],
        A[3],  A[7],  A[11], A[15],
    };

    XMDUMMY_INITIALIZE_VECTOR(Temp0);
    Temp0 = __vupkd3d(Temp0, VPACK_NORMSHORT2);
    XMVECTOR Constant0001 = __vpermwi(Temp0, 0xAB);

    // Load the a/b matrices
    {
        ALIGNED_LOAD(AR, TransposeA);
        ALIGNED_LOAD(BR, B);
    }

    Temp0 = __vspltw(BR0, 0);
    Temp1 = __vspltw(BR1, 0);
    Temp2 = __vspltw(BR2, 0);
    Temp3 = __vspltw(BR3, 0);
    ResultRow0 = __vmulfp(Temp0,  AR0);
    ResultRow0 = __vmaddfp(Temp1, AR1, ResultRow0);
    ResultRow0 = __vmaddfp(Temp2, AR2, ResultRow0);
    ResultRow0 = __vmaddfp(Constant0001, Temp3, ResultRow0);

    Temp0 = __vspltw(BR0, 1);
    Temp1 = __vspltw(BR1, 1);
    Temp2 = __vspltw(BR2, 1);
    Temp3 = __vspltw(BR3, 1);
    ResultRow1 = __vmulfp(Temp0,  AR0);
    ResultRow1 = __vmaddfp(Temp1, AR1, ResultRow1);
    ResultRow1 = __vmaddfp(Temp2, AR2, ResultRow1);
    ResultRow1 = __vmaddfp(Constant0001, Temp3, ResultRow1);

    Temp0 = __vspltw(BR0, 2);
    Temp1 = __vspltw(BR1, 2);
    Temp2 = __vspltw(BR2, 2);
    Temp3 = __vspltw(BR3, 2);
    ResultRow2 = __vmulfp(Temp0,  AR0);
    ResultRow2 = __vmaddfp(Temp1, AR1, ResultRow2);
    ResultRow2 = __vmaddfp(Temp2, AR2, ResultRow2);
    ResultRow2 = __vmaddfp(Constant0001, Temp3, ResultRow2);

    __stvx(ResultRow0, Result, 0);
    __stvx(ResultRow1, Result, 16);
    __stvx(ResultRow2, Result, 32);
}


XENON_VERSION(BuildPositionOrientationWorldPoseComposite)(real32 const* PositionAliased,
                                                          real32 const* OrientationAliased,
                                                          real32 const* ParentMatrixAliased,
                                                          real32 const* InverseWorld4x4Aliased,
                                                          real32*       ResultAliased,
                                                          real32*       ResultWorldMatrixAliased)
{
    DECLARE_UNALIASED(Position, real32 const);
    DECLARE_UNALIASED(Orientation, real32 const);
    DECLARE_UNALIASED(ParentMatrix, real32 const);
    DECLARE_UNALIASED(InverseWorld4x4, real32 const);
    DECLARE_UNALIASED(Result, real32);
    DECLARE_UNALIASED(ResultWorldMatrix, real32);

    // Position:          not aligned
    // Orientation:       not aligned
    // ParentMatrix:      align 16
    // InverseWorld4x4:   not aligned
    // Result:            align 16
    // ResultWorldMatrix: align 16
    Assert(IS_ALIGNED_16(ParentMatrix));
    Assert(IS_ALIGNED_16(Result));
    Assert(IS_ALIGNED_16(ResultWorldMatrix));

    // Convert Position and orientation to a Matrix (T) with
    //  R R R 0
    //  R R R 0
    //  R R R 0
    //  P P P 1
    // Where R is the 3x3 from the quaternion and P is the position
    //
    // ResultWorldMatrix = 4x3 Mult: (T * ParentMatrix)
    //
    // Result = 4x3 Mult: (IW * ResultWorldMatrix)

    // Nicked from xmmatrix.inl, with the addition of the unaligned
    // quat and pos loads.  We could probably just use the XM function
    // for quat->mat, we're really only nicking the 1110 constant and
    // inserting one extra maddfp.
    XMMATRIX PosOrient;
    {
        XMVECTOR Quaternion;
        XMVECTOR PosVector;
        XMVECTOR Q0, Q1;
        XMVECTOR V0, V1, V2;
        XMVECTOR R1, R2;
        XMVECTOR ZO;
        XMVECTOR Constant1110;

        XMDUMMY_INITIALIZE_VECTOR(ZO);

        // Unaligned quat load
        Quaternion = __lvlx(Orientation, 0);
        V0         = __lvrx(Orientation, 16);
        Quaternion = __vor(Quaternion, V0);

        // Unaligned pos load
        PosVector  = __lvlx(Position, 0);
        V1         = __lvrx(Position, 16);
        PosVector  = __vor(PosVector, V1);

        // Straight from xmmatrix.inl until the note below...
        Q0 = __vaddfp(Quaternion, Quaternion);
        Q1 = __vmulfp(Quaternion, Q0);

        ZO = __vupkd3d(ZO, VPACK_NORMSHORT2);
        Constant1110 = __vpermwi(ZO, 0xFE);

        V0 = __vpermwi(Q1, 0x40);
        V1 = __vpermwi(Q1, 0xA4);

        PosOrient.r[0] = __vsubfp(Constant1110, V0);
        PosOrient.r[0] = __vsubfp(PosOrient.r[0], V1);

        V0 = __vpermwi(Quaternion, 0x7);
        V1 = __vpermwi(Q0, 0x9B);
        V0 = __vmulfp(V0, V1);

        V1 = __vspltw(Quaternion, 3);
        V2 = __vpermwi(Q0, 0x63);
        V1 = __vmulfp(V1, V2);

        R1 = __vaddfp(V0, V1);
        R2 = __vsubfp(V0, V1);

        PosOrient.r[0] = __vrlimi(PosOrient.r[0], ZO, 1, 3);
        PosOrient.r[1] = __vpermwi(PosOrient.r[0], 0x7);

        V0 = __vpermwi(R1, 0x42);
        PosOrient.r[2] = __vsldoi(R2, R1, 2 << 2);
        V0 = __vrlimi(V0, R2, 0x6, 3);
        PosOrient.r[2] = __vpermwi(PosOrient.r[2], 0x88);

        // We need to account for the position, so this is slightly different...
        PosOrient.r[3] = __vpermwi(ZO, 0xAB);
        PosOrient.r[3] = __vmaddfp(PosVector, Constant1110, PosOrient.r[3]);

        PosOrient.r[2] = __vrlimi(PosOrient.r[2], PosOrient.r[0], 0x3, 0);
        PosOrient.r[1] = __vmrglw(V0, PosOrient.r[1]);
        PosOrient.r[0] = __vrlimi(PosOrient.r[0], V0, 0x6, 3);
    }

    // ResultWorldMatrix = 4x3 Mult: (T * ParentMatrix)
    XenonAlignedColumnMatrix4x3Multiply(ResultWorldMatrix,
                                        (real32 const*)&PosOrient.m[0][0],
                                        ParentMatrix);

    // Result = 4x3 Mult: (IW * ResultWorldMatrix)
    XenonAligned101ColumnMatrix4x3Multiply(Result,
                                           InverseWorld4x4,
                                           ResultWorldMatrix);
}


XENON_VERSION(BuildPositionWorldPoseComposite)(real32 const* PositionAliased,
                                               real32 const* ParentMatrixAliased,
                                               real32 const* InverseWorld4x4Aliased,
                                               real32*       ResultAliased,
                                               real32*       ResultWorldMatrixAliased)
{
    DECLARE_UNALIASED(Position, real32 const);
    DECLARE_UNALIASED(ParentMatrix, real32 const);
    DECLARE_UNALIASED(InverseWorld4x4, real32 const);
    DECLARE_UNALIASED(Result, real32);
    DECLARE_UNALIASED(ResultWorldMatrix, real32);

    // Position:          not aligned
    // ParentMatrix:      align 16
    // InverseWorld4x4:   not aligned
    // Result:            align 16
    // ResultWorldMatrix: align 16
    Assert(IS_ALIGNED_16(ParentMatrix));
    Assert(IS_ALIGNED_16(Result));
    Assert(IS_ALIGNED_16(ResultWorldMatrix));

    {for(int Idx = 0; Idx < 12; ++Idx)
    {
        ResultWorldMatrix[Idx] = ParentMatrix[Idx];
    }}

    ResultWorldMatrix[12] = (Position[0] * ParentMatrix[0] +
                             Position[1] * ParentMatrix[4] +
                             Position[2] * ParentMatrix[8] +
                             ParentMatrix[12]);
    ResultWorldMatrix[13] = (Position[0] * ParentMatrix[1] +
                             Position[1] * ParentMatrix[5] +
                             Position[2] * ParentMatrix[9] +
                             ParentMatrix[13]);
    ResultWorldMatrix[14] = (Position[0] * ParentMatrix[2] +
                             Position[1] * ParentMatrix[6] +
                             Position[2] * ParentMatrix[10] +
                             ParentMatrix[14]);
    ResultWorldMatrix[15] = ParentMatrix[15];

    XenonAligned101ColumnMatrix4x3Multiply(Result, InverseWorld4x4, ResultWorldMatrix);
}


XENON_VERSION(BuildIdentityWorldPoseComposite)(real32 const* ParentMatrixAliased,
                                               real32 const* InverseWorld4x4Aliased,
                                               real32*       ResultAliased,
                                               real32*       ResultWorldMatrixAliased)
{
    DECLARE_UNALIASED(ParentMatrix, real32 const);
    DECLARE_UNALIASED(InverseWorld4x4, real32 const);
    DECLARE_UNALIASED(Result, real32);
    DECLARE_UNALIASED(ResultWorldMatrix, real32);

    // ParentMatrix:      not aligned
    // InverseWorld4x4:   not aligned
    // Result:            align 16
    // ResultWorldMatrix: align 16
    Assert(IS_ALIGNED_16(ParentMatrix));
    Assert(IS_ALIGNED_16(Result));
    Assert(IS_ALIGNED_16(ResultWorldMatrix));

    {for(int Idx = 0; Idx < 16; ++Idx)
    {
        ResultWorldMatrix[Idx] = ParentMatrix[Idx];
    }}

    XenonAligned101ColumnMatrix4x3Multiply(Result, InverseWorld4x4, ResultWorldMatrix);
}


XENON_VERSION(BuildFullWorldPoseComposite)(transform const& Transform,
                                           real32 const*    ParentMatrixAliased,
                                           real32 const*    InverseWorld4x4Aliased,
                                           real32*          ResultAliased,
                                           real32*          ResultWorldMatrixAliased)
{
    DECLARE_UNALIASED(ParentMatrix, real32 const);
    DECLARE_UNALIASED(InverseWorld4x4, real32 const);
    DECLARE_UNALIASED(Result, real32);
    DECLARE_UNALIASED(ResultWorldMatrix, real32);

    // ParentMatrix:      not aligned
    // InverseWorld4x4:   not aligned
    // Result:            align 16
    // ResultWorldMatrix: align 16
    Assert(IS_ALIGNED_16(ParentMatrix));
    Assert(IS_ALIGNED_16(Result));
    Assert(IS_ALIGNED_16(ResultWorldMatrix));

    ALIGN16(matrix_4x4) Temp;
    Assert(IS_ALIGNED_16(&Temp));

    BuildCompositeTransform4x4(Transform, (real32 *)Temp);

    XenonAligned110ColumnMatrix4x3Multiply(ResultWorldMatrix, (real32 *)Temp, ParentMatrix);
    XenonAligned101ColumnMatrix4x3Multiply(Result, InverseWorld4x4, ResultWorldMatrix);
}


XENON_VERSION(BuildPositionOrientationWorldPoseOnly)(real32 const* PositionAliased,
                                                     real32 const* OrientationAliased,
                                                     real32 const* ParentMatrixAliased,
                                                     real32*       ResultWorldMatrixAliased)
{
    DECLARE_UNALIASED(Position, real32 const);
    DECLARE_UNALIASED(Orientation, real32 const);
    DECLARE_UNALIASED(ParentMatrix, real32 const);
    DECLARE_UNALIASED(ResultWorldMatrix, real32);

    // Position:          not aligned
    // Orientation:       not aligned
    // ParentMatrix:      align 16
    // ResultWorldMatrix: align 16
    Assert(IS_ALIGNED_16(ParentMatrix));
    Assert(IS_ALIGNED_16(ResultWorldMatrix));

    // Convert Position and orientation to a Matrix (T) with
    //  R R R 0
    //  R R R 0
    //  R R R 0
    //  P P P 1
    // Where R is the 3x3 from the quaternion and P is the position
    //
    // ResultWorldMatrix = 4x3 Mult: (T * ParentMatrix)

    // Nicked from xmmatrix.inl, with the addition of the unaligned
    // quat and pos loads.  We could probably just use the XM function
    // for quat->mat, we're really only nicking the 1110 constant and
    // inserting one extra maddfp.
    XMMATRIX PosOrient;
    {
        XMVECTOR Quaternion;
        XMVECTOR PosVector;
        XMVECTOR Q0, Q1;
        XMVECTOR V0, V1, V2;
        XMVECTOR R1, R2;
        XMVECTOR ZO;
        XMVECTOR Constant1110;

        XMDUMMY_INITIALIZE_VECTOR(ZO);

        // Unaligned quat load
        Quaternion = __lvlx(Orientation, 0);
        V0         = __lvrx(Orientation, 16);
        Quaternion = __vor(Quaternion, V0);

        // Unaligned pos load
        PosVector  = __lvlx(Position, 0);
        V1         = __lvrx(Position, 16);
        PosVector  = __vor(PosVector, V1);

        // Straight from xmmatrix.inl until the note below...
        Q0 = __vaddfp(Quaternion, Quaternion);
        Q1 = __vmulfp(Quaternion, Q0);

        ZO = __vupkd3d(ZO, VPACK_NORMSHORT2);
        Constant1110 = __vpermwi(ZO, 0xFE);

        V0 = __vpermwi(Q1, 0x40);
        V1 = __vpermwi(Q1, 0xA4);

        PosOrient.r[0] = __vsubfp(Constant1110, V0);
        PosOrient.r[0] = __vsubfp(PosOrient.r[0], V1);

        V0 = __vpermwi(Quaternion, 0x7);
        V1 = __vpermwi(Q0, 0x9B);
        V0 = __vmulfp(V0, V1);

        V1 = __vspltw(Quaternion, 3);
        V2 = __vpermwi(Q0, 0x63);
        V1 = __vmulfp(V1, V2);

        R1 = __vaddfp(V0, V1);
        R2 = __vsubfp(V0, V1);

        PosOrient.r[0] = __vrlimi(PosOrient.r[0], ZO, 1, 3);
        PosOrient.r[1] = __vpermwi(PosOrient.r[0], 0x7);

        V0 = __vpermwi(R1, 0x42);
        PosOrient.r[2] = __vsldoi(R2, R1, 2 << 2);
        V0 = __vrlimi(V0, R2, 0x6, 3);
        PosOrient.r[2] = __vpermwi(PosOrient.r[2], 0x88);

        // We need to account for the position, so this is slightly different...
        PosOrient.r[3] = __vpermwi(ZO, 0xAB);
        PosOrient.r[3] = __vmaddfp(PosVector, Constant1110, PosOrient.r[3]);

        PosOrient.r[2] = __vrlimi(PosOrient.r[2], PosOrient.r[0], 0x3, 0);
        PosOrient.r[1] = __vmrglw(V0, PosOrient.r[1]);
        PosOrient.r[0] = __vrlimi(PosOrient.r[0], V0, 0x6, 3);
    }

    // ResultWorldMatrix = 4x3 Mult: (T * ParentMatrix)
    XenonAlignedColumnMatrix4x3Multiply(ResultWorldMatrix,
                                        (real32 const*)&PosOrient.m[0][0],
                                        ParentMatrix);
}


XENON_VERSION(BuildPositionWorldPoseOnly)(real32 const* PositionAliased,
                                          real32 const* ParentMatrixAliased,
                                          real32*       ResultWorldMatrixAliased)
{
    DECLARE_UNALIASED(Position, real32 const);
    DECLARE_UNALIASED(ParentMatrix, real32 const);
    DECLARE_UNALIASED(ResultWorldMatrix, real32);

    // Position:          not aligned
    // ParentMatrix:      align 16
    // InverseWorld4x4:   not aligned
    // Result:            align 16
    // ResultWorldMatrix: align 16
    Assert(IS_ALIGNED_16(ParentMatrix));
    Assert(IS_ALIGNED_16(ResultWorldMatrix));

    {
        {for(int Idx = 0; Idx < 12; ++Idx)
        {
            ResultWorldMatrix[Idx] = ParentMatrix[Idx];
        }}

        ResultWorldMatrix[12] = (Position[0] * ParentMatrix[0] +
                                 Position[1] * ParentMatrix[4] +
                                 Position[2] * ParentMatrix[8] +
                                 ParentMatrix[12]);
        ResultWorldMatrix[13] = (Position[0] * ParentMatrix[1] +
                                 Position[1] * ParentMatrix[5] +
                                 Position[2] * ParentMatrix[9] +
                                 ParentMatrix[13]);
        ResultWorldMatrix[14] = (Position[0] * ParentMatrix[2] +
                                 Position[1] * ParentMatrix[6] +
                                 Position[2] * ParentMatrix[10] +
                                 ParentMatrix[14]);
        ResultWorldMatrix[15] = ParentMatrix[15];
    }

}


XENON_VERSION(BuildIdentityWorldPoseOnly)(real32 const* ParentMatrixAliased,
                                          real32*       ResultWorldMatrixAliased)
{
    DECLARE_UNALIASED(ParentMatrix, real32 const);
    DECLARE_UNALIASED(ResultWorldMatrix, real32);

    // ParentMatrix:      not aligned
    // InverseWorld4x4:   not aligned
    // Result:            align 16
    // ResultWorldMatrix: align 16
    Assert(IS_ALIGNED_16(ParentMatrix));
    Assert(IS_ALIGNED_16(ResultWorldMatrix));

    {for(int Idx = 0; Idx < 16; ++Idx)
    {
        ResultWorldMatrix[Idx] = ParentMatrix[Idx];
    }}
}


XENON_VERSION(BuildFullWorldPoseOnly)(transform const& Transform,
                                      real32 const*    ParentMatrixAliased,
                                      real32*          ResultWorldMatrixAliased)
{
    DECLARE_UNALIASED(ParentMatrix, real32 const);
    DECLARE_UNALIASED(ResultWorldMatrix, real32);

    // ParentMatrix:      not aligned
    // InverseWorld4x4:   not aligned
    // Result:            align 16
    // ResultWorldMatrix: align 16
    Assert(IS_ALIGNED_16(ParentMatrix));
    Assert(IS_ALIGNED_16(ResultWorldMatrix));

    ALIGN16(matrix_4x4) Temp;
    Assert(IS_ALIGNED_16(&Temp));

    BuildCompositeTransform4x4(Transform, (real32 *)Temp);

    XenonAligned110ColumnMatrix4x3Multiply(ResultWorldMatrix, (real32 *)Temp, ParentMatrix);
}


XENON_VERSION(BuildSingleCompositeFromWorldPose)(real32 const* InverseWorld4x4Aliased,
                                                 real32 const* WorldMatrixAliased,
                                                 real32*       ResultCompositeAliased)
{
    DECLARE_UNALIASED(InverseWorld4x4, real32 const);
    DECLARE_UNALIASED(WorldMatrix, real32 const);
    DECLARE_UNALIASED(ResultComposite, real32);

    Assert(IS_ALIGNED_16(WorldMatrixAliased));
    Assert(IS_ALIGNED_16(ResultCompositeAliased));

    XenonAligned101ColumnMatrix4x3Multiply(ResultComposite, InverseWorld4x4, WorldMatrix);
}

XENON_VERSION(BuildSingleCompositeFromWorldPoseTranspose)(real32 const* InverseWorld4x4Aliased,
                                                          real32 const* WorldMatrixAliased,
                                                          real32*       ResultCompositeAliased)
{
    DECLARE_UNALIASED(InverseWorld4x4, real32 const);
    DECLARE_UNALIASED(WorldMatrix, real32 const);
    DECLARE_UNALIASED(ResultComposite, real32);

    Assert(IS_ALIGNED_16(WorldMatrixAliased));
    Assert(IS_ALIGNED_16(ResultCompositeAliased));

    XenonAligned101ColumnMatrix4x3MultiplyTranspose(ResultComposite, InverseWorld4x4, WorldMatrix);
}


static void
SetPointers(void)
{
#ifdef USE_ACCELERATED_BONE_OPS

    BuildIdentityWorldPoseComposite = BuildIdentityWorldPoseComposite_Xenon;
    BuildPositionWorldPoseComposite = BuildPositionWorldPoseComposite_Xenon;
    BuildPositionOrientationWorldPoseComposite = BuildPositionOrientationWorldPoseComposite_Xenon;
    BuildFullWorldPoseComposite = BuildFullWorldPoseComposite_Xenon;

    BuildIdentityWorldPoseOnly = BuildIdentityWorldPoseOnly_Xenon;
    BuildPositionWorldPoseOnly = BuildPositionWorldPoseOnly_Xenon;
    BuildPositionOrientationWorldPoseOnly = BuildPositionOrientationWorldPoseOnly_Xenon;
    BuildFullWorldPoseOnly = BuildFullWorldPoseOnly_Xenon;

    BuildSingleCompositeFromWorldPose = BuildSingleCompositeFromWorldPose_Xenon;
    BuildSingleCompositeFromWorldPoseTranspose = BuildSingleCompositeFromWorldPoseTranspose_Xenon;

#else

    BuildIdentityWorldPoseComposite = BuildIdentityWorldPoseComposite_Generic;
    BuildPositionWorldPoseComposite = BuildPositionWorldPoseComposite_Generic;
    BuildPositionOrientationWorldPoseComposite = BuildPositionOrientationWorldPoseComposite_Generic;
    BuildFullWorldPoseComposite = BuildFullWorldPoseComposite_Generic;

    BuildIdentityWorldPoseOnly = BuildIdentityWorldPoseOnly_Generic;
    BuildPositionWorldPoseOnly = BuildPositionWorldPoseOnly_Generic;
    BuildPositionOrientationWorldPoseOnly = BuildPositionOrientationWorldPoseOnly_Generic;
    BuildFullWorldPoseOnly = BuildFullWorldPoseOnly_Generic;

    BuildSingleCompositeFromWorldPose = BuildSingleCompositeFromWorldPose_Generic;
    BuildSingleCompositeFromWorldPoseTranspose = BuildSingleCompositeFromWorldPoseTranspose_Generic;

#endif
}

static void
BuildIdentityWorldPoseComposite_Stub(real32 const *ParentMatrix,
                                     real32 const *InverseWorld4x4,
                                     real32 *Result,
                                     real32 *ResultWorldMatrix)
{
    SetPointers();
    BuildIdentityWorldPoseComposite(ParentMatrix,
                                    InverseWorld4x4,
                                    Result,
                                    ResultWorldMatrix);
}

static void
BuildPositionWorldPoseComposite_Stub(real32 const *Position,
                                     real32 const *ParentMatrix,
                                     real32 const *InverseWorld4x4,
                                     real32 *Result, real32 *ResultWorldMatrix)
{
    SetPointers();
    BuildPositionWorldPoseComposite(Position, ParentMatrix,
                                    InverseWorld4x4, Result,
                                    ResultWorldMatrix);
}

static void
BuildPositionOrientationWorldPoseComposite_Stub(real32 const *Position,
                                                real32 const *Orientation,
                                                real32 const *ParentMatrix,
                                                real32 const *InverseWorld4x4,
                                                real32 *Result,
                                                real32 * ResultWorldMatrix)
{
    SetPointers();
    BuildPositionOrientationWorldPoseComposite(Position,
                                               Orientation,
                                               ParentMatrix,
                                               InverseWorld4x4,
                                               Result,
                                               ResultWorldMatrix);
}

static void
BuildFullWorldPoseComposite_Stub(transform const &Transform,
                                 real32 const *ParentMatrix,
                                 real32 const *InverseWorld4x4,
                                 real32 *Result,
                                 real32 * ResultWorldMatrix)
{
    SetPointers();
    BuildFullWorldPoseComposite(Transform, ParentMatrix,
                                InverseWorld4x4, Result, ResultWorldMatrix);
}


static void
BuildIdentityWorldPoseOnly_Stub(real32 const *ParentMatrix,
                                real32 *ResultWorldMatrix)
{
    SetPointers();
    BuildIdentityWorldPoseOnly(ParentMatrix,
                               ResultWorldMatrix);
}

static void
BuildPositionWorldPoseOnly_Stub(real32 const *Position,
                                real32 const *ParentMatrix,
                                real32 *ResultWorldMatrix)
{
    SetPointers();
    BuildPositionWorldPoseOnly(Position, ParentMatrix,
                               ResultWorldMatrix);
}

static void
BuildPositionOrientationWorldPoseOnly_Stub(real32 const *Position,
                                           real32 const *Orientation,
                                           real32 const *ParentMatrix,
                                           real32 * ResultWorldMatrix)
{
    SetPointers();
    BuildPositionOrientationWorldPoseOnly(Position,
                                          Orientation,
                                          ParentMatrix,
                                          ResultWorldMatrix);
}

static void
BuildFullWorldPoseOnly_Stub(transform const &Transform,
                            real32 const *ParentMatrix,
                            real32 * ResultWorldMatrix)
{
    SetPointers();
    BuildFullWorldPoseOnly(Transform, ParentMatrix,
                           ResultWorldMatrix);
}

static void
BuildSingleCompositeFromWorldPose_Stub(real32 const *InverseWorld4x4,
                                       real32 const *WorldMatrix,
                                       real32 *ResultComposite)
{
    SetPointers();
    BuildSingleCompositeFromWorldPose(InverseWorld4x4,
                                      WorldMatrix,
                                      ResultComposite);
}

static void
BuildSingleCompositeFromWorldPoseTranspose_Stub(real32 const *InverseWorld4x4,
                                                real32 const *WorldMatrix,
                                                real32 *ResultComposite)
{
    SetPointers();
    BuildSingleCompositeFromWorldPoseTranspose(InverseWorld4x4,
                                               WorldMatrix,
                                               ResultComposite);
}


#define STUB_POINTER(Name)                                                  \
    OPTIMIZED_DISPATCH(Name) = (OPTIMIZED_DISPATCH_TYPE(Name) *)Name##_Stub

BEGIN_GRANNY_NAMESPACE;

STUB_POINTER(BuildIdentityWorldPoseComposite);
STUB_POINTER(BuildPositionWorldPoseComposite);
STUB_POINTER(BuildPositionOrientationWorldPoseComposite);
STUB_POINTER(BuildFullWorldPoseComposite);

STUB_POINTER(BuildIdentityWorldPoseOnly);
STUB_POINTER(BuildPositionWorldPoseOnly);
STUB_POINTER(BuildPositionOrientationWorldPoseOnly);
STUB_POINTER(BuildFullWorldPoseOnly);

STUB_POINTER(BuildSingleCompositeFromWorldPose);
STUB_POINTER(BuildSingleCompositeFromWorldPoseTranspose);

END_GRANNY_NAMESPACE;
