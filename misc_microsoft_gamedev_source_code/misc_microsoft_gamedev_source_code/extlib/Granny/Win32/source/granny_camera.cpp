// ========================================================================
// $File: //jeffr/granny/rt/granny_camera.cpp $
// $DateTime: 2007/11/16 11:44:14 $
// $Change: 16564 $
// $Revision: #21 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_CAMERA_H)
#include "granny_camera.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_UNITS_H)
#include "granny_units.h"
#endif

#if !defined(GRANNY_FLOATS_H)
#include "granny_floats.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

void GRANNY
InitializeDefaultCamera(camera &Camera)
{
    Camera.WpOverHp = NTSCTelevisionPhysicalAspectRatio;
    Camera.WrOverHr = NTSCTelevisionPhysicalAspectRatio;
    Camera.WwOverHw = 1.0f;

    Camera.FOVY = RadiansFromDegrees(60.0f);
    Camera.NearClipPlane = 4.0f;
    Camera.FarClipPlane = 1000.0f;

    Camera.OutputZRange = CameraOutputZNegativeOneToOne;
    Camera.ZRangeEpsilon = 0.0f;

    Camera.UseQuaternionOrientation = true;

    VectorZero3(Camera.Position);
    VectorEquals4(Camera.Orientation, GlobalWAxis);
    MatrixIdentity3x3(Camera.OrientationMatrix);
    VectorZero3(Camera.ElevAzimRoll);
    VectorZero3(Camera.Offset);

    BuildCameraMatrices(Camera);
}

void GRANNY
SetCameraAspectRatios(camera &Camera,
                      real32 PhysicalAspectRatio,
                      real32 ScreenWidth, real32 ScreenHeight,
                      real32 WindowWidth, real32 WindowHeight)
{
    Camera.WpOverHp = PhysicalAspectRatio;
    Camera.WrOverHr = ScreenWidth / ScreenHeight;
    Camera.WwOverHw = WindowWidth / WindowHeight;
}

void GRANNY
MoveCameraRelative(camera &Camera, real32 X, real32 Y, real32 Z)
{
    ScaleVectorAdd3(Camera.Position, X, Camera.InverseView4x4[0]);
    ScaleVectorAdd3(Camera.Position, Y, Camera.InverseView4x4[1]);
    ScaleVectorAdd3(Camera.Position, Z, Camera.InverseView4x4[2]);
}

