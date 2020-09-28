

//-----------------------------------------------------------------------------
// Vertex Definitions
//-----------------------------------------------------------------------------




texture occlusionPositionTexture;
sampler elementPositionMap = sampler_state
{
    Texture   = (occlusionPositionTexture);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture occlusionNormalTexture;
sampler elementNormalMap = sampler_state
{
    Texture   = (occlusionNormalTexture);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture occlusionIndexTexture;
sampler indexMap = sampler_state
{
    Texture   = (occlusionIndexTexture);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture occlusionLastResultTexture;
sampler lastResultMap = sampler_state
{
    Texture   = (occlusionLastResultTexture);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
};


int		g_sourceTexWidth;
int		g_sourceTexHeight;
float	g_sourceToTargetTexHeightFactor;		// Used to convert normalized texture coordinates from source
												// texture to target texture.  The height of these textures
												// is different since render targets need to be power of two.
												// The width is the same for both.

float4	g_factorBias;
float4	g_offsetBias;


struct VS_OUTPUT
{
    float4 position		: POSITION;	// Should be window space position here
    float2 uv			: TEXCOORD0;
};
 

//==============================================================================
// SolidAngle
//==============================================================================
float SolidAngle(float3 v, float d2, float3 receiverNormal,
					float3 emitterNormal, float emitterArea)
{
    return (1 - rsqrt(emitterArea/d2 + 1)) *
        saturate(dot(emitterNormal, v)) * saturate(3*dot(receiverNormal, v));
}


//-----------------------------------------------------------------------------
// AO Pixel Shader
//-----------------------------------------------------------------------------
float4 ambientOcclusionPS( VS_OUTPUT IN ) : COLOR
{
	float emitterArea;
	float4 emitterPosition;
	float4 emitterNormal;
	float2 receiverIndex = IN.uv;
	float3 receiverPosition = tex2Dlod(elementPositionMap, float4(receiverIndex.x,receiverIndex.y,0,0)).xyz;
	//receiverPosition = (receiverPosition * g_factorBias) + g_offsetBias;
	float3 receiverNormal = tex2Dlod(elementNormalMap, float4(receiverIndex.x,receiverIndex.y,0,0)).xyz;
    receiverNormal = (receiverNormal * 2.0f) - 1.0f;
	float3 v;
    float total = 0;
    float4 emitterIndex = float4(0.5f/g_sourceTexWidth, 0.5f/g_sourceTexHeight, 0.0f, 0.0f);
    float value;
    float d2;
    float result;
    

    while (emitterIndex.x >= 0.0f)  // while not finished traversal
    {
        emitterPosition = tex2Dlod(elementPositionMap, float4(emitterIndex.x,emitterIndex.y,0,0));
		//emitterPosition = (emitterPosition * g_factorBias) + g_offsetBias;
        emitterArea = emitterPosition.w;
        emitterNormal = tex2Dlod(elementNormalMap, float4(emitterIndex.x,emitterIndex.y,0,0));
        emitterNormal = (emitterNormal * 2.0f) - 1.0f;
        emitterIndex = tex2Dlod(indexMap, float4(emitterIndex.x,emitterIndex.y,0,0));	// go to next element
        v = emitterPosition.xyz - receiverPosition;			// calc receiver to emitter vector
        d2 = dot(v, v) + 1e-16;								// calc distance squared to emitter - make sure it's not 0
        if (d2 < -emitterArea*4) 							// parents have negative area
        {
			emitterIndex.xy = emitterIndex.zw;				// use alternate index (go down hierarchy)
            emitterArea = 0;								// ignore this element
        }

		v *= rsqrt(d2);										// normalize v
		value = SolidAngle(v, d2, receiverNormal, emitterNormal.xyz, abs(emitterArea));
		
		total += value;
    }

    result = saturate(1 - total);							// return accessibility only
    return result;
}

//-----------------------------------------------------------------------------
// AO Pixel Shader
//-----------------------------------------------------------------------------
float4 ambientOcclusionPass2PS( VS_OUTPUT IN ) : COLOR
{
	float emitterArea;
	float4 emitterPosition;
	float4 emitterNormal;
	float2 receiverIndex = IN.uv;
	float3 receiverPosition = tex2Dlod(elementPositionMap, float4(receiverIndex.x,receiverIndex.y,0,0)).xyz;
	//receiverPosition = (receiverPosition * g_factorBias) + g_offsetBias;
	float3 receiverNormal = tex2Dlod(elementNormalMap, float4(receiverIndex.x,receiverIndex.y,0,0)).xyz;
    receiverNormal = (receiverNormal * 2.0f) - 1.0f;
	float3 v;
    float total = 0;
    float4 emitterIndex = float4(0.5f/g_sourceTexWidth, 0.5f/g_sourceTexHeight, 0.0f, 0.0f);
    float value;
    float d2;
    float result;

    while (emitterIndex.x >= 0.0f)  // while not finished traversal
    {
        emitterPosition = tex2Dlod(elementPositionMap, float4(emitterIndex.x,emitterIndex.y,0,0));
		//emitterPosition = (emitterPosition * g_factorBias) + g_offsetBias;
        emitterArea = emitterPosition.w;
        emitterNormal = tex2Dlod(elementNormalMap, float4(emitterIndex.x,emitterIndex.y,0,0));
        emitterNormal = (emitterNormal * 2.0f) - 1.0f;
        emitterIndex = tex2Dlod(indexMap, float4(emitterIndex.x,emitterIndex.y,0,0));	// go to next element
        v = emitterPosition.xyz - receiverPosition;			// calc receiver to emitter vector
        d2 = dot(v, v) + 1e-16;								// calc distance squared to emitter - make sure it's not 0
        if (d2 < -emitterArea*4) 							// parents have negative area
        {
			emitterIndex.xy = emitterIndex.zw;				// use alternate index (go down hierarchy)
            emitterArea = 0;								// ignore this element
        }

		v *= rsqrt(d2);										// normalize v
		value = SolidAngle(v, d2, receiverNormal, emitterNormal.xyz, abs(emitterArea));
        value *= tex2Dlod(lastResultMap, float4(emitterIndex.x, emitterIndex.y * g_sourceToTargetTexHeightFactor, 0, 0)).x; // modulate by last pass result
             
		total += value;
    }

    //result = saturate(1 - total);							// return accessibility only
	result = saturate(1 - total) * 0.6 + tex2Dlod(lastResultMap, float4(receiverIndex.x, receiverIndex.y * g_sourceToTargetTexHeightFactor, 0, 0)).x * 0.4;
    return result;
}
//-----------------------------------------------------------------------------
// Simple Effect (1 technique with 1 pass)
//-----------------------------------------------------------------------------

technique Technique0
{
    pass OcclusionPass1
    {
		PixelShader  = compile ps_3_0 ambientOcclusionPS();
    }
    
    pass OcclusionPass2
    {
		PixelShader  = compile ps_3_0 ambientOcclusionPass2PS();
    }     
}

