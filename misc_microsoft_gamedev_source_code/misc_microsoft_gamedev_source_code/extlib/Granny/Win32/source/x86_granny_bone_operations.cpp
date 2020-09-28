// ========================================================================
// $File: //jeffr/granny/rt/x86/x86_granny_bone_operations.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #7 $
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

#if !defined(X86_GRANNY_CPU_QUERIES_H)
#include "x86_granny_cpu_queries.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

SSE_VERSION(BuildPositionOrientationWorldPoseComposite)
(real32 const *Position, real32 const *Orientation,
 real32 const *ParentMatrix, real32 const *InverseWorld4x4,
 real32 *Result, real32 * ResultWorldMatrix)
{
  real32 XsYs;

  static real32 onevalue = 1.0f;

  __asm
  {
    #define QM0 xmm0
    #define QM1 xmm1

    #define TEMP0 xmm2
    #define TEMP1 xmm3
    #define TEMP2 xmm4
    #define TEMP3 xmm6
    #define TEMP4 xmm7

    #define RCOL0 xmm5
    #define RCOL1 TEMP3 // don't use while TEMP3 is outstanding
    #define RCOL2 TEMP4 // don't use while TEMP4 is outstanding
    #define RCOL3 TEMP2 // don't use while TEMP2 is outstanding

    #define IW eax
    #define ORI eax

    #define WSM edi
    #define PM ecx
    #define POS esi

    #define R edx

    // Turn quaternion into 3x3 matrix

    mov      ORI,[ Orientation ]
    mov      PM, [ ParentMatrix ]
    mov      R, [ Result ]
    mov      WSM, [ ResultWorldMatrix ]
    movups   QM0, xmmword ptr [ ORI ]
    mov      IW, [ InverseWorld4x4 ]

    prefetchnta [ PM ]
    prefetcht0  [ R ]
    prefetcht0  [ WSM ]
    prefetchnta [ IW ]

    movaps   TEMP3, QM0
    movaps   TEMP0, QM0
    movaps   TEMP1, QM0
    movaps   TEMP2, QM0

    shufps   QM0, QM0, 144
    shufps   TEMP3, TEMP3, 72
    mulps    QM0, TEMP3

    shufps   TEMP0, TEMP0, 60
    mulps    TEMP0, TEMP1

    mulps    TEMP2, TEMP2

    addps    QM0, QM0
    addps    TEMP0, TEMP0
    addps    TEMP2, TEMP2

    movaps   QM1, QM0
    addps    QM0, TEMP0
    subps    QM1, TEMP0

    movaps   TEMP1, TEMP2
    movss    TEMP0, TEMP2
    movss    TEMP3, TEMP2
    movss    TEMP4, dword ptr [ onevalue ]
    shufps   TEMP2, TEMP2, 2
    shufps   TEMP1, TEMP2, 1

    movss    QM1, TEMP4
    movss    QM0, TEMP4

    addss    TEMP3, TEMP2 // X+Z
    addss    TEMP2, TEMP1 // Z+Y
    addss    TEMP0, TEMP1 // X+Y

    subss    QM0, TEMP3
    subss    QM1, TEMP2
    subss    TEMP4, TEMP0

    movss    dword ptr [XsYs], TEMP4

    // ok, at this point:
    // QM0 0 = XS+ZS, QM0 1 = XZ+WY, QM0 2 = XY+WZ, QM0 3 = YZ+WX
    // QM1 0 = YS+ZS, QM1 1 = XZ-WY, QM1 2 = XY-WZ, QM1 3 = YZ-WX
    // stack var XsYs = XS+YS


    // now start the A*B matrix multiply

    mov      POS, [ Position ]

    movss    TEMP0, QM1
    movaps   TEMP1, QM0
    movaps   TEMP2, QM1

    shufps   TEMP0, TEMP0, 0
    shufps   TEMP1, TEMP1, 170
    shufps   TEMP2, TEMP2, 85

    mulps    TEMP0, xmmword ptr [ PM ]
    mulps    TEMP1, xmmword ptr [ PM + 16 ]
    addps    TEMP0, TEMP1
    mulps    TEMP2, xmmword ptr [ PM + 32 ]
    addps    TEMP0, TEMP2
    movaps   xmmword ptr [ WSM ], TEMP0


    movaps   TEMP0, QM1
    movss    TEMP1, QM0
    movaps   TEMP2, QM0

    shufps   TEMP0, TEMP0, 170
    shufps   TEMP1, TEMP1, 0
    shufps   TEMP2, TEMP2, 255

    mulps    TEMP0, xmmword ptr [ PM ]
    mulps    TEMP1, xmmword ptr [ PM + 16 ]
    addps    TEMP0, TEMP1
    mulps    TEMP2, xmmword ptr [ PM + 32 ]
    addps    TEMP0, TEMP2
    movaps   xmmword ptr [ WSM + 16 ], TEMP0

    movss    TEMP2, dword ptr [ XsYs ]

    shufps   QM0, QM0, 85
    shufps   QM1, QM1, 255
    shufps   TEMP2, TEMP2, 0

    mulps    QM0, xmmword ptr [ PM ]
    mulps    QM1, xmmword ptr [ PM + 16 ]
    addps    QM0, QM1
    mulps    TEMP2, xmmword ptr [ PM + 32 ]
    addps    QM0, TEMP2
    movaps   xmmword ptr [ WSM + 32 ], QM0


    movss    TEMP0, dword ptr [ POS ]
    movss    TEMP1, dword ptr [ POS + 4 ]
    movss    TEMP2, dword ptr [ POS + 8 ]

    shufps   TEMP0, TEMP0, 0
    shufps   TEMP1, TEMP1, 0
    shufps   TEMP2, TEMP2, 0

    mulps    TEMP0, xmmword ptr [ PM ]
    mulps    TEMP1, xmmword ptr [ PM + 16 ]
    addps    TEMP0, TEMP1
    mulps    TEMP2, xmmword ptr [ PM + 32 ]
    addps    TEMP0, xmmword ptr [ PM + 48 ]
    addps    TEMP0, TEMP2
    movaps   xmmword ptr [ WSM + 48 ], TEMP0



    // now do the C * AB matrix mulitply
    movss    RCOL0, dword ptr [ IW ]
    movss    TEMP0, dword ptr [ IW + 4 ]
    movss    TEMP1, dword ptr [ IW + 8 ]

    shufps   RCOL0, RCOL0, 0
    shufps   TEMP0, TEMP0, 0
    shufps   TEMP1, TEMP1, 0

    mulps    RCOL0, xmmword ptr [ WSM ]
    mulps    TEMP0, xmmword ptr [ WSM + 16 ]
    addps    RCOL0, TEMP0
    mulps    TEMP1, xmmword ptr [ WSM + 32 ]
    addps    RCOL0, TEMP1


    movss    RCOL1, dword ptr [ IW + 16 ]
    movss    TEMP0, dword ptr [ IW + 16 + 4 ]
    movss    TEMP1, dword ptr [ IW + 16 + 8 ]

    shufps   RCOL1, RCOL1, 0
    shufps   TEMP0, TEMP0, 0
    shufps   TEMP1, TEMP1, 0

    mulps    RCOL1, xmmword ptr [ WSM ]
    mulps    TEMP0, xmmword ptr [ WSM + 16 ]
    addps    RCOL1, TEMP0
    mulps    TEMP1, xmmword ptr [ WSM + 32 ]
    addps    RCOL1, TEMP1


    movss    RCOL2, dword ptr [ IW + 32 ]
    movss    TEMP0, dword ptr [ IW + 32 + 4 ]
    movss    TEMP1, dword ptr [ IW + 32 + 8 ]

    shufps   RCOL2, RCOL2, 0
    shufps   TEMP0, TEMP0, 0
    shufps   TEMP1, TEMP1, 0

    mulps    RCOL2, xmmword ptr [ WSM ]
    mulps    TEMP0, xmmword ptr [ WSM + 16 ]
    addps    RCOL2, TEMP0
    mulps    TEMP1, xmmword ptr [ WSM + 32 ]
    addps    RCOL2, TEMP1


    movss    RCOL3, dword ptr [ IW + 48 ]
    movss    TEMP0, dword ptr [ IW + 48 + 4 ]
    movss    TEMP1, dword ptr [ IW + 48 + 8 ]

    shufps   RCOL3, RCOL3, 0
    shufps   TEMP0, TEMP0, 0
    shufps   TEMP1, TEMP1, 0

    mulps    RCOL3, xmmword ptr [ WSM ]
    mulps    TEMP0, xmmword ptr [ WSM + 16 ]
    addps    RCOL3, TEMP0
    mulps    TEMP1, xmmword ptr [ WSM + 32 ]
    addps    RCOL3, xmmword ptr [ WSM + 48 ]
    addps    RCOL3, TEMP1


    movaps   xmmword ptr [ R ], RCOL0
    movaps   xmmword ptr [ R + 16 ], RCOL1
    movaps   xmmword ptr [ R + 32 ], RCOL2
    movaps   xmmword ptr [ R + 48 ], RCOL3

    #undef QM0
    #undef QM1

    #undef TEMP0
    #undef TEMP1
    #undef TEMP2
    #undef TEMP3
    #undef TEMP4

    #undef RCOL0
    #undef RCOL1
    #undef RCOL2
    #undef RCOL3

    #undef IW
    #undef ORI

    #undef WSM
    #undef PM
    #undef POS

    #undef R
  }
}

