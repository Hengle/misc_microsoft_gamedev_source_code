#if !defined(GRANNY_CAMERA_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_camera.h $
// $DateTime: 2007/05/04 11:10:03 $
// $Change: 14896 $
// $Revision: #15 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(CameraGroup);

EXPTYPE enum camera_output_z_range
{
    CameraOutputZZeroToOne,           // D3D
    CameraOutputZNegativeOneToOne,    // OpenGL
    CameraOutputZNegativeOneToZero,   // Wii-TF?
};

EXPTYPE_EPHEMERAL struct camera
{
    // Aspect ratios
    real32 WpOverHp; // Physical aspect ratio
    real32 WrOverHr; // Resolution aspect ratio
    real32 WwOverHw; // Window aspect ratio

    // View parameters
    real32 FOVY;
    real32 NearClipPlane;
    real32 FarClipPlane;
    real32 ZRangeEpsilon;
    camera_output_z_range OutputZRange;

    // Location parameters
    bool UseQuaternionOrientation;
    quad Orientation;
    matrix_3x3 OrientationMatrix;

    triple Position;
    triple ElevAzimRoll; // Elevation/Azimuth/Roll
    triple Offset;

    matrix_4x4 View4x4;
    matrix_4x4 InverseView4x4;
    matrix_4x4 Projection4x4;
    matrix_4x4 InverseProjection4x4;
};
#define InfiniteFarClipPlane 0.0f EXPMACRO

EXPAPI GS_SAFE void InitializeDefaultCamera(camera &Camera);
EXPAPI GS_SAFE void SetCameraAspectRatios(camera &Camera,
                                          real32 PhysicalAspectRatio,
                                          real32 ScreenWidth, real32 ScreenHeight,
                                          real32 WindowWidth, real32 WindowHeight);
EXPAPI GS_SAFE void MoveCameraRelative(camera &Camera, real32 X, real32 Y, real32 Z);
EXPAPI GS_SAFE void BuildCameraMatrices(camera &Camera);

EXPAPI GS_SAFE void GetCameraLocation(camera const &Camera, real32 *Result);
EXPAPI GS_SAFE void GetCameraLeft(camera const &Camera, real32 *Result);
EXPAPI GS_SAFE void GetCameraRight(camera const &Camera, real32 *Result);
EXPAPI GS_SAFE void GetCameraUp(camera const &Camera, real32 *Result);
EXPAPI GS_SAFE void GetCameraDown(camera const &Camera, real32 *Result);
EXPAPI GS_SAFE void GetCameraForward(camera const &Camera, real32 *Result);
EXPAPI GS_SAFE void GetCameraBack(camera const &Camera, real32 *Result);

EXPAPI GS_SAFE void WindowSpaceToWorldSpace(camera const &Camera,
                                            real32 Width, real32 Height,
                                            real32 const *ScreenSpacePoint,
                                            real32 *Result);
EXPAPI GS_SAFE void WorldSpaceToWindowSpace(camera const &Camera,
                                            real32 Width, real32 Height,
                                            real32 const *WorldSpacePoint,
                                            real32 *Result);

EXPAPI GS_SAFE void GetPickingRay(camera const &Camera, real32 Width, real32 Height,
                                  real32 MouseX, real32 MouseY,
                                  real32 *Origin, real32 *Normal);

void EnsureCameraSeesPoint(camera &Camera, real32 const *Point);

EXPAPI GS_SAFE void GetCameraRelativePlanarBases(camera const &Camera,
                                                 bool ScreenOrthogonal,
                                                 real32 const *PlaneNormal,
                                                 real32 const *PointOnPlane,
                                                 real32 *XBasis,
                                                 real32 *YBasis);

EXPAPI GS_SAFE real32 GetMostLikelyPhysicalAspectRatio(int32x ScreenWidth,
                                                       int32x ScreenHeight);

EXPAPI GS_SAFE real32 FindAllowedLODError(real32 ErrorInPixels,
                                          int32x ViewportHeightInPixels,
                                          real32 CameraFOVY,
                                          real32 DistanceFromCamera);

#define LCD17PhysicalAspectRatio 1.25f EXPMACRO
#define NTSCTelevisionPhysicalAspectRatio 1.33f EXPMACRO
#define PALTelevisionPhysicalAspectRatio 1.33f EXPMACRO
#define WidescreenMonitorPhysicalAspectRatio 1.56f EXPMACRO
#define EuropeanFilmAspectRatio 1.66f EXPMACRO
#define USDigitalTelevisionAspectRatio 1.78f EXPMACRO
#define USAcademyFlatPhysicalAspectRatio 1.85f EXPMACRO
#define USPanavisionAspectRatio 2.20f EXPMACRO
#define USAnamorphicScopePhysicalAspectRatio 2.35f EXPMACRO

void MouseZoomCamera(camera &Camera, real32 dX, real32 dY, real32 dZ);
void MousePanCamera(camera &Camera, real32 dX, real32 dY, real32 dZ);
void MouseOrbitCamera(camera &Camera, real32 dX, real32 dY, real32 dZ);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_CAMERA_H
#endif
