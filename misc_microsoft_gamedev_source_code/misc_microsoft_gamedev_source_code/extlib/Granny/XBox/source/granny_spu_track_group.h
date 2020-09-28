#if !defined(GRANNY_SPU_TRACK_GROUP_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_spu_track_group.h $
// $DateTime: 2007/09/07 12:15:29 $
// $Change: 15917 $
// $Revision: #6 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

#if !defined(GRANNY_TRANSFORM_H)
#include "granny_transform.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(SPUGroup);

struct transform_track;

#define SPUTransformTrackNoCurve (0xffffffff) EXPMACRO

EXPTYPE struct spu_transform_track
{
    uint32  FromNameIndex;
    uint32  Flags;

    real32    AnimLODValue;
    transform LODTransform;

    uint32   PosCurveOffset;
    uint32   OriCurveOffset;
    uint32   SSCurveOffset;

    uint32  Reserved;
};
EXPCONST EXPGROUP(spu_transform_track) extern data_type_definition SPUTransformTrackType[];
CompileAssert(IS_ALIGNED_16(sizeof(spu_transform_track)));

EXPTYPE struct spu_track_group
{
    char const *Name;
    int32       Flags;

    int32        TrackNameCount;
    char const** TrackNames;

    int32                TransformTrackCount;
    spu_transform_track* TransformTracks;

    int32  CurveByteCount;
    uint8* CurveBytes;
};
EXPCONST EXPGROUP(spu_track_group) extern data_type_definition SPUTrackGroupType[];


END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_SPU_TRACK_GROUP_H
#endif
