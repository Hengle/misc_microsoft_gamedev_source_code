//==============================================================================
//VERTEX SHADER
//==============================================================================

//#define DEMO_HACK


sampler vertSampler_pos = sampler_state
{
    MipFilter = NONE;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP ;
};
sampler vertSampler_basis = sampler_state
{
    MipFilter = NONE;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

sampler vertSampler_light = sampler_state
{
    MipFilter = NONE;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

sampler vertSampler_ao = sampler_state
{
    MipFilter = NONE;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

sampler vertSampler_alpha = sampler_state
{
    MipFilter = NONE;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};




float4   g_terrainVals;         //numXVerts, scale, reloffsetX,reloffsetZ
float4   g_terrainVals2;         //u,v,0,0

float4x4 g_quadrantMatrix;


float    g_rcpMapSize;           // 1/4096

float4   g_posCompMin;         
float4   g_posCompRange;

//==============================================================================
// GiveTBN
//==============================================================================
void  GiveTBN(out float3x3 TBN, float3 a, float3 b, float3 c)
{
   float3 normal;
   normal=normalize(cross(a-b,a-c));
	
	float3 binormal = normalize(normal.xzy * float3(0, 1, -1));
   float3 temp2 = normal.zxy * binormal.yzx;
   float3 tangent = normalize((((normal.yzx) * (-binormal.zxy)) - temp2)); 
     
   TBN[0] = tangent;
   TBN[1] = binormal;
   TBN[2] = normal;
}

//==============================================================================
// GiveTBNFromSampler
//==============================================================================
void GiveTBNFromNormal(float3 normal, out float3x3 TBN)
{
  
   float3 tangent, binormal, temp;
      
   normal = ((normal * 2.0f) - 1.0f);
   

   binormal = normalize(normal.xzy * float3(0, 1, -1));
   tangent = cross(binormal, normal);

   TBN[0] = tangent;
   TBN[1] = binormal;
   TBN[2] = normal;
   
 
}

int gPatchIndexSwap[]={0,1,4,5, 2,3,6,7, 8,9,12,13, 10,11,14,15};
float4 gPatchOffsetNumbers = float4(0,0,4,0.25);	//xUVOffset, yUVOffset, xModVal, yModVal
//==============================================================================
// chunkPosFromPatch - Helper class...
//==============================================================================
float2 chunkPosFromPatch(int vIndex,int vQuadID, float2 vUV)
{
// Re-order the parametric coordinates based on the quad id.
    float2 uv = vUV * ( vQuadID == 0 );
    uv += float2( 1.0 - vUV.x,       vUV.y ) * ( vQuadID == 1 ); 
    uv += float2( 1.0 - vUV.x, 1.0 - vUV.y ) * ( vQuadID == 2 );
    uv += float2(       vUV.x, 1.0 - vUV.y ) * ( vQuadID == 3 );
        
    vIndex = gPatchIndexSwap[vIndex];
    
	float2 cP = float2(floor(fmod(vIndex,gPatchOffsetNumbers.z)),floor(vIndex *gPatchOffsetNumbers.w));
	
	
    cP.x = ((cP.x+uv.x)*16) + gPatchOffsetNumbers.x;		//puts parametric coords into our chunkspace
    cP.y = ((cP.y+uv.y)*16) + gPatchOffsetNumbers.y;
    
    return cP;
}

//==============================================================================
// VIndexToPosPATCHShadow - Uncompress verts
//==============================================================================
float3 VIndexToPosPATCHShadow(int vIndex,int vQuadID, float2 vUV,out float2 vQNuv)
{
  // First compute the integer x, y coordinates of the top left corner of the quad from the index.
    float2 gPos = chunkPosFromPatch(vIndex,vQuadID,vUV);
    
    //calculate our atlasUV
    gPos += g_terrainVals.zw;
    float2 vertTexAtlasUV = (gPos * g_terrainVals.x);
    vQNuv = vertTexAtlasUV;
    
    float3 offset = tex2Dlod( vertSampler_pos,   float4(vertTexAtlasUV.y,vertTexAtlasUV.x,0.f,0.f) );
    offset = ((offset) *g_posCompRange)-g_posCompMin;
	
    float3 pos = (float3( gPos.x , 0, gPos.y )* g_terrainVals.y)+offset ;
    

    return pos;
} 
float4 VIndexToPosPATCHShadowAlpha(int vIndex,int vQuadID, float2 vUV,out float2 vQNuv)
{
  // First compute the integer x, y coordinates of the top left corner of the quad from the index.
    float2 gPos = chunkPosFromPatch(vIndex,vQuadID,vUV);
    
    //calculate our atlasUV
    gPos += g_terrainVals.zw;
    float2 vertTexAtlasUV = (gPos * g_terrainVals.x);
    vQNuv = vertTexAtlasUV;
    
    
    float3 offset = tex2Dlod( vertSampler_pos,   float4(vertTexAtlasUV.y,vertTexAtlasUV.x,0.f,0.f) );
    offset = ((offset) *g_posCompRange)-g_posCompMin;
	
    float4 pos;
    pos.xyz = (float3( gPos.x , 0, gPos.y )* g_terrainVals.y)+offset ;
    pos.w =  tex2Dlod( vertSampler_alpha,   float4(vertTexAtlasUV.x,vertTexAtlasUV.y,0.f,0.f) );
    

    return pos;
} 
//==============================================================================
// VIndexToPosPATCH - Uncompress verts
//==============================================================================
float3 VIndexToPosPATCH(int vIndex,int vQuadID, float2 vUV,
						out float2 uvCoord0,out float2 QNuv0,out float3x3 TBN, out float ao, out float alpha,out float4 additiveLocalLight)
{
  // First compute the integer x, y coordinates of the top left corner of the quad from the index.
    float2 gPos = chunkPosFromPatch(vIndex,vQuadID,vUV);
    
     
    uvCoord0 = (gPos / 64.f);		//calculate our UV0 for texturing
   
		
    //calculate our atlasUV
    gPos += g_terrainVals.zw;
    float2 vertTexAtlasUV = (gPos * g_terrainVals.x);
    QNuv0=vertTexAtlasUV;
    
    float3 offset = tex2Dlod( vertSampler_pos,   float4(vertTexAtlasUV.y,vertTexAtlasUV.x,0.f,0.f) );
    offset = ((offset) *g_posCompRange)-g_posCompMin;
	
    float3 pos = (float3( gPos.x , 0, gPos.y )* g_terrainVals.y)+offset ;
    
    float4 norm = tex2Dlod( vertSampler_basis,   float4(vertTexAtlasUV.y,vertTexAtlasUV.x,0.f,0.f) );
    GiveTBNFromNormal(norm,TBN);
    
    ao = tex2Dlod( vertSampler_ao,   float4(vertTexAtlasUV.x,vertTexAtlasUV.y,0.f,0.f) );
    
    alpha = tex2Dlod( vertSampler_alpha,   float4(vertTexAtlasUV.x,vertTexAtlasUV.y,0.f,0.f) );
    
    additiveLocalLight = tex2Dlod( vertSampler_light,   float4(vertTexAtlasUV.x,vertTexAtlasUV.y,0.f,0.f) );
    
    return pos;
} 

uniform float gNumBatchedXChunks = 1;
//==============================================================================
// VIndexToPosPATCH - Uncompress verts
//==============================================================================
float3 VIndexToPosPATCHSkirt(int vIndex,int vQuadID, float2 vUV,
						out float2 QNuv0,
						out float3 normal, 
						out float alpha)
{
	const float cNumPatchesPerChunk = 4 * gNumBatchedXChunks ;
	const float rcpNumPatchesPerChunk = 1.0f/cNumPatchesPerChunk;

  // First compute the integer x, y coordinates of the top left corner of the quad from the index.
      
    float2 uv = vUV * ( vQuadID == 0 );
    uv += float2( 1.0 - vUV.x,       vUV.y ) * ( vQuadID == 1 ); 
    uv += float2( 1.0 - vUV.x, 1.0 - vUV.y ) * ( vQuadID == 2 );
    uv += float2(       vUV.x, 1.0 - vUV.y ) * ( vQuadID == 3 );
        
         
	float2 cP = float2(floor(fmod(vIndex,cNumPatchesPerChunk)),floor(vIndex *rcpNumPatchesPerChunk));
	//float2 cP = float2(floor(fmod(vIndex,8)),floor(vIndex *0.125));
	
	
    cP.x = ((cP.x+uv.x)*16);		//puts parametric coords into our chunkspace
    cP.y = ((cP.y+uv.y)*16);
    
    float2 gPos = cP;//chunkPosFromPatch(vIndex,vQuadID,vUV);

    //calculate our atlasUV
    gPos += g_terrainVals.zw;
    
    float2 vertTexAtlasUV = (gPos * g_terrainVals.x);
    QNuv0=vertTexAtlasUV;
    
    float3 offset = tex2Dlod( vertSampler_pos,   float4(vertTexAtlasUV.y,vertTexAtlasUV.x,0.f,0.f) );
    offset = ((offset) *g_posCompRange)-g_posCompMin;
	
    float3 pos = (float3( gPos.x , 0, gPos.y )* g_terrainVals.y)+offset ;
    
    normal = tex2Dlod( vertSampler_basis,   float4(vertTexAtlasUV.y,vertTexAtlasUV.x,0.f,0.f) ).xyz;
    normal = ((normal * 2.0f) - 1.0f);
    

	alpha = tex2Dlod( vertSampler_alpha,   float4(vertTexAtlasUV.x,vertTexAtlasUV.y,0.f,0.f) );

    return pos;
} 


//==============================================================================
// VIndexToPosPATCH - Uncompress verts
//==============================================================================
float3 VIndexToPosPATCHSkirtHighResTex(int vIndex,int vQuadID, float2 vUV,
						out float2 QNuv0,
						out float3 normal,
						out float alpha)
{
	float2 gPos = chunkPosFromPatch(vIndex,vQuadID,vUV);

    QNuv0.yx = (gPos / 64.f);		//calculate our UV0 for texturing


    //calculate our atlasUV
    gPos += g_terrainVals.zw;
    
    float2 vertTexAtlasUV = (gPos * g_terrainVals.x);
    //QNuv0=vertTexAtlasUV;
    
    float3 offset = tex2Dlod( vertSampler_pos,   float4(vertTexAtlasUV.y,vertTexAtlasUV.x,0.f,0.f) );
    offset = ((offset) *g_posCompRange)-g_posCompMin;
	
    float3 pos = (float3( gPos.x , 0, gPos.y )* g_terrainVals.y)+offset ;
    
    normal = tex2Dlod( vertSampler_basis,   float4(vertTexAtlasUV.y,vertTexAtlasUV.x,0.f,0.f) ).xyz;
    normal = ((normal * 2.0f) - 1.0f);
    
    alpha = tex2Dlod( vertSampler_alpha,   float4(vertTexAtlasUV.x,vertTexAtlasUV.y,0.f,0.f) );


    return pos;
} 


//==============================================================================
// VIndexToPosPATCH - Uncompress verts
//==============================================================================
float3 VIndexToPosPATCHSkirtLowResTex(int vIndex,int vQuadID, float2 vUV,
						out float2 QNuv0,
						out float3 normal,
						out float alpha)
{
	float2 gPos = chunkPosFromPatch(vIndex,vQuadID,vUV);

    //calculate our atlasUV
    gPos += g_terrainVals.zw;
    
    float2 vertTexAtlasUV = (gPos * g_terrainVals.x);
    QNuv0=vertTexAtlasUV;
    
    float3 offset = tex2Dlod( vertSampler_pos,   float4(vertTexAtlasUV.y,vertTexAtlasUV.x,0.f,0.f) );
    offset = ((offset) *g_posCompRange)-g_posCompMin;
	
    float3 pos = (float3( gPos.x , 0, gPos.y )* g_terrainVals.y)+offset ;
    
    normal = tex2Dlod( vertSampler_basis,   float4(vertTexAtlasUV.y,vertTexAtlasUV.x,0.f,0.f) ).xyz;
    normal = ((normal * 2.0f) - 1.0f);
    
    alpha = tex2Dlod( vertSampler_alpha,   float4(vertTexAtlasUV.x,vertTexAtlasUV.y,0.f,0.f) );


    return pos;
} 