static void
BuildProjectionMatrix(camera &Camera, real32 *Matrix, real32 *InverseMatrix)
{
    real32 TanHalfFOVY = Tan(0.5f * Camera.FOVY);

    Matrix[0] = (Camera.WrOverHr /
                 (TanHalfFOVY * Camera.WwOverHw * Camera.WpOverHp));
    Matrix[1] = 0.0f;
    Matrix[2] = 0.0f;
    Matrix[3] = 0.0f;

    Matrix[4] = 0.0f;
    Matrix[5] = 1.0f / (TanHalfFOVY);
    Matrix[6] = 0.0f;
    Matrix[7] = 0.0f;

    Matrix[8] = 0.0f;
    Matrix[9] = 0.0f;
    Matrix[11] = -1.0f;

    Matrix[12] = 0.0f;
    Matrix[13] = 0.0f;
    Matrix[15] = 0.0f;

    real32 NearPlusFar = Camera.NearClipPlane + Camera.FarClipPlane;
    real32 NearMinusFar = Camera.NearClipPlane - Camera.FarClipPlane;
    real32 NearTimesFar = Camera.NearClipPlane * Camera.FarClipPlane;

    if(Camera.OutputZRange == CameraOutputZNegativeOneToOne)
    {
        if(Camera.FarClipPlane == InfiniteFarClipPlane)
        {
            Matrix[10] = (Camera.ZRangeEpsilon - 1.0f);
            Matrix[14] = (Camera.ZRangeEpsilon - 2.0f) * Camera.NearClipPlane;
        }
        else
        {
            Matrix[10] = NearPlusFar / NearMinusFar;
            Matrix[14] = 2.0f * NearTimesFar / NearMinusFar;
        }
    }
    else if (Camera.OutputZRange == CameraOutputZNegativeOneToZero)
    {
        if(Camera.FarClipPlane == InfiniteFarClipPlane)
        {
            InvalidCodePath("NYI");
        }
        else
        {
            Matrix[10] = Camera.NearClipPlane / (NearMinusFar);
            Matrix[14] = NearTimesFar / NearMinusFar;
        }
    }
    else
    {
        if(Camera.FarClipPlane == InfiniteFarClipPlane)
        {
            Matrix[10] = (Camera.ZRangeEpsilon - 1.0f);
            Matrix[14] = (Camera.ZRangeEpsilon - 1.0f) * Camera.NearClipPlane;
        }
        else
        {
            Matrix[10] = Camera.FarClipPlane / NearMinusFar;
            Matrix[14] = NearTimesFar / NearMinusFar;
        }
    }

    InverseMatrix[0] = 1.0f / Matrix[0];
    InverseMatrix[1] = 0.0f;
    InverseMatrix[2] = 0.0f;
    InverseMatrix[3] = 0.0f;

    InverseMatrix[4] = 0.0f;
    InverseMatrix[5] = 1.0f / Matrix[5];
    InverseMatrix[6] = 0.0f;
    InverseMatrix[7] = 0.0f;

    InverseMatrix[8] = 0.0f;
    InverseMatrix[9] = 0.0f;
    InverseMatrix[10] = 0.0f;
    InverseMatrix[11] = 1.0f / Matrix[14];

    InverseMatrix[12] = 0.0f;
    InverseMatrix[13] = 0.0f;
    InverseMatrix[14] = 1.0f / Matrix[11];
    InverseMatrix[15] = -(Matrix[10] / (Matrix[14] * Matrix[11]));

#if 0
    matrix_4x4 Test;
    ColumnMatrixMultiply4x4((real32 *)Test, (real32 const *)Matrix,
                            (real32 const *)InverseMatrix);

    quad Test1 = {0, 0, -Camera.NearClipPlane, 1};
    quad Test2 = {0, 0, -Camera.FarClipPlane, 1};
    TransposeVectorTransform4(Test1, Matrix);
    TransposeVectorTransform4(Test2, Matrix);
#endif
}

static void
BuildViewMatrix(camera &Camera, real32 *Matrix, real32 *InverseMatrix)
{
    matrix_3x3 PostEARAxes, Axes, EARRotation, Temp;

#if 1
    if(Camera.UseQuaternionOrientation)
    {
        MatrixEqualsQuaternion3x3((real32 *)Axes, Camera.Orientation);
    }
    else
    {
        MatrixEquals3x3((real32 *)Axes,
                        (real32 const *)Camera.OrientationMatrix);
    }

    YRotationColumns3x3(EARRotation, Camera.ElevAzimRoll[1]);
    MatrixMultiply3x3((real32 *)PostEARAxes,
                      (real32 *)Axes,
                      (real32 *)EARRotation);

    XRotationColumns3x3(EARRotation, Camera.ElevAzimRoll[0]);
    MatrixMultiply3x3((real32 *)Temp, (real32 *)PostEARAxes,
                      (real32 *)EARRotation);

    ZRotationColumns3x3(EARRotation, Camera.ElevAzimRoll[2]);
    MatrixMultiply3x3((real32 *)PostEARAxes, (real32 *)Temp,
                      (real32 *)EARRotation);
#else
    MatrixIdentity3x3(PostEARAxes);
#endif

    Matrix[0] = PostEARAxes[0][0];
    Matrix[1] = PostEARAxes[0][1];
    Matrix[2] = PostEARAxes[0][2];
    Matrix[3] = 0.0f;

    Matrix[4] = PostEARAxes[1][0];
    Matrix[5] = PostEARAxes[1][1];
    Matrix[6] = PostEARAxes[1][2];
    Matrix[7] = 0.0f;

    Matrix[8] = PostEARAxes[2][0];
    Matrix[9] = PostEARAxes[2][1];
    Matrix[10] = PostEARAxes[2][2];
    Matrix[11] = 0.0f;

    triple InversePosition;
    TransposeVectorTransform3(InversePosition, (real32 const *)PostEARAxes,
                              Camera.Position);
    VectorAdd3(InversePosition, Camera.Offset);
    VectorNegate3(InversePosition);

    Matrix[12] = InversePosition[0];
    Matrix[13] = InversePosition[1];
    Matrix[14] = InversePosition[2];
    Matrix[15] = 1.0f;

    InverseMatrix[0] = Matrix[0];
    InverseMatrix[1] = Matrix[4];
    InverseMatrix[2] = Matrix[8];
    InverseMatrix[3] = 0.0f;

    InverseMatrix[4] = Matrix[1];
    InverseMatrix[5] = Matrix[5];
    InverseMatrix[6] = Matrix[9];
    InverseMatrix[7] = 0.0f;

    InverseMatrix[8] = Matrix[2];
    InverseMatrix[9] = Matrix[6];
    InverseMatrix[10] = Matrix[10];
    InverseMatrix[11] = 0.0f;

    triple Position;
    VectorTransform3(Position, (real32 const *)PostEARAxes, Camera.Offset);
    VectorAdd3(Position, Camera.Position);

    InverseMatrix[12] = Position[0];
    InverseMatrix[13] = Position[1];
    InverseMatrix[14] = Position[2];
    InverseMatrix[15] = 1.0f;

#if 0
    matrix_4x4 Test;
    ColumnMatrixMultiply4x4((real32 *)Test, (real32 const *)Matrix,
                            (real32 const *)InverseMatrix);
#endif
}

