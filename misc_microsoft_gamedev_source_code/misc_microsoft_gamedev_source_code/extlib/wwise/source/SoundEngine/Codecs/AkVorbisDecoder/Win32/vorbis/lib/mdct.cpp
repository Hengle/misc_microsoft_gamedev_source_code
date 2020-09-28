/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2003    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: normalized modified discrete cosine transform
           power of two length transform only [64 <= n ]
 last mod: $Id: mdct.c,v 1.9.6.5 2003/04/29 04:03:27 xiphmont Exp $

 Original algorithm adapted long ago from _The use of multirate filter
 banks for coding of high quality digital audio_, by T. Sporer,
 K. Brandenburg and B. Edler, collection of the European Signal
 Processing Conference (EUSIPCO), Amsterdam, June 1992, Vol.1, pp
 211-214

 The below code implements an algorithm that no longer looks much like
 that presented in the paper, but the basic structure remains if you
 dig deep enough to see it.

 This module DOES NOT INCLUDE code to generate/apply the window
 function.  Everybody has their own weird favorite including me... I
 happen to like the properties of y=sin(.5PI*sin^2(x)), but others may
 vehemently disagree.

 ********************************************************************/

#include "vorbis/ivorbiscodec.h"
#include "os.h"
#include "misc.h"
#include "mdct.h"
#include "mdct_lookup.h"

#include "emmintrin.h"

static const __m128	vfsign1	= {  1.0f,  1.0f, -1.0f, -1.0f };
static const __m128	vfsign2	= { -1.0f, -1.0f,  1.0f,  1.0f };
static const __m128	vfsign3	= { -1.0f,  1.0f,  1.0f, -1.0f };
static const __m128	vfsign4	= {  1.0f, -1.0f, -1.0f,  1.0f };

// #define cPI3_8 (0x30fbc54d) -> 0.38268343237353648635257803199467 = cos(3 * pi / 8)
#define CosThreePiOverEight	0.38268343237353648635257803199467f

// #define cPI2_8 (0x5a82799a) -> 0.70710678152139614407038136574923 = cos(2 * pi / 8)
#define CosPiOverFour	0.70710678152139614407038136574923f

// #define cPI1_8 (0x7641af3d) -> 0.92387953303934984516322139891014 = cos(pi / 8)
#define CosPiOverEight	0.92387953303934984516322139891014f

static __m128	vfcPI2_8		= { CosPiOverFour,CosPiOverFour,CosPiOverFour,CosPiOverFour };
static __m128	vfcPI_3131		= { CosThreePiOverEight, CosPiOverEight, CosThreePiOverEight, CosPiOverEight };
static __m128	vfcPI_m313m1	= { -CosThreePiOverEight, CosPiOverEight, CosThreePiOverEight, -CosPiOverEight };
static __m128	vfcPI_1313		= { CosPiOverEight, CosThreePiOverEight, CosPiOverEight, CosThreePiOverEight };
static __m128	vfcPI_m131m3	= { -CosPiOverEight, CosThreePiOverEight, CosPiOverEight, -CosThreePiOverEight };
//====================================================================================================
//====================================================================================================
STIN void presymmetry(DATA_TYPE *in,int n2,int step)
{
	DATA_TYPE	*aX;
	DATA_TYPE	*bX;
	float		*T;
	int n4=n2>>1;

	aX	= in+n2-3;
	T	= fsincos_lookup0;

	do
	{
		float	fr0= (float)aX[0];
		float	fr2= (float)aX[2];
		fXPROD31( fr0, fr2, T[0], T[1], &aX[0], &aX[2] );

		T+=step;
		aX-=4;
	}
	while(aX>=in+n4);

	do
	{
		float	fr0= (float)aX[0];
		float	fr2= (float)aX[2];
		fXPROD31( fr0, fr2, T[1], T[0], &aX[0], &aX[2] );

		T-=step;
		aX-=4;
	}
	while(aX>=in);

	aX            = in+n2-4;
	bX            = in;
	T             = fsincos_lookup0;
	do
	{
		float	fri0= aX[0];
		float	fri2= aX[2];
		float	fro0= bX[0];
		float	fro2= bX[2];

		fXNPROD31( fro2, fro0, T[1], T[0], &aX[0], &aX[2] );
		T+=step;
		fXNPROD31( fri2, fri0, T[0], T[1], &bX[0], &bX[2] );

		aX-=4;
		bX+=4;
	}
	while(aX>=in+n4);
}
//====================================================================================================
// 8 point butterfly (in place)
//	A = [ A3, A2, A1, A0 ] B = [ B3, B2, B1, B0 ]
//	_mm_shuffle_ps( A, B, _MM_SHUFFLE(a, b, c, d)) = [ Ba, Bb, Ac, Ad ]
//====================================================================================================
STIN void mdct_butterfly_8_SSE2(DATA_TYPE *x)
{
	AKASSERT(((AkUInt32)x & 0x0F) == 0);

	__m128i* vpx = (__m128i*)x;

	__m128 vx0		= _mm_cvtepi32_ps(vpx[0]);								// x[3],x[2],x[1],x[0]
	__m128 vx4		= _mm_cvtepi32_ps(vpx[1]);								// x[7],x[6],x[5],x[4]
	// vOdd = [ x7, x5, x3, x1 ]
	__m128 vOdd		= _mm_shuffle_ps(vx0, vx4, _MM_SHUFFLE(3,1,3,1) );
	// vEven = [ x6, x4, x2, x0 ]
	__m128 vEven	= _mm_shuffle_ps(vx0, vx4, _MM_SHUFFLE(2,0,2,0) );
//--------------------------------------------------------------------------------
//	vSum0 = x[0] + x[1];
//	vSum1 = x[2] + x[3];
//	vSum2 = x[4] + x[5];
//	vSum3 = x[6] + x[7];
//--------------------------------------------------------------------------------
	__m128 vSum		= _mm_add_ps( vEven, vOdd );
//--------------------------------------------------------------------------------
//	vDiff0 = x[0] - x[1];
//	vDiff1 = x[2] - x[3];
//	vDiff2 = x[4] - x[5];
//	vDiff3 = x[6] - x[7];
//--------------------------------------------------------------------------------
	__m128 vDiff	= _mm_sub_ps( vEven, vOdd );
//--------------------------------------------------------------------------------
//	x[0] = vDiff2 + vDiff1
//	x[1] = vDiff3 - vDiff0
//	x[2] = vDiff2 - vDiff1
//	x[3] = vDiff3 + vDiff0
//--------------------------------------------------------------------------------
	// vMix1 = [ vDiff3, vDiff2, vDiff3, vDiff2 ]
	__m128 vMix1	= _mm_shuffle_ps(vDiff, vDiff, _MM_SHUFFLE(3,2,3,2) );
	// vMix2 = [ vDiff0, vDiff1, vDiff0, vDiff1 ]
	__m128 vMix2	= _mm_shuffle_ps(vDiff, vDiff, _MM_SHUFFLE(0,1,0,1) );
	// vMix2 = [ vDiff0, -vDiff1, -vDiff0, vDiff1 ]
	vMix2			= _mm_mul_ps(vMix2, vfsign4);
	// vMix2 = [ vDiff2+vDiff1, vDiff3-vDiff0, vDiff2-vDiff1, vDiff3+vDiff0 ]
	vMix2			= _mm_add_ps( vMix1, vMix2);
	vpx[0]			= _mm_cvtps_epi32(vMix2);
//--------------------------------------------------------------------------------
//	x[4] = vSum2 - vSum0
//	x[5] = vSum3 - vSum1
//	x[6] = vSum2 + vSum0
//	x[7] = vSum3 + vSum1
//--------------------------------------------------------------------------------
	// vMix1 = [ vSum3, vSum2, vSum3, vSum2 ]
	vMix1			= _mm_shuffle_ps(vSum, vSum, _MM_SHUFFLE(3,2,3,2) );
	// vMix2 = [ vSum1, vSum0, vSum1, vSum0 ]
	vMix2			= _mm_shuffle_ps(vSum, vSum, _MM_SHUFFLE(1,0,1,0) );
	// vMix2 = [ vSum1, vSum0, -vSum1, -vSum0 ]
	vMix2			= _mm_mul_ps(vMix2, vfsign2);
	// vB = [ vSum3 + vSum1, vSum2 + vSum0, vSum3 - vSum1, vSum2 - vSum0 ]
	vMix2			= _mm_add_ps( vMix1, vMix2 );
	vpx[1]			= _mm_cvtps_epi32(vMix2);

	MB();
}

