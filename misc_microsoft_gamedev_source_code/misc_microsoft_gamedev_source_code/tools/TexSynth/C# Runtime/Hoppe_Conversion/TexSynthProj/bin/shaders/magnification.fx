/** 

  Magnification shader - Sylvain Lefebvre - (c) Microsoft Corp.
  
  Display a synthesized texture from a synthesized index map and the examplar.
  Perform magnification if a high res exemplar is provided.
  
  'Parallel Controllable Texture Synthesis' - Sylvain Lefebvre and Hugues Hoppe
  http://research.microsoft.com/projects/ParaTexSyn/

*/

//-----------------------------------------------------------------------------

// Index map
texture IndexMap;

// Index map sampler
sampler S_IndexMap = sampler_state
{
  Texture   = (IndexMap);
  MipFilter = Point;  // nearest mode !
  MinFilter = Point;
  MagFilter = Point;
  AddressU  = Wrap;
  AddressV  = Wrap;
};

// Exemplar
texture Exemplar;

// Exemplar sampler
sampler S_Exemplar = sampler_state
{
  Texture   = (Exemplar);

  MipFilter = Linear;
  MinFilter = Linear;
  MagFilter = Linear;
  
  AddressU  = Wrap;
  AddressV  = Wrap;
};

// local frames
texture LocalFrames;

// local frames sampler
sampler S_LocalFrames = sampler_state
{
  Texture   = <LocalFrames>;
  MipFilter = Point;
  MinFilter = Point;
  MagFilter = Point;
  AddressU  = Wrap;
  AddressV  = Wrap;
};

//-----------------------------------------------------------------------------

// Vertex declaration
struct VS_OUTPUT
{
  float4 Pos         : POSITION;
  float2 TexCoords   : TEXCOORD0;
  float2 TexCoords0  : TEXCOORD1;
  float2 TexCoords1  : TEXCOORD2;
  float2 TexCoords2  : TEXCOORD3;
  float2 TexCoords3  : TEXCOORD4;
  float4 Mip         : TEXCOORD5;
};

//-----------------------------------------------------------------------------

// Texture parameters

float4  DispRes;      // zw = 1/xy       - Screen display resolution

float4  IndexMapRes;  // zw = 1/xy       - Index map resolution

float4  ExemplarRes; // y = 1/x  zw=0,0  - Exemplar resolution: low res version used for synthesis

float   ExemplarHighRes; //              - Exemplar resolution: high res version

float   NumExHighresLevels; //           - Number of MIP-mapping levels of the exemplar

// Current view window (for orthogonal display)

float4  DispTrl=float4(0,0,0,0);

float2  DispScl=float2(1,1);

float   invZoomFactor;

// Define faster positive modulo op

#define fmodp(x,n) ((n)*frac((x)/(n)))

//-----------------------------------------------------------------------------


// computes exemplar coordinates from ij
float2 tex2ex(float2 ij)
{
  return (ij);
}


//-----------------------------------------------------------------------------


float4 ps_magnification_no_branching_ddx_ddy_ps2a(VS_OUTPUT      In) : COLOR
{
  
  float2 interp   = frac(In.TexCoords0*IndexMapRes.xy);  
  float2 offs     = (0.5-interp)*IndexMapRes.zw;

  float2 tc0=In.TexCoords0+offs;
  float2 tc1=In.TexCoords1+offs;
  float2 tc2=In.TexCoords2+offs;
  float2 tc3=In.TexCoords3+offs;

  float2 local         = interp*ExemplarRes.y;
  float2 invlocal      = local - ExemplarRes.y;
  float4 swizzle_local = float4(local,invlocal);

  float2 filter_tc = In.TexCoords0*(IndexMapRes.xy*ExemplarRes.y);   // Texture coordinates must be in 'exemplar space' for filtering
                                                                    // (IndexMapRes.xy*ExemplarRes.y) represents the number of time
                                                                    // the exemplar can be tiled within the buffer texture
  float2 duvdx     = ddx(filter_tc);
  float2 duvdy     = ddy(filter_tc);

  float3 ij00 = tex2D(S_IndexMap,tc0);
  float2 ij10 = tex2D(S_IndexMap,tc1);
  float2 ij11 = tex2D(S_IndexMap,tc2);
  float2 ij01 = tex2D(S_IndexMap,tc3);
  
  float4 c00  = tex2D(S_Exemplar,float2(tex2ex(ij00) + swizzle_local.xy),duvdx,duvdy);
  float4 c01  = tex2D(S_Exemplar,float2(tex2ex(ij01) + swizzle_local.xw),duvdx,duvdy);
  float4 c11  = tex2D(S_Exemplar,float2(tex2ex(ij11) + swizzle_local.zw),duvdx,duvdy);
  float4 c10  = tex2D(S_Exemplar,float2(tex2ex(ij10) + swizzle_local.zy),duvdx,duvdy);

  return lerp(lerp(c00,c10,interp.x),lerp(c01,c11,interp.x),interp.y);
}