void GRANNY
BuildCameraMatrices(camera &Camera)
{
    BuildViewMatrix(Camera,
                    (real32 *)Camera.View4x4,
                    (real32 *)Camera.InverseView4x4);
    BuildProjectionMatrix(Camera,
                          (real32 *)Camera.Projection4x4,
                          (real32 *)Camera.InverseProjection4x4);
}

void GRANNY
GetCameraLocation(camera const &Camera, real32 *Result)
{
    VectorEquals3(Result, Camera.InverseView4x4[3]);
}

void GRANNY
GetCameraLeft(camera const &Camera, real32 *Result)
{
    VectorNegate3(Result, Camera.InverseView4x4[0]);
}

void GRANNY
GetCameraRight(camera const &Camera, real32 *Result)
{
    VectorEquals3(Result, Camera.InverseView4x4[0]);
}

void GRANNY
GetCameraUp(camera const &Camera, real32 *Result)
{
    VectorEquals3(Result, Camera.InverseView4x4[1]);
}

void GRANNY
GetCameraDown(camera const &Camera, real32 *Result)
{
    VectorNegate3(Result, Camera.InverseView4x4[1]);
}

void GRANNY
GetCameraForward(camera const &Camera, real32 *Result)
{
    VectorNegate3(Result, Camera.InverseView4x4[2]);
}

void GRANNY
GetCameraBack(camera const &Camera, real32 *Result)
{
    VectorEquals3(Result, Camera.InverseView4x4[2]);
}

void GRANNY
WindowSpaceToWorldSpace(camera const &Camera,
                        real32 Width, real32 Height,
                        real32 const *ScreenSpacePoint,
                        real32 *Result)
{
    real32 const ZMin = -1.0f;
    real32 const ZMax = 1.0f;

    quad Homogenous;
    Homogenous[0] = (2.0f*ScreenSpacePoint[0] - Width) / Width;
    Homogenous[1] = (2.0f*ScreenSpacePoint[1] - Height) / Height;
    Homogenous[2] = (2.0f*ScreenSpacePoint[2] - ZMax - ZMin) / (ZMax - ZMin);
    Homogenous[3] = 1.0f;

    TransposeVectorTransform4x4(Homogenous,
                                (real32 const *)Camera.InverseProjection4x4);
    TransposeVectorTransform4x4(Homogenous,
                                (real32 const *)Camera.InverseView4x4);

    real32 InverseW = 1.0f / Homogenous[3];
    Result[0] = Homogenous[0] * InverseW;
    Result[1] = Homogenous[1] * InverseW;
    Result[2] = Homogenous[2] * InverseW;
}