STIN void mdct_butterfly_8(DATA_TYPE *x)
{
	REG_TYPE r0   = x[0] + x[1];
	REG_TYPE r1   = x[0] - x[1];
	REG_TYPE r2   = x[2] + x[3];
	REG_TYPE r3   = x[2] - x[3];
	REG_TYPE r4   = x[4] + x[5];
	REG_TYPE r5   = x[4] - x[5];
	REG_TYPE r6   = x[6] + x[7];
	REG_TYPE r7   = x[6] - x[7];

	x[0] = r5   + r3;
	x[1] = r7   - r1;
	x[2] = r5   - r3;
	x[3] = r7   + r1;
	x[4] = r4   - r0;
	x[5] = r6   - r2;
	x[6] = r4   + r0;
	x[7] = r6   + r2;

	MB();
}
//====================================================================================================
// 16 point butterfly (in place, 4 register)
//====================================================================================================
STIN void mdct_butterfly_16_SSE2(DATA_TYPE *x)
{
	AKASSERT(((AkUInt32)x & 0x0F) == 0);

	__m128i* vpx	= (__m128i*)x;

	__m128	vftemp, vftemp2,vfr;
//--------------------------------------------------------------------------------
//	r[0]		= x[ 8] - x[ 9];
//	r[1]		= x[10] - x[11];
//	r[2]		= x[ 1] - x[ 0];
//	r[3]		= x[ 3] - x[ 2];
//--------------------------------------------------------------------------------
	__m128 vfx0	= _mm_cvtepi32_ps(vpx[0]);									//  x3, x2,x1,x0
	__m128 vfx8	= _mm_cvtepi32_ps(vpx[2]);									// x11,x10,x9,x8

	vfr			= _mm_shuffle_ps(vfx8, vfx0, _MM_SHUFFLE(3,1,2,0));			// vr    = [    x3,    x1,     x10,    x8 ]
	vftemp		= _mm_shuffle_ps(vfx8, vfx0, _MM_SHUFFLE(2,0,3,1));			// vtemp = [    x2,    x0,     x11,    x9 ]
	vfr			= _mm_sub_ps(vfr, vftemp);									// vr    = [ x3-x2, x1-x0, x10-x11, x8-x9 ]
//--------------------------------------------------------------------------------
//	x[ 8]		= x[ 8] + x[ 9];
//	x[ 9]		= x[ 1] + x[ 0];
//	x[10]		= x[10] + x[11];
//	x[11]		= x[ 3] + x[ 2];
//--------------------------------------------------------------------------------
	vftemp		= _mm_shuffle_ps(vfx8, vfx0, _MM_SHUFFLE(3,1,2,0));			// vtemp  = [    x3,      x1,   x10,    x8 ]
	vftemp		= _mm_shuffle_ps(vftemp, vftemp, _MM_SHUFFLE(3,1,2,0));		// vtemp  = [    x3,     x10,    x1,    x8 ]
	vftemp2		= _mm_shuffle_ps(vfx8, vfx0, _MM_SHUFFLE(2,0,3,1));			// vtemp2 = [    x2,      x0,   x11,    x9 ]
	vftemp2		= _mm_shuffle_ps(vftemp2, vftemp2, _MM_SHUFFLE(3,1,2,0));	// vtemp2 = [    x2,     x11,    x0,    x9 ]
	vftemp2		= _mm_add_ps(vftemp, vftemp2);								// vx8    = [ x3+x2, x10+x11, x1+x0, x8+x9 ]
	vpx[2]		= _mm_cvtps_epi32(vftemp2);
//--------------------------------------------------------------------------------
//	x[ 0]		= MULT31((r[0] - r[1]) , cPI2_8);
//	x[ 1]		= MULT31((r[2] + r[3]) , cPI2_8);
//	x[ 2]		= MULT31((r[0] + r[1]) , cPI2_8);
//	x[ 3]		= MULT31((r[3] - r[2]) , cPI2_8);
//--------------------------------------------------------------------------------
	vftemp		= _mm_shuffle_ps(vfr,vfr,_MM_SHUFFLE(3,0,2,0));			// vtemp  = [    r3,    r0,     r2,    r0 ]
	vftemp2		= _mm_shuffle_ps(vfr,vfr,_MM_SHUFFLE(2,1,3,1));			// vtemp2 = [    r2,    r1,     r3,    r1 ]
	vftemp2		= _mm_mul_ps(vftemp2,vfsign3);							// vtemp2 = [   -r2,    r1,     r3,   -r1 ]
	vftemp2		= _mm_add_ps(vftemp,vftemp2);							// vtemp2 = [ r3-r2, r0+r1,  r2+r3, r0-r1 ]
	vftemp2		= _mm_mul_ps(vftemp2,vfcPI2_8);
	vpx[0]		= _mm_cvtps_epi32(vftemp2);

	MB();

	__m128 vfx4		= _mm_cvtepi32_ps(vpx[1]);			// x[ 7],x[ 6],x[ 5],x[ 4]
	__m128 vfx12	= _mm_cvtepi32_ps(vpx[3]);			// x[15],x[14],x[13],x[12]
//--------------------------------------------------------------------------------
//	r0		= x[ 4] - x[ 5];
//	r1		= x[ 7] - x[ 6];
//	r2		= x[12] - x[13];
//	r3		= x[14] - x[15];
//--------------------------------------------------------------------------------
	vfr			= _mm_shuffle_ps(vfx4,vfx12,_MM_SHUFFLE(2,0,3,0));		// vfr = [     x14,     x12,    x7,    x4 ]
	vftemp		= _mm_shuffle_ps(vfx4,vfx12,_MM_SHUFFLE(3,1,2,1));		// vfr = [     x15,     x13,    x6,    x5 ]
	vfr			= _mm_sub_ps(vfr,vftemp);								// vfr = [ x14-x15, x12-x13, x7-x6, x4-x5 ]
//--------------------------------------------------------------------------------
//	x[12]	= x[12] + x[13];
//	x[13]	= x[ 5] + x[ 4];
//	x[14]	= x[14] + x[15];
//	x[15]	= x[ 7] + x[ 6];
//--------------------------------------------------------------------------------
	vftemp		= _mm_shuffle_ps(vfx12,vfx4,_MM_SHUFFLE(3,1,2,0));		// vftemp  = [     x7,     x5,    x14,     x12 ]
	vftemp		= _mm_shuffle_ps(vftemp,vftemp,_MM_SHUFFLE(3,1,2,0));	// vftemp  = [     x7,    x14,     x5,     x12 ]
	vftemp2		= _mm_shuffle_ps(vfx12,vfx4,_MM_SHUFFLE(2,0,3,1));		// vftemp2 = [     x6,     x4,    x15,	   x13 ]
	vftemp2		= _mm_shuffle_ps(vftemp2,vftemp2,_MM_SHUFFLE(3,1,2,0));	// vftemp2 = [     x6,     x15,    x4,     x13 ]
	vftemp2		= _mm_add_ps(vftemp,vftemp2);							// vftemp2 = [  x7+x6, x14+x15, x5+x4, x12+x13 ]
	vpx[3]		= _mm_cvtps_epi32(vftemp2);
//--------------------------------------------------------------------------------
//	x[ 4]	= r2;
//	x[ 5]	= r1; 
//	x[ 6]	= r3;
//	x[ 7]	= r0;
//--------------------------------------------------------------------------------
	vfr			= _mm_shuffle_ps(vfr,vfr,_MM_SHUFFLE(0,3,1,2));			// vfr = [ r0, r3, r1, r2 ]
	vpx[1]		= _mm_cvtps_epi32(vfr);

	MB();

	mdct_butterfly_8(x);
	mdct_butterfly_8(x+8);
}

