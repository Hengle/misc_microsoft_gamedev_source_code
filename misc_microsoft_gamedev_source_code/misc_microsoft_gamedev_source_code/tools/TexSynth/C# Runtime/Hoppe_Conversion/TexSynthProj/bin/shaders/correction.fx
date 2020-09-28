/** 

  Correction shader
  
  Sylvain Lefebvre - Hugues Hoppe - (c) Microsoft Corp.
  
----------------

NOTES

- Similarity sets are used as C[ S[p+d] - d ]
  Another possibility is C[ S[p+d] ] - d *but* this would prevent storing projected
  neighborhoods together with candidates indices. Indeed after fetching C[ S[p+d] ], the
  candidate index C[ S[p+d] ] - d must be computed and then its projected neighborhood
  retrieved.

- tex2D does not behave properly with 
  MipFilter = Point;
  MinFilter = Point;
  MagFilter = Linear;
  
  =>  ***  ALWAYS CHECK THAT ANISOTROPIC filtering is disabled in settings!!  ***

FIXME

TODO

*/

#include "config.h"

//-----------------------------------------------------------------------------

// #define ANISOSYNTH  // enables anisometric synthesis 
                      // TODO: for perf measurement, do not
                      //       use when not performing aniso syn.

// #define LIMIT_TO_4D // comment for release

// #define PROJECT_MAD // comment for release

// #define USE_INDIRECTION // comment for release
// #undef  USE_INDIRECTION // DEBUG

//-----------------------------------------------------------------------------

#define TEX2D_EX(S,uv) tex2D(S,uv)

#define TEX2D_N(S,uv)  tex2D(S,float4(uv,0,0)) // tex2dlod ?


//-----------------------------------------------------------------------------

// recolored exemplar texture

uniform texture Recolored_0_3;

sampler S_Recolored_0_3 = sampler_state
{
  Texture   = (Recolored_0_3);
  MipFilter = Point;
  MinFilter = Point;
  MagFilter = Linear;
  AddressU  = Wrap;
  AddressV  = Wrap;
};



// quantization

uniform float4      UnqRecolored_Scale_0_3;

uniform float4      UnqRecolored_Mean_0_3;


// synthesis neighborhoods

uniform texture Neighborhoods_0_3;

sampler S_Neighborhoods_0_3 = sampler_state
{
  Texture   = (Neighborhoods_0_3);
  MipFilter = Point;
  MinFilter = Point;
  MagFilter = Linear;
  AddressU  = Wrap;
  AddressV  = Wrap;
};


// quantization

uniform float4      UnqNeighborhoods_Scale_0_3;
uniform float4      UnqNeighborhoods_Mean_0_3;


// previous pass texture
uniform texture Previous;

sampler S_Previous = sampler_state
{
  Texture   = (Previous);
  MipFilter = Point;
  MinFilter = Point;
  MagFilter = Point;
  AddressU  = Wrap;
  AddressV  = Wrap;
};

// k-nearests indices texture (similarity sets)

uniform texture SimilaritySet;

sampler S_SimilaritySet = sampler_state
{
  Texture   = (SimilaritySet);
  MipFilter = Point;
  MinFilter = Point;
  MagFilter = Point;
  AddressU  = Wrap;
  AddressV  = Wrap;
};

// local frames

uniform texture LocalFrames;

sampler S_LocalFrames = sampler_state        // TODO: when available, use autogenmipmap on localframe texture
{
  Texture   = <LocalFrames>;
  MipFilter = Point;
  MinFilter = Point;
  MagFilter = Linear; // Linear?
  AddressU  = Clamp;
  AddressV  = Clamp;
};

uniform texture LocalFramesInverse;

sampler S_LocalFramesInverse = sampler_state
{
  Texture   = <LocalFramesInverse>;
  MipFilter = Point;
  MinFilter = Point;
  MagFilter = Linear; // Linear?
  AddressU  = Clamp;
  AddressV  = Clamp;
};

// pixpack texture

uniform texture TexPixPackOffsets;

sampler S_TexPixPackOffsets = sampler_state
{
  Texture   = <TexPixPackOffsets>;
  MipFilter = Point;
  MinFilter = Point;
  MagFilter = Point;
  AddressU  = Clamp;
  AddressV  = Clamp;
};

// indirection texture

uniform texture Indirection;

sampler S_Indirection = sampler_state
{
  Texture   = <Indirection>;
  MipFilter = Point;
  MinFilter = Point;
  MagFilter = Point;
  AddressU  = Clamp;
  AddressV  = Clamp;
};

