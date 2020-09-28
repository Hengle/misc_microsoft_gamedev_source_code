
//--------------------------------------------------------------------------------------
// Vertex and pixel shaders for gradient background rendering
//--------------------------------------------------------------------------------------

struct GRADIENT_VS_IN                                              
{                                                         
   float4   Position     : POSITION;                      
   float4   Color        : COLOR0;                        
};                                                        
                                                          
struct GRADIENT_VS_OUT                                             
{                                                         
   float4 Position       : POSITION;                      
   float4 Diffuse        : COLOR0;                        
};                                                        
                                                          
GRADIENT_VS_OUT AtgGradientVertexShader( GRADIENT_VS_IN In )                   
{                                                         
   GRADIENT_VS_OUT Out;                                            
   Out.Position = In.Position;                            
   Out.Diffuse  = In.Color;                               
   return Out;                                            
}                                                         
                                                                                                                    
float4 AtgGradientPixelShader( GRADIENT_VS_OUT In ) : COLOR0          
{                                                         
   return In.Diffuse;                                     
}                                                        

//--------------------------------------------------------------------------------------
// Vertex and pixel shaders for font rendering
//--------------------------------------------------------------------------------------

struct FONT_VS_IN                                              
{                                                         
   float2   Pos          : POSITION;                      
   float2   Tex          : TEXCOORD0;                     
};                                                        
                                                          
struct FONT_VS_OUT                                             
{                                                         
   float4 Position       : POSITION;                      
   float4 Diffuse        : COLOR0;                        
   float2 TexCoord0      : TEXCOORD0;                     
};                                                        
                                                          
uniform float4   Color    : register(c1);                 
uniform float2   TexScale : register(c2);                 
                                                          
FONT_VS_OUT AtgFontVertexShader( FONT_VS_IN In )                       
{                                                         
   FONT_VS_OUT Out;                                            
   Out.Position.x  = (In.Pos.x-0.5);                      
   Out.Position.y  = (In.Pos.y-0.5);                      
   Out.Position.z  = ( 0.0 );                             
   Out.Position.w  = ( 1.0 );                             
   Out.Diffuse     = Color;                               
   Out.TexCoord0.x = In.Tex.x * TexScale.x;               
   Out.TexCoord0.y = In.Tex.y * TexScale.y;               
   return Out;                                            
}                                                         
                                                          
uniform float4 ChannelSelector : register(c0);            
uniform float4 Mask            : register(c1);            
                                                          
sampler        FontTexture     : register(s0);            
                                                          
float4 AtgFontPixelShader( FONT_VS_OUT In ) : COLOR0              
{                                                         
   // Fetch a texel from the font texture                 
   float4 FontTexel = tex2D( FontTexture, In.TexCoord0 ); 
                                                          
   // Select the color from the channel                   
   float value = dot( FontTexel, ChannelSelector );       
                                                          
   // For white pixels, the high bit is 1 and the low     
   // bits are luminance, so r0.a will be > 0.5. For the  
   // RGB channel, we want to lop off the msb and shift   
   // the lower bits up one bit. This is simple to do     
   // with the _bx2 modifier. Since these pixels are      
   // opaque, we emit a 1 for the alpha channel (which    
   // is 0.5 x2 ).                                        
                                                          
   // For black pixels, the high bit is 0 and the low     
   // bits are alpha, so r0.a will be < 0.5. For the RGB  
   // channel, we emit zero. For the alpha channel, we    
   // just use the x2 modifier to scale up the low bits   
   // of the alpha.                                       
   float4 Color;                                          
   Color.rgb = ( value > 0.5f ? 2*value-1 : 0.0f );       
   Color.a   = 2 * ( value > 0.5f ? 1.0f : value );       
                                                          
   // Apply a mask value, which let's ignore the above    
   // and use the original texel with all it's channels   
   Color = lerp( Color, FontTexel, Mask );                
                                                          
   // Return the texture color modulated with the vertex  
   // color                                               
   return Color * In.Diffuse;                             
}                                                         