void GRANNY
WorldSpaceToWindowSpace(camera const &Camera,
                        real32 Width, real32 Height,
                        real32 const *WorldSpacePoint,
                        real32 *Result)
{
    quad Homogenous = {WorldSpacePoint[0],
                       WorldSpacePoint[1],
                       WorldSpacePoint[2],
                       1.0f};
    TransposeVectorTransform4x4(Homogenous,
                                (real32 const *)Camera.View4x4);
    TransposeVectorTransform4x4(Homogenous,
                                (real32 const *)Camera.Projection4x4);

    real32 InverseW = 1.0f / Homogenous[3];
    Homogenous[0] = Homogenous[0] * InverseW;
    Homogenous[1] = Homogenous[1] * InverseW;
    Homogenous[2] = Homogenous[2] * InverseW;


    Result[0] = 0.5f * (Homogenous[0] * Width + Width);
    Result[1] = 0.5f * (Homogenous[1] * Height + Height);

    if(Camera.OutputZRange == CameraOutputZNegativeOneToOne)
    {
        // OGL Mode.
        // Homogenous[2] is in device-coordinates, i.e. [0,1].
        // But most OGL apps expect it in clip coordinates, which are [-1,1].
        // So convert for them. The Z-range stuff in OGL would need to be
        // applied by the app, since the granny_camera doesn't get told about them.
        Result[2] = ( 2.0f * Homogenous[2] ) - 1.0f;
    }
    else
    {
        // D3D Mode.
        // No need to modify this - it's already in device coordinates [0,1]
        // For D3D, the app will need to apply the D3DVIEWPORT.MinZ and .MaxZ scaling itself,
        // since the granny_camera doesn't get told about those.
        Result[2] = Homogenous[2];
    }
}

void GRANNY
GetPickingRay(camera const &Camera,
              real32 Width, real32 Height,
              real32 MouseX, real32 MouseY,
              real32 *Origin,
              real32 *Normal)
{
    GetCameraLocation(Camera, Origin);

#if 0
    // This method is too inaccurate if the camera is far away
    // from the origin, due to the catastrophic cancellation of
    // the subtract
    triple ScreenPoint = {MouseX, MouseY, -1.0f};
    triple WorldPoint;
    WindowSpaceToWorldSpace(Camera, Width, Height,
                            ScreenPoint, WorldPoint);

    VectorSubtract3(Normal, WorldPoint, Origin);
    Normalize3(Normal);
#else

    real32 const ZMin = Camera.NearClipPlane;

    triple ScreenSpacePoint = {MouseX, MouseY, ZMin};
    quad Homogenous;
    Homogenous[0] = (2.0f*ScreenSpacePoint[0] - Width) / Width;
    Homogenous[1] = (2.0f*ScreenSpacePoint[1] - Height) / Height;
#if 0
    real32 const ZMax = Camera.FarClipPlane;
    Homogenous[2] = (2.0f*ScreenSpacePoint[2] - (ZMax + ZMin)) / (ZMax - ZMin);
#else
    // ... bake out the maths and ...
    Homogenous[2] = -1.0f;
#endif
    Homogenous[3] = 1.0f;

    TransposeVectorTransform4x4(Homogenous,
                     (real32 const *)Camera.InverseProjection4x4);
    Homogenous[3] = 0.0f;
    TransposeVectorTransform4x4(Homogenous,
                     (real32 const *)Camera.InverseView4x4);

    Normal[0] = Homogenous[0];
    Normal[1] = Homogenous[1];
    Normal[2] = Homogenous[2];

    NormalizeOrZero3(Normal);
#endif

}

void GRANNY
EnsureCameraSeesPoint(camera &Camera, real32 const *Point)
{
    // Figure out where the camera is (after displacements and such)
    triple Location;
    triple Up;
    triple Right;
    triple Forward;
    GetCameraLocation(Camera, Location);
    GetCameraUp(Camera, Up);
    GetCameraRight(Camera, Right);
    GetCameraForward(Camera, Forward);

#if DEBUG
    triple ScreenSpacePoint = {0.0f, 0.0f, -1.0f};
    triple WorldSpacePoint;
    WindowSpaceToWorldSpace(Camera, 2.0f, 2.0f, ScreenSpacePoint, WorldSpacePoint);
    VectorSubtract3(WorldSpacePoint, Location);
    real32 TestX = InnerProduct3(WorldSpacePoint, Right);
    real32 TestY = InnerProduct3(WorldSpacePoint, Up);
    real32 TestZ = InnerProduct3(WorldSpacePoint, Forward);
    TestX = TestX;      // Stops annoying warnings.
    TestY = TestY;
    TestZ = TestZ;
#endif

    // Map the point into camera-space (I could alternatively do this
    // with the 4x4 matrix)
    triple ToInclusion;
    VectorSubtract3(ToInclusion, Point, Location);
    real32 Px = InnerProduct3(Right, ToInclusion);
    real32 Py = InnerProduct3(Up, ToInclusion);
    real32 Pz = InnerProduct3(Forward, ToInclusion);

    real32 Pz_x = AbsoluteValue(Px) * Camera.Projection4x4[0][0];
    real32 Pz_y = AbsoluteValue(Py) * Camera.Projection4x4[1][1];
    real32 MinPz = Maximum(Pz_x, Pz_y);
    real32 DiffZ = MinPz - Pz;
    if(DiffZ > 0.0f)
    {
        Camera.Offset[2] += DiffZ;
    }
}