SSE_VERSION(BuildPositionWorldPoseComposite)
(real32 const *Position, real32 const *ParentMatrix,
 real32 const *InverseWorld4x4,
 real32 *Result, real32 *ResultWorldMatrix)
{
    real32 *WorldPtr = (real32 *)ResultWorldMatrix;
    WorldPtr[0] = ParentMatrix[0];
    WorldPtr[1] = ParentMatrix[1];
    WorldPtr[2] = ParentMatrix[2];
    WorldPtr[3] = ParentMatrix[3];
    WorldPtr[4] = ParentMatrix[4];
    WorldPtr[5] = ParentMatrix[5];
    WorldPtr[6] = ParentMatrix[6];
    WorldPtr[7] = ParentMatrix[7];
    WorldPtr[8] = ParentMatrix[8];
    WorldPtr[9] = ParentMatrix[9];
    WorldPtr[10] = ParentMatrix[10];
    WorldPtr[11] = ParentMatrix[11];
    WorldPtr[12] = (Position[0]*ParentMatrix[0] +
                    Position[1]*ParentMatrix[4] +
                    Position[2]*ParentMatrix[8] +
                    ParentMatrix[12]);
    WorldPtr[13] = (Position[0]*ParentMatrix[1] +
                    Position[1]*ParentMatrix[5] +
                    Position[2]*ParentMatrix[9] +
                    ParentMatrix[13]);
    WorldPtr[14] = (Position[0]*ParentMatrix[2] +
                    Position[1]*ParentMatrix[6] +
                    Position[2]*ParentMatrix[10] +
                    ParentMatrix[14]);
    WorldPtr[15] = ParentMatrix[15];

  __asm
  {
    #define QM0 xmm0
    #define QM1 xmm1

    #define TEMP0 xmm2
    #define TEMP1 xmm3
    #define TEMP2 xmm4
    #define TEMP3 xmm6
    #define TEMP4 xmm7

    #define RCOL0 xmm5
    #define RCOL1 TEMP3 // don't use while TEMP3 is outstanding
    #define RCOL2 TEMP4 // don't use while TEMP4 is outstanding
    #define RCOL3 TEMP2 // don't use while TEMP2 is outstanding

    #define IW eax

    #define WSM edi
    #define PM ecx
    #define POS esi

    #define R edx

    // Turn quaternion into 3x3 matrix

    mov      PM, [ ParentMatrix ]
    mov      R, [ Result ]
    mov      WSM, [ ResultWorldMatrix ]
    mov      IW, [ InverseWorld4x4 ]

    prefetchnta [ PM ]
    prefetcht0  [ R ]
    prefetcht0  [ WSM ]
    prefetchnta [ IW ]

    // WSM - World space result
    // IW - Inverse world

    // now do the C * AB matrix mulitply
    movss    RCOL0, dword ptr [ IW ]
    movss    TEMP0, dword ptr [ IW + 4 ]
    movss    TEMP1, dword ptr [ IW + 8 ]

    shufps   RCOL0, RCOL0, 0
    shufps   TEMP0, TEMP0, 0
    shufps   TEMP1, TEMP1, 0

    mulps    RCOL0, xmmword ptr [ WSM ]
    mulps    TEMP0, xmmword ptr [ WSM + 16 ]
    addps    RCOL0, TEMP0
    mulps    TEMP1, xmmword ptr [ WSM + 32 ]
    addps    RCOL0, TEMP1


    movss    RCOL1, dword ptr [ IW + 16 ]
    movss    TEMP0, dword ptr [ IW + 16 + 4 ]
    movss    TEMP1, dword ptr [ IW + 16 + 8 ]

    shufps   RCOL1, RCOL1, 0
    shufps   TEMP0, TEMP0, 0
    shufps   TEMP1, TEMP1, 0

    mulps    RCOL1, xmmword ptr [ WSM ]
    mulps    TEMP0, xmmword ptr [ WSM + 16 ]
    addps    RCOL1, TEMP0
    mulps    TEMP1, xmmword ptr [ WSM + 32 ]
    addps    RCOL1, TEMP1


    movss    RCOL2, dword ptr [ IW + 32 ]
    movss    TEMP0, dword ptr [ IW + 32 + 4 ]
    movss    TEMP1, dword ptr [ IW + 32 + 8 ]

    shufps   RCOL2, RCOL2, 0
    shufps   TEMP0, TEMP0, 0
    shufps   TEMP1, TEMP1, 0

    mulps    RCOL2, xmmword ptr [ WSM ]
    mulps    TEMP0, xmmword ptr [ WSM + 16 ]
    addps    RCOL2, TEMP0
    mulps    TEMP1, xmmword ptr [ WSM + 32 ]
    addps    RCOL2, TEMP1


    movss    RCOL3, dword ptr [ IW + 48 ]
    movss    TEMP0, dword ptr [ IW + 48 + 4 ]
    movss    TEMP1, dword ptr [ IW + 48 + 8 ]

    shufps   RCOL3, RCOL3, 0
    shufps   TEMP0, TEMP0, 0
    shufps   TEMP1, TEMP1, 0

    mulps    RCOL3, xmmword ptr [ WSM ]
    mulps    TEMP0, xmmword ptr [ WSM + 16 ]
    addps    RCOL3, TEMP0
    mulps    TEMP1, xmmword ptr [ WSM + 32 ]
    addps    RCOL3, xmmword ptr [ WSM + 48 ]
    addps    RCOL3, TEMP1


    movaps   xmmword ptr [ R ], RCOL0
    movaps   xmmword ptr [ R + 16 ], RCOL1
    movaps   xmmword ptr [ R + 32 ], RCOL2
    movaps   xmmword ptr [ R + 48 ], RCOL3

    #undef QM0
    #undef QM1

    #undef TEMP0
    #undef TEMP1
    #undef TEMP2
    #undef TEMP3
    #undef TEMP4

    #undef RCOL0
    #undef RCOL1
    #undef RCOL2
    #undef RCOL3

    #undef IW
    #undef ORI

    #undef WSM
    #undef PM
    #undef POS

    #undef R
  }
}

