// ========================================================================
// $File: //jeffr/granny/rt/granny_periodic_loop.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_PERIODIC_LOOP_H)
#include "granny_periodic_loop.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

BEGIN_GRANNY_NAMESPACE;

data_type_definition PeriodicLoopType[] =
{
    {Real32Member, "Radius"},
    {Real32Member, "dAngle"},
    {Real32Member, "dZ"},

    {Real32Member, "BasisX", 0, 3},
    {Real32Member, "BasisY", 0, 3},
    {Real32Member, "Axis", 0, 3},

    {EndMember},
};

END_GRANNY_NAMESPACE;

void GRANNY
ZeroPeriodicLoop(periodic_loop &Loop)
{
    SetUInt8(SizeOf(Loop), 0, &Loop);
}

#if COMPILER_MSVC
#pragma optimize("", off)
#endif
void GRANNY
FitPeriodicLoop(real32 const *StartPosition, real32 const *StartOrientation,
                real32 const *EndPosition, real32 const *EndOrientation,
                real32 Seconds, periodic_loop &Loop)
{
    quad dOrientation;

    VectorEquals4(dOrientation, StartOrientation);
    Conjugate4(dOrientation);

    real32 InvStart[9];
    MatrixEqualsQuaternion3x3(InvStart, dOrientation);
    QuaternionMultiply4(dOrientation, dOrientation, EndOrientation);

    VectorEquals3(Loop.Axis, dOrientation);
    Normalize3(Loop.Axis);

    triple Basis;
    VectorSubtract3(Basis, EndPosition, StartPosition);
    VectorTransform3(Basis, InvStart);
    Loop.dZ = InnerProduct3(Basis, Loop.Axis);
    ScaleVectorAdd3(Basis,
                    -Loop.dZ,
                    Loop.Axis);

    Loop.dAngle =
        2.0f * (real32)(IntrinsicATan2(VectorLength3(dOrientation), dOrientation[3]));

    real32 Distance = VectorLengthSquared3(Basis);
    real32 Divisor = 2.0f * (1.0f - Cos(Loop.dAngle));
    if(Divisor > 0.0f)
    {
        Loop.Radius = SquareRoot(Distance / Divisor);
    }
    else
    {
        Loop.Radius = 0;
    }

    if(Loop.Radius > 0.0001f)
    {
        triple Center;
        VectorEquals3(Center, Basis);
        VectorScale3(Center, 0.5f);

        VectorEquals3(Loop.BasisY, Basis);
        Normalize3(Loop.BasisY);

        VectorCrossProduct3(Loop.BasisX, Loop.BasisY, Loop.Axis);
        Normalize3(Loop.BasisX);

        // TODO: Yes, it's another arbitrary epsilon.  Yay.
        if(AbsoluteValue(Loop.dAngle - Pi32) > 0.00001f)
        {
            real32 Sign = -1.0f;
            if(AbsoluteValue(Loop.dAngle) > Pi32)
            {
                Sign = 1.0f;
            }
            real32 BasisLength = VectorLengthSquared3(Basis);
            real32 SquareTerm = Square(Loop.Radius) - 0.25f*BasisLength;
            ScaleVectorAdd3(Center, Sign * SquareRoot(SquareTerm), Loop.BasisX);
        }

        triple ZeroArm;
        VectorSubtract3(ZeroArm, GlobalZeroVector, Center);

        VectorEquals3(Loop.BasisX, ZeroArm);
        Normalize3(Loop.BasisX);
        VectorCrossProduct3(Loop.BasisY, Loop.Axis, Loop.BasisX);
        Normalize3(Loop.BasisY);
    }
    else
    {
        VectorZero3(Loop.BasisX);
        VectorZero3(Loop.BasisY);
    }

    if(Seconds > 0.0f)
    {
        Loop.dAngle /= Seconds;
        Loop.dZ /= Seconds;
    }
}
#if COMPILER_MSVC
#pragma optimize("", on)
#endif

void GRANNY
ComputePeriodicLoopVector(periodic_loop const &Loop,
                          real32 Seconds,
                          real32 *Result)
{
    VectorScale3(Result, Loop.dZ * Seconds, Loop.Axis);
    ScaleVectorAdd3(Result, Loop.Radius*(Cos(Loop.dAngle * Seconds) - 1.0f),
                    Loop.BasisX);
    ScaleVectorAdd3(Result, Loop.Radius*Sin(Loop.dAngle * Seconds),
                    Loop.BasisY);
}

void GRANNY
ComputePeriodicLoopLog(periodic_loop const &Loop,
                       real32 Seconds,
                       real32 *Result)
{
    VectorScale3(Result, Loop.dAngle * Seconds, Loop.Axis);
}

void GRANNY
StepPeriodicLoop(periodic_loop const &Loop, real32 Seconds,
                 real32 *Point,
                 real32 *Orientation)
{
    triple Movement;
    ComputePeriodicLoopVector(Loop, Seconds, Movement);
    NormalQuaternionTransform3(Movement, Orientation);
    VectorAdd3(Point, Movement);

    quad dOrientation;
    ConstructQuaternion4(dOrientation, Loop.Axis,
                         Loop.dAngle * Seconds);
    QuaternionMultiply4(Orientation, Orientation, dOrientation);
}
