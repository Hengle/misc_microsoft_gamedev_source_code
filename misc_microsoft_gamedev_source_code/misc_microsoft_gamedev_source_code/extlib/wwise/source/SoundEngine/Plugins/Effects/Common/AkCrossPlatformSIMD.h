/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// Cross platform SIMD usage macro
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_CROSSPLATFORMSIMD_H_
#define _AK_CROSSPLATFORMSIMD_H_

#if defined(WIN32)

// Platform specific defines for prefetching
#define AKSIMD_ARCHCACHELINESIZE (64)		// 32 is optimal for P3, 128 -> P4, 64 for many others...
#define AKSIMD_ARCHMAXPREFETCHSIZE (1024)
#define AKSIMD_PREFETCHMEMORY( __offset__, __add__ ) _mm_prefetch(((char *)(__add__))+(__offset__), _MM_HINT_NTA ) 

// Platform specfic defines for SIMD operations
#define AkReal32Vector __m128
#define AKSIMD_GETELEMENT( __vName, __num__ )			((__vName).m128_f32[(__num__)])
#define AKSIMD_LOAD1( __scalar__, __vName__ )			(__vName__ = _mm_load1_ps( &(__scalar__) ))
#define AKSIMD_LOADVEC( __addr__, __vName__ )			(__vName__ = _mm_load_ps( __addr__ ))
#define AKSIMD_LOADUNALIGNEDVEC( __addr__, __vName__ )	(__vName__ = _mm_loadu_ps( __addr__ ))
#define AKSIMD_STOREVEC( __vName__, __addr__ )			(_mm_store_ps( __addr__, __vName__ ))
#define AKSIMD_STOREUNALIGNEDVEC( __vName__, __addr__ )	(_mm_storeu_ps( __addr__, __vName__ ))
#define AKSIMD_MUL( __a__, __b__ )						( _mm_mul_ps( __a__, __b__ ) )
#define AKSIMD_ADD( __a__, __b__ )						( _mm_add_ps( __a__, __b__ ) )

#elif defined(XBOX360)

// Platform specific defines for prefetching
#define AKSIMD_ARCHCACHELINESIZE (128)	
#define AKSIMD_ARCHMAXPREFETCHSIZE (1024)
#define AKSIMD_PREFETCHMEMORY( __offset__, __add__ ) __dcbt( __offset__, (const void *)__add__ )

// Platform specfic defines for SIMD operations
#define AkReal32Vector __vector4
#define AKSIMD_GETELEMENT( __vName, __num__ ) ((__vName).v[(__num__)])
#define AKSIMD_LOAD1( __scalar__, __vName__ ) \
	((__vName__).v[0] = (__scalar__)); \
	((__vName__).v[1] = (__scalar__)); \
	((__vName__).v[2] = (__scalar__)); \
	((__vName__).v[3] = (__scalar__));
#define AKSIMD_LOADVEC( __addr__, __vName__ )			(__vName__ = *(reinterpret_cast<__vector4*>(__addr__)))
#define AKSIMD_LOADUNALIGNEDVEC( __addr__, __vName__ )	(__vName__ = __loadunalignedvector( __addr__ ))
#define AKSIMD_STOREUNALIGNEDVEC( __vName__, __addr__ )	(__storeunalignedvector( __vName__, __addr__ ))
#define AKSIMD_STOREVEC( __vName__, __addr__ )			*(reinterpret_cast<__vector4*>(reinterpret_cast<char*>(__addr__))) = (__vName__)
#define AKSIMD_MUL( __a__, __b__ )						( __vmulfp( __a__, __b__ ) )
#define AKSIMD_ADD( __a__, __b__ )						( __vaddfp( __a__, __b__ ) )

#elif defined (__PPU__)

static const vector float __vMinusZero__ = { -0.0f, -0.0f, -0.0f, -0.0f };

