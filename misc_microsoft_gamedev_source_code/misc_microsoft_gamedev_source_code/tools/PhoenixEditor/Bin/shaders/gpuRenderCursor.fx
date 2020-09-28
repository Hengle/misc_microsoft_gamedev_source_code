
#include "gputerrain.fxh"
//-----------------------------------------------------------------------------
// Vertex Definitions
//-----------------------------------------------------------------------------

texture albedoTextureArray;
sampler albedoArray = sampler_state
{
    Texture   = (albedoTextureArray);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    
    AddressU = WRAP;
    AddressV = WRAP;
    AddressW = CLAMP;
};

texture albedoDecalTexture;
sampler albedoDecal = sampler_state
{
    Texture   = (albedoDecalTexture);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
};


texture alphaMaskCursor;
sampler AlphaTextureCursor = sampler_state
{
    Texture   = (alphaMaskCursor);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    
    AddressU = BORDER;
    AddressV = BORDER;
    AddressW = CLAMP;
};



struct VS_INPUT
{
    float3 position	: POSITION;
};

struct VS_OUTPUT
{
    float4 hposition	: POSITION;
    float2 uv0			: TEXCOORD0;
    float2 uv1			: TEXCOORD1;
    float4 color		: COLOR0;
};
 
float4 g_centerPoint = float4(0,0,0,0);	//in splat mode, W contains rotation
float4 g_brushInfo = float4(2,0.5,0,0); //radius, hotspot,texUScale,texVScale  || radius,texIntensity,rotation, 1
float4 g_colorValue = float4(1,1,1,1);

float4 g_hotspotColor = float4(1.0f, 0.0f, 0.0f, 1.0f);
float4 g_falloffColor = float4(1.0f, 1.0f, 0.0f, 0.0f);

//-----------------------------------------------------------------------------
float4 getVertexColor(float3 position, int curveType)
{
	float inflFactor=0;

	// Get distance from this vertex to the center point.
	float dist = distance(position,g_centerPoint);

	// Skip if too far.
	if (dist <= g_brushInfo.x)
	{
		if(dist <= g_brushInfo.y)
		{
			inflFactor = 1.0f;
		}
		else
		{
			inflFactor = (dist-g_brushInfo.y) / (g_brushInfo.x-g_brushInfo.y);
			
			if(curveType == 0)
				inflFactor = (float)sin((1.0f - inflFactor) * (1.570796326795f));
			if(curveType == 1)
				inflFactor = (float) (1.0f - inflFactor);
			if(curveType == 2)
				inflFactor = 1.0f + (float)sin(inflFactor * (-1.570796326795f));
		}
	}
	
	return lerp(g_falloffColor, g_hotspotColor, inflFactor);
}
//-----------------------------------------------------------------------------
float4 getVertexColor2D(float3 position, int curveType)
{
	float inflFactor=0;

	// Get distance from this vertex to the center point.
	position.y = g_centerPoint.y;
	float dist = distance(position,g_centerPoint);

	// Skip if too far.
	if (dist <= g_brushInfo.x)
	{
		if(dist <= g_brushInfo.y)
		{
			inflFactor = 1.0f;
		}
		else
		{
			inflFactor = (dist-g_brushInfo.y) / (g_brushInfo.x-g_brushInfo.y);
			
			if(curveType == 0)
				inflFactor = (float)sin((1.0f - inflFactor) * (1.570796326795f));
			if(curveType == 1)
				inflFactor = (float) (1.0f - inflFactor);
			if(curveType == 2)
				inflFactor = 1.0f + (float)sin(inflFactor * (-1.570796326795f));
		}
	}
	
	return lerp(g_falloffColor, g_hotspotColor, inflFactor);
}
//----------------------------------------------------------------------------- 
float2 updateUVs(float2 uvIN, float3 position)
{
	float2 uvOut = float2(0,0);
	float dist = distance(position,g_centerPoint);

	// Skip if too far.
	//if (dist <= g_brushInfo.x)
	{
		uvOut.y = (((g_centerPoint.x-position.x)/(-g_brushInfo.x))*0.5)+0.5;
		uvOut.x = (((g_centerPoint.z-position.z)/(-g_brushInfo.x))*0.5)+0.5;
	}
	return uvOut;
}