//-----------------------------------------------------------------------------


float4 ps_magnification_no_branching_ddx_ddy(VS_OUTPUT      In) : COLOR
{
  float2 interp   = frac(In.TexCoords0*IndexMapRes.xy);  
  float2 offs     = (0.5-interp)*IndexMapRes.zw;

  float2 tc0=In.TexCoords0+offs;
  float2 tc1=In.TexCoords1+offs;
  float2 tc2=In.TexCoords2+offs;
  float2 tc3=In.TexCoords3+offs;

  float2 local         = interp*ExemplarRes.y;
  float2 invlocal      = local - ExemplarRes.y;
  float4 swizzle_local = float4(local,invlocal);

  float2 filter_tc = In.TexCoords0*(IndexMapRes.xy*ExemplarRes.y);   // Texture coordinates must be in 'exemplar space' for filtering
                                                                    // (IndexMapRes.xy*ExemplarRes.y) represents the number of time
                                                                    // the exemplar can be tiled within the buffer texture
  float2 duvdx     = ddx(filter_tc);
  float2 duvdy     = ddy(filter_tc);
  float  lod       = (NumExHighresLevels + log2(max(length(duvdx),length(duvdy))));
                                              // ^ assumes isotropic filtering to make use of tex2Dlod
  float2 ij00 = tex2D(S_IndexMap,tc0);
  float2 ij10 = tex2D(S_IndexMap,tc1);
  float2 ij11 = tex2D(S_IndexMap,tc2);
  float2 ij01 = tex2D(S_IndexMap,tc3);
  
  float4 c00  = tex2Dlod(S_Exemplar,float4(tex2ex(ij00) + swizzle_local.xy,0,lod));
  float4 c01  = tex2Dlod(S_Exemplar,float4(tex2ex(ij01) + swizzle_local.xw,0,lod));
  float4 c11  = tex2Dlod(S_Exemplar,float4(tex2ex(ij11) + swizzle_local.zw,0,lod));
  float4 c10  = tex2Dlod(S_Exemplar,float4(tex2ex(ij10) + swizzle_local.zy,0,lod));

  return lerp(lerp(c00,c10,interp.x),lerp(c01,c11,interp.x),interp.y);  
}


//-----------------------------------------------------------------------------


float4 ps_magnification_ortho_no_branching(VS_OUTPUT      In) : COLOR
{
  float2 interp    = frac(In.TexCoords0*IndexMapRes.xy);
  float2 offs      = (0.5-interp)*IndexMapRes.zw;

  //interp.x = -interp.x;
  //interp.y = -interp.y;

  float2 tc0       = In.TexCoords0+offs;
  float2 tc1       = In.TexCoords1+offs;
  float2 tc2       = In.TexCoords2+offs;
  float2 tc3       = In.TexCoords3+offs;

  float2 local         = interp*ExemplarRes.y;
  float2 invlocal      = local - ExemplarRes.y;
  float4 swizzle_local = float4(local,invlocal);
  
  float  lod  = In.Mip.w;
  
  // Compute texture coordinate at exemplar resolution
  //float2 tc   = In.Tex0*IndexMapRes*EemplarRes.y;
  //tc          = tc - frac(tc*ExemplarRes.xx)*ExemplarRes.yy;

  float4 ij00 = tex2D(S_IndexMap,tc0);
  float2 ij10 = tex2D(S_IndexMap,tc1);
  float2 ij11 = tex2D(S_IndexMap,tc2);
  float2 ij01 = tex2D(S_IndexMap,tc3);

  float2x2 M00  = tex2Dlod(S_LocalFrames,float4(tc0,0,lod));
  float2x2 M01  = tex2Dlod(S_LocalFrames,float4(tc1,0,lod));
  float2x2 M11  = tex2Dlod(S_LocalFrames,float4(tc2,0,lod));
  float2x2 M10  = tex2Dlod(S_LocalFrames,float4(tc3,0,lod));

  float4 c00  = tex2Dlod(S_Exemplar,float4(tex2ex(ij00) + mul(M00,swizzle_local.xy),0,lod));
  float4 c01  = tex2Dlod(S_Exemplar,float4(tex2ex(ij01) + mul(M01,swizzle_local.xw),0,lod));
  float4 c11  = tex2Dlod(S_Exemplar,float4(tex2ex(ij11) + mul(M11,swizzle_local.zw),0,lod));
  float4 c10  = tex2Dlod(S_Exemplar,float4(tex2ex(ij10) + mul(M10,swizzle_local.zy),0,lod));

  if (0) {
    // to mask empty part of atlases (only with 4 channel buffers)
    // also does not work with advection
    if (ij00.w != 0.0) {
      return 1;
    } else {
      float4 clr = lerp(lerp(c00,c10,interp.x),lerp(c01,c11,interp.x),interp.y);
      return clr;
    }
  } else {
    float4 clr = lerp(lerp(c00,c10,interp.x),lerp(c01,c11,interp.x),interp.y);
    return clr;
  }
}


