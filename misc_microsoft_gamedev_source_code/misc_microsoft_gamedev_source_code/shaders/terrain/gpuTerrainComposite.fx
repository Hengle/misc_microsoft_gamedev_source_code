#include "..\shared\helpers.inc"
#include "..\shared\intrinsics.inc"
#include "gpuTerrainshaderRegs.inc"

float g_explicitMipValue=0;
float g_RCPnumLayers=0;
float g_RCPnumAlignedLayers=0;


//for batch splatting!
float4 g_LayerData[MAX_NUM_LAYERS]		: register(LAYER_DATA);	//layerIndex, u0, v0, 1/layerIndex
float4 g_HDRData[32]					: register(HDR_DATA_CURRENT);	//offsetU, offsetV, offsetScale,hdrScale
 
sampler targetSampler = sampler_state
{
    MipFilter = NONE;
    
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    
    AddressU = WRAP;
    AddressV = WRAP;
    AddressW = CLAMP;
    
    SEPARATEZFILTERENABLE = TRUE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};


sampler alphasSampler = sampler_state
{
    MipFilter = NONE;
    
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
    
    SEPARATEZFILTERENABLE = TRUE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};

//for single shot decals!

float4 gUVScale				: register(DECAL_UVSCALE);			//uScale, vScale, indexPrim, indexSec 
float4 gDecalInf			: register(DECAL_PROPERTIES);		//rotationangle, uOffset, vOffset, HDRScale

sampler targetDecalSampler = sampler_state
{
    MipFilter = NONE;
    
    MinFilter = POINT;
    MagFilter = POINT;
    
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
};

sampler decalOpacitySampler = sampler_state
{
    MipFilter = NONE;
    
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
};



//------------------------------------------------------------------------------
// Vertex shader output
//------------------------------------------------------------------------------
struct VSOUT
{
    float4 Position			: POSITION;
    float2 alphaUVs			: TEXCOORD0;    
    float2 targetUVs		: TEXCOORD1;
    float4 indexes			: TEXCOORD2;
};


//------------------------------------------------------------------------------
// Name: CompsVertex()
//------------------------------------------------------------------------------
VSOUT CompsVertex( float4 Position  : POSITION,
					float2 TexCoord0 : TEXCOORD0,
					float2 TexCoord1 : TEXCOORD1,
					int ind : INDEX )
{
    VSOUT Output;
    Output.Position		= Position;
    
    int indx = ind *0.083333333334f; // this number has to match the number of verts in the layer, is now 12 verts per layer due to D3DPT_RECTLIST
	float layer = (indx * g_RCPnumLayers.x);
	
	
    Output.alphaUVs			= TexCoord1;
    Output.targetUVs   	= TexCoord0 * g_LayerData[indx].yz;
    
	
    int prim = (indx * 0.25);// + 0.000003f;
    int sec = (indx % 4);// + 0.00003f;

	float uvPrim = (prim * g_RCPnumAlignedLayers )+ 0.00003f;
    Output.indexes = float4(uvPrim,sec,g_LayerData[indx].w,g_LayerData[indx].x+ 0.00003f);

    return Output;
}

//------------------------------------------------------------------------------
float getAlphaValue(float2 indexes, float2 uv0)
{
	float4 alphaPrim = tex3D(alphasSampler,float3(uv0,indexes.x));
	
	return alphaPrim[(int)indexes.y];
}
//------------------------------------------------------------------------------
float4 CompsPixel_albedo_ARGB8(VSOUT In) : COLOR
{
   float4 target	= tex3Dlod(targetSampler,float4(In.targetUVs ,In.indexes.z,g_explicitMipValue));
   
   float alpha		= getAlphaValue(In.indexes,In.alphaUVs);
   return float4(target.r,target.g,target.b,alpha);
}

//------------------------------------------------------------------------------
float4 CompsPixel_normal_ARGB8(VSOUT In) : COLOR
{
   float3 target	= tex3Dlod(targetSampler,float4(In.targetUVs ,In.indexes.z,g_explicitMipValue)).xyz ;
   
   float alpha		= getAlphaValue(In.indexes,In.alphaUVs);
   return float4(target.r,target.g,target.b,alpha);
}