STIN void mdct_butterfly_16(DATA_TYPE *x)
{
	REG_TYPE r0, r1, r2, r3;

	r0		= x[ 8] - x[ 9];
	r1		= x[10] - x[11];
	r2		= x[ 1] - x[ 0];
	r3		= x[ 3] - x[ 2];

	x[ 8]	= x[ 8] + x[ 9];
	x[ 9]	= x[ 1] + x[0];
	x[10]	= x[10] + x[11];
	x[11]	= x[ 3] + x[2];

	x[ 0] = MULT31((r0 - r1) , cPI2_8);
	x[ 1] = MULT31((r2 + r3) , cPI2_8);
	x[ 2] = MULT31((r0 + r1) , cPI2_8);
	x[ 3] = MULT31((r3 - r2) , cPI2_8);

	MB();

	r0		= x[ 4] - x[ 5];
	r1		= x[ 7] - x[ 6];
	r2		= x[12] - x[13];
	r3		= x[14] - x[15];

	x[12]	= x[12] + x[13];
	x[13]	= x[ 5] + x[ 4];
	x[14]	= x[14] + x[15];
	x[15]	= x[ 7] + x[ 6];

	x[ 4]	= r2;
	x[ 5]	= r1; 
	x[ 6]	= r3;
	x[ 7]	= r0;

	MB();

	mdct_butterfly_8(x);
	mdct_butterfly_8(x+8);
}
//====================================================================================================
// 32 point butterfly (in place, 4 register)
//====================================================================================================
STIN void mdct_butterfly_32_SSE2(DATA_TYPE *x)
{
	AKASSERT(((AkUInt32)x & 0x0F) == 0);

	__m128i* vpx	= (__m128i*)x;
	__m128	vfr,vftemp, vftemp2;
//--------------------------------------------------------------------------------
//	r0		= x[16] - x[17];
//	r1		= x[18] - x[19];
//	r2		= x[ 1] - x[ 0];
//	r3		= x[ 3] - x[ 2];
//--------------------------------------------------------------------------------
	__m128 vfx0		= _mm_cvtepi32_ps(vpx[0]);
	__m128 vfx16	= _mm_cvtepi32_ps(vpx[4]);
	vftemp		= _mm_shuffle_ps(vfx16,vfx0,_MM_SHUFFLE(3,1,2,0));		// vtemp  = [    x3,    x1,     x18,     x16 ]
	vftemp2		= _mm_shuffle_ps(vfx16,vfx0,_MM_SHUFFLE(2,0,3,1));		// vtemp2 = [    x2,    x0,     x19,     x17 ]
	vfr			= _mm_sub_ps(vftemp, vftemp2);							// vr     = [ x3-x2, x1-x0, x18-x19, x16-x17 ]
//--------------------------------------------------------------------------------
//	x[16]	= x[16] + x[17];
//	x[17]	= x[ 1] + x[ 0];
//	x[18]	= x[18] + x[19];
//	x[19]	= x[ 3] + x[ 2];
//--------------------------------------------------------------------------------
	vftemp		= _mm_shuffle_ps(vftemp,vftemp,_MM_SHUFFLE(3,1,2,0));	// vtemp  = [    x3,     x18,    x1,     x16 ]
	vftemp2		= _mm_shuffle_ps(vftemp2,vftemp2,_MM_SHUFFLE(3,1,2,0));	// vtemp2 = [    x2,     x19,    x0,     x17 ]
	vftemp2		= _mm_add_ps(vftemp, vftemp2);							// vx16   = [ x3+x2, x18+x19, x1+x0, x16+x17 ] 
	vpx[4]		= _mm_cvtps_epi32(vftemp2);
//--------------------------------------------------------------------------------
//	XNPROD31( r0, r1, cPI3_8, cPI1_8, &x[ 0], &x[ 2] );
//	XNPROD31(  a,  b,      t,      v,     *x,     *y )
//	{
//		*x = (a * t) - (b * v);
//		*y = (b * t) + (a * v);
//	}
//	XPROD31 ( r2, r3, cPI1_8, cPI3_8, &x[ 1], &x[ 3] );
//	XPROD31 (  a,  b,      t,      v,     *x,     *y )
//	{
//		*x = (a * t) + (b * v);
//		*y = (b * t) - (a * v);
//	}
//
//	x[ 0] = (r0 * cPI3_8) - (r1 * cPI1_8)
//	x[ 1] = (r2 * cPI1_8) + (r3 * cPI3_8)
//	x[ 2] = (r1 * cPI3_8) + (r0 * cPI1_8)
//	x[ 3] = (r3 * cPI1_8) - (r2 * cPI3_8)
//--------------------------------------------------------------------------------
	vftemp		= _mm_shuffle_ps(vfr,vfr,_MM_SHUFFLE(3,1,2,0));			// vtemp  = [      r3,     r1,     r2,      r0 ]
	vftemp		= _mm_mul_ps(vftemp,vfcPI_3131);						// vtemp  = [  r3*c18, r1*c38, r2*c18,  r0*c38 ]
	vftemp2		= _mm_shuffle_ps(vfr,vfr,_MM_SHUFFLE(2,0,3,1));			// vtemp2 = [      r2,     r0,     r3,      r1 ]
	vftemp2		= _mm_mul_ps(vftemp2,vfcPI_m131m3);						// vtemp2 = [ -r2*c38, r0*c18, r3*c38, -r1*c18 ]
	vftemp2		= _mm_add_ps(vftemp,vftemp2);
	vpx[0]		= _mm_cvtps_epi32(vftemp2);

	MB();
//--------------------------------------------------------------------------------
//	r0		= x[20] - x[21];
//	r1		= x[22] - x[23];
//	r2		= x[ 5] - x[ 4];
//	r3		= x[ 7] - x[ 6];
//--------------------------------------------------------------------------------
	__m128 vfx4		= _mm_cvtepi32_ps(vpx[1]);
	__m128 vfx20	= _mm_cvtepi32_ps(vpx[5]);
	vftemp		= _mm_shuffle_ps(vfx20,vfx4,_MM_SHUFFLE(3,1,2,0));		// vtemp  = [    x7,    x5,     x22,     x20 ]
	vftemp2		= _mm_shuffle_ps(vfx20,vfx4,_MM_SHUFFLE(2,0,3,1));		// vtemp2 = [    x6,    x4,     x23,     x21 ]
	vfr			= _mm_sub_ps(vftemp, vftemp2);							// vr     = [ x7-x6, x5-x4, x22-x23, x20-x21 ]
//--------------------------------------------------------------------------------
//	x[20]	= x[20] + x[21];
//	x[21]	= x[ 5] + x[ 4];
//	x[22]	= x[22] + x[23];
//	x[23]	= x[ 7] + x[ 6];
//--------------------------------------------------------------------------------
	vftemp		= _mm_shuffle_ps(vftemp,vftemp,_MM_SHUFFLE(3,1,2,0));	// vtemp  = [    x7,     x22,    x5,     x20 ]
	vftemp2		= _mm_shuffle_ps(vftemp2,vftemp2,_MM_SHUFFLE(3,1,2,0));	// vtemp2 = [    x6,     x23,    x4,     x21 ]
	vftemp2		= _mm_add_ps(vftemp, vftemp2);							// vx20   = [ x7+x6, x22+x23, x5+x4, x20+x21 ]
	vpx[5]		= _mm_cvtps_epi32(vftemp2);
//--------------------------------------------------------------------------------
//	x[ 4] = MULT31((r0 - r1) , cPI2_8);
//	x[ 5] = MULT31((r3 + r2) , cPI2_8);
//	x[ 6] = MULT31((r0 + r1) , cPI2_8);
//	x[ 7] = MULT31((r3 - r2) , cPI2_8);
//--------------------------------------------------------------------------------
	vftemp		= _mm_shuffle_ps(vfr,vfr,_MM_SHUFFLE(3,0,3,0));		// vtemp  = [  r3, r0, r3,  r0 ]
	vftemp2		= _mm_shuffle_ps(vfr,vfr,_MM_SHUFFLE(2,1,2,1));		// vtemp2 = [  r2, r1, r2,  r1 ]
	vftemp2		= _mm_mul_ps(vftemp2,vfsign3);						// vtemp2 = [ -r2, r1, r2, -r1 ]
	vftemp2		= _mm_add_ps(vftemp,vftemp2);
	vftemp2		= _mm_mul_ps(vftemp2,vfcPI2_8);
	vpx[1]		= _mm_cvtps_epi32(vftemp2);

	MB();
//--------------------------------------------------------------------------------
//	r0		= x[24] - x[25];
//	r1		= x[26] - x[27];
//	r2		= x[ 9] - x[ 8];
//	r3		= x[11] - x[10];
//--------------------------------------------------------------------------------
	__m128 vfx8		= _mm_cvtepi32_ps(vpx[2]);
	__m128 vfx24	= _mm_cvtepi32_ps(vpx[6]);
	vftemp		= _mm_shuffle_ps(vfx24,vfx8,_MM_SHUFFLE(3,1,2,0));			// vtemp  = [     x11,    x9,     x26,     x24 ]
	vftemp2		= _mm_shuffle_ps(vfx24,vfx8,_MM_SHUFFLE(2,0,3,1));			// vtemp2 = [     x10,    x8,     x27,     x25 ]
	vfr			= _mm_sub_ps(vftemp,vftemp2);								// vr     = [ x11-x10, x9-x8, x26-x27, x24-x25 ]
//--------------------------------------------------------------------------------
//	x[24]	= x[24] + x[25];           
//	x[25]	= x[ 9] + x[ 8];
//	x[26]	= x[26] + x[27];
//	x[27]	= x[11] + x[10];
//--------------------------------------------------------------------------------
	vftemp		= _mm_shuffle_ps(vftemp,vftemp,_MM_SHUFFLE(3,1,2,0));		// vtemp  = [     x11,     x26,    x9,     x24 ]
	vftemp2		= _mm_shuffle_ps(vftemp2,vftemp2,_MM_SHUFFLE(3,1,2,0));		// vtemp2 = [     x10,     x27,    x8,     x25 ]
	vftemp2		= _mm_add_ps(vftemp,vftemp2);								// vx24   = [ x11+x10, x26+x27, x9+x8, x24+x25 ]
	vpx[6]		= _mm_cvtps_epi32(vftemp2);
//--------------------------------------------------------------------------------
//	XNPROD31( r0, r1, cPI1_8, cPI3_8, &x[ 8], &x[10] );
//	XNPROD31(  a,  b,      t,       v,    *x,     *y )
//	{
//		*x = (a * t) - (b * v);
//		*y = (b * t) + (a * v);
//	}
//	XPROD31 ( r2, r3, cPI3_8, cPI1_8, &x[ 9], &x[11] );
//	XPROD31 (  a,  b,      t,      v,     *x,     *y )
//	{
//		*x = (a * t) + (b * v);
//		*y = (b * t) - (a * v);
//	}
//
//	x[ 8] = (r0 * cPI1_8) - (r1 * cPI3_8)
//	x[ 9] = (r2 * cPI3_8) + (r3 * cPI1_8)
//	x[10] = (r1 * cPI1_8) + (r0 * cPI3_8)
//	x[11] = (r3 * cPI3_8) - (r2 * cPI1_8)
//--------------------------------------------------------------------------------
	vftemp		= _mm_shuffle_ps(vfr,vfr,_MM_SHUFFLE(3,1,2,0));		// vtemp  = [      r3,     r1,     r2,      r0 ]
	vftemp		= _mm_mul_ps(vftemp,vfcPI_1313);					// vtemp  = [  r3*c38, r1*c18, r2*c38,  r0*c18 ]
	vftemp2		= _mm_shuffle_ps(vfr,vfr,_MM_SHUFFLE(2,0,3,1));		// vtemp2 = [      r2,     r0,     r3,      r1 ]
	vftemp2		= _mm_mul_ps(vftemp2,vfcPI_m313m1);					// vtemp2 = [  r2*c18, r0*c38, r3*c18,  r1*c38 ]
	vftemp2		= _mm_add_ps(vftemp,vftemp2);
	vpx[2]		= _mm_cvtps_epi32(vftemp2);

	MB();
//--------------------------------------------------------------------------------
//	r0		= x[28] - x[29];
//	r1		= x[30] - x[31];
//	r2		= x[12] - x[13];
//	r3		= x[15] - x[14];
//--------------------------------------------------------------------------------
	__m128 vfx12	= _mm_cvtepi32_ps(vpx[3]);
	__m128 vfx28	= _mm_cvtepi32_ps(vpx[7]);
	vftemp		= _mm_shuffle_ps(vfx28,vfx12,_MM_SHUFFLE(3,0,2,0));		// vtemp  = [     x15,     x12,     x30,     x28 ]
	vftemp2		= _mm_shuffle_ps(vfx28,vfx12,_MM_SHUFFLE(2,1,3,1));		// vtemp2 = [     x14,     x13,     x31,     x29 ]
	vfr			= _mm_sub_ps(vftemp,vftemp2);							// vr     = [ x15-x14, x12-x13, x30-x31, x28-x29 ]
//--------------------------------------------------------------------------------
//	x[28]	= x[28] + x[29];           
//	x[29]	= x[13] + x[12];
//	x[30]	= x[30] + x[31];
//	x[31]	= x[15] + x[14];
//--------------------------------------------------------------------------------
	vftemp		= _mm_shuffle_ps(vfx28,vfx12,_MM_SHUFFLE(3,1,2,0));		// vtemp  = [     x15,     x13,     x30,     x28 ]
	vftemp		= _mm_shuffle_ps(vftemp,vftemp,_MM_SHUFFLE(3,1,2,0));	// vtemp  = [     x15,     x30,     x13,     x28 ]
	vftemp2		= _mm_shuffle_ps(vfx28,vfx12,_MM_SHUFFLE(2,0,3,1));		// vtemp2 = [     x14,     x12,     x31,     x29 ]
	vftemp2		= _mm_shuffle_ps(vftemp2,vftemp2,_MM_SHUFFLE(3,1,2,0));	// vtemp2 = [     x14,     x31,     x12,     x29 ]
	vftemp2		= _mm_add_ps(vftemp, vftemp2);							// vx28   = [ x15+x14, x30+x31, x13+x12, x28+x29 ]
	vpx[7]		= _mm_cvtps_epi32(vftemp2);
//--------------------------------------------------------------------------------
//	x[12]	= r0;
//	x[13]	= r3; 
//	x[14]	= r1;
//	x[15]	= r2;
//--------------------------------------------------------------------------------
	vftemp		= _mm_shuffle_ps(vfr,vfr,_MM_SHUFFLE(2,1,3,0));			// vtemp2 = [ r2, r1, r3, r0 ]
	vpx[3]		= _mm_cvtps_epi32(vftemp);

	MB();

	mdct_butterfly_16(x);
	mdct_butterfly_16(x + 16);
}
STIN void mdct_butterfly_32(DATA_TYPE *x)
{
	REG_TYPE r0, r1, r2, r3;

	r0		= x[16] - x[17];
	r1		= x[18] - x[19];
	r2		= x[ 1] - x[ 0];
	r3		= x[ 3] - x[ 2];

	x[16]	= x[16] + x[17];
	x[17]	= x[ 1] + x[ 0];
	x[18]	= x[18] + x[19];
	x[19]	= x[ 3] + x[ 2];

	XNPROD31( r0, r1, cPI3_8, cPI1_8, &x[ 0], &x[ 2] );
	XPROD31 ( r2, r3, cPI1_8, cPI3_8, &x[ 1], &x[ 3] );
	MB();

	r0		= x[20] - x[21];
	r1		= x[22] - x[23];
	r2		= x[ 5] - x[ 4];
	r3		= x[ 7] - x[ 6];

	x[20]	= x[20] + x[21];
	x[21]	= x[ 5] + x[ 4];
	x[22]	= x[22] + x[23];
	x[23]	= x[ 7] + x[ 6];

	x[ 4] = MULT31((r0 - r1) , cPI2_8);
	x[ 5] = MULT31((r3 + r2) , cPI2_8);
	x[ 6] = MULT31((r0 + r1) , cPI2_8);
	x[ 7] = MULT31((r3 - r2) , cPI2_8);
	MB();

	r0		= x[24] - x[25];
	r1		= x[26] - x[27];
	r2		= x[ 9] - x[ 8];
	r3		= x[11] - x[10];

	x[24]	= x[24] + x[25];           
	x[25]	= x[ 9] + x[ 8];
	x[26]	= x[26] + x[27];
	x[27]	= x[11] + x[10];

	XNPROD31( r0, r1, cPI1_8, cPI3_8, &x[ 8], &x[10] );
	XPROD31 ( r2, r3, cPI3_8, cPI1_8, &x[ 9], &x[11] );
	MB();

	r0		= x[28] - x[29];
	r1		= x[30] - x[31];
	r2		= x[12] - x[13];
	r3		= x[15] - x[14];

	x[28]	= x[28] + x[29];           
	x[29]	= x[13] + x[12];
	x[30]	= x[30] + x[31];
	x[31]	= x[15] + x[14];

	x[12]	= r0;
	x[13]	= r3; 
	x[14]	= r1;
	x[15]	= r2;

	MB();

	mdct_butterfly_16(x);
	mdct_butterfly_16(x + 16);
}
//====================================================================================================
// N/stage point generic N stage butterfly (in place, 2 register)
//====================================================================================================
STIN void mdct_butterfly_generic_SSE2(DATA_TYPE *x,int points,int step)
{
	AKASSERT(((AkUInt32)x & 0x0F) == 0);
	AKASSERT((step % 4) == 0);
	AKASSERT((points % 4) == 0);

	__m128	*pfT		= (__m128*)fsincos_lookup0;
	__m128	*LoopEnd	= (__m128*)(fsincos_lookup0 + 1024);
	__m128i	*x1			= (__m128i*)(x + points - 4);
	__m128i	*x2			= (__m128i*)(x + (points>>1) - 4);

	__m128	vfr,vftemp,vftemp2;

	step /= 4;

	do
	{
		__m128 vfx1		= _mm_cvtepi32_ps(x1[0]);
		__m128 vfx2		= _mm_cvtepi32_ps(x2[0]);
//--------------------------------------------------------------------------------
//		r0		= x1[0] - x1[1];
//		r1		= x1[3] - x1[2];
//		r2		= x2[1] - x2[0];
//		r3		= x2[3] - x2[2];
//--------------------------------------------------------------------------------
		vfr			= _mm_shuffle_ps(vfx1,vfx2,_MM_SHUFFLE(3,1,3,0));		// vfr    = [ x2[3], x2[1], x1[3], x1[0] ]
		vftemp		= _mm_shuffle_ps(vfx1,vfx2,_MM_SHUFFLE(2,0,2,1));		// vftemp = [ x2[2], x2[0], x1[2], x1[1] ]
		vfr			= _mm_sub_ps(vfr, vftemp);
//--------------------------------------------------------------------------------
//		x1[0]	= x1[0] + x1[1];
//		x1[1]	= x2[1] + x2[0];
//		x1[2]	= x1[2] + x1[3];
//		x1[3]	= x2[3] + x2[2];
//--------------------------------------------------------------------------------
		vftemp		= _mm_shuffle_ps(vfx2,vfx1,_MM_SHUFFLE(2,0,3,1));		// vftemp  = [ x1[2], x1[0], x2[3], x2[1] ]
		vftemp		= _mm_shuffle_ps(vftemp,vftemp,_MM_SHUFFLE(1,3,0,2));	// vftemp  = [ x2[3], x1[2], x2[1], x1[0] ]
		vftemp2		= _mm_shuffle_ps(vfx2,vfx1,_MM_SHUFFLE(3,1,2,0));		// vftemp2 = [ x1[3], x1[1], x2[2], x2[0] ]
		vftemp2		= _mm_shuffle_ps(vftemp2,vftemp2,_MM_SHUFFLE(1,3,0,2));	// vftemp2 = [ x2[2], x1[3], x2[0], x1[1] ]
		vftemp2		= _mm_add_ps(vftemp, vftemp2);
		x1[0]		= _mm_cvtps_epi32(vftemp2);
//--------------------------------------------------------------------------------
//	XPROD31(a,b,t,v,*x,*y)
//	{
//		*x = (a * t) + (b * v);
//		*y = (b * t) - (a * v);
//	}
//
//	XPROD31( r1, r0, T[0], T[1], &x2[0], &x2[2] );
//	x2[0] = (r1 * T[0]) + (r0 * T[1])
//	x2[2] = (r0 * T[0]) - (r1 * T[1])
//
//	XPROD31( r2, r3, T[0], T[1], &x2[1], &x2[3] );
//	x2[1] = (r2 * T[0]) + (r3 * T[1])
//	x2[3] = (r3 * T[0]) - (r2 * T[1])
//
//	x2[0] = (r1 * T[0]) + (r0 * T[1])
//	x2[1] = (r2 * T[0]) + (r3 * T[1])
//	x2[2] = (r0 * T[0]) - (r1 * T[1])
//	x2[3] = (r3 * T[0]) - (r2 * T[1])
//--------------------------------------------------------------------------------
		__m128	vfT,vfT0;

		vfT			= *pfT;													// vT     = [     T3,     T2,    T1,    T0 ]
		vfT0		= _mm_shuffle_ps(vfT,vfT,_MM_SHUFFLE(0,0,0,0));			// vT0    = [     T0,     T0,    T0,    T0 ]
		vftemp		= _mm_shuffle_ps(vfr,vfr,_MM_SHUFFLE(3,0,2,1));			// vtemp  = [     r3,     r0,    r2,    r1 ]
		vftemp		= _mm_mul_ps(vftemp, vfT0);								// vtemp  = [  r3*T0,  r0*T0, r2*T0, r1*T0 ]
		vfT0		= _mm_shuffle_ps(vfT,vfT,_MM_SHUFFLE(1,1,1,1));			// vT0    = [     T1,     T1,    T1,    T1 ]
		vftemp2		= _mm_shuffle_ps(vfr,vfr,_MM_SHUFFLE(2,1,3,0));			// vtemp2 = [     r2,     r1,    r3,    r0 ]
		vftemp2		= _mm_mul_ps(vftemp2, vfT0);							// vtemp2 = [  r2*T1,  r1*T1, r3*T1, r0*T1 ]
		vftemp2		= _mm_mul_ps(vftemp2,vfsign1);							// vtemp2 = [ -r2*T1, -r1*T1, r3*T1, r0*T1 ]
		vftemp2		= _mm_add_ps(vftemp,vftemp2);							// vtemp += [ -r2*T1, -r1*T1, r3*T1, r0*T1 ]
		x2[0]		= _mm_cvtps_epi32(vftemp2);

		pfT += step;
		--x1;
		--x2;
	}
	while(pfT < LoopEnd);

	LoopEnd = (__m128*)fsincos_lookup0;

	do
	{
		__m128 vfx1		= _mm_cvtepi32_ps(x1[0]);
		__m128 vfx2		= _mm_cvtepi32_ps(x2[0]);
//--------------------------------------------------------------------------------
//		r0		= x1[0] - x1[1];
//		r1		= x1[2] - x1[3];
//		r2		= x2[0] - x2[1];
//		r3		= x2[3] - x2[2];
//--------------------------------------------------------------------------------
		vfr			= _mm_shuffle_ps(vfx1,vfx2,_MM_SHUFFLE(3,0,2,0));		// vfr    = [ x2[3], x2[0], x1[2], x1[0] ]
		vftemp		= _mm_shuffle_ps(vfx1,vfx2,_MM_SHUFFLE(2,1,3,1));		// vftemp = [ x2[2], x2[1], x1[3], x1[1] ]
		vfr			= _mm_sub_ps(vfr, vftemp);
//--------------------------------------------------------------------------------
//		x1[0]	= x1[0] + x1[1];
//		x1[1]	= x2[1] + x2[0];
//		x1[2]	= x1[2] + x1[3];
//		x1[3]	= x2[3] + x2[2];
//--------------------------------------------------------------------------------
		vftemp		= _mm_shuffle_ps(vfx2,vfx1,_MM_SHUFFLE(2,0,3,1));		// vftemp  = [ x1[2], x1[0], x2[3], x2[1] ]
		vftemp		= _mm_shuffle_ps(vftemp,vftemp,_MM_SHUFFLE(1,3,0,2));	// vftemp  = [ x2[3], x1[2], x2[1], x1[0] ]
		vftemp2		= _mm_shuffle_ps(vfx2,vfx1,_MM_SHUFFLE(3,1,2,0));		// vftemp2 = [ x1[3], x1[1], x2[2], x2[0] ]
		vftemp2		= _mm_shuffle_ps(vftemp2,vftemp2,_MM_SHUFFLE(1,3,0,2));	// vftemp2 = [ x2[2], x1[3], x2[0], x1[1] ]
		vftemp2		= _mm_add_ps(vftemp,vftemp2);
		x1[0]		= _mm_cvtps_epi32(vftemp2);
//--------------------------------------------------------------------------------
//	XNPROD31(a,b,t,v,*x,*y)
//	{
//		*x = (a * t) - (b * v);
//		*y = (b * t) + (a * v);
//	}
//
//	XNPROD31( r0, r1, T[0], T[1], &x2[0], &x2[2] );
//	x2[0] = (r0 * T[0]) - (r1 * T[1])
//	x2[2] = (r1 * T[0]) + (r0 * T[1])
//
//	XNPROD31( r3, r2, T[0], T[1], &x2[1], &x2[3] );
//	x2[1] = (r3 * T[0]) - (r2 * T[1])
//	x2[3] = (r2 * T[0]) + (r3 * T[1])
//
//	x2[0] = (r0 * T[0]) - (r1 * T[1])
//	x2[1] = (r3 * T[0]) - (r2 * T[1])
//	x2[2] = (r1 * T[0]) + (r0 * T[1])
//	x2[3] = (r2 * T[0]) + (r3 * T[1])
//--------------------------------------------------------------------------------
		__m128	vfT,vfT0;

		vfT			= *pfT;													// vT     = [    T3,   T2,      T1,     T0 ]
		vfT0		= _mm_shuffle_ps(vfT,vfT,_MM_SHUFFLE(0,0,0,0));			// vT0    = [    T0,   T0,      T0,     T0 ]
		vftemp		= _mm_shuffle_ps(vfr,vfr,_MM_SHUFFLE(2,1,3,0));			// vtemp  = [    r2,    r1,     r3,     r0 ]
		vftemp		= _mm_mul_ps(vftemp, vfT0);								// vtemp  = [ r2*T0, r1*T0,  r3*T0,  r0*T0 ]
		vfT0		= _mm_shuffle_ps(vfT,vfT,_MM_SHUFFLE(1,1,1,1));			// vT0    = [    T1,    T1,     T1,     T1 ]
		vftemp2		= _mm_shuffle_ps(vfr,vfr,_MM_SHUFFLE(3,0,2,1));			// vtemp2 = [    r3,    r0,     r2,     r1 ]
		vftemp2		= _mm_mul_ps(vftemp2,vfT0);								// vtemp2 = [ r3*T1, r0*T1,  r2*T1,  r1*T1 ]
		vftemp2		= _mm_mul_ps(vftemp2,vfsign2);							// vtemp2 = [ r3*T1, r0*T1, -r2*T1,  r1*T1 ]
		vftemp2		= _mm_add_ps(vftemp,vftemp2);							// vtemp += [ r3*T1, r0*T1, -r2*T1, -r1*T1 ]
		x2[0]		= _mm_cvtps_epi32(vftemp2);

		pfT -= step;
		--x1;
		--x2;
	}
	while(pfT > LoopEnd);
}

