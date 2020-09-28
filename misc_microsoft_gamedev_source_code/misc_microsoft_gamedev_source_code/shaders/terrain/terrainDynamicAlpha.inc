
sampler dynamicAlphaSampler = sampler_state
{
    MipFilter = NONE;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP ;
};

uniform float    g_dynamicAlphaTexWidth = 256;           // 1/4096

float giveDynamicAlphaValue( float2 texCoords )
{
     float imgWidth = g_dynamicAlphaTexWidth-1;
     
     int2 floord = (texCoords * float2(imgWidth,imgWidth)) * float2(0.25f,1);   //floord should be in [0,imgWidth>>2]x[0,imgHeight] space
     
     float2 scaled = floord / float2(imgWidth * 0.25f,imgWidth);
     
     float4 pixel= tex2D( dynamicAlphaSampler, scaled );       // Output color
     
     int xInd = (texCoords * imgWidth)%4;  
     float res =0;
     if(xInd ==0) res = pixel.x;
     if(xInd ==1) res = pixel.y;
     if(xInd ==2) res = pixel.z;
     if(xInd ==3) res = pixel.w;
     
     return res;
 }       