//-----------------------------------------------------------------------------


float4 ps_magnification_ortho_branching(VS_OUTPUT      In) : COLOR
{
  float2 interp   = frac(In.TexCoords0*IndexMapRes.xy);  
  float2 offs     = (0.5-interp)*IndexMapRes.zw;

  float2 tc0=In.TexCoords0+offs;  
  float2 tc1=In.TexCoords1+offs;  
  float2 tc2=In.TexCoords2+offs;  
  float2 tc3=In.TexCoords3+offs;  

  float2 local         = interp*ExemplarRes.y;
  float2 invlocal      = local - ExemplarRes.y;
  float4 swizzle_local = float4(local,invlocal);

  float  lod  = In.Mip.w;

  float3 ij00 = tex2D(S_IndexMap,tc0);
  float4 c00  = tex2Dlod(S_Exemplar,float4(tex2ex(ij00) + swizzle_local.xy,0,lod));

  if (ij00.b < 0.5) // The blue channel flags patch boundaries - It is computed in a previous pass
    // patch interior - default bilinear interpolation is valid
	  return (c00);
  else
  {
    // patch boundary - must do bilinear interpolation between neighbouring patches (4 at most)
    float2 ij10 = tex2Dlod(S_IndexMap,float4(tc1,0,0));
    float2 ij11 = tex2Dlod(S_IndexMap,float4(tc2,0,0));
    float2 ij01 = tex2Dlod(S_IndexMap,float4(tc3,0,0));

    float4 c01  = tex2Dlod(S_Exemplar,float4(tex2ex(ij01) + swizzle_local.xw,0,lod));
    float4 c11  = tex2Dlod(S_Exemplar,float4(tex2ex(ij11) + swizzle_local.zw,0,lod));
    float4 c10  = tex2Dlod(S_Exemplar,float4(tex2ex(ij10) + swizzle_local.zy,0,lod));

    return lerp(lerp(c00,c10,interp.x),lerp(c01,c11,interp.x),interp.y);
  }
  
}

//-----------------------------------------------------------------------------
/**

  Vertex program for orthogonal display.  
  
*/
//-----------------------------------------------------------------------------


struct VS_INPUT
{
    float4 Pos       : POSITION;
    float2 Tex1      : TEXCOORD1;
};



