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
#define NAME_NAME(a,b,c) CAT2(GeneratedDeformP##a,c)
#else
#define SVT_NAME(a,b,c) CAT3(pw##a,b,c)
#define NAME_NAME(a,b,c) CAT3(GeneratedDeformPW##a,b,c)
#endif

#define DEFORM_P 1
#if 0
#define SOURCE_VERTEX_TYPE SVT_NAME(3,BONE_COUNT,_vertex)
#define DEST_VERTEX_TYPE p3_vertex
#define INDIRECT 1
#define NAME NAME_NAME(3,BONE_COUNT,I)
#include "granny_generic_deformers.inl"
#undef INDIRECT
#define INDIRECT 0
#undef NAME
#define NAME NAME_NAME(3,BONE_COUNT,D)
#include "granny_generic_deformers.inl"
#undef INDIRECT
#undef NAME
#undef SOURCE_VERTEX_TYPE 
#undef DEST_VERTEX_TYPE
#endif

#define DEFORM_N 1
#define SOURCE_VERTEX_TYPE SVT_NAME(n3,BONE_COUNT,3_vertex)
#define DEST_VERTEX_TYPE pn33_vertex
#define INDIRECT 1
#define NAME NAME_NAME(N3,BONE_COUNT,3I)
#include "granny_generic_deformers.inl"
#undef INDIRECT
#define INDIRECT 0
#undef NAME
#define NAME NAME_NAME(N3,BONE_COUNT,3D)
#include "granny_generic_deformers.inl"
#undef INDIRECT
#undef NAME
#undef SOURCE_VERTEX_TYPE 
#undef DEST_VERTEX_TYPE

#define DEFORM_G 1
#define SOURCE_VERTEX_TYPE SVT_NAME(ng3,BONE_COUNT,33_vertex)
#define DEST_VERTEX_TYPE png333_vertex
#define INDIRECT 1
#define NAME NAME_NAME(NG3,BONE_COUNT,33I)
#include "granny_generic_deformers.inl"
#undef INDIRECT
#define INDIRECT 0
#undef NAME
#define NAME NAME_NAME(NG3,BONE_COUNT,33D)
#include "granny_generic_deformers.inl"
#undef INDIRECT
#undef NAME
#undef SOURCE_VERTEX_TYPE 
#undef DEST_VERTEX_TYPE

#define DEFORM_B 1
#define SOURCE_VERTEX_TYPE SVT_NAME(ngb3,BONE_COUNT,333_vertex)
#define DEST_VERTEX_TYPE pngb3333_vertex
#define INDIRECT 1
#define NAME NAME_NAME(NGB3,BONE_COUNT,333I)
#include "granny_generic_deformers.inl"
#undef INDIRECT
#define INDIRECT 0
#undef NAME
#define NAME NAME_NAME(NGB3,BONE_COUNT,333D)
#include "granny_generic_deformers.inl"
#undef INDIRECT
#undef NAME
#undef SOURCE_VERTEX_TYPE 
#undef DEST_VERTEX_TYPE

#undef SVT_NAME
#undef NAME_NAME
#undef BONE_COUNT
#undef DEFORM_P
#undef DEFORM_N
#undef DEFORM_G
#undef DEFORM_B
