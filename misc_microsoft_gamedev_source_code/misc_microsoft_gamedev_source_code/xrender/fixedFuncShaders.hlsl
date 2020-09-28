
sampler gSampler0 : register(s0) = sampler_state  
{                                                    
  AddressU   = CLAMP;                            
  AddressV   = CLAMP;                            
  AddressW   = CLAMP;                            
  MipFilter  = LINEAR;                           
  MinFilter  = LINEAR;                           
  MagFilter  = LINEAR;                           
};
                                                
sampler gSampler1 : register(s1);                 

struct InParams                                   
{                                                 
#ifdef DIFFUSE                                    
  float4 color0 : COLOR0;                        
#endif
                                            
#ifdef SPEC                                       
  float4 color1 : COLOR1;                        
#endif
                                            
#ifdef UV0                                        
  float2 tex0 : TEXCOORD0;                       
#endif
                                            
#ifdef UV1                                        
  float2 tex1 : TEXCOORD1;                       
#endif                                            
};                                                

float4 PSFixedFunc(InParams params) : COLOR              
{                                                 
#ifdef DIFFUSE                                    
  float4 c = params.color0;                      
#else                                             
  float4 c = float4(1,1,1,1);                    
#endif
                                            
#ifdef UV0                                        
  c = c * tex2D(gSampler0, params.tex0);         
#endif
                                            
#ifdef UV1                                        
  c = c * tex2D(gSampler1, params.tex1);         
#endif
                                            
#ifdef SPEC                                       
  c = c + params.color1;                         
#endif
                                            
#ifdef  OVERBRIGHT                                
  c *= 2.0f;                                     
#endif
                                            
  return c;                                      
}



sampler gSamplerVis : register(s0);                       
float4 gMul : register(c0);                             
float4 gAdd : register(c1);                             
float gSlice : register(c2);
                            
float4 PSDepthVis(float2 tex0 : TEXCOORD0) : COLOR            
{                                                       
  float depth = tex3D(gSamplerVis, float3(tex0.x, tex0.y, gSlice)).r;
//   depth -= 8388608.0;                                  
//   depth *= (1.0 / 8388608.0);                          
  return gMul * depth + gAdd;                          
}

float4 PSAlphaVis(float2 tex0 : TEXCOORD0) : COLOR            
{                                                       
  return tex2D(gSamplerVis, tex0).a;                     
}
                   
float4 PSRedVis(float2 tex0 : TEXCOORD0) : COLOR            
{                                                       
  return tex2D(gSamplerVis, tex0).r;                     
}

float4 PSGreenVis(float2 tex0 : TEXCOORD0) : COLOR            
{                                                       
  return tex2D(gSamplerVis, tex0).g;                     
}

float4 PSBlueVis(float2 tex0 : TEXCOORD0) : COLOR            
{                                                       
  return tex2D(gSamplerVis, tex0).b;                     
}




float4x4 gWorldViewProjMatrix : register(c0);            

#ifdef USE_CONSTANT_DIFFUSE                              
float4   gColorConstant       : register(c4);            
#endif
                                                   
void VSFixedFunc(                                               
   in float4 InPosition : POSITION                       

#ifdef DIFFUSE                                           
  ,in  float4 InColor0 : COLOR0                         
#endif                                                   

#ifdef SPEC                                              
  ,in float4 InColor1 : COLOR1                          
#endif                                                   

#ifdef UV0                                               
  ,in float4 InTex0 : TEXCOORD0                         
#endif                                                   

#ifdef UV1                                               
  ,in float4 InTex1 : TEXCOORD1                         
#endif                                                   

  ,out float4 OutPosition : POSITION                    

#if defined(DIFFUSE) || defined(USE_CONSTANT_DIFFUSE)    
  ,out float4 OutColor0 : COLOR0                        
#endif                                                   

#ifdef SPEC                                              
  ,out float4 OutColor1 : COLOR1                        
#endif                                                   

#ifdef UV0                                               
  ,out float4 OutTex0 : TEXCOORD0                       
#endif                                                   

#ifdef UV1                                               
  ,out float4 OutTex1 : TEXCOORD1                       
#endif                                                   
  )                                                     
{                                                        
  OutPosition = mul(InPosition, gWorldViewProjMatrix);
    
#ifdef DIFFUSE                                           
  OutColor0   = InColor0;                               
#endif
                                                   
#ifdef USE_CONSTANT_DIFFUSE                              
  OutColor0   = gColorConstant;                         
#endif
                                                   
#ifdef SPEC                                              
  OutColor1 = InColor1;                                 
#endif
                                                   
#ifdef UV0                                               
  OutTex0 = InTex0;                                     
#endif
                                                   
#ifdef UV1                                               
  OutTex1 = InTex1;                                     
#endif                                                   
}