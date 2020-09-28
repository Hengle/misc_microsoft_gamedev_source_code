

texture targetTexture;
sampler targetSampler = sampler_state
{
    Texture   = (targetTexture);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    
    AddressU = WRAP;
    AddressV = WRAP;
    AddressW = CLAMP;
};


texture alphaTexture;
sampler alphasSampler = sampler_state
{
    Texture   = (alphaTexture);
    MipFilter = NONE;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
};

texture targetTextureDecal;
sampler targetDecalSampler = sampler_state
{
    Texture   = (targetTextureDecal);
    MipFilter = NONE;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    
    AddressU = WRAP;
    AddressV = WRAP;
    AddressW = CLAMP;
};

texture decalAlphaTexture;
sampler decalAlphaSampler = sampler_state
{
    Texture   = (decalAlphaTexture);
    MipFilter = NONE;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
};


float gTargetAlphaLayer = 1;
float gContribOverride = 1;
float gAlphaOverride = 1;
//------------------------------------------------------------------------------
// Vertex shader output
//------------------------------------------------------------------------------
struct VSOUT
{
    float4 Position		: POSITION;
    float2 uv0			: TEXCOORD0;    
    float2 uv1			: TEXCOORD1;    
};

float2 gUVScale = float2(2,2);
float4 gDecalInf = float4(1,0,0,0);	//rotationangle, uOffset, vOffset, 0
float4 gColorOverload = float4(0,0,0,0);

//------------------------------------------------------------------------------
// Name: CompsVertex()
//------------------------------------------------------------------------------
VSOUT CompsVertex( float4 Position  : POSITION,
                         float2 TexCoord0 : TEXCOORD0,
                         float2 TexCoord1 : TEXCOORD1 )
{
    VSOUT Output;
    Output.Position  = Position;
    Output.uv0 = TexCoord0;
    Output.uv1 = TexCoord1;
    
    return Output;
}

//------------------------------------------------------------------------------
float4 CompsPixel_blend0_Splat(VSOUT In) : COLOR
{
	
   float4 albedo	= tex2D(targetSampler,In.uv0.xy * gUVScale) * gContribOverride;
   float alpha		= tex2D(alphasSampler,In.uv1.xy);
   
   return float4(albedo.r,albedo.g,albedo.b,alpha);
}

//-----------------------------------------------------------------------------
float2 changeUVs(float2 input, float2 offset, float2 scale, float rotAngle)
{
	float3x3 rotMatrix;
	rotMatrix[0] = float3(cos(rotAngle), sin(rotAngle),0);
	rotMatrix[1] = float3(-sin(rotAngle), cos(rotAngle),0);
	rotMatrix[2] = float3(0,0,1);
	
	float3x3 scaleMatrix;
	scaleMatrix[0] = float3(scale.x,0,0);
	scaleMatrix[1] = float3(0,scale.y,0);
	scaleMatrix[2] = float3(0,0,1);
	
	float3x3 transMatrix;
	transMatrix[0] = float3(1,0,offset.x );
	transMatrix[1] = float3(0,1,offset.y);
	transMatrix[2] = float3(0,0,1);
	
	
	float3x3 combMatrix;
	combMatrix = mul(scaleMatrix,transMatrix);
	combMatrix = mul(rotMatrix,combMatrix);

	float3 results = float3(input.xy,1);
	
	results = mul(combMatrix,results);
	
	results += float3(0.5,0.5,0);
	
	return results.xy;
}
//------------------------------------------------------------------------------
float4 CompsPixel_blend0_Decal(VSOUT In) : COLOR
{
	float2 newUVs = changeUVs(In.uv0.yx,-gDecalInf.zy,gUVScale.xy,gDecalInf.x);

   float4 albedo	= tex2D(targetDecalSampler,newUVs) * gContribOverride;
   albedo.a         = tex2D(decalAlphaSampler,newUVs);
   float alpha		= tex2D(alphasSampler,In.uv0.xy);
   
   albedo *= gColorOverload;
   return float4(albedo.r,albedo.g,albedo.b,albedo.a*alpha);
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
technique Technique0
{
    pass Splat
    {
		VertexShader = compile vs_3_0 CompsVertex();
		PixelShader  = compile ps_3_0 CompsPixel_blend0_Splat();
    }
    pass Decal
    {
		VertexShader = compile vs_3_0 CompsVertex();
		PixelShader  = compile ps_3_0 CompsPixel_blend0_Decal();
    }

    
}