// eigen vectors for projection during synthesis

uniform float4 Ev0[9*2];   // Ev[n] is n-ith eigen vector
uniform float4 Ev1[9*2];
uniform float4 Ev2[9*2];
uniform float4 Ev3[9*2];
uniform float4 Ev4[9*2];
uniform float4 Ev5[9*2];
uniform float4 Ev6[9*2];
uniform float4 Ev7[9*2];



uniform float IsLevel0;		// level 0 flag - used to handle special case with compress_clr
uniform float LevelId;		// level id
uniform float4 LocalFramesRes;// local frames texture resolution (w,h,1/w,1/h)
uniform float2 QuadrantIndex;// subpass index
uniform float PassId;// Pass id (correction pass id)

// Exemplar size
uniform float  ExemplarSize;
uniform float  HalfOverExemplarSize;

// Neighbour offsets
uniform float  NeighOffset;       // neighborhood offset used for upsampling
uniform float  NeighOffsetLSynth; // neighborhood offset within exemplar level used for synthesis


uniform float4 WindowPos;// Window pos (x,y,w,h)
uniform float2 RelativeWindowPos;// Relative window pos (for 'shrinking' btw sub-passes)


uniform float4 PrevQuad;// Previous quad
uniform float  Coherence;// Controls coherence over jumps
uniform float  CutAvoidance;// Cut avoidance
uniform float4 PixPackOffsets[25];// Offsets for pixel packing

// Indirection texture positionning
uniform float4 IndirectionTexPos=float4(0,0,0,0); // -(x+.5)/w,-(y+.5)/h,1/w,1/h

// Time - for fun effects
uniform float Time;

//-----------------------------------------------------------------------------

// From vertices

//-----------------------------------------------------------------------------

#include "pack.fx"
#include "coordinates.fx"
#include "lookups.fx"
#include "output.fx"


#include "rand.fx"

//-----------------------------------------------------------------------------

#define M_PI 3.14159265358979323846

struct VS_OUTPUT
{
  float4 Pos         : POSITION;
  float2 tc			   : TEXCOORD0;
};
// ---------------------------------------------------------------------------
// Globals (varying per-pixel)

float2x2 J     = {1.0,0.0,
                  0.0,1.0};
float2x2 J_inv = {1.0,0.0,
                  0.0,1.0};

// ---------------------------------------------------------------------------
// Find candidate from neighbor (S[P+delta] - J * delta

float2 neigh_explr_2(float2 u,float2 offs)
{
#ifdef ANISOSYNTH
  float2 c = u + mul(J,offs*(NeighOffset/ExemplarSize));
  return (c);
#else
  return (u + offs*(NeighOffset/ExemplarSize));
#endif
}

float4 neigh_explr_4(float4 u,float2 offs)
{
#ifdef ANISOSYNTH
  float4 c = u + mul(J,offs*(NeighOffset/ExemplarSize)).xyxy;
  return (c);
#else
  return (u + offs.xyxy*(NeighOffset/ExemplarSize));
#endif
}

// ---------------------------------------------------------------------------
// Find neighbours in previous (computes P+Delta)
#define RADIUS 2

// compute from P and delta
float2 neigh_prev_calc(float2 A,float2 P,float2 delta,float4 prevquad)
{
  float2 o=(delta.xy + QuadrantIndex)*0.5;
  return ( ( P + 0.5 + floor(o) ) * prevquad.zw + o );
}

// use a texture lookup from delta
float2 neigh_prev_tex(float2 A,float2 P,float2 delta,float4 prevquad)
{
  float2 deltatex = ((delta + RADIUS) + 0.5) /5.0;
  float2 offs     = tex2D(S_TexPixPackOffsets,deltatex);
  
//  float2 deltatex = floor((delta + RADIUS) + 0.5);// /5.0;
 // int k = deltatex.x + deltatex.y*5;
 // float4 offs		= PixPackOffsets[k];
  
  float2 n        = ( ( P + 0.5 )*prevquad.zw + offs.xy);
  return (n);
}

// use constants
float2 neigh_prev(float2 A,float2 P,float2 delta,float4 prevquad)
{
	float2 deltatex = ((delta + RADIUS) + 0.5) /5.0;
	float2 offs     = tex2D(S_TexPixPackOffsets,deltatex);
	float2 n =( ( P + 0.5 )*prevquad.zw + offs);//PixPackOffsets[(delta.x+RADIUS)+(delta.y+RADIUS)*(2*RADIUS+1)].xy );
	return (n);
}

