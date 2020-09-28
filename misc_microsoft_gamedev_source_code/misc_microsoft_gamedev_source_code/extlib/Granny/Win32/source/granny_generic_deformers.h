#if !defined(GRANNY_GENERIC_DEFORMERS_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_generic_deformers.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE;

struct bone_deformer_parameters;

void AddGenericDeformers(void);

void DeformPWN313I(int32x Count, void const *SourceInit, void *DestInit,
                   int32x const *TransformTable, matrix_4x4 const *Transforms,
                   int32x CopySize, int32x SourceStride, int32x DestStride);
void DeformPWN313D(int32x Count, void const *SourceInit, void *DestInit,
                   matrix_4x4 const *Transforms,
                   int32x CopySize, int32x SourceStride, int32x DestStride);
void DeformPWNGB31333I(int32x Count, void const *SourceInit, void *DestInit,
                       int32x const *TransformTable, matrix_4x4 const *Transforms,
                       int32x CopySize, int32x SourceStride, int32x DestStride);
void DeformPWNGB31333D(int32x Count, void const *SourceInit, void *DestInit,
                       matrix_4x4 const *Transforms,
                       int32x CopySize, int32x SourceStride, int32x DestStride);

#define BONE_COUNT 0
#include "granny_generic_deformer_wrapper.inl"

#define BONE_COUNT 1
#include "granny_generic_deformer_wrapper.inl"

#define BONE_COUNT 2
#include "granny_generic_deformer_wrapper.inl"

#if 0
#define BONE_COUNT 3
#include "granny_generic_deformer_wrapper.inl"
#endif

#define BONE_COUNT 4
#include "granny_generic_deformer_wrapper.inl"

void DeformPN33I(int32x Count, void const *SourceInit, void *DestInit,
                 int32x const *TransformTable, matrix_4x4 const *Transforms,
                 int32x CopySize, int32x SourceStride, int32x DestStride);
void DeformPN33D(int32x Count, void const *SourceInit, void *DestInit,
                 matrix_4x4 const *Transforms, int32x CopySize, int32x SourceStride, int32x DestStride);
void DeformPNGB3333I(int32x Count, void const *SourceInit, void *DestInit,
                     int32x const *TransformTable, matrix_4x4 const *Transforms,
                     int32x CopySize, int32x SourceStride, int32x DestStride);
void DeformPNGB3333D(int32x Count, void const *SourceInit, void *DestInit,
                     matrix_4x4 const *Transforms, int32x CopySize, int32x SourceStride, int32x DestStride);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_GENERIC_DEFORMERS_H
#endif
