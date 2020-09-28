
#include "gputerrain.fxh"
#include "gputerraintexturing.fxh"
#include "fogHelpers.fxh"
//-----------------------------------------------------------------------------
// Vertex Definitions
//-----------------------------------------------------------------------------

texture vertalphasTexture;
sampler vertalphaSampler = sampler_state
{
    Texture   = (vertalphasTexture);
    MipFilter = NONE;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture vertLightTexture;
sampler vertLightSampler = sampler_state
{
    Texture   = (vertLightTexture);
    MipFilter = NONE;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture vertTessTexture;
sampler vertTessSampler = sampler_state
{
    Texture   = (vertTessTexture);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

struct VS_INPUT
{
    float3 position	: POSITION;
};

struct VS_OUTPUT
{
    float4 hposition	: POSITION;
    float2 uv0			: TEXCOORD0;
    float3 sunDir       : TEXCOORD1;
    float4 color		: COLOR0;
    float3 weight_fog	: TEXCOORD2;		// selection weight (soft), Z fog density, Planar fog density
    float2 vPosUVs		: TEXCOORD3;
    float4 wPos			: TEXCOORD4;		//world x, y, z, DIST FROM CAMERA (z of post-projection transofmration)
    float4 rPos			: TEXCOORD5;		//same as hposition
};
 
//-----------------------------------------------------------------------------
// Simple Vertex Shader
//-----------------------------------------------------------------------------
float3 gDirLightDir = float3(1.f,1.f,1.f);
float gAmbientLight =0;

float tex2D_billinear4x(uniform sampler2D tex, float4 t, float2 Scales )
{
	float size = Scales.x;
	float scale = Scales.y;

	float4 tAB0 = tex2Dlod(tex, t);

	float2 f = frac(t.xy * size);
	float2 tAB = lerp(tAB0.xz, tAB0.yw, f.x);
	return lerp(tAB.x, tAB.y, f.y);
}


VS_OUTPUT myvs( VS_INPUT IN )
{
    VS_OUTPUT OUT;

	float2 uv0 = float2(0,0);
	float3x3 TBN;
	float weight;
	float ao;
	float2 vPosUVs = float2(0,0);
	float3 pos = VIndexToPos(int(IN.position.x), vPosUVs,uv0, TBN, weight, ao);

	float4 tPos;

	if(g_skirtNodeEnabled)
	{
		float2 skirtCoord = float2((pos.x / g_worldSize.x), (pos.z / g_worldSize.y));
		skirtCoord *= (g_skirtVals.y - 1) / g_skirtVals.w;	// Skirt texture coordinates (normalized)

		float skirtHeightOffset = tex2D_billinear4x( skirtSampler, float4(skirtCoord.x,skirtCoord.y,0.f,0.f), float2(g_skirtVals.w, 0.0f));
		pos.y += skirtHeightOffset;

		//calculate our screen positions
		tPos = mul( float4(pos.x,pos.y,pos.z,1.f ),worldViewProj);
		
		// Transform position to world space
		pos = mul(float4(pos.x,pos.y,pos.z,1.f ), world);
			
		// Transform TBN by world matrix
		TBN[0] = mul( TBN[0],(float3x3)world);
		TBN[1] = mul( TBN[1],(float3x3)world);
		TBN[2] = mul( TBN[2],(float3x3)world);
	}
	else
	{
		//calculate our screen positions
		tPos = mul( float4(pos.x,pos.y,pos.z,1.f ),worldViewProj);
		
	}
	OUT.hposition = tPos;
		
	//lighting
	gDirLightDir=normalize(gDirLightDir);
	float lum = 1.f;
	OUT.color =  max(0,dot(TBN[2],gDirLightDir)); 
	OUT.color.w = ao;
	OUT.weight_fog.x = weight;
	OUT.weight_fog.y = computeRadialFogDensity(pos);
	OUT.weight_fog.z = computePlanarFogDensity(pos);


	
	
		
	
	OUT.sunDir.x = dot(TBN[0].xyz, gDirLightDir.xyz);
	OUT.sunDir.y = dot(TBN[1].xyz, gDirLightDir.xyz);
	OUT.sunDir.z = dot(TBN[2].xyz, gDirLightDir.xyz);
   
	
	//texCoords
	OUT.uv0 = uv0;
	OUT.vPosUVs = vPosUVs;
	

	OUT.wPos = float4(pos, tPos.z);
	OUT.rPos = tPos;
	
	return OUT;
}

VS_OUTPUT mynormalvs( VS_INPUT IN )
{
    VS_OUTPUT OUT;

	float2 uv0 = float2(0,0);
	float3x3 TBN;
	float2 vPosUVs = float2(0,0);
	float3 pos = VIndexToNormPos(int(IN.position.x), vPosUVs,uv0, TBN);
   
	//calculate our world positions
	OUT.hposition = mul( float4(pos.x,pos.y,pos.z,1.f ),worldViewProj);

	//lighting
	OUT.color =  float4(1.0f, 0.5f, 0.0f, 1.0f);
	OUT.weight_fog.xyz = 0.0f;
	OUT.sunDir=float4(0,0,0,0);
	
	//texCoords
	OUT.uv0 = uv0;
	OUT.vPosUVs = vPosUVs;
	
	OUT.wPos = float4(pos,1);
	OUT.rPos =mul( float4(pos.x,pos.y,pos.z,1.f ),worldViewProj);
	
	return OUT;
} 


VS_OUTPUT skirtOffsetVS( VS_INPUT IN )
{
    VS_OUTPUT OUT;
	float3 pos = VSkirtIndexToPos(int(IN.position.x));

	//calculate our world positions
	OUT.hposition = mul( float4(pos.x,pos.y,pos.z,1.f ),worldViewProj);

	OUT.uv0 = float2(0.0f, 0.0f);
	OUT.sunDir = float3(0.0f, 0.0f, 0.0f);
	OUT.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	OUT.weight_fog.xyz = 0.0f;		
	OUT.vPosUVs = float2(0.0f, 0.0f);
	
	OUT.wPos = float4(pos,1);
	OUT.rPos =mul( float4(pos.x,pos.y,pos.z,1.f ),worldViewProj);
	
	
	return OUT;
}

uniform float gGridToggle = 0;
uniform float gGridSpacing = 1;
uniform float4 gGridColor0 = float4(1,1,1,1);
uniform float4 gGridColor1 = float4(1,1,1,1);
uniform float4 gGridColor2 = float4(1,1,1,1);
uniform float4 gGridColor3 = float4(0,0,1,1);

float ambient = 0.5f;
//-----------------------------------------------------------------------------
// Simple Pixel Shader
//-----------------------------------------------------------------------------
float4 mypsUnique( VS_OUTPUT IN ) : COLOR
{  
	float4 albedo =   getUniqueAlbedoColor(IN.uv0);
   
	float3 normal = (getUniqueNormal(IN.uv0));
	float diffuse = clamp(max(0,dot(normal,normalize(IN.sunDir))),0.6,1);
	float4 localLight = tex2D(vertLightSampler,IN.vPosUVs); 
	float4 tess = tex2D( vertTessSampler, IN.vPosUVs);
	
	float ao = (IN.color.w);//linear to SRGB
	float4 result = albedo * ao * clamp(diffuse+localLight,0,1);
   
	// apply mask color
	float weight = (0.5f + (0.5f *(1.0f - IN.weight_fog.x)));
    result *= float4(1,weight,weight,1);
    
    // apply vertex tessellation color
    result += tess;
    
    result.xyz = computeFog(result, IN.weight_fog.y, IN.weight_fog.z);

    float al = tex2D( vertalphaSampler,   IN.vPosUVs);

    result.w = al;
 
	if(gGridToggle)
	{
		int gs4 = 4;//gGridSpacing *4;
		float xOn = fmod(IN.wPos.x,gGridSpacing);
		float zOn = fmod(IN.wPos.z,gGridSpacing);
		
		if(xOn<0.05 && zOn > 0.05)
		{
			int xGS = fmod(IN.wPos.x,gs4);
			if(xGS <1)					result.xyz = lerp(result.xyz,gGridColor3.xyz,gGridColor3.w);
			else if(xGS <2)				result.xyz = lerp(result.xyz,gGridColor2.xyz,gGridColor2.w);
			else if(xGS <3)				result.xyz = lerp(result.xyz,gGridColor1.xyz,gGridColor1.w);
			else if(xGS <4)				result.xyz = lerp(result.xyz,gGridColor0.xyz,gGridColor0.w);
		}	
		else if(xOn>0.05 && zOn < 0.05)
		{
			int zGS = fmod(IN.wPos.z,gs4);
			if(zGS <1)					result.xyz = lerp(result.xyz,gGridColor3.xyz,gGridColor3.w);
			else if(zGS <2)				result.xyz = lerp(result.xyz,gGridColor2.xyz,gGridColor2.w);
			else if(zGS <3)				result.xyz = lerp(result.xyz,gGridColor1.xyz,gGridColor1.w);
			else if(zGS <4)				result.xyz = lerp(result.xyz,gGridColor0.xyz,gGridColor0.w);
		}

		if(fmod(IN.wPos.x, g_worldSize.x * 0.5)<1 || fmod(IN.wPos.z, g_worldSize.y * 0.5)<1)
		{
			result.xyz = float3(1,0,0);
		}
		else if (fmod(IN.wPos.x, g_worldSize.x * 0.25)<1 || fmod(IN.wPos.z, g_worldSize.y * 0.25)<1)
		{
			result.xyz = float3(1,0,1);
		}
		else if (fmod(IN.wPos.x, g_worldSize.x * 0.125)<1 || fmod(IN.wPos.z, g_worldSize.y * 0.125)<1)
		{
			result.xyz = float3(1,1,0);
		}
		else if (fmod(IN.wPos.x, g_worldSize.x * 0.0625)<1 || fmod(IN.wPos.z, g_worldSize.y * 0.0625)<1)
		{
			result.xyz = float3(0,1,0);
		}

    }
 
    
	return result;
	
}

//-----------------------------------------------------------------------------
float4 mypsFullSelectedOnly( VS_OUTPUT IN ) : COLOR
{  
 	float4 albedo =   getUniqueAlbedoColor(IN.uv0);
   
	float3 normal = (getUniqueNormal(IN.uv0));
	float diffuse = max(0,dot(normal,normalize(IN.sunDir)));
   float4 result = albedo * (diffuse+(ambient*IN.color.w));

   result.w = IN.weight_fog.x;
   return result;
}
//-----------------------------------------------------------------------------

float4 flatPS( VS_OUTPUT IN ) : COLOR
{
	
	float diffuse = max(0,IN.color);//clamp(,0.6,1);
	diffuse *= 0.5f *IN.color.w;//CLM scale value between 0 & 0.5f due to PT's request
	
	// Deformation are easier to see without ambient
    float4 result = float4(diffuse,diffuse,diffuse,1);


	// apply mask color
	float weight = (0.5f + (0.5f *(1.0f - IN.weight_fog.x)));
    result *= float4(1,weight,weight,1);
    
    result.w = 1.0f;  
    
	result.x += gGridToggle*(fmod(IN.wPos.x,2)<0.05);
    result.y += gGridToggle*(fmod(IN.wPos.z,2)<0.05); 

    return result;
}

float4 flatPSSelectedOnly( VS_OUTPUT IN ) : COLOR
{
	float4 result = IN.color;
	result.w = IN.weight_fog.x;
	
    return result;
}

float4 flatPSAmbientOcclusionOnly( VS_OUTPUT IN ) : COLOR
{
	float ao = IN.color.w;
	//convert this from linear to SRGB
	ao = (ao);
	
    float4 result = float4(ao, ao, ao, 1.0f);
    
	// apply mask color
	float weight = (0.5f + (0.5f *(1.0f - IN.weight_fog.x)));
    result *= float4(1,weight,weight,1);
    
    result.w = 1.0f;
    
    return result;
}

float4 skirtOffsetPS( VS_OUTPUT IN ) : COLOR
{
    float4 result = g_SkirtWireColor;
    return result;
}

float4 perfCostPS( VS_OUTPUT IN ) : COLOR
{
    return g_perfColor;
}

float4 BlendingEval( VS_OUTPUT IN ) : COLOR
{
    return blendModeGen(IN.uv0);
}

float4 TextureNormals( VS_OUTPUT IN ) : COLOR
{
	float3 normal = (normalGen(IN.uv0));
    return float4(normal.xyz,1.f);
}


float4 depthOnly( VS_OUTPUT IN ) : COLOR
{
	//float dist = length(gWorldCameraPos - IN.wPos);
    return IN.wPos.w;//dist;//float4(1,0,0,1);//float4(dist,dist,dist,dist);
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
    
    float d = myDepth;
    //clip me if i'm farther than previous depth
    float zBufPrecision = 0.07f; //Humm.. this needs to be pretty high for some reason....
    clip(prevDepth - (myDepth+zBufPrecision));
   // if(prevDepth - (myDepth+zBufPrecision)<0)
	//	d = 7777 + (prevDepth - myDepth);
	
    return d;//myDepth;
}


//-----------------------------------------------------------------------------
// Simple Effect (1 technique with 1 pass)
//-----------------------------------------------------------------------------

technique Technique0
{

    pass LitTextured
    {
		VertexShader = compile vs_3_0 myvs();
		PixelShader  = compile ps_3_0 mypsUnique();
    }
    
    pass NoTexturing
    {
		VertexShader = compile vs_3_0 myvs();
		PixelShader  = compile ps_3_0 flatPS();
    }
  
    
    pass LitTexturedSelectedOnly
    {
		VertexShader = compile vs_3_0 myvs();
		PixelShader  = compile ps_3_0 mypsFullSelectedOnly();
    }    
     
    
    pass NoTexturingSelectedOnly
    {
		VertexShader = compile vs_3_0 myvs();
		PixelShader  = compile ps_3_0 flatPSSelectedOnly();
    }  
     
    
    pass TextureEval
    {
		VertexShader = compile vs_3_0 myvs();
		PixelShader  = compile ps_3_0 BlendingEval();
    }  
    
    
	pass TextureNormals
    {
		VertexShader = compile vs_3_0 myvs();
		PixelShader  = compile ps_3_0 TextureNormals();
    }  
    
    pass AmbientOcclusion
    {
		VertexShader = compile vs_3_0 myvs();
		PixelShader  = compile ps_3_0 flatPSAmbientOcclusionOnly();
    }
    
    
    pass SkirtOffset
    {
		VertexShader = compile vs_3_0 skirtOffsetVS();
		PixelShader  = compile ps_3_0 skirtOffsetPS();
    }   
    
    pass PerfEval
    {
		VertexShader = compile vs_3_0 myvs();
		PixelShader  = compile ps_3_0 perfCostPS();
    } 
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
}