//-----------------------------------------------------------------------------

float4 decode_simset_4(float4 k,float2 u)
{
  return (k*255.01-128.0)/ExemplarSize + u.xyxy;
}

float2 decode_simset_2(float2 k,float2 u)
{
  return (k*255.01-128.0)/ExemplarSize + u.xy;
}

//-----------------------------------------------------------------------------
// Projection
//--------



#define PROJ_COMPUTED_NEIGHBOR(C03,N) {                    \
        C03 = C03*UnqRecolored_Scale_0_3 + UnqRecolored_Mean_0_3; \
        projEv0123 += float4(dot(Ev0[N*2+0],C03),              \
                             dot(Ev1[N*2+0],C03),              \
                             dot(Ev2[N*2+0],C03),              \
                             dot(Ev3[N*2+0],C03));             \
}




//-----------------------------------------------------------------------------
// Min search
//--------

#define KCOH_COMPUTE_MIN(CIJ,N03,COH) {                                \
        float4 diff03 = N03*UnqNeighborhoods_Scale_0_3 + sub0123;          \
        float  d      = (dot(diff03,diff03)) * COH;   \
        if (d <= minreg.z)                                                 \
        {                                                                  \
          minreg=float4(CIJ,d,0);                                          \
        }                                                                  \
		}

#define KCOH_SEARCH_EX(S_p_plus_delta,Delta,W) {                           \
        float4 ij0ij1   = tex2D(S_SimilaritySet,S_p_plus_delta);           \
		    float4 C_S      = decode_simset_4(ij0ij1,S_p_plus_delta);          \
        float4 k0k1_cij = neigh_explr_4(C_S, - Delta.xy );                 \
		    float4 n_0_3;                                                      \
        n_0_3  = TEX2D_N(S_Neighborhoods_0_3 , k0k1_cij.zw);               \
        KCOH_COMPUTE_MIN(k0k1_cij.zw,n_0_3,Coherence*W);             \
        n_0_3  = TEX2D_N(S_Neighborhoods_0_3 , k0k1_cij.xy);               \
        KCOH_COMPUTE_MIN(k0k1_cij.xy,n_0_3, 1.0*W);             \
        }

#define KCOH_PIJ_SEARCH_UNROLL_EX(Delta) {                                 \
        float2 S_p_plus_delta,p_plus_delta;                                \
        p_plus_delta = neigh_prev(aij,rij,Delta,PrevQuad);                 \
        S_p_plus_delta = lookup_S(p_plus_delta);                           \
        KCOH_SEARCH_EX(S_p_plus_delta,Delta,1.0);                          \
		}


//-----------------------------------------------------------------------------
// Averages
//--------

#define COMPUTE_AVERAGE_3(DeltaS,DeltaS_N0,DeltaS_N1,C03) {                 \
	      float2 u = lookup_S(neigh_prev(aij,rij,DeltaS,PrevQuad));               \
        float4 c03_u  = tex2D(S_Recolored_0_3,u);                               \
	      float2 uN0    = lookup_S(neigh_prev(aij,rij,DeltaS+DeltaS_N0,PrevQuad));\
	      uN0 = uN0 + ( -DeltaS_N0 )*(NeighOffset/ExemplarSize);                  \
        float4 c03_N0 = tex2D(S_Recolored_0_3,uN0);                             \
	      float2 uN1    = lookup_S(neigh_prev(aij,rij,DeltaS+DeltaS_N1,PrevQuad));\
	      uN1 = uN1 + ( -DeltaS_N1 )*(NeighOffset/ExemplarSize);                  \
        float4 c03_N1 = tex2D(S_Recolored_0_3,uN1);                             \
        C03 = (c03_u + c03_N0 + c03_N1) /*/ 3.0*/;                              \
}