//------------------------------------------------------------------------------
float4 CompsPixel_emissive_HDR(VSOUT In) : COLOR
{
   float4 target	= tex3Dlod(targetSampler,float4(In.targetUVs,In.indexes.z,g_explicitMipValue));		
   target.xyz = unpackDXT5H(target,g_HDRData[In.indexes.w].x);
   
   
   float alpha		= getAlphaValue(In.indexes,In.alphaUVs);
   return float4(target.xyz,alpha);
   
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
VSOUT CompsVertex_decal( float4 Position  : POSITION,
					float2 TexCoord0 : TEXCOORD0,
					int ind : INDEX )
{
    VSOUT Output;
    Output.Position		= Position;
    
    Output.alphaUVs			= TexCoord0;
    Output.targetUVs	= TexCoord0;
    Output.indexes = float4(0,0,0,0);

    return Output;
}
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
float4 CompsPixel_albedo_ARGB8_Decal(VSOUT In) : COLOR
{
	float2 newUVs = changeUVs(In.alphaUVs.yx,-gDecalInf.zy,gUVScale.xy,gDecalInf.x);

   float4 albedo	= tex2D(targetDecalSampler,newUVs);
   albedo.a         = tex2D(decalOpacitySampler,newUVs);
   float alpha		= getAlphaValue(gUVScale.zw,In.alphaUVs);
   
   
   float v= albedo.a*alpha;
   return float4(albedo.r,albedo.g,albedo.b,v);
  //return float4(alpha,alpha,alpha,alpha);
}
//------------------------------------------------------------------------------
float4 CompsPixel_normal_ARGB8_Decal(VSOUT In) : COLOR
{
	float2 newUVs = changeUVs(In.alphaUVs.yx,-gDecalInf.zy,gUVScale.xy,gDecalInf.x);

   float4 albedo	= tex2D(targetDecalSampler,newUVs);
   albedo.a         = tex2D(decalOpacitySampler,newUVs);
   float alpha		= getAlphaValue(gUVScale.zw,In.alphaUVs);
   
   
   float v= albedo.a*alpha;
   return float4(albedo.r,albedo.g,albedo.b,v);
 
}
//------------------------------------------------------------------------------
float4 CompsPixel_emissive_HDR_Decal(VSOUT In) : COLOR
{
	float2 newUVs = changeUVs(In.alphaUVs.yx,-gDecalInf.zy,gUVScale.xy,gDecalInf.x);

   float4 albedo	= tex2D(targetDecalSampler,newUVs);
   albedo.xyz		= unpackDXT5H(albedo,gUVScale.w);
   albedo.a         = tex2D(decalOpacitySampler,newUVs);
   float alpha		= getAlphaValue(gUVScale.zw,In.alphaUVs);
   
   
   float v= albedo.a*alpha;
   return float4(albedo.r,albedo.g,albedo.b,v);
 
}


//------------------------------------------------------------------------------
float4 CompsPixel_albedo_ARGB8_SplatSet_Decal(VSOUT In) : COLOR
{
	float2 newUVs = changeUVs(In.alphaUVs.yx,-gDecalInf.zy,1.0f/gUVScale.xy,gDecalInf.x);

   float4 albedo	= tex3Dlod(targetSampler,float4(In.targetUVs * g_LayerData[0].yz,gUVScale.z,g_explicitMipValue));	
   albedo.a         = tex2D(decalOpacitySampler,newUVs);

   return float4(albedo.r,albedo.g,albedo.b,albedo.a);
}
//------------------------------------------------------------------------------
float4 CompsPixel_normal_ARGB8_SplatSet_Decal(VSOUT In) : COLOR
{
	float2 newUVs = changeUVs(In.alphaUVs.yx,-gDecalInf.zy,1.0f/gUVScale.xy,gDecalInf.x);

   float4 albedo	= tex3Dlod(targetSampler,float4(In.targetUVs * g_LayerData[0].yz,gUVScale.z,g_explicitMipValue));	
   albedo.a         = tex2D(decalOpacitySampler,newUVs);


   return float4(albedo.r,albedo.g,albedo.b,albedo.a);
 
}
//------------------------------------------------------------------------------
float4 CompsPixel_emissive_HDR_SplatSet_Decal(VSOUT In) : COLOR
{
	float2 newUVs = changeUVs(In.alphaUVs.yx,-gDecalInf.zy,1.0f/gUVScale.xy,gDecalInf.x);

   float4 albedo	= tex3Dlod(targetSampler,float4(In.targetUVs * g_LayerData[0].yz,gUVScale.z,g_explicitMipValue));	
   albedo.xyz		= unpackDXT5H(albedo,gUVScale.w);
	albedo.a         = tex2D(decalOpacitySampler,newUVs);

   return float4(albedo.r,albedo.g,albedo.b,albedo.a);
 
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
technique composeTexture
{
//SPLATTING------------------------------------------
    pass albedo_RGBA8
    {
		VertexShader = compile vs_3_0 CompsVertex();
		PixelShader  = compile ps_3_0 CompsPixel_albedo_ARGB8();
    }
    pass normal_RGBA8
    {
		VertexShader = compile vs_3_0 CompsVertex();
		PixelShader  = compile ps_3_0 CompsPixel_normal_ARGB8();
    }
    pass specular_RGBA8
    {
		VertexShader = compile vs_3_0 CompsVertex();
		PixelShader  = compile ps_3_0 CompsPixel_albedo_ARGB8();
    }
    pass envMask_RGBA8
    {
		VertexShader = compile vs_3_0 CompsVertex();
		PixelShader  = compile ps_3_0 CompsPixel_albedo_ARGB8();
    }
	pass emissive_HDR
    {
		VertexShader = compile vs_3_0 CompsVertex();
		PixelShader  = compile ps_3_0 CompsPixel_emissive_HDR();
    }
//DECALS------------------------------------------
    pass albedo_RGBA8
    {
		VertexShader = compile vs_3_0 CompsVertex_decal();
		PixelShader  = compile ps_3_0 CompsPixel_albedo_ARGB8_Decal();
    }
    pass normal_RGBA8
    {
		VertexShader = compile vs_3_0 CompsVertex_decal();
		PixelShader  = compile ps_3_0 CompsPixel_normal_ARGB8_Decal();
    }
    pass specular_RGBA8
    {
		VertexShader = compile vs_3_0 CompsVertex_decal();
		PixelShader  = compile ps_3_0 CompsPixel_albedo_ARGB8_Decal();
    }
    pass envMask_RGBA8
    {
		VertexShader = compile vs_3_0 CompsVertex_decal();
		PixelShader  = compile ps_3_0 CompsPixel_albedo_ARGB8_Decal();
    }
	pass emissive_HDR
    {
		VertexShader = compile vs_3_0 CompsVertex_decal();
		PixelShader  = compile ps_3_0 CompsPixel_emissive_HDR_Decal();
    }
    
//SPLATSET DECALS------------------------------------------
    pass albedo_RGBA8
    {
		VertexShader = compile vs_3_0 CompsVertex_decal();
		PixelShader  = compile ps_3_0 CompsPixel_albedo_ARGB8_SplatSet_Decal();
    }
    pass normal_RGBA8
    {
		VertexShader = compile vs_3_0 CompsVertex_decal();
		PixelShader  = compile ps_3_0 CompsPixel_normal_ARGB8_SplatSet_Decal();
    }
    pass specular_RGBA8
    {
		VertexShader = compile vs_3_0 CompsVertex_decal();
		PixelShader  = compile ps_3_0 CompsPixel_albedo_ARGB8_SplatSet_Decal();
    }
    pass envMask_RGBA8
    {
		VertexShader = compile vs_3_0 CompsVertex_decal();
		PixelShader  = compile ps_3_0 CompsPixel_albedo_ARGB8_SplatSet_Decal();
    }
	pass emissive_HDR
    {
		VertexShader = compile vs_3_0 CompsVertex_decal();
		PixelShader  = compile ps_3_0 CompsPixel_emissive_HDR_SplatSet_Decal();
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


/*
	CLM [09.08.06]
	This entire file is under the assumption that we're either copying
	DXT1 or DXT5 textures, which have been aliased to appear to be vertexStreams
	IF you impliment this for another texture type, you have to be sure to change the appropriate stride considerations
*/

sampler FullScreenQuadFetchConstant : register(vf0);

float4 ExportAddress : register(c4);
static float4 ExportConst = { 0.0, 1.0, 0.0, 0.0 };

void vFetchAndMemExport(int vIndex)
{
	float4 vDstData = float4(1,0,1,0);
	int kkIndex =vIndex;
  
   asm
   {
   		vfetch_full vDstData, vIndex, FullScreenQuadFetchConstant, DataFormat=FMT_32_32_32_32_FLOAT, Stride=4, NumFormat=integer
	};
			
	asm
	{
        alloc export=1
        mad eA, vIndex, ExportConst, ExportAddress
        max eM0.xyzw, vDstData, vDstData
    };    
}


void copyTextureVS(int Index : INDEX)
{
	vFetchAndMemExport(Index);
}


float4 copyTexturePS(): COLOR
{
	return float4(0,0,0,0);
}

//------------------------------------------------------------------------------
technique copyTextures
{
    pass albedo
    {

// This version causes the GPU to deadlock. Use the purposely slowed asm version instead.
//		VertexShader = compile vs_3_0 copyTextureVS();
		
#if 1	
		  VertexShader = asm
        {
            //
            // Generated by 2.0.4025.0
            //
            // Parameters:
            //
            //   float4 ExportAddress;
            //
            //
            // Registers:
            //
            //   Name          Reg   Size
            //   ------------- ----- ----
            //   ExportAddress c4       1
            //
            //
            // Default values:
            //
            //   ExportAddress
            //     c4   = { 0, 0, 0, 0 };
            //
            
            // Shader type: vertex 
            
            xvs_3_0
#if 0            
            defconst ExportAddress, float, vector, [1, 4], c4, { 0, 0, 0, 0 }
#endif            
            config AutoSerialize=false
            config AutoResource=false
            config VsMaxReg=12
            config VsResource=1
            config VsExportMode=multipass
            
            dcl_index r0.x
            
            def c252, 0, 0, 0, 0
            def c253, 0, 0, 0, 0
            def c254, 0, 0, 0, 0
            def c255, 0, 1, 0, 0
            
            
            /*    0.0 */     exec
            /*    4   */     vfetch_full r1, r0.x, vf0, 
                             DataFormat=FMT_32_32_32_32_FLOAT, 
                             Stride=4, 
                             NumFormat=integer

                             serialize
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             mov r2, r1
                             mov r3, r1
                             mov r4, r1
                             mov r5, r1
                             mov r6, r1
                             mov r7, r1
                             mov r8, r1
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             mov r1, r8
                             mov r2, r7
                             mov r3, r6
                             mov r4, r5
                             mov r5, r4
                             mov r6, r3
                             mov r7, r2
                             mov r8, r1
                             
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             mov r2, r1
                             mov r3, r1
                             mov r4, r1
                             mov r5, r1
                             mov r6, r1
                             mov r7, r1
                             mov r12, r1
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             nop                                                                  
                             mov r1, r8
                             mov r2, r7
                             mov r3, r6
                             mov r4, r5
                             mov r5, r4
                             mov r6, r3
                             mov r7, r2
                             mov r8, r1
                                                              
            /*    0.1 */     alloc export=1
            /*    1.0 */     exec
            /*    5   */     mad eA, r0.x, c255.xyx, c4
            /*    6   */     mov eM0, r1
            /*    1.1 */     alloc position
            /*    2.0 */     exec
            /*    7   */     sgt oPos, -r0_abs.x, c255.x
            /*    2.1 */     alloc interpolators
            /*    3.0 */     exece
            /*    3.1 */     cnop
        };
#endif        
		
		PixelShader  = compile ps_3_0 copyTexturePS();
    }
}