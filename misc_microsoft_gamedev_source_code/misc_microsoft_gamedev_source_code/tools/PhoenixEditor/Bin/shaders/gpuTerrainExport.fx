
//-----------------------------------------------------------------------------
// Vertex Definitions
//-----------------------------------------------------------------------------
float4x4 world			: World;				// This matrix will be loaded by the application
float4x4 worldViewProj	: WorldViewProjection; // This matrix will be loaded by the application

texture positionsTexture; // our vertex displacement texture
sampler vertSampler = sampler_state
{
    Texture   = (positionsTexture);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

uniform float4 g_terrainVals;         //numXTiles, scale, minXVert,minZVert
uniform float  g_chunkScale = 0.25;
uniform float g_numWorldXVerts = 256;


struct VS_INPUT
{
    float3 position	: POSITION;
};

struct VS_OUTPUT
{
    float4 hposition	: POSITION;
    
    float4 rPos			: TEXCOORD5;		//same as hposition
};
 
//-----------------------------------------------------------------------------
// Simple Vertex Shader
//-----------------------------------------------------------------------------


//==============================================================================
// GiveVertFromSampler
//==============================================================================
void GiveVertFromSampler(float3 wPos, float2 uvCoord, out float3 nPos)
{
   nPos = float3(0,0,0);
   
   float4 position = tex2Dlod( vertSampler,   float4(uvCoord.y,uvCoord.x,0.f,0.f) );
   nPos = position.xyz;
   
   wPos.xz = (wPos.xz*g_terrainVals.y)+(g_terrainVals.zw*g_chunkScale) ; 
   nPos+=wPos;
}
//==============================================================================
// VIndexToPos
//==============================================================================
float3 VIndexToPos(int vIndex)
{
   float sizeDivX    = 1.f / float(g_terrainVals.x);
   float3 nPos       =  float3(0,0,0);
 
   nPos.x =  floor(vIndex * sizeDivX);
   nPos.z =  ceil(fmod(vIndex, g_terrainVals.x)-0.003);
      
   float2 uvCoord0 = (float2(nPos.x, nPos.z) + g_terrainVals.zw)/ g_numWorldXVerts;//float(g_terrainVals.x);

   GiveVertFromSampler(nPos, uvCoord0, nPos);
   
   return nPos;
}


VS_OUTPUT myvs( VS_INPUT IN )
{
    VS_OUTPUT OUT;

	float3 pos = VIndexToPos(int(IN.position.x));

	float4 tPos = mul( float4(pos.x,pos.y,pos.z,1.f ),worldViewProj);

	OUT.hposition = tPos;
	OUT.rPos = tPos;
	
	return OUT;
}

/////////////////////////////////////////////
/////////////////////////////////////////////
//DEPTH ONLY (for XTH GEN!)
float4 depthOnly( VS_OUTPUT IN ) : COLOR
{
	float myDepth =  IN.rPos.z / IN.rPos.w;
	//myDepth+=0.009;
    return myDepth;
}

/////////////////////////////////////////////
/////////////////////////////////////////////
//DEPTH PEELING (for AO GEN!)
texture prevDepthTexture;
sampler prevDepthSampler = sampler_state
{
    Texture   = (prevDepthTexture);
    MipFilter = Point;
    MinFilter = Point;
    MagFilter = Point;
    
    AddressU = WRAP;
    AddressV = WRAP;
    AddressW = CLAMP;
};
float gWindowWidth = 256;
float4 depthPeel( VS_OUTPUT IN ) : COLOR
{
	//translate our screenspace coords to raster coords..
	float4 rasterPt = IN.rPos / IN.rPos.w;
	rasterPt.xy = 0.5f * rasterPt.xy  + float2(0.5f,0.5f);
	//rasterPt.xy += 0.5f / 256.0f;//halfpixeloffset	//ODD, this was needed in the sample app...
	rasterPt.y = 1-rasterPt.y;		//Ugg??????
	
    float myDepth =  IN.rPos.z / IN.rPos.w;
    float prevDepth = tex2D(prevDepthSampler, rasterPt.xy);
    
    
    //clip me if i'm farther than previous depth
    //CLM this was 0.07 first, moving it to 0.01 made the entire thing darker 
    //althoug this may add extra passes to the depth peeling, causing more overhead (it was moved to 0.07 beacuse there were depth fragments that "SHOULD" not cause an extra pass, causing an extra pass..)
    float zBufPrecision = 0.01f; 
    clip(prevDepth - (myDepth+zBufPrecision));
   
	
    return myDepth;
}
/////////////////////////////////////////////
/////////////////////////////////////////////
//HEIGHT ONLY (for SIMREP GEN!)

VS_OUTPUT myvsHeight( VS_INPUT IN )
{
    VS_OUTPUT OUT;

	float3 pos = VIndexToPos(int(IN.position.x));

	float4 tPos = mul( float4(pos.x,pos.y,pos.z,1.f ),worldViewProj);

	OUT.hposition = tPos;
	OUT.rPos = float4(pos.x,pos.y,pos.z,1.f );
	
	return OUT;
}

float4 heightOnly( VS_OUTPUT IN ) : COLOR
{
	float myDepth =  IN.rPos.y;
    return myDepth;
}


//-----------------------------------------------------------------------------
// Simple Effect (1 technique with 1 pass)
//-----------------------------------------------------------------------------

technique Technique0
{
     pass DepthOnly
    {
		VertexShader = compile vs_3_0 myvs();
		PixelShader  = compile ps_3_0 depthOnly();
    }  
    
    pass DepthPeel
    {
		VertexShader = compile vs_3_0 myvs();
		PixelShader  = compile ps_3_0 depthPeel();
    } 
    
    pass HeightPeel
    {
		VertexShader = compile vs_3_0 myvsHeight();
		PixelShader  = compile ps_3_0 heightOnly();
    }   
}