void GRANNY
GetCameraRelativePlanarBases(camera const &Camera,
                             bool ScreenOrthogonal,
                             real32 const *PlaneNormal,
                             real32 const *PointOnPlane,
                             real32 *XBasis,
                             real32 *YBasis)
{
    // The planar X movement is always assumed to be the right-vector
    // of the camera, since regardless of perspective, left-to-right
    // movement is always projected orthogonally.
    GetCameraRight(Camera, XBasis);

    triple CrossVector;
    if(ScreenOrthogonal)
    {
        // If we want screen-orthogonal movement, then we have to
        // create a synthetic "camera right" vector which is actually
        // the normal of the plane that contains the camera, the
        // point on the plane, and the camera up vector

        triple CameraLocation, CameraUp;
        GetCameraLocation(Camera, CameraLocation);
        GetCameraUp(Camera, CameraUp);

        triple CameraToPoint;
        VectorSubtract3(CameraToPoint, PointOnPlane, CameraLocation);
        VectorCrossProduct3(CrossVector, CameraToPoint, CameraUp);
    }
    else
    {
        // If we want screen-perspective movement, then we don't
        // have to do anything at all, because the camera's right
        // vector itself will be the correct vector to cross with
        // the plane normal to get the Y basis vector (since if it's
        // not screen-orthogonal, it's movement plane orthogonal,
        // hence the Y basis must be perpendicular to the X basis
        // by definition)

        VectorEquals3(CrossVector, XBasis);
    }

    VectorCrossProduct3(YBasis, PlaneNormal, CrossVector);
    NormalizeOrZero3(YBasis);
}

real32 GRANNY
GetMostLikelyPhysicalAspectRatio(int32x ScreenWidth, int32x ScreenHeight)
{
    real32 Aspect = (real32)ScreenWidth / (real32)ScreenHeight;
    if(Aspect > 1.4f)
    {
        if(Aspect > 1.6f)
        {
            return(USDigitalTelevisionAspectRatio);
        }
        else
        {
            return(WidescreenMonitorPhysicalAspectRatio);
        }
    }
    else
    {
        return(NTSCTelevisionPhysicalAspectRatio);
    }
}

void GRANNY
MouseZoomCamera(camera &Camera, real32 dX, real32 dY, real32 dZ)
{
    // TODO: This needs to be a proper differential equation

    Camera.Offset[2] -= dY * 0.005f * Camera.Offset[2];
    Camera.Offset[2] -= dZ * 0.1f * Camera.Offset[2];

    if(Camera.Offset[2] < Camera.NearClipPlane)
    {
        Camera.Offset[2] = Camera.NearClipPlane;
    }
}

void GRANNY
MousePanCamera(camera &Camera, real32 dX, real32 dY, real32 dZ)
{
    MoveCameraRelative(Camera,
                       -dX * 0.001f * Camera.Offset[2],
                       -dY * 0.001f * Camera.Offset[2],
                       0.0f);
}

void GRANNY
MouseOrbitCamera(camera &Camera, real32 dX, real32 dY, real32 dZ)
{
    Camera.ElevAzimRoll[1] -= dX * 0.5f * 3.14f / 180.0f;
    Camera.ElevAzimRoll[0] += dY * 0.5f * 3.14f / 180.0f;
}


real32 GRANNY
FindAllowedLODError(real32 ErrorInPixels,
                    int32x ViewportHeightInPixels,
                    real32 CameraFOVY,
                    real32 DistanceFromCamera)
{
    return ( ErrorInPixels * DistanceFromCamera * Tan ( 0.5f * CameraFOVY ) )
        / ( 0.5f * (real32)ViewportHeightInPixels );
}