STIN void mdct_butterfly_generic(DATA_TYPE *x,int points,int step)
{
	register float		*T = fsincos_lookup0;
	register DATA_TYPE	*x1	= x + points - 4;
	register DATA_TYPE	*x2	= x + (points>>1) - 4;
	REG_TYPE	r0, r1, r2, r3;

	do
	{
		r0		= x1[0] - x1[1];
		x1[0]	+= x1[1];

		r1		= x1[3] - x1[2];
		x1[2]	+= x1[3];

		r2		= x2[1] - x2[0];
		x1[1]	= x2[1] + x2[0];

		r3		= x2[3] - x2[2];
		x1[3]	= x2[3] + x2[2];

		fXPROD31( r1, r0, T[0], T[1], &x2[0], &x2[2] );
		fXPROD31( r2, r3, T[0], T[1], &x2[1], &x2[3] );

		T += step;
		x1 -= 4; 
		x2 -= 4;
	}
	while(T<fsincos_lookup0 + 1024);

	do
	{
		r0		= x1[0] - x1[1];
		x1[0]	+= x1[1];

		r1		= x1[2] - x1[3];
		x1[2]	+= x1[3];

		r2		= x2[0] - x2[1];
		x1[1]	= x2[1] + x2[0];

		r3		= x2[3] - x2[2];
		x1[3]	= x2[3] + x2[2];

		fXNPROD31( r0, r1, T[0], T[1], &x2[0], &x2[2] );
		fXNPROD31( r3, r2, T[0], T[1], &x2[1], &x2[3] );

		T -= step;
		x1 -= 4; 
		x2 -= 4;
	}
	while(T>fsincos_lookup0);
}
//====================================================================================================
//====================================================================================================
STIN void mdct_butterflies(DATA_TYPE *x,int points,int shift)
{
	int stages=8-shift;
	int i,j;

	for(i = 0 ; --stages > 0 ; i++)
	{
		for(j = 0 ; j < (1<<i) ; j++)
		{
			mdct_butterfly_generic(x+(points>>i)*j,points>>i,4<<(i+shift));
		}
	}

	for(j=0;j<points;j+=32)
	{
		mdct_butterfly_32(x+j);
	}
}