// Platform specific defines for prefetching
#define AKSIMD_ARCHCACHELINESIZE (128)
#define AKSIMD_ARCHMAXPREFETCHSIZE (1024)
#define AKSIMD_PREFETCHMEMORY( __offset__, __add__ )

// Platform specfic defines for SIMD operations
#define AkReal32Vector vector float
#define AKSIMD_GETELEMENT( __vName, __num__ )			(*(((AkReal32*)(&(__vName)))+(__num__)))
#define AKSIMD_LOAD1( __scalar__, __vName__ )			*(float*)&(__vName__) = (__scalar__);		\
														*((float*)&(__vName__) + 1) = (__scalar__);	\
														*((float*)&(__vName__) + 2) = (__scalar__);	\
														*((float*)&(__vName__) + 3) = (__scalar__);
#define AKSIMD_LOADVEC( __addr__, __vName__ )			(__vName__ = *(reinterpret_cast<vector float*>(__addr__)))
#define AKSIMD_LOADUNALIGNEDVEC( __addr__, __vName__ )	__vName__ = vec_lvlx( 0, (unsigned char*)(__addr__) );					\
														vector unsigned char vRight = vec_lvrx( 16, (unsigned char*)in_ptr );	\
														__vName__ = vec_or( (__vName__), vRight );
//#define AKSIMD_STOREUNALIGNEDVEC( __vName__, __addr__ )
#define AKSIMD_STOREVEC( __vName__, __addr__ )			*(reinterpret_cast<vector float*>(reinterpret_cast<char*>(__addr__))) = (__vName__)
#define AKSIMD_MUL( __a__, __b__ )						(vec_madd( (__a__), (__b__), __vMinusZero__ ))
#define AKSIMD_ADD( __a__, __b__ )						(vec_add( (__a__), (__b__) ))

#elif defined (__SPU__)

// Platform specfic defines for SIMD operations
#define AkReal32Vector vec_float4
#define AKSIMD_GETELEMENT( __vName, __num__ )			(*(((vec_float4*)(&(__vName)))+(__num__)))
#define AKSIMD_LOAD1( __scalar__, __vName__ )			__vName__ = spu_splats(__scalar__)
#define AKSIMD_LOADVEC( __addr__, __vName__ )			__vName__ = *((vec_float4*)(__addr__))
#define AKSIMD_STOREVEC( __vName__, __addr__ )			*((vec_float4*)(__addr__)) = (__vName__)
// PhM : these two have not been tested yet
#define AKSIMD_LOADUNALIGNEDVEC( __addr__, __vName__ )	unsigned int shift = (unsigned int)(__addr__) & 0xF;	\
														vec_float4 v0 = *((vec_float4*)(__addr__));				\
														vec_float4 v1 = *((vec_float4*)(__addr__) + 1);			\
														__vName__ = spu_or(	spu_slqwbyte(v0,shift),				\
																			spu_rlmaskqwbyte(v1,shift - 16))
#define AKSIMD_STOREUNALIGNEDVEC( __vName__, __addr__ )	unsigned int shift = (unsigned int)(__addr__) & 0xF;					\
														vec_uint4 mask = (vec_uint4)spu_rlmaskqwbyte((vec_uchar16)0xFF,-shift);	\
														vec_float4 *pv0 = (vec_float4*)(__addr__);								\
														vec_float4 *pv1 = (vec_float4*)(__addr__) + 1;							\
														vec_float4 v0 = *pv0;													\
														vec_float4 v1 = *pv1;													\
														__vName__ = spu_rlqwbyte(__vName__,-shift);								\
														*pv0 = spu_sel(v0,__vName__,mask);										\
														*pv1 = spu_sel(__vName__,v1,mask)

#define AKSIMD_MUL( __a__, __b__ )						(spu_mul( (__a__), (__b__) ))
#define AKSIMD_ADD( __a__, __b__ )						(spu_add( (__a__), (__b__) ))

#else

#error Missing platform macros and defines

#endif

#endif // _AK_CROSSPLATFORMSIMD_H_