SSE_VERSION(BuildIdentityWorldPoseComposite)
(real32 const *ParentMatrix,
 real32 const *InverseWorld4x4,
 real32 *Result, real32 *ResultWorldMatrix)
{
    real32 *WorldPtr = (real32 *)ResultWorldMatrix;
    WorldPtr[0] = ParentMatrix[0];
    WorldPtr[1] = ParentMatrix[1];
    WorldPtr[2] = ParentMatrix[2];
    WorldPtr[3] = ParentMatrix[3];
    WorldPtr[4] = ParentMatrix[4];
    WorldPtr[5] = ParentMatrix[5];
    WorldPtr[6] = ParentMatrix[6];
    WorldPtr[7] = ParentMatrix[7];
    WorldPtr[8] = ParentMatrix[8];
    WorldPtr[9] = ParentMatrix[9];
    WorldPtr[10] = ParentMatrix[10];
    WorldPtr[11] = ParentMatrix[11];
    WorldPtr[12] = ParentMatrix[12];
    WorldPtr[13] = ParentMatrix[13];
    WorldPtr[14] = ParentMatrix[14];
    WorldPtr[15] = ParentMatrix[15];

  __asm
  {
    #define QM0 xmm0
    #define QM1 xmm1

    #define TEMP0 xmm2
    #define TEMP1 xmm3
    #define TEMP2 xmm4
    #define TEMP3 xmm6
    #define TEMP4 xmm7

    #define RCOL0 xmm5
    #define RCOL1 TEMP3 // don't use while TEMP3 is outstanding
    #define RCOL2 TEMP4 // don't use while TEMP4 is outstanding
    #define RCOL3 TEMP2 // don't use while TEMP2 is outstanding

    #define IW eax

    #define WSM edi
    #define PM ecx
    #define POS esi

    #define R edx

    // Turn quaternion into 3x3 matrix

    mov      PM, [ ParentMatrix ]
    mov      R, [ Result ]
    mov      WSM, [ ResultWorldMatrix ]
    mov      IW, [ InverseWorld4x4 ]

    prefetchnta [ PM ]
    prefetcht0  [ R ]
    prefetcht0  [ WSM ]
    prefetchnta [ IW ]

    // WSM - World space result
    // IW - Inverse world

    // now do the C * AB matrix mulitply
    movss    RCOL0, dword ptr [ IW ]
    movss    TEMP0, dword ptr [ IW + 4 ]
    movss    TEMP1, dword ptr [ IW + 8 ]

    shufps   RCOL0, RCOL0, 0
    shufps   TEMP0, TEMP0, 0
    shufps   TEMP1, TEMP1, 0

    mulps    RCOL0, xmmword ptr [ WSM ]
    mulps    TEMP0, xmmword ptr [ WSM + 16 ]
    addps    RCOL0, TEMP0
    mulps    TEMP1, xmmword ptr [ WSM + 32 ]
    addps    RCOL0, TEMP1


    movss    RCOL1, dword ptr [ IW + 16 ]
    movss    TEMP0, dword ptr [ IW + 16 + 4 ]
    movss    TEMP1, dword ptr [ IW + 16 + 8 ]

    shufps   RCOL1, RCOL1, 0
    shufps   TEMP0, TEMP0, 0
    shufps   TEMP1, TEMP1, 0

    mulps    RCOL1, xmmword ptr [ WSM ]
    mulps    TEMP0, xmmword ptr [ WSM + 16 ]
    addps    RCOL1, TEMP0
    mulps    TEMP1, xmmword ptr [ WSM + 32 ]
    addps    RCOL1, TEMP1


    movss    RCOL2, dword ptr [ IW + 32 ]
    movss    TEMP0, dword ptr [ IW + 32 + 4 ]
    movss    TEMP1, dword ptr [ IW + 32 + 8 ]

    shufps   RCOL2, RCOL2, 0
    shufps   TEMP0, TEMP0, 0
    shufps   TEMP1, TEMP1, 0

    mulps    RCOL2, xmmword ptr [ WSM ]
    mulps    TEMP0, xmmword ptr [ WSM + 16 ]
    addps    RCOL2, TEMP0
    mulps    TEMP1, xmmword ptr [ WSM + 32 ]
    addps    RCOL2, TEMP1


    movss    RCOL3, dword ptr [ IW + 48 ]
    movss    TEMP0, dword ptr [ IW + 48 + 4 ]
    movss    TEMP1, dword ptr [ IW + 48 + 8 ]

    shufps   RCOL3, RCOL3, 0
    shufps   TEMP0, TEMP0, 0
    shufps   TEMP1, TEMP1, 0

    mulps    RCOL3, xmmword ptr [ WSM ]
    mulps    TEMP0, xmmword ptr [ WSM + 16 ]
    addps    RCOL3, TEMP0
    mulps    TEMP1, xmmword ptr [ WSM + 32 ]
    addps    RCOL3, xmmword ptr [ WSM + 48 ]
    addps    RCOL3, TEMP1


    movaps   xmmword ptr [ R ], RCOL0
    movaps   xmmword ptr [ R + 16 ], RCOL1
    movaps   xmmword ptr [ R + 32 ], RCOL2
    movaps   xmmword ptr [ R + 48 ], RCOL3

    #undef QM0
    #undef QM1

    #undef TEMP0
    #undef TEMP1
    #undef TEMP2
    #undef TEMP3
    #undef TEMP4

    #undef RCOL0
    #undef RCOL1
    #undef RCOL2
    #undef RCOL3

    #undef IW
    #undef ORI

    #undef WSM
    #undef PM
    #undef POS

    #undef R
  }
}

