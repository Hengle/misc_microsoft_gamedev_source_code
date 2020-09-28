#if !defined(GRANNY_DEGREE_OF_FREEDOM_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_degree_of_freedom.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(TransformGroup);

struct transform;

EXPTYPE enum degree_of_freedom
{
    NoDOFs = 0,

    XTranslation = 0x001,
    YTranslation = 0x002,
    ZTranslation = 0x004,
    XRotation    = 0x008,
    YRotation    = 0x010,
    ZRotation    = 0x020,
    XScaleShear  = 0x040,
    YScaleShear  = 0x080,
    ZScaleShear  = 0x100,

    TranslationDOFs = XTranslation | YTranslation | ZTranslation,
    RotationDOFs = XRotation | YRotation | ZRotation,
    ScaleShearDOFs = XScaleShear | YScaleShear | ZScaleShear,

    AllDOFs = TranslationDOFs | RotationDOFs | ScaleShearDOFs
};

EXPAPI GS_SAFE bool ClipPositionDOFs(real32 *Position, uint32x AllowedDOFs);
EXPAPI GS_SAFE bool ClipAngularVelocityDOFs(real32 *Orientation, uint32x AllowedDOFs);
EXPAPI GS_SAFE bool ClipOrientationDOFs(real32 *Orientation, uint32x AllowedDOFs);
EXPAPI GS_SAFE void ClipTransformDOFs(transform &Result, uint32x AllowedDOFs);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_DEGREE_OF_FREEDOM_H
#endif
