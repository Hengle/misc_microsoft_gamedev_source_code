
uniform float  GlobalScale        = 1.0;
uniform int SelectedJMap=0;
uniform float  ExemplarMainDirection = 0;

// Painted JMap
texture PaintedJMap;

// sampler
sampler S_PaintedJMap = sampler_state
{
  Texture   = (PaintedJMap);
  MipFilter = point;
  MinFilter = point;
  MagFilter = point;
  AddressU  = Wrap;
  AddressV  = Wrap;
};


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
struct PS_OUTPUT
{
  float4 J     : COLOR0;
  float4 J_inv : COLOR1;
};

float4 Viewport; 
float2 DestRegionCoord; 
float2 DestRegionSize; 
float2 InvDestRegionSize; 
VS_OUTPUT vs_image_processing(VS_INPUT In) 
{ 
	VS_OUTPUT o; 
	o.Pos = float4(float2(In.Pos.x,-In.Pos.y) + float2(-1.0,1.0) / Viewport.zw ,0.0,1.0); 
	o.Tex = In.Tex; 
	return (o); 
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float2x2 inverse(float2x2 M)
{
  float2x2 IM;
  float det =  M[0][0]*M[1][1] - M[0][1]*M[1][0]; 
  IM[0][0]  =  M[1][1];
  IM[0][1]  = -M[0][1];
  IM[1][0]  = -M[1][0];
  IM[1][1]  =  M[0][0];
  IM = IM / det;
  return (IM);
}
//-----------------------------------------------------------------------------
// Paint JMap
//-----------------------------------------------------------------------------

static const float PI = 3.14159265;

void ps_paint_jmap(VS_OUTPUT In,out PS_OUTPUT Out)
{ 
  float3 t_scl = tex2D(S_PaintedJMap,In.Tex);
  // rotate by exemplar main dir
  float   a = ExemplarMainDirection;
  float2 sc;
  sincos(a,sc.x,sc.y);
  float2x2 R = float2x2( sc.y, sc.x,
                        -sc.x, sc.y );
  float3 b=float3(mul(R,(t_scl.xy*2.0-1.0)),0);
  // compute jocabian frame (TODO can be simplified!)
  b=normalize(b);
  float3 n = float3(0,0,1);
  float3 t = cross(b,n);
  float  s = (2.0 + 2.0*t_scl.z)*GlobalScale;
  // compute J and J_inv
  float3 dfdu = float3(1,0,0);
  float3 dfdv = float3(0,1,0);
  float2x2 M  = inverse(float2x2(t.x,b.x,t.y,b.y)) * s;
  float2x2 I  = inverse(M);
  
  Out.J     = float4(M[0][0],M[0][1],
                     M[1][0],M[1][1]);
  Out.J_inv = float4(I[0][0],I[0][1],
                     I[1][0],I[1][1]);
}
//-----------------------------------------------------------------------------
// Defines some canonical Jacobian maps for testing / demo purpose
//-----------------------------------------------------------------------------



void ps_selected_jmap(VS_OUTPUT In,out PS_OUTPUT Out)
{
  float3 t,b;
  float  scale=1.0;
  if        (SelectedJMap == 0)     // NOTE: these branching are *not* efficient. This shader is meant to produce still images
  {
    // indentity
    t = float3(1,0,0);
    b = float3(0,1,0);
    scale = GlobalScale*2.0;
  } 
  else if (SelectedJMap == 1) 
  {
    // s-twist
    float3 n = float3(0,0,1);
    float  i = max(1.0-length(In.Tex-0.5),0);
    float  a = lerp(0,3.14,i);
    t        = float3(cos(a),sin(a),0)*0.5;
    b        = (cross(t,n));
    scale    = GlobalScale;
  } 
  else if (SelectedJMap == 2) 
  {
    // s-twist with scaling
    float3 n = float3(0,0,1);
    float  i = max(1.0-length(In.Tex-0.5),0);
    float  a = lerp(0,3.14,i);
    b        = float3(cos(a),sin(a),0)*0.5*(1.0-i*0.75);
    t        = (cross(b,n));
    scale    = GlobalScale;
  } 
  else if (SelectedJMap == 3) 
  {
    // scaling, 0.5 2 0.5
    float3 n = float3(0,0,1);
    float  i = abs(In.Tex.x-0.5)*2.0;
    t        = float3(1,0,0);
    b        = float3(0,1,0);
    scale    = GlobalScale*3.0*(0.5+0.5*i);
  }
  else if (SelectedJMap == 4) 
  {
    // rotation
    float  a = In.Tex.x*4.0;
    float3 n = float3(0,0,1);
    t        = float3( cos(a),-sin(a),0);
    b        = float3( sin(a), cos(a),0);
    scale    = GlobalScale*3.0;
  } 
  else if (SelectedJMap == 5) 
  {
    // rotation + scaling
    float2 ctr = float2(.5,.5);
    float2 p = ctr - In.Tex;
    float  a = atan2(p.y,p.x);
    a = lerp(a,0,max(0,-2.0*(In.Tex.x-ctr.x)));
    float3 n = float3(0,0,1);
    t        = float3(cos(a),sin(a),0);
    b        = cross(t,n);
    scale    = (2.0+max(0.0,-(In.Tex.x-ctr.x)*3.0))*GlobalScale;
  } 
  else if (SelectedJMap == 6) 
  {
    // sink
    float2 ctr = float2(.5,.5);
    float2 p = ctr - In.Tex;
    float  a = atan2(p.y,p.x);
    float3 n = float3(0,0,1);
    float  i = max(1.0-length(In.Tex-0.5),0);
    t        = float3(cos(a),sin(a),0)*0.5*(1.0-i*0.75);
    b        = (cross(t,n));
    scale    = GlobalScale;  
  } 
  else if (SelectedJMap == 7) 
  {
    // 45 degree rotation
    float  a = 3.14159265/4.0;
    float3 n = float3(0,0,1);
    t        = float3( cos(a),-sin(a),0);
    b        = float3( sin(a), cos(a),0);
    scale    = GlobalScale*2.0;
  } 
  else 
  {
    t = float3(1,0,0);
    b = float3(0,1,0);
  }
  // compute J and J_inv
  float3 dfdu = float3(1,0,0);
  float3 dfdv = float3(0,1,0);
  float2x2 M  = inverse(float2x2(t.x,b.x,t.y,b.y)) * scale; //computeJs(t,b,dfdu,dfdv) * scale;
  float2x2 I  = inverse(M);
  
  Out.J     = float4(M[0][0],M[0][1],
                     M[1][0],M[1][1]);
  Out.J_inv = float4(I[0][0],I[0][1],
                     I[1][0],I[1][1]);
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void ps_identity(VS_OUTPUT In, out PS_OUTPUT Out)
{
  Out.J = float4(1.0,  0.0, 0.0,  1.0);	
  Out.J_inv = float4(1.0,  0.0,0.0,  1.0);
}


technique t_identity 
{ 
	pass P0 
	{ 
		VertexShader = compile vs_3_0 vs_image_processing (); 
		PixelShader  = compile ps_3_0 ps_identity (); 
	} 
}
technique t_paint_jmap
{
  pass P0
  {
	  VertexShader = compile vs_2_0 vs_image_processing();
	  PixelShader  = compile ps_3_0 ps_paint_jmap();
  }  
}
technique t_selected
	{ 
      pass P0 
      { 
        VertexShader = compile vs_3_0 vs_image_processing (); 
  	    PixelShader  = compile ps_3_0 ps_selected_jmap (); 
      } 
    }
//-----------------------------------------------------------------------------
