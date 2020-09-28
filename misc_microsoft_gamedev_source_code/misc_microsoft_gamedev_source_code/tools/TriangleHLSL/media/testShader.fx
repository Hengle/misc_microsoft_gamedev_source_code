#define MAX_BONES 75

float4x4 gViewToProj;

float4 gViewToProjMul;
float4 gViewToProjAdd;

bool gOneBone;
bool gFourBones;

float4x3 gRigidBindToView;
float4x3 gBindToViewMatrices[MAX_BONES];

static float4 mulVec4x3(float4 v, float4x3 m)
{
   return float4(
      dot(v, m._m00_m10_m20_m30),
      dot(v, m._m01_m11_m21_m31),
      dot(v, m._m02_m12_m22_m32),
      1);
}

static float3 mulVec4x3(float3 v, float4x3 m)
{
   return float3(
      dot(v, m._m00_m10_m20),
      dot(v, m._m01_m11_m21),
      dot(v, m._m02_m12_m22));
}

static float4x3 CalcBindToViewMatrix(int4 Indices, float4 Weights)
{
   float4x3 result = gBindToViewMatrices[Indices[0]];
   
   if (!gOneBone)
   {
      result += gBindToViewMatrices[Indices[1]] * Weights[1];

      if (gFourBones)
      {
         result += gBindToViewMatrices[Indices[2]] * Weights[2];
         result += gBindToViewMatrices[Indices[3]] * Weights[3];
      }
   }
      
   return result;
}

struct VertexIn
{                                             
    float4 Position  : POSITION;                

#ifdef NORMAL    
    float3 Normal    : NORMAL;
#endif    
    
#ifdef BUMP    
    float3 Tangent   : TANGENT;
    float3 Binormal  : BINORMAL;
#endif    

#ifdef UV0
    float2 UV0       : TEXCOORD0;    
#endif

#ifdef UV1
    float2 UV1       : TEXCOORD1;    
#endif    

#if !RIGID  
    int4 InIndices   : BLENDINDICES;
    float4 InWeights : BLENDWEIGHT;
#endif
};                                            
                                              
struct VertexOut
{                                             
    float4 Position  : POSITION;                

#ifdef NORMAL        
    float3 Normal    : NORMAL;
#endif    
    
#ifdef BUMP    
    float3 Tangent   : TANGENT;
    float3 Binormal  : BINORMAL;
#endif    

#ifdef UV0
    float2 UV0       : TEXCOORD0;    
#endif

#ifdef UV1
    float2 UV1       : TEXCOORD1;    
#endif    
};                                             
                                              
VertexOut vsMain(VertexIn in)
{                                              
   VertexOut out;                                

#if RIGID
   float4x3 bindToView = gRigidBindToView;
#else  
   float4x3 bindToView = CalcBindToViewMatrix(in.InIndices, in.InWeights);
#endif  

   float4 viewPos = mulVec4x3(in.Position, bindToView);
   viewPos.w = 1;
    
   Out.Pos = viewPos.xyzz * gViewToProjMul + gViewToProjAdd;

#ifdef NORMAL
   out.Normal = mulVec4x3(in.Normal, bindToView);
#endif   

#ifdef BUMP
   Out.Tangent = mulVec4x3(in.Tangent, bindToView);
   Out.Binormal = mulVec4x3(in.Binormal, bindToView);
#endif

#ifdef UV0  
   out.UV0 = in.UV0;
#endif

#ifdef UV1
   out.UV1 = in.UV1;
#endif
  
 return Out;                                
}                                            

struct PixelIn                                 
{
#ifdef UV0
    float2 UV0 : TEXCOORD0;
#endif
#ifdef UV1
    float2 UV1 : TEXCOORD1;
#endif    
};                                            

sampler DiffuseSampler = sampler_state
{
   MipFilter = LINEAR;
   MinFilter = LINEAR;
   MagFilter = LINEAR;
    
   AddressU = WRAP;
   AddressV = WRAP;
   AddressW = CLAMP;
};

float4 psMain(PixelIn in) : COLOR                
{
#ifdef UV0
   return tex2D(DiffuseSampler, in.UV0); 
#else
   return float4(1,1,1,1);
#endif
} 

technique T0
{
    pass P0
    {
        VertexShader = compile vs_2_0 vsMain();
        PixelShader = compile ps_2_0 psMain();
    }
}

