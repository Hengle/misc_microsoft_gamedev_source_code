
#include "gputerrain.fxh"
//-----------------------------------------------------------------------------
// Vertex Definitions
//-----------------------------------------------------------------------------




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
VS_OUTPUT myvsQUAD( VS_INPUT IN )
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

float4 texFlatColor( VS_OUTPUT IN ) : COLOR
{
	//float2 newUVs = rotateUVs(IN.uv0,g_centerPoint.w);

    return float4( 1,0,0, 0.5);//g_brushInfo.y);
}


//-----------------------------------------------------------------------------

technique Technique0
{
	pass RenderQuad
	{
		VertexShader = compile vs_3_0 myvsQUAD();
		PixelShader  = compile ps_3_0 texFlatColor();
	}

}