SSE_VERSION(BuildFullWorldPoseComposite)
(transform const &Transform,
 real32 const *ParentMatrix, real32 const *InverseWorld4x4,
 real32 *Result, real32 * ResultWorldMatrix)
{
    matrix_4x4 Temp;
    BuildCompositeTransform4x4(Transform, (real32 *)Temp);
    ColumnMatrixMultiply4x3(ResultWorldMatrix, (real32 *)Temp, ParentMatrix);
    ColumnMatrixMultiply4x3(Result, InverseWorld4x4, ResultWorldMatrix);
}

#if !FORCE_SSE_VERSION

static void
SetPointers(void)
{
    if(SSEIsAvailable())
    {
        BuildIdentityWorldPoseComposite = BuildIdentityWorldPoseComposite_SSE;
        BuildPositionWorldPoseComposite = BuildPositionWorldPoseComposite_SSE;
        BuildPositionOrientationWorldPoseComposite = BuildPositionOrientationWorldPoseComposite_SSE;
        BuildFullWorldPoseComposite = BuildFullWorldPoseComposite_SSE;
    }
    else
    {
        BuildIdentityWorldPoseComposite = BuildIdentityWorldPoseComposite_Generic;
        BuildPositionWorldPoseComposite = BuildPositionWorldPoseComposite_Generic;
        BuildPositionOrientationWorldPoseComposite = BuildPositionOrientationWorldPoseComposite_Generic;
        BuildFullWorldPoseComposite = BuildFullWorldPoseComposite_Generic;
    }

    BuildIdentityWorldPoseOnly = BuildIdentityWorldPoseOnly_Generic;
    BuildPositionWorldPoseOnly = BuildPositionWorldPoseOnly_Generic;
    BuildPositionOrientationWorldPoseOnly = BuildPositionOrientationWorldPoseOnly_Generic;
    BuildFullWorldPoseOnly = BuildFullWorldPoseOnly_Generic;

    BuildSingleCompositeFromWorldPose = BuildSingleCompositeFromWorldPose_Generic;
    BuildSingleCompositeFromWorldPoseTranspose = BuildSingleCompositeFromWorldPoseTranspose_Generic;
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
    BuildFullWorldPoseOnly(Transform, ParentMatrix, ResultWorldMatrix);
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


#define STUB_POINTER(Name) \
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
#endif
