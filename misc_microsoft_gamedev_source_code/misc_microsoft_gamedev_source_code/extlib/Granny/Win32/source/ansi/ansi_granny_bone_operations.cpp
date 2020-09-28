// ========================================================================
// $File: //jeffr/granny/rt/ansi/ansi_granny_bone_operations.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #6 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_BONE_OPERATIONS_H)
#include "granny_bone_operations.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

BEGIN_GRANNY_NAMESPACE;

OPTIMIZED_DISPATCH(BuildIdentityWorldPoseComposite) = BuildIdentityWorldPoseComposite_Generic;
OPTIMIZED_DISPATCH(BuildPositionWorldPoseComposite) = BuildPositionWorldPoseComposite_Generic;
OPTIMIZED_DISPATCH(BuildPositionOrientationWorldPoseComposite) = BuildPositionOrientationWorldPoseComposite_Generic;
OPTIMIZED_DISPATCH(BuildFullWorldPoseComposite) = BuildFullWorldPoseComposite_Generic;

OPTIMIZED_DISPATCH(BuildIdentityWorldPoseOnly) = BuildIdentityWorldPoseOnly_Generic;
OPTIMIZED_DISPATCH(BuildPositionWorldPoseOnly) = BuildPositionWorldPoseOnly_Generic;
OPTIMIZED_DISPATCH(BuildPositionOrientationWorldPoseOnly) = BuildPositionOrientationWorldPoseOnly_Generic;
OPTIMIZED_DISPATCH(BuildFullWorldPoseOnly) = BuildFullWorldPoseOnly_Generic;

OPTIMIZED_DISPATCH(BuildSingleCompositeFromWorldPose) = BuildSingleCompositeFromWorldPose_Generic;
OPTIMIZED_DISPATCH(BuildSingleCompositeFromWorldPoseTranspose) = BuildSingleCompositeFromWorldPoseTranspose_Generic;

END_GRANNY_NAMESPACE;