STIN void mdct_butterflies_SSE2(DATA_TYPE *x,int points,int shift)
{
	int stages=8-shift;
	int i,j;

	for(i = 0 ; --stages > 0 ; i++)
	{
		for(j = 0 ; j < (1<<i) ; j++)
		{
			mdct_butterfly_generic_SSE2(x+(points>>i)*j,points>>i,4<<(i+shift));
		}
	}

	for(j=0;j<points;j+=32)
	{
		mdct_butterfly_32_SSE2(x+j);
	}
}
//====================================================================================================
//====================================================================================================
static unsigned char bitrev[16]={0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};

STIN int bitrev12(int x)
{
	return	bitrev[x>>8]
			|(bitrev[(x&0x0f0)>>4]<<4)
			|(((int)bitrev[x&0x00f])<<8);
}
//====================================================================================================
//====================================================================================================
STIN void mdct_bitreverse(DATA_TYPE *x,int n,int shift)
{
	int          bit   = 0;
	DATA_TYPE   *w     = x+(n>>1);

	do
	{
		DATA_TYPE  b     = bitrev12(bit++);
		DATA_TYPE *xx    = x + (b>>shift);
		REG_TYPE  r;

		w    -= 2;

		if(w>xx)
		{
			r      = xx[0];
			xx[0]  = w[0];
			w[0]   = r;

			r      = xx[1];
			xx[1]  = w[1];
			w[1]   = r;
		}
	}
	while(w>x);
}
//====================================================================================================
//====================================================================================================
STIN void mdct_step7(DATA_TYPE *x,int n,int step)
{
	DATA_TYPE   *w0 = x;
	DATA_TYPE   *w1 = x+(n>>1);
	float		*T = (step>=4)?(fsincos_lookup0+(step>>1)):fsincos_lookup1;
	float		*Ttop  = T+1024;
	float		fr0,fr1,fr2,fr3;

	do
	{
		w1    -= 2;

		fr0		= (float)(w0[0] + w1[0]);
		fr1		= (float)(w1[1] - w0[1]);
		fr2		= (fr0 * T[1] + fr1 * T[0]) * 0.5f;
		fr3		= (fr1 * T[1] - fr0 * T[0]) * 0.5f;

		T+=step;

		fr0     = (float)((w0[1] + w1[1])>>1);
		fr1     = (float)((w0[0] - w1[0])>>1);
		w0[0]  = (DATA_TYPE)(fr0 + fr2);
		w0[1]  = (DATA_TYPE)(fr1 + fr3);
		w1[0]  = (DATA_TYPE)(fr0 - fr2);
		w1[1]  = (DATA_TYPE)(fr3 - fr1);

		w0    += 2;
	}
	while(T<Ttop);

	do
	{
		w1    -= 2;

		fr0     = (float)(w0[0] + w1[0]);
		fr1     = (float)(w1[1] - w0[1]);
		T-=step;
		fr2		= (fr0 * T[0] + fr1 * T[1]) * 0.5f;
		fr3     = (fr1 * T[0] - fr0 * T[1]) * 0.5f;

		fr0     = (float)((w0[1] + w1[1])>>1);
		fr1     = (float)((w0[0] - w1[0])>>1);
		w0[0]  = (DATA_TYPE)(fr0 + fr2);
		w0[1]  = (DATA_TYPE)(fr1 + fr3);
		w1[0]  = (DATA_TYPE)(fr0 - fr2);
		w1[1]  = (DATA_TYPE)(fr3 - fr1);

		w0    += 2;
	}
	while(w0<w1);
}
//====================================================================================================
//====================================================================================================
STIN void mdct_step8(DATA_TYPE *x, int n, int step)
{
	float		*T;
	float		*V;
	DATA_TYPE	*iX =x+(n>>1);
	step>>=2;

	switch(step)
	{
	default:
		T = (step>=4)?(fsincos_lookup0+(step>>1)):fsincos_lookup1;
		do
		{
			float	fr0	= (float)x[0];
			float	fr1	= (float)-x[1];
			fXPROD31( fr0, fr1, T[0], T[1], x, x+1);
			T+=step;
			x  +=2;
		}
		while(x<iX);
		break;

	case 1: 
		{
			/* linear interpolation between table values: offset=0.5, step=1 */
			float		fr0,fr1,fv0,fv1,ft0,ft1;

			T	= fsincos_lookup0;
			V	= fsincos_lookup1;
			ft0	= (*T++) * 0.5f;
			ft1	= (*T++) * 0.5f;

			do
			{
				fr0	= (float)x[0];
				fr1	= (float)-x[1];
				fv0	= (*V++) * 0.5f;
				fv1	= (*V++) * 0.5f;
				ft0	+= fv0;
				ft1	+= fv1;
				fXPROD31( fr0, fr1, ft0, ft1, x, x+1 );

				fr0 =  x[2];
				fr1 = -x[3];
				ft0 = (*T++) * 0.5f;
				ft1 = (*T++) * 0.5f;
				fv0 += ft0;
				fv1 += ft1;
				fXPROD31( fr0, fr1, fv0, fv1, x+2, x+3 );

				x += 4;
			}
			while(x<iX);

			break;
		}

	case 0: 
		{
			/* linear interpolation between table values: offset=0.25, step=0.5 */
			float		fq0,fq1,fr0,fr1,fv0,fv1,ft0,ft1;

			T	= fsincos_lookup0;
			V	= fsincos_lookup1;
			ft0	= *T++;
			ft1	= *T++;
			do
			{
				fv0  = *V++;
				fv1  = *V++;
				fq0 = (fv0 - ft0) * 0.25f;
				fq1 = (fv1 - ft1) * 0.25f;
				ft0 += fq0;
				ft1 += fq1;
				fr0  = (float)x[0];
				fr1  = (float)-x[1];	
				fXPROD31( fr0, fr1, ft0, ft1, x, x+1 );

				ft0  = fv0 - fq0;
				ft1  = fv1 - fq1;
				fr0  = (float)x[2];
				fr1  = (float)-x[3];
				fXPROD31( fr0, fr1, ft0, ft1, x+2, x+3 );

				ft0  = *T++;
				ft1  = *T++;
				fq0 = (ft0 - fv0) * 0.25f;
				fq1 = (ft1 - fv1) * 0.25f;
				fv0 += fq0;
				fv1 += fq1;
				fr0  = (float)x[4];
				fr1  = (float)-x[5];
				fXPROD31( fr0, fr1, fv0, fv1, x+4, x+5 );

				fv0  = ft0 - fq0;
				fv1  = ft1 - fq1;
				fr0  = (float)x[6];
				fr1  = (float)-x[7];
				fXPROD31( fr0, fr1, fv0, fv1, x+5, x+6 );

				x+=8;
			}
			while(x<iX);
			break;
		}
	}
}
//====================================================================================================
// partial; doesn't perform last-step deinterleave/unrolling.  That
//  can be done more efficiently during pcm output
//====================================================================================================
void mdct_backward(int n, DATA_TYPE *in)
{
	int shift;
	int step;

	for (shift=4;!(n&(1<<shift));shift++);
	shift=13-shift;
	step=2<<shift;

	presymmetry(in,n>>1,step);

	mdct_butterflies(in,n>>1,shift);

	mdct_bitreverse(in,n,shift);

	mdct_step7(in,n,step);

	mdct_step8(in,n,step);
}