#define COMPUTE_AVERAGE_3_ANISO(DeltaE,DeltaS,DeltaS_N0,DeltaS_N1,C03) {   \
	      float2 u = lookup_S(neigh_prev_tex(aij,rij,DeltaS,PrevQuad));          \
	      float2 exsp_JdeltaS = mul(J,DeltaS)*(NeighOffset/ExemplarSize);        \
	      float2 exsp_deltaE = DeltaE*(NeighOffsetLSynth/ExemplarSize);          \
	      float2 exsp_delta_sum = - exsp_JdeltaS + exsp_deltaE;                  \
	      u        = u + exsp_delta_sum;                                         \
        float4 c03_u  = tex2D(S_Recolored_0_3,u);                              \
	      float2 uN0    = lookup_S(neigh_prev_tex(aij,rij,DeltaS+DeltaS_N0,PrevQuad));  \
	      uN0 = uN0 + exsp_delta_sum - mul(J,DeltaS_N0*(NeighOffset/ExemplarSize));     \
        float4 c03_N0 = tex2D(S_Recolored_0_3,uN0);                            \
	      float2 uN1    = lookup_S(neigh_prev_tex(aij,rij,DeltaS+DeltaS_N1,PrevQuad));  \
	      uN1 = uN1 + exsp_delta_sum - mul(J,DeltaS_N1*(NeighOffset/ExemplarSize));     \
        float4 c03_N1 = tex2D(S_Recolored_0_3,uN1);                            \
        C03 = (c03_u + c03_N0 + c03_N1) /*/ 3.0*/;                             \
}

//-----------------------------------------------------------------------------
// Fragment programs
//--------


float2 closest(float2 delta)
{
  return round( 1.306*normalize(delta) ); // TODO optimization: replace by texture lkup in small tex ?
}

//--------

void deltaAverageFromDeltaNeighbors(float2 delta,
                                    out float4 deltaN)
{
  float3 d_z=float3(delta,0);	
	float x_mul_y = (delta.x * delta.y);
	// diagonal case \ /
	float4 d_a = delta.xyxy - d_z.zyxz;
	// straight case - |
	float4 d_b = delta.xyxy + float4(-delta.yx,delta.yx);
	// select
	deltaN = x_mul_y != 0 ? d_a : d_b;
}

//--------

// 4 neighbors with averaging

float4 ps_correction_pixpack_4_avg(VS_OUTPUT In,float2 rij,float2 aij,bool advection) : COLOR
{
  float4 projEv0123=float4(0,0,0,0);

  float4 TL_0_3;
  float4 TR_0_3;
  float4 BL_0_3;
  float4 BR_0_3;
  
#ifdef ANISOSYNTH
 
   float2 lftc  = (aij*NeighOffset+.5)*LocalFramesRes.zw;

   J     = tex2D(S_LocalFrames       ,lftc);
  J_inv = tex2D(S_LocalFramesInverse,lftc);
  
	// deltaS
  float2 dS_p1_p1 = closest(mul(J_inv,float2(1, 1)));
  float2 dS_p1_m1 = closest(mul(J_inv,float2(1,-1)));

	// deltaN (for averages)
	float2 dN_p1_0;
	float2 dN_0_p1;
	if (1) 
	{
	  // reference impl.
	  dN_p1_0  = closest(mul(J_inv,float2(1, 0)));
	  dN_0_p1  = closest(mul(J_inv,float2(0, 1)));
	} 
	else 
	{
	  // slightly faster impl. - small loss in quality however on complex jmap
	  float4 dN_p1_0_dN_0_p1;
	  deltaAverageFromDeltaNeighbors(dS_p1_p1,dN_p1_0_dN_0_p1);
	  dN_p1_0 = dN_p1_0_dN_0_p1.xy;
	  dN_0_p1 = dN_p1_0_dN_0_p1.zw;
	}
                   // neighbor   // relative deltas to take candidates for average
  COMPUTE_AVERAGE_3_ANISO(float2(-1,-1), -dS_p1_p1, -dN_p1_0, -dN_0_p1, TL_0_3);
  COMPUTE_AVERAGE_3_ANISO(float2( 1,-1),  dS_p1_m1,  dN_p1_0, -dN_0_p1, TR_0_3);
  COMPUTE_AVERAGE_3_ANISO(float2(-1, 1), -dS_p1_m1, -dN_p1_0,  dN_0_p1, BL_0_3);
  COMPUTE_AVERAGE_3_ANISO(float2( 1, 1),  dS_p1_p1,  dN_p1_0,  dN_0_p1, BR_0_3);

#else

	COMPUTE_AVERAGE_3(float2(-1,-1),float2( 0,-1),float2(-1, 0)  ,TL_0_3); // TODO rewrite with same order as above
	COMPUTE_AVERAGE_3(float2( 1,-1),float2( 0,-1),float2( 1, 0)  ,TR_0_3);
	COMPUTE_AVERAGE_3(float2(-1, 1),float2( 0, 1),float2(-1, 0)  ,BL_0_3);
	COMPUTE_AVERAGE_3(float2( 1, 1),float2( 0, 1),float2( 1, 0)  ,BR_0_3);

#endif

// Neighbors location (x denotes a neighbor used in averaging)
//   x   x
// x 0   1 x
//     
// x 3   4 x
//   x   x
//
// Invloved subpasses
//   2   2
// 3 4   4 3
//     1
// 3 4   4 3
//   2   2

	PROJ_COMPUTED_NEIGHBOR(TL_0_3, 0); // top left
	PROJ_COMPUTED_NEIGHBOR(TR_0_3, 1); // top right
	
	PROJ_COMPUTED_NEIGHBOR(BL_0_3, 2); // bottom left
	PROJ_COMPUTED_NEIGHBOR(BR_0_3, 3); // bottom right


	// subtract mean from projected neighborhood
	// -> unquantize offset is build in (ie. the -1.0 of (v*2.0-1.0)*scale + mean )
	projEv0123 -= UnqNeighborhoods_Mean_0_3;
	// prepare for min computation
	//                 ("- UnqNeighborhoods_Scale" baked in mean - see above)
	float4 sub0123 = /*- UnqNeighborhoods_Scale_0_3*/ - projEv0123;

	// search best candidates (k-coherent search)

	float4 minreg=float4(0.0,0.0,999999.0,0.0);
	
//if (!advection) 
{
  // removing these candidates improve perf. with advection, 
  // while still giving good results
  KCOH_PIJ_SEARCH_UNROLL_EX(float2(-1,-1));
  KCOH_PIJ_SEARCH_UNROLL_EX(float2( 1,-1));
  KCOH_PIJ_SEARCH_UNROLL_EX(float2(-1, 1));
//  KCOH_PIJ_SEARCH_UNROLL_EX(float2( 1, 1));
}

//  KCOH_PIJ_SEARCH_UNROLL_EX(float2( 0,-1));
//  KCOH_PIJ_SEARCH_UNROLL_EX(float2(-1, 0));
//  KCOH_PIJ_SEARCH_UNROLL_EX(float2( 1, 0));
//  KCOH_PIJ_SEARCH_UNROLL_EX(float2( 0, 1));
  
//  KCOH_PIJ_SEARCH_UNROLL_EX(float2( 0, 0));


  return (float4(minreg.xy,0,0));

}


