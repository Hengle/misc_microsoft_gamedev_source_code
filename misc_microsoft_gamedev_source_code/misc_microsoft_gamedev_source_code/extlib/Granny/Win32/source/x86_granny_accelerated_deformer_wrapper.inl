/* ========================================================================
   $RCSfile: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
   ======================================================================== */

/*
You define BONE_COUNT, the number of bones to look up
*/

#define _CAT2(a,b) a##b
#define _CAT3(a,b,c) a##b##c

#define CAT2(a,b) _CAT2(a,b)
#define CAT3(a,b,c) _CAT3(a,b,c)

#if (BONE_COUNT == 0)
#define SVT_NAME(a,b,c) CAT2(p##a,c)
#define NAME_NAME(a,b,c) CAT2(GeneratedSSEDeformP##a,c)
#define FALLBACK_NAME_NAME(a,b,c) CAT2(GeneratedDeformP##a,c)
#else
#define SVT_NAME(a,b,c) CAT3(pw##a,b,c)
#define NAME_NAME(a,b,c) CAT3(GeneratedSSEDeformPW##a,b,c)
#define FALLBACK_NAME_NAME(a,b,c) CAT3(GeneratedDeformPW##a,b,c)
#endif

#define DEFORM_P 1
#if 0
#define SOURCE_VERTEX_TYPE SVT_NAME(t3,BONE_COUNT,2_vertex)
#define DEST_VERTEX_TYPE pt32_vertex
#define INDIRECT 1
#define NAME NAME_NAME(T3,BONE_COUNT,2I)
#define FALLBACK_NAME FALLBACK_NAME_NAME(3,BONE_COUNT,I)
#include "x86_granny_accelerated_deformers_generated.i"
#undef INDIRECT
#define INDIRECT 0
#undef NAME
#define NAME NAME_NAME(T3,BONE_COUNT,2D)
#define FALLBACK_NAME FALLBACK_NAME_NAME(3,BONE_COUNT,D)
#include "x86_granny_accelerated_deformers_generated.i"
#undef INDIRECT
#undef NAME
#undef SOURCE_VERTEX_TYPE 
#undef DEST_VERTEX_TYPE
#endif

#define DEFORM_N 1
#define SOURCE_VERTEX_TYPE SVT_NAME(nt3,BONE_COUNT,32_vertex)
#define DEST_VERTEX_TYPE pnt332_vertex
#define INDIRECT 1
#define NAME NAME_NAME(NT3,BONE_COUNT,32I)
#define FALLBACK_NAME FALLBACK_NAME_NAME(N3,BONE_COUNT,3I)
#include "x86_granny_accelerated_deformers_generated.inl"
#undef INDIRECT
#define INDIRECT 0
#undef NAME
#undef FALLBACK_NAME
#define NAME NAME_NAME(NT3,BONE_COUNT,32D)
#define FALLBACK_NAME FALLBACK_NAME_NAME(N3,BONE_COUNT,3D)
#include "x86_granny_accelerated_deformers_generated.inl"
#undef INDIRECT
#undef NAME
#undef FALLBACK_NAME
#undef SOURCE_VERTEX_TYPE 
#undef DEST_VERTEX_TYPE

#define DEFORM_G 1
#define SOURCE_VERTEX_TYPE SVT_NAME(ngt3,BONE_COUNT,332_vertex)
#define DEST_VERTEX_TYPE pngt3332_vertex
#define INDIRECT 1
#define NAME NAME_NAME(NGT3,BONE_COUNT,332I)
#define FALLBACK_NAME FALLBACK_NAME_NAME(NG3,BONE_COUNT,33I)
#include "x86_granny_accelerated_deformers_generated.inl"
#undef INDIRECT
#define INDIRECT 0
#undef NAME
#undef FALLBACK_NAME
#define NAME NAME_NAME(NGT3,BONE_COUNT,332D)
#define FALLBACK_NAME FALLBACK_NAME_NAME(NG3,BONE_COUNT,33D)
#include "x86_granny_accelerated_deformers_generated.inl"
#undef INDIRECT
#undef NAME
#undef FALLBACK_NAME
#undef SOURCE_VERTEX_TYPE 
#undef DEST_VERTEX_TYPE

#define DEFORM_B 1
#define SOURCE_VERTEX_TYPE SVT_NAME(ngbt3,BONE_COUNT,3332_vertex)
#define DEST_VERTEX_TYPE pngbt33332_vertex
#define INDIRECT 1
#define NAME NAME_NAME(NGBT3,BONE_COUNT,3332I)
#define FALLBACK_NAME FALLBACK_NAME_NAME(NGB3,BONE_COUNT,333I)
#include "x86_granny_accelerated_deformers_generated.inl"
#undef INDIRECT
#define INDIRECT 0
#undef NAME
#undef FALLBACK_NAME
#define NAME NAME_NAME(NGBT3,BONE_COUNT,3332D)
#define FALLBACK_NAME FALLBACK_NAME_NAME(NGB3,BONE_COUNT,333D)
#include "x86_granny_accelerated_deformers_generated.inl"
#undef INDIRECT
#undef NAME
#undef FALLBACK_NAME
#undef SOURCE_VERTEX_TYPE 
#undef DEST_VERTEX_TYPE

#undef FALLBACK_NAME_NAME
#undef SVT_NAME
#undef NAME_NAME
#undef BONE_COUNT
#undef DEFORM_P
#undef DEFORM_N
#undef DEFORM_G
#undef DEFORM_B
