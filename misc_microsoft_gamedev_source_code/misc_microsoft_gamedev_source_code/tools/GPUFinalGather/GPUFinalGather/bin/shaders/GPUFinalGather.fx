
float4x4 worldViewProj	: WorldViewProjection;	// This matrix will be loaded by the application
float3 globalRayDirection;



struct VS_OUTPUT
{
    float4 Position		: POSITION;  // vertex position 
    float4 color		: COLOR0;
    float4 hPos			: TEXCOORD0;        
    float4 wPos			: TEXCOORD1; 
};



VS_OUTPUT basicVS(  float4 vPosition : POSITION,
					float4 vColor : COLOR0)
{
    VS_OUTPUT Output;
	
    // Transform the vertex into projection space. 
    Output.Position = mul( vPosition, worldViewProj );
    Output.hPos = mul( vPosition, worldViewProj );
    Output.wPos = vPosition;
    	
    Output.color = vColor;
    return Output;
}


/////////////////////////////////////////////
//////////////////////////////////////////////
///////////////////////////////////////////////
//STEP 1
float4 fullPositionPS( VS_OUTPUT IN ) : COLOR0
{	
    return float4(IN.wPos.xyz,1);
}


/////////////////////////////////////////////
//////////////////////////////////////////////
///////////////////////////////////////////////
///STEP 5
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

texture worldPosFromCamTexture;
sampler worldPosFromCamSampler = sampler_state
{
    Texture   = (worldPosFromCamTexture);
    MipFilter = Point;
    MinFilter = Point;
    MagFilter = Point;
    
    AddressU = WRAP;
    AddressV = WRAP;
    AddressW = CLAMP;
};

texture depthLayerTexture;
sampler depthLayerSampler = sampler_state
{
    Texture   = (depthLayerTexture);
    MipFilter = Point;
    MinFilter = Point;
    MagFilter = Point;
    
    AddressU = WRAP;
    AddressV = WRAP;
    AddressW = CLAMP;
};
float4x4 gWorldToCamera;
float4x4 gWorldToLight;



void quad2DVS(	in float4 vPosition : POSITION,
				in float2 vUV0 : TEXCOORD0,
				out float4 oPosition : POSITION,
				out float2 oUV0 : TEXCOORD0)
{

	oPosition = vPosition;
	oUV0 = vUV0;
}

void projectComparePS(	
						in float4 vPosition : POSITION,
						in float2 oUV0 : TEXCOORD0,
						out float4 depth : COLOR0,
						out float4 col : COLOR1 )
{	
	float4 worldVertPos = tex2D(worldPosFromCamSampler, oUV0);
	clip(worldVertPos.w-0.1);//don't handle pixels that are NULL in the frame buffer
	worldVertPos.w=1;
	
	//project world point to light space
	float4 worldVertLightSpace = mul(worldVertPos,gWorldToLight);
	worldVertLightSpace /= worldVertLightSpace.w;
	float depthFromLight = worldVertLightSpace.z;
	
	
	
	worldVertLightSpace.xy = 0.5 * worldVertLightSpace.xy + float2( 0.5, 0.5 );
	worldVertLightSpace.xy += 0.5f / 256.0f;//halfpixeloffset
	float depthLayerDist = tex2D(prevDepthSampler, worldVertLightSpace.xy)+ 0.05f;//0.00005f = shadow map bias
	
	
	//E stores the nearest intersection point from Vertex A in world space, along the global ray direction
	//if(depthLayerDist < depthFromLight) E=D;
	float diff =  depthFromLight - depthLayerDist;
	clip(diff);
	
	
	float4 depthLayerColor = tex2D(depthLayerSampler, worldVertLightSpace.xy);
	//clip(depthLayerColor.w - 0.01);
	col = float4(depthLayerColor.xyz,1);
	if(depthLayerColor.w - 0.01 == 0)//if the depth layer pixel is 0, then there's no fragment here.
		col = float4(1,1,1,1);
    //col = float4(depthLayerColor.xyz,1);
    depth =0;
}

/////////////////////////////////////////////
//////////////////////////////////////////////
///////////////////////////////////////////////
///STEP 6


void depthComparePS( VS_OUTPUT IN,
	out float4 depth : COLOR0,
	out float4 col : COLOR1 )
{
	//translate our screenspace coords to raster coords..
	float4 rasterPt = IN.hPos / IN.hPos.w;
	rasterPt.xy = 0.5f * rasterPt.xy  + float2(0.5f,0.5f);
	rasterPt.xy += 0.5f / 256.0f;//halfpixeloffset
	
    float myDepth =  IN.hPos.z / IN.hPos.w;
    float prevDepth = tex2D(prevDepthSampler, rasterPt.xy);
    
    //clip me if i'm farther than previous depth
    float zBufPrecision = 1.0f/512.0f;		//bias offset to ensure our non-linear depth doesn't get boned	
    clip(prevDepth - (myDepth+zBufPrecision) );
	
    depth= myDepth;
	col = float4(IN.color.xyz,1);	//CLM this should be the irradiance from pass 1
}


/////////////////////////////////////////////
//////////////////////////////////////////////
///////////////////////////////////////////////
///STEP 9
texture irradianceTexture;
sampler irradianceSampler = sampler_state
{
    Texture   = (irradianceTexture);
    MipFilter = Linear;
    MinFilter = Linear;
    MagFilter = Linear;
    
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
};
void accumulatePS(	
						in float4 vPosition : POSITION,
						in float2 oUV0 : TEXCOORD0,
						out float4 col : COLOR0 )
{	
	col = tex2D(irradianceSampler, oUV0);
}

/////////////////////////////////////////////
//////////////////////////////////////////////
///////////////////////////////////////////////
technique Technique0
{
  
    pass Step1_GatherPosition
    {
		VertexShader = compile vs_3_0 basicVS();
		PixelShader  = compile ps_3_0 fullPositionPS();
    }

    pass Step5_ProjectLayer
    {
    	VertexShader = compile vs_3_0 quad2DVS();
		PixelShader  = compile ps_3_0 projectComparePS();
    }
    
    pass Step6_DepthColor_Compare
    {
		VertexShader = compile vs_3_0 basicVS();
		PixelShader  = compile ps_3_0 depthComparePS();
    }
    
    pass Step9_Accumulate
    {
		VertexShader = compile vs_3_0 quad2DVS();
		PixelShader  = compile ps_3_0 accumulatePS();
    }
}