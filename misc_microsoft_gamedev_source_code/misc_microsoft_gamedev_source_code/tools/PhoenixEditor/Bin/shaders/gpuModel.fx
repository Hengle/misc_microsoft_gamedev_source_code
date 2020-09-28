

#include "fogHelpers.fxh"

float4x4 world			: World;				// This matrix will be loaded by the application
float4x4 ViewProj		: ViewProjection;		// This matrix will be loaded by the application
float4x4 worldViewProj	: WorldViewProjection;	// This matrix will be loaded by the application

float4  g_playerColor = float4(0,0,1,1);

bool	g_bUseAlbedo		= false;
bool	g_bUsePlayerColor	= false;
bool	g_bUseOpacity		= false;

texture albedoTexture;
sampler albedoTextureSampler = sampler_state
{
    Texture   = (albedoTexture);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    
    AddressU = WRAP;
    AddressV = WRAP;
    AddressW = CLAMP;
};

texture xformTexture;
sampler xformTextureSampler = sampler_state
{
    Texture   = (xformTexture);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    
    AddressU = WRAP;
    AddressV = WRAP;
    AddressW = CLAMP;
};

texture opacityTexture;
sampler opacityTextureSampler = sampler_state
{
    Texture   = (opacityTexture);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    
    AddressU = WRAP;
    AddressV = WRAP;
    AddressW = CLAMP;
};


//-----------------------------------------------------------------------------
// Vertex Definitions
//-----------------------------------------------------------------------------


struct VS_OUTPUT
{
    float4 Position		: POSITION;  // vertex position 
    float2 uv0			: TEXCOORD0;        
    float4 Diffuse		: COLOR0;   
    float2 fog			: TEXCOORD1;		// Radial fog density, Planar fog density
};


float3 lightPos = float3(1.f,1.f,1.f);
float ambient = 0.5f;

VS_OUTPUT basicVS(  float4 vPosition : POSITION,
					float3 vNormal : NORMAL,
					float2 vUV : TEXCOORD0,
					float4 vPosition0 : TEXCOORD5,
					float4 vPosition1 : TEXCOORD6,
					float4 vPosition2 : TEXCOORD7,
					float4 vPosition3 : TEXCOORD8 )
{
    VS_OUTPUT Output;
	
	float4x4 instMat;
	instMat[0] = vPosition0;
	instMat[1] = vPosition1;
	instMat[2] = vPosition2;
	instMat[3] = vPosition3;
	instMat = mul(world, instMat);
	
	float4 instPos = mul( vPosition, instMat );
	
    // Transform the vertex into projection space. 
    Output.Position = mul( instPos, ViewProj );
    
    // Output the diffuse color.
    float3 normal = mul( vNormal, (float3x3) world );
	Output.Diffuse  = max(0,dot(normalize(lightPos), normal));

    // Output uv's
    Output.uv0 = vUV;
    

	float3 worldpos = mul(instPos, world);
	
	Output.fog.x = computeRadialFogDensity(worldpos);
	Output.fog.y = computePlanarFogDensity(worldpos);
	
	    
    	
    return Output;
}

float4 fullPS( VS_OUTPUT IN ) : COLOR0
{
	float4 color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	
	if(g_bUseAlbedo)
	{
		color = tex2D(albedoTextureSampler, IN.uv0);
	}

	if(g_bUsePlayerColor)
	{
		float xform	= tex2D(xformTextureSampler, IN.uv0);
		color = lerp(g_playerColor * color, color, xform);
	}
	
	// Modulate color with lighting
	float4 result = color * clamp(IN.Diffuse + ambient, 0, 1);

	// Fog
    result.xyz = computeFog(result, IN.fog.x, IN.fog.y);


	if(g_bUseOpacity)
	{
		float4 opacity	= tex2D(opacityTextureSampler, IN.uv0);
		result.w = opacity.r;
	}
	else
	{
		result.w = 1;
	}
	
    return result;
}


float4 noTexturePS( VS_OUTPUT IN ) : COLOR0
{	
	float4 result = clamp(IN.Diffuse + ambient, 0, 1);
	result.w = 1.0f;
	
    return result;
}

float4 whitePS( VS_OUTPUT IN ) : COLOR0
{	
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
/////////////////////////////////////////////
/////////////////////////////////////////////
//DEPTH PEELING (for AO GEN!)

struct VS_OUTPUT_DEPTH
{
    float4 hposition	: POSITION;
    
    float4 rPos			: TEXCOORD5;		//same as hposition
};

VS_OUTPUT_DEPTH myvsDepthPeel(   float4 vPosition : POSITION,
					 float3 vNormal : NORMAL,
					 float2 vUV : TEXCOORD0)
{
    VS_OUTPUT_DEPTH OUT;

	float4 tPos = mul( vPosition, worldViewProj );
	OUT.hposition = tPos;
	OUT.rPos = tPos;
	
	return OUT;
}

VS_OUTPUT_DEPTH myvsHeightPeel(   float4 vPosition : POSITION,
					 float3 vNormal : NORMAL,
					 float2 vUV : TEXCOORD0)
{
    VS_OUTPUT_DEPTH OUT;

	float4 tPos = mul( vPosition, worldViewProj );
	OUT.hposition = tPos;
	OUT.rPos = mul( vPosition, world );
	
	return OUT;
}

float4 mypsHeightOnly( VS_OUTPUT_DEPTH IN ) : COLOR
{
	float myDepth =  IN.rPos.y;
	//myDepth+=0.009;
    return myDepth;
}

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
float4 mypsDepthPeel( VS_OUTPUT_DEPTH IN ) : COLOR
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

float4 mypsDepthOnly( VS_OUTPUT_DEPTH IN ) : COLOR
{
	float myDepth =  IN.rPos.z / IN.rPos.w;
	//myDepth+=0.009;
    return myDepth;
}


technique Technique0
{
    pass LitTextured
    {
		VertexShader = compile vs_3_0 basicVS();
		PixelShader  = compile ps_3_0 fullPS();
    }
    
    pass NoTexturing
    {
		VertexShader = compile vs_3_0 basicVS();
		PixelShader  = compile ps_3_0 noTexturePS();
    }
    
    pass White
    {
		VertexShader = compile vs_3_0 basicVS();
		PixelShader  = compile ps_3_0 whitePS();
    }
    
    pass DepthPeelForAOGen
    {
		VertexShader = compile vs_3_0 myvsDepthPeel();
		PixelShader  = compile ps_3_0 mypsDepthPeel();
    }  
     pass DepthOnly
    {
		VertexShader = compile vs_3_0 myvsDepthPeel();
		PixelShader  = compile ps_3_0 mypsDepthOnly();
    }  
     pass HeightOnlyForSimRep
    {
		VertexShader = compile vs_3_0 myvsHeightPeel();
		PixelShader  = compile ps_3_0 mypsHeightOnly();
    }   
}