//-----------------------------------------------------------------------------
float2 rotateUVs(float2 input,float angle)
{
	float2x2 rotMatrix;
	rotMatrix[0] = float2(cos(angle), sin(angle));
	rotMatrix[1] = float2(-sin(angle), cos(angle));
	float2 results = input - float2(0.5,0.5);
	results = mul(rotMatrix,results);
	results += float2(0.5,0.5);
	
	return results;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Simple Vertex Shader
//-----------------------------------------------------------------------------
VS_OUTPUT myvs( VS_INPUT IN, uniform int curveType, uniform bool intersection2D )
{
    VS_OUTPUT OUT;

	float2 uv0 = float2(0,0);
	float3x3 TBN;
	float weight;
	float ao;
	float2 vPosUVs = float2(0,0);
	float3 pos = VIndexToPos(int(IN.position.x), vPosUVs, uv0, TBN, weight, ao);

	if(intersection2D)
		OUT.color = getVertexColor2D(pos, curveType);
	else
		OUT.color = getVertexColor(pos, curveType);

	pos += (TBN[2] * 0.05f);	//offset it a bit so that we don't zfight...
	
	//calculate our world positions
	OUT.hposition = mul( float4(pos.x,pos.y,pos.z,1.f ),worldViewProj);

	
	//texCoords
	OUT.uv0 = uv0;
	OUT.uv1 = uv0;
	
	return OUT;
}
float4 myps( VS_OUTPUT IN) : COLOR
{
	return IN.color;
}
//-----------------------------------------------------------------------------
VS_OUTPUT myvsTex( VS_INPUT IN )
{
    VS_OUTPUT OUT;

	float2 uv0 = float2(0,0);
	float3x3 TBN;
	float weight;
	float ao;
	float2 vPosUVs = float2(0,0);
	float3 pos = VIndexToPos(int(IN.position.x), vPosUVs,uv0, TBN, weight, ao);

	OUT.color = float4(1,1,1,1);

	pos += (TBN[2] * 0.05f);	//offset it a bit so that we don't zfight...
	
	//calculate our world positions
	OUT.hposition = mul( float4(pos.x,pos.y,pos.z,1.f ),worldViewProj);

	
	//texCoords
	OUT.uv0 = updateUVs(uv0,pos);
	OUT.uv1 = vPosUVs *(2* g_brushInfo.zw);
	
	
	
	return OUT;
}
//-----------------------------------------------------------------------------

float4 texPS( VS_OUTPUT IN ) : COLOR
{
	float2 newUVs = rotateUVs(IN.uv0,g_centerPoint.w);
	float4 diffuse = tex2D(albedoArray,IN.uv1 );
	float alpha = tex2D(AlphaTextureCursor,newUVs);
	
    return float4(diffuse.xyz * g_colorValue.xyz,alpha * g_brushInfo.y);//float4(diffuse.x,diffuse.y,diffuse.z,alpha);
}


float4 texPSDecal( VS_OUTPUT IN ) : COLOR
{
	float2 newUVs = rotateUVs(IN.uv0,g_brushInfo.z);
	float4 diffuse = tex2D(albedoDecal,newUVs );
	float alpha = tex2D(AlphaTextureCursor,newUVs );
	
    return float4(diffuse.xyz * g_colorValue.xyz,alpha);//float4(diffuse.x,diffuse.y,diffuse.z,alpha);
}


float4 texPSMask( VS_OUTPUT IN ) : COLOR
{
	float alpha = tex2D(AlphaTextureCursor,IN.uv0);
    return alpha*g_colorValue;
}

//-----------------------------------------------------------------------------

technique Technique0
{
    pass RenderVertsSphereFunc1
    {
		VertexShader = compile vs_3_0 myvs(0, false);
		PixelShader  = compile ps_3_0 myps();
    }
    pass RenderVertsSphereFunc2
    {
		VertexShader = compile vs_3_0 myvs(1, false);
		PixelShader  = compile ps_3_0 myps();
    }
    pass RenderVertsSphereFunc3
    {
		VertexShader = compile vs_3_0 myvs(2, false);
		PixelShader  = compile ps_3_0 myps();
    }
    pass RenderVertsCylinderFunc1
    {
		VertexShader = compile vs_3_0 myvs(0, true);
		PixelShader  = compile ps_3_0 myps();
    }
    pass RenderVertsCylinderFunc2
    {
		VertexShader = compile vs_3_0 myvs(1, true);
		PixelShader  = compile ps_3_0 myps();
    }
    pass RenderVertsCylinderFunc3
    {
		VertexShader = compile vs_3_0 myvs(2, true);
		PixelShader  = compile ps_3_0 myps();
    }
    
    pass RenderTexture
    {
		VertexShader = compile vs_3_0 myvsTex();
		PixelShader  = compile ps_3_0 texPS();
    }
	pass RenderTextureMask
    {
		VertexShader = compile vs_3_0 myvsTex();
		PixelShader  = compile ps_3_0 texPSMask();
    }
    pass RenderTextureDecal
    {
		VertexShader = compile vs_3_0 myvsTex();
		PixelShader  = compile ps_3_0 texPSDecal();
    }
}
