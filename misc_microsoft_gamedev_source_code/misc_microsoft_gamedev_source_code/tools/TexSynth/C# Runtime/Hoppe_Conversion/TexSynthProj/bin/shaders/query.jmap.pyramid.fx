/** 

  Shader to build jacobian map pyramid
  (to be used with D3DGenMipMapPyramid)
  
  Sylvain Lefebvre 2005-11-07 - (c) Microsoft Corp.

*/

//-----------------------------------------------------------------------------
    struct VS_OUTPUT 
    { 
      float4 Pos         : POSITION; 
      float2 Tex         : TEXCOORD1; 
    }; 
    struct VS_INPUT 
    { 
        float4 Pos       : POSITION; 
        float2 Tex       : TEXCOORD1; 
    }; 
    float4 Viewport; 
    float2 DestRegionCoord; 
    float2 DestRegionSize; 
    float2 InvDestRegionSize; 
    VS_OUTPUT vs_image_processing(VS_INPUT In) 
    { 
      VS_OUTPUT o; 
      o.Pos = float4(float2(In.Pos.x,-In.Pos.y) + float2(-1.0,1.0) / Viewport.zw ,0.0,1.0); 
      o.Tex = DestRegionCoord + In.Tex*DestRegionSize; 
      return (o); 
      }
    

texture ChildrenTexture;

sampler S_ChildrenTexture = sampler_state
{
  Texture   = (ChildrenTexture);
  MipFilter = Point;
  MinFilter = Point;
  MagFilter = Point;
  AddressU  = Wrap;
  AddressV  = Wrap;
};

float4 ps_main(VS_OUTPUT In) : COLOR
{
  float2 t00 = (floor(In.Tex)*2.0               + 0.5)*InvDestRegionSize*0.5;
  float2 t01 = (floor(In.Tex)*2.0 + float2(0,1) + 0.5)*InvDestRegionSize*0.5;
  float2 t11 = (floor(In.Tex)*2.0 + float2(1,1) + 0.5)*InvDestRegionSize*0.5;
  float2 t10 = (floor(In.Tex)*2.0 + float2(1,0) + 0.5)*InvDestRegionSize*0.5;
  
  float4 c00 = tex2D(S_ChildrenTexture,t00);
  float4 c01 = tex2D(S_ChildrenTexture,t01);
  float4 c11 = tex2D(S_ChildrenTexture,t11);
  float4 c10 = tex2D(S_ChildrenTexture,t10);
  
  return (c00+c01+c11+c10)/4.0;

  //return float4(floor(In.Tex)*InvDestRegionSize,0,0);
}


technique t_main 
	{ 
      pass P0 
      { 
        VertexShader = compile vs_3_0 vs_image_processing(); 
  	    PixelShader  = compile ps_3_0 ps_main(); 
      } 
    }

//-----------------------------------------------------------------------------
