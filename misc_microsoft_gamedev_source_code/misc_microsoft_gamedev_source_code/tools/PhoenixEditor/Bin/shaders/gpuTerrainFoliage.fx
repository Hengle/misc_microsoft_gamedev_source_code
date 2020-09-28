
//-----------------------------------------------------------------------------
// Vertex Definitions
//-----------------------------------------------------------------------------
float4x4 worldViewProj	: WorldViewProjection; // This matrix will be loaded by the application

texture terrainPositionsTexture; // our vertex displacement texture
sampler terrainVertSampler = sampler_state
{
    Texture   = (terrainPositionsTexture);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
};
uniform float4 gQNData = float4(0,0,0,0);


///////-----------------------------------------------------------------------------
texture foliagePositionsTexture; // our vertex displacement texture
sampler gFoliagePositions = sampler_state
{
	Texture   = (foliagePositionsTexture);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP ;
};
texture foliageUVsTexture; // our vertex displacement texture
sampler gFoliageUVs = sampler_state
{
	Texture   = (foliageUVsTexture);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP ;
};

texture foliageAlbedoTexture; // our vertex displacement texture
sampler gFoliageAlbedo = sampler_state
{
	Texture   = (foliageAlbedoTexture);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP ;
};
texture foliageOpacityTexture; // our vertex displacement texture
sampler gFoliageOpacity = sampler_state
{
	Texture   = (foliageOpacityTexture);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP ;
};
uniform float gRCPBladeImageWidth=0.25f;
///////-----------------------------------------------------------------------------

struct VS_INPUT
{
    float3 position	: POSITION;
};

struct VS_OUTPUT
{
    float4 hposition	: POSITION;
    
    float2 uv0			: TEXCOORD0;		//same as hposition
};
///////-----------------------------------------------------------------------------

#define cNumBladesPerNode 64

#define M_PI 3.14159265358979323846
#define fmodp(x,n) ((n)*frac((x)/(n)))
float2 our_rand(float2 ij)
{
  const float4 a=float4(M_PI * M_PI * M_PI * M_PI, exp(4.0),  1.0, 0.0);
  const float4 b=float4(pow(13.0, M_PI / 2.0), sqrt(1997.0),  0.0, 1.0);

  float2 xy0    = ij/M_PI;
  float2 xym    = fmodp(xy0.xy,257.0)+1.0;
  float2 xym2   = frac(xym*xym);
  float4 pxy    = float4(xym.yx * xym2 , frac(xy0));
  float2 xy1    = float2(dot(pxy,a) , dot(pxy,b));
  float2 result = frac(xy1);
  
  return result;
}
float2 rotate_2d( float2 input, float theta )
{
	float2 temp;
	float sintheta = sin( theta );
	float costheta = cos( theta );
	temp.x = input.x * costheta + input.y * sintheta;
	temp.y = input.y * costheta - input.x * sintheta;
	return temp;
}
//----------------------------------------------------------------------------- 
float3 getTerrainPos(float2 uvVal)
{
	float2 uvVal2 = float2(	uvVal.x / ((float)cNumBladesPerNode*2),
							uvVal.y / ((float)cNumBladesPerNode*2));
	float3 offset = tex2Dlod( terrainVertSampler,   float4(uvVal2.x,uvVal2.y,0.f,0.f) ).xyz;
		
	return offset;
}
///////-----------------------------------------------------------------------------

VS_OUTPUT myvs( VS_INPUT IN )
{
    VS_OUTPUT OUT;
    
    int iIndex = IN.position.x;
    int bladeID = IN.position.y;

    float3 pos = float3(0,0,0);
    
    int P = floor(fmod(iIndex + 0.1f,10.0f));
    int bladeIndex = (int)(iIndex / 10);
    
    
    float2 trns = float2((int)(bladeIndex % cNumBladesPerNode),(int)(bladeIndex / cNumBladesPerNode));
	float3 oPos=getTerrainPos(trns);
	
	//get our blade vertPos
	float2 posUV = float2(P / 9.0f,0);
	posUV.x = ((posUV.x-0.01) * gRCPBladeImageWidth) + (bladeID * gRCPBladeImageWidth);
	float4 bPos = tex2Dlod( gFoliagePositions,   float4(posUV.x,posUV.y,0.f,0.f) );
	pos = bPos;
	
	float2 rnd = our_rand(float2(bladeIndex,bladeIndex));
	float2 rotation = rotate_2d(pos.xz,rnd.y * 360);
	float2 jitterTrans = ((rnd*2)-1) * 0.7;

	pos.xz = rotation;
	pos.xz += jitterTrans;
	pos.y *= rnd.x;	//random scaling
	pos.xz += (gQNData.zw + trns + oPos.xz)* gQNData.y;
	pos.y += oPos.y;
	
	
	float4 tPos = mul( float4(pos.x,pos.y,pos.z,1.f ),worldViewProj);

	OUT.hposition = tPos;
	
	float4 bUVs = tex2Dlod( gFoliageUVs,   float4(posUV.x,posUV.y,0.f,0.f) );
	OUT.uv0.x = bUVs.x;		//saturate((P %2) * gRCPBladeImageWidth + (bladeID * gRCPBladeImageWidth));
	OUT.uv0.y = 1-bUVs.y;	//1-(floor(P * 0.5f) * 0.25);
	
	
	
	return OUT;
}
///////-----------------------------------------------------------------------------
///////-----------------------------------------------------------------------------
///////-----------------------------------------------------------------------------
float4 myps( VS_OUTPUT IN ) : COLOR
{
	//return float4(IN.uv0.x,IN.uv0.y,0,1);//
	 float4 albedo = tex2D(gFoliageAlbedo,IN.uv0);
	 float opacity = tex2D(gFoliageOpacity,IN.uv0);
    return float4(albedo.xyz,opacity);//float4(IN.uv0.x,0,0,1);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

technique Technique0
{
     pass Pass0
    {
		VertexShader = compile vs_3_0 myvs();
		PixelShader  = compile ps_3_0 myps();
    }  
    
}