VS_OUTPUT vs_display(VS_INPUT In) 
{
  VS_OUTPUT o;
  
  float2 pos    = (In.Pos.xy-0.5)*2.0 + float2(-1.0,-1.0)*DispRes.zw;
  
  float2 tc     = In.Tex1;
  float2 zoomed = ((tc-0.5)*invZoomFactor+0.5);
  float2 tctr   = zoomed + DispTrl.zw*IndexMapRes.zw;
  
  float idxmap_ratio = IndexMapRes.y/IndexMapRes.x;
  float disp_ratio   = DispRes.y/DispRes.x;
  float ratio        = disp_ratio/idxmap_ratio;
  o.Pos = float4(float2(pos.x*ratio/* - (1-ratio)*0.9 */,-pos.y),0.0,1.0);

  o.TexCoords  = tctr;
  
  o.TexCoords0 = tctr - 0.5*IndexMapRes.zw;
  o.TexCoords1 = tctr - 0.5*IndexMapRes.zw + float2(1.0,0.0)*IndexMapRes.zw;
  o.TexCoords2 = tctr - 0.5*IndexMapRes.zw + float2(1.0,1.0)*IndexMapRes.zw;
  o.TexCoords3 = tctr - 0.5*IndexMapRes.zw + float2(0.0,1.0)*IndexMapRes.zw;
  
  float2 zoom_xy = invZoomFactor*IndexMapRes.xy*ExemplarRes.y*DispRes.zw;
  float  lod     = (NumExHighresLevels + log2(max(zoom_xy.x,zoom_xy.y)));
  o.Mip          = float4(zoom_xy,0,lod);

  return (o);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
/**

 Other shaders for visualization

*/
//-----------------------------------------------------------------------------


float4 ps_display_show_indices(VS_OUTPUT      In) : COLOR
{
  float2 tc   = In.TexCoords;
  
  float4 ij   = tex2D(S_IndexMap,tc);

  //return (float4( ij.xy ,0 ,0));

  return (float4( ij.xyz ,0));

  //return (tex2D(S_Exemplar,tex2ex(ij.zw)));

  //return (float4( ij.w, 0, ij.z ,0)); 
  
  //return (float4( ij.zw ,0,0)); 
}

//-----------

float4 ps_display_show_indices_novp(VS_OUTPUT      In) : COLOR
{
  float2 tc   = In.TexCoords*DispScl + DispTrl.xy;
  
  float4 ij   = tex2D(S_IndexMap,tc);

  //return (float4( ij.xy ,0 ,0));

  return (float4( ij.xyz ,0));

  // return (float4( ij.w, 0, ij.z ,0));
}

//-----------

float2 tex2ex_patches(float2 ij)
{
  return (0.5*ExemplarRes.y + ij);
}

//-----------

float4 ps_display_show_patches(VS_OUTPUT      In) : COLOR
{
  // snap to nearest  - FIXME: should not this be done automatically by nearest lookup mode ?
  // => pb: small differences btw frac/floor and nearest sampling
  float2 tc   = (floor(In.TexCoords * IndexMapRes) + 0.5) / IndexMapRes;
  
  float2 ij   = tex2D(S_IndexMap,tc);
  
  float2 delta= floor(tex2ex_patches(ij)*ExemplarRes.xx) - floor((tc  + DispTrl.xy)*IndexMapRes);
  
//  return float4(fmodp(delta,ExemplarRes.xx)*ExemplarRes.yy,0,0); // show offset map
  
  float2 clr2 = fmodp(delta,ExemplarRes.xx);
  float3 clr  = float3(clr2.x*61.0,
                       1.0-15.0*clr2.y,
                       clr2.x*7.0+clr2.y*13.0);
  return (float4( frac(clr/256.0) ,0));
}

//-----------

float4 ps_display_show_patches_novp(VS_OUTPUT      In) : COLOR
{
  float2 tc   = In.TexCoords*DispScl + DispTrl.xy;

  // snap to nearest  - FIXME: should not this be done automatically by nearest lookup mode ?
  // => pb: small differences btw frac/floor and nearest sampling
  tc   = (floor(tc * IndexMapRes) + 0.5) / IndexMapRes;
  
  float2 ij   = tex2D(S_IndexMap,tc);

  float2 delta= floor(tex2ex_patches(ij)*ExemplarRes.xx) - floor((tc  + DispTrl.xy)*IndexMapRes);
  float2 clr2 = fmodp(delta,ExemplarRes.xx);
  float3 clr  = float3(clr2.x*61.0,
                       1.0-15.0*clr2.y,
                       clr2.x*7.0+clr2.y*13.0);
  return (float4( frac(clr/256.0) ,0));
}

//-----------

float4 ps_display_color(VS_OUTPUT      In) : COLOR
{
  float2 tc           = In.TexCoords;
  
  float4 ij_err_outside = tex2D(S_IndexMap,tc).xyzw;
  
    return (tex2D(S_Exemplar,tex2ex(ij_err_outside.xy)));

}

//-----------

float4 ps_display_color_novp(VS_OUTPUT      In) : COLOR
{
  float2 tc   = In.TexCoords*DispScl + DispTrl.xy;
  
  float2 interp   = frac(tc*IndexMapRes.xy);  
  float2 offs     = (0.5-interp)*IndexMapRes.zw;

  float2 tc0=tc+offs;  
  float2 tc1=tc+offs+float2(1.0,0.0)*IndexMapRes.zw;  
  float2 tc2=tc+offs+float2(1.0,1.0)*IndexMapRes.zw;  
  float2 tc3=tc+offs+float2(0.0,1.0)*IndexMapRes.zw;  

  float2 local         = interp*ExemplarRes.y;
  float2 invlocal      = local - ExemplarRes.y;
  float4 swizzle_local = float4(local,invlocal);

  float  lod  = In.Mip.w; // MIP-mapping level selection is computed from the vertex program
                          // Works only for orthogonal viewing - for general case,
                          // see 'ps_display_high_res_no_branching_ddx_ddy'

  float3 ij00 = tex2D(S_IndexMap,tc0);
  float2 ij10 = tex2D(S_IndexMap,tc1);
  float2 ij11 = tex2D(S_IndexMap,tc2);
  float2 ij01 = tex2D(S_IndexMap,tc3);

  float4 c00  = tex2Dlod(S_Exemplar,float4(tex2ex(ij00) + swizzle_local.xy,0,lod));
  float4 c01  = tex2Dlod(S_Exemplar,float4(tex2ex(ij01) + swizzle_local.xw,0,lod));
  float4 c11  = tex2Dlod(S_Exemplar,float4(tex2ex(ij11) + swizzle_local.zw,0,lod));
  float4 c10  = tex2Dlod(S_Exemplar,float4(tex2ex(ij10) + swizzle_local.zy,0,lod));

  return lerp(lerp(c00,c10,interp.x),lerp(c01,c11,interp.x),interp.y);
  
/*
  float2 tc   = In.TexCoords*DispScl + DispTrl.xy;
  
  float2 ij   = tex2D(S_IndexMap,tc);
  
  return (tex2D(S_Exemplar,tex2ex(ij)));
  */
/*
  // DEBUG
  float4 ijclr   = tex2D(S_IndexMap,tc);
  #define EXACT(p) (p*255.5/256.0) //((0.5+floor(p.xy * 255.0 + 0.5))/256.0)
  //return (float4( ijclr.zw ,0,0) );
  //return (float4( tex2Dlod(S_Exemplar,float4(EXACT(ijclr.xy),0,0)).xy ,0,0) );
  return (float4(32.0*abs(tex2Dlod(S_Exemplar, float4(EXACT(ijclr.xy),0,0)) - ijclr.zw) ,0,0));  // linear
*/
}

//-----------------------------------------------------------------------------

float4 ps_display(VS_OUTPUT      In) : COLOR
{
	return (tex2D(S_IndexMap,In.TexCoords*DispScl + DispTrl.xy));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/**

  Technique declarations

*/

//-----------------------------------------------------------------------------

technique t_display_magnification
{
  pass P0
  {
      VertexShader = compile vs_3_0 vs_display();
//	  PixelShader  = compile ps_3_0 ps_magnification_ortho_branching();
	  PixelShader  = compile ps_3_0 ps_magnification_ortho_no_branching();
//	  PixelShader  = compile ps_3_0 ps_magnification_no_branching_ddx_ddy(); 
//	  PixelShader  = compile ps_2_a ps_magnification_no_branching_ddx_ddy_ps2a(); 
  }  
}

//-----------------------------------------------------------------------------

technique t_display_color
{
  pass P0
  {
    VertexShader = compile vs_2_0 vs_display();
	  PixelShader  = compile ps_2_0 ps_display_color();
  }  
}

//-----------------------------------------------------------------------------

technique t_display_patches
{
  pass P0
  {
    VertexShader = compile vs_2_0 vs_display();
	  PixelShader  = compile ps_2_0 ps_display_show_patches();
  }  
}

//-----------------------------------------------------------------------------

technique t_display_indices
{
  pass P0
  {
    VertexShader = compile vs_2_0 vs_display();
	  PixelShader  = compile ps_2_0 ps_display_show_indices();
  }  
}

//-----------------------------------------------------------------------------

technique t_display_color_novp
{
  pass P0
  {
	  PixelShader = compile ps_3_0 ps_display_color_novp();
  }  
}

//-----------------------------------------------------------------------------

technique t_display_patches_novp
{
  pass P0
  {
	  PixelShader = compile ps_2_0 ps_display_show_patches_novp();
  }  
}

//-----------------------------------------------------------------------------

technique t_display_indices_novp
{
  pass P0
  {
	  PixelShader = compile ps_2_0 ps_display_show_indices_novp();
  }  
}

//-----------------------------------------------------------------------------

technique t_display
{
  pass P0
  {
	  PixelShader = compile ps_2_0 ps_display();
  }  
}

//-----------------------------------------------------------------------------