//-----------------------------------------------------------------------------

float4 ps_correction(VS_OUTPUT In,uniform bool advection) : COLOR
{
  float2 tc=In.tc;//TexCoords1;

  float2 rij=floor(tc);
  
  float2 aij   =relative2absolute(rij*2.0 + QuadrantIndex,WindowPos);




/*	//CLM only adjust pixels that actually lie on our boundries.
	float4 ot = float4(0,0,0,0);
	int2 sVal = (In.TexCoords0 * 512) % int2(128,128); // 512 = buffer size
	if( sVal.x >120 || sVal.x <8 || sVal.y >120 ||sVal.y <8)
		ot = (ps_correction_pixpack_4_avg(In,rij,aij,advection));
	else
		ot = tex2D( S_Previous , neigh_prev(aij,rij,float2(0,0),PrevQuad)) ;
		
	return ot;
*/	
	return (ps_correction_pixpack_4_avg(In,rij,aij,advection));
   // direct copy (pass-trhough)
  //return (tex2D( S_Previous , neigh_prev(aij,rij,float2(0,0),PrevQuad)) );
}


//-----------------------------------------------------------------------------

struct VS_INPUT
{
    float4 Pos       : POSITION;
    float2 Tex1      : TEXCOORD1;
};


uniform float QuadZ = 0.0;

VS_OUTPUT vs_multexcoord(VS_INPUT In)
{
  VS_OUTPUT o;
  
  o.Pos       = float4(float2(In.Pos.x,-In.Pos.y) + float2(-1.0,1.0)/WindowPos.zw ,QuadZ,1.0);
  float2 tc = In.Tex1*WindowPos.zw + RelativeWindowPos.xy;
  o.tc = tc;

  return (o);
}


//-----------------------------------------------------------------------------
// HLSL Techniques
//---------


//-----------------------------------------------------------------------------

// technique for advection, removes diagonal candidates
technique t_correction_advect
{
  pass P0
  {
    VertexShader = compile vs_3_0 vs_multexcoord();
  	PixelShader  = compile ps_3_0 ps_correction(true);
  }  
}

//-----------------------------------------------------------------------------

// technique for standard correction
technique t_correction
{
  pass P0
  {
    VertexShader = compile vs_3_0 vs_multexcoord();
  	PixelShader  = compile ps_3_0 ps_correction(false);
  }  
}