void mdct_backward_SSE2(int n, DATA_TYPE *in)
{
	int shift;
	int step;

	for (shift=4;!(n&(1<<shift));shift++);
	shift=13-shift;
	step=2<<shift;

	presymmetry(in,n>>1,step);

	mdct_butterflies_SSE2(in,n>>1,shift);

	mdct_bitreverse(in,n,shift);

	mdct_step7(in,n,step);

	mdct_step8(in,n,step);
}
//====================================================================================================
//====================================================================================================
void mdct_shift_right(int n, DATA_TYPE *in, DATA_TYPE *right)
{
	int i;
	n>>=2;
	in+=1;

	for(i=0;i<n;i++)
	{
		right[i]=in[i<<1];
	}
}
//====================================================================================================
//====================================================================================================
void mdct_unroll_lap(	int			n0,
						int			n1,
						int			lW,
						int			W,
						DATA_TYPE	*in,
						DATA_TYPE	*right,
						LOOKUP_T	*w0,
						LOOKUP_T	*w1,
						ogg_int16_t	*out,
						int			step,
						int			start, /* samples, this frame */
						int			end    /* samples, this frame */)
{
	DATA_TYPE *l=in+(W&&lW ? n1>>1 : n0>>1);
	DATA_TYPE *r=right+(lW ? n1>>2 : n0>>2);
	DATA_TYPE *post;
	LOOKUP_T *wR=(W && lW ? w1+(n1>>1) : w0+(n0>>1));
	LOOKUP_T *wL=(W && lW ? w1         : w0        );

	int preLap=(lW && !W ? (n1>>2)-(n0>>2) : 0 );
	int halfLap=(lW && W ? (n1>>2) : (n0>>2) );
	int postLap=(!lW && W ? (n1>>2)-(n0>>2) : 0 );
	int n,off;

	/* preceeding direct-copy lapping from previous frame, if any */
	if(preLap)
	{
		n      = (end<preLap?end:preLap);
		off    = (start<preLap?start:preLap);
		post   = r-n;
		r     -= off;
		start -= off;
		end   -= n;
		while(r>post)
		{
			*out = CLIP_TO_15((*--r)>>9);
			out+=step;
		}
	}

	/* cross-lap; two halves due to wrap-around */
	n      = (end<halfLap?end:halfLap);
	off    = (start<halfLap?start:halfLap);
	post   = r-n;
	r     -= off;
	l     -= off*2;
	start -= off;
	wR    -= off;
	wL    += off;
	end   -= n;

	while(r>post)
	{
		l-=2;
		*out = CLIP_TO_15((MULT31(*--r,*--wR) + MULT31(*l,*wL++))>>9);
		out+=step;
	}

	n      = (end<halfLap?end:halfLap);
	off    = (start<halfLap?start:halfLap);
	post   = r+n;
	r     += off;
	l     += off*2;
	start -= off;
	end   -= n;
	wR    -= off;
	wL    += off;
	while(r<post)
	{
		*out = CLIP_TO_15((MULT31(*r++,*--wR) - MULT31(*l,*wL++))>>9);
		out+=step;
		l+=2;
	}

	/* preceeding direct-copy lapping from previous frame, if any */
	if(postLap)
	{
		n      = (end<postLap?end:postLap);
		off    = (start<postLap?start:postLap);
		post   = l+n*2;
		l     += off*2;
		while(l<post)
		{
			*out = CLIP_TO_15((-*l)>>9);
			out+=step;
			l+=2;
		}
	}
}
