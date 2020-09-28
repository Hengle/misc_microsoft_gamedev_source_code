#if !defined(GRANNY_SPU_ANIMATION_INFO_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_spu_animation_info.h $
// $DateTime: 2007/09/07 12:15:29 $
// $Change: 15917 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(SPUGroup);

struct spu_animation;
struct file;

EXPTYPE struct spu_animation_info
{
    int32 SPUAnimationCount;
    spu_animation** SPUAnimations;

    // Extended data
    variant ExtendedData;
};
EXPCONST EXPGROUP(spu_animation_info) extern data_type_definition SPUAnimationInfoType[];

EXPAPI GS_READ spu_animation_info* GetSPUAnimationInfo(file &File);


END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_SPU_ANIMATION_INFO_H
#endif
