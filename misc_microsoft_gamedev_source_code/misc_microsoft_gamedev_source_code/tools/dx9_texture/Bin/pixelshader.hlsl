sampler gCoeff0Sampler : register(s0);
sampler gCoeff1Sampler : register(s1);
sampler gCoeff2Sampler : register(s2);
sampler gCoeff3Sampler : register(s3);
sampler gCoeff4Sampler : register(s4);

sampler gChromaSampler : register(s5);

sampler gBasis0Sampler : register(s6);
sampler gBasis1Sampler : register(s7);
sampler gBasis2Sampler : register(s8);
sampler gBasis3Sampler : register(s9);

float4 gChromaRange : register(c0);
float4 gChromaOfs : register(c1);
float4 gTextureDim : register(c2);

float4 gCoeff0Scale : register(c3);
float4 gCoeff1Scale : register(c4);
float4 gCoeff2Scale : register(c5);
float4 gCoeff3Scale : register(c6);
float4 gCoeff4Scale : register(c7);

float4 main(in float2 uv0 : TEXCOORD0) : COLOR0
{
	float xf = floor(frac(uv0.x * gTextureDim.x) * 4.0f) / 4.0f + .5f/4.0f;
	float yf = floor(frac(uv0.y * gTextureDim.y) * 4.0f) / 4.0f + .5f/4.0f;
	float4 basis0 = tex2D(gBasis0Sampler, float2(xf,yf));
				
	float dc = tex2D(gCoeff0Sampler, uv0).g;
	float4 t1 = tex2D(gCoeff1Sampler, uv0);
	float4 t2 = tex2D(gCoeff2Sampler, uv0);
	float4 t3 = tex2D(gCoeff3Sampler, uv0);
		
	dc = (((dc * 2.0f) - 1.0f) * gCoeff0Scale.a) * basis0.a;
	
	t1 = (t1 - float4(123.0f/255.0f, 125.0f/255.0f, 123.0f/255.0f, 128/255.0f)) * (255.0f/127.0f) * gCoeff1Scale;
	float t1b = t1.b;
	t1 = sign(t1) * t1 * t1;
	t1.b = t1b;
			
	t2 = (t2 - float4(123.0f/255.0f, 125.0f/255.0f, 123.0f/255.0f, 128/255.0f)) * (255.0f/127.0f) * gCoeff2Scale; 
	float t2b = t2.b;
	t2 = sign(t2) * t2 * t2;
	t2.b = t2b;
	
	t3 = (t3 - float4(123.0f/255.0f, 125.0f/255.0f, 123.0f/255.0f, 128/255.0f)) * (255.0f/127.0f) * gCoeff3Scale; 
	float t3b = t3.b;
	t3 = sign(t3) * t3 * t3;
	t3.b = t3b;
	
	float4 basis1 = tex2D(gBasis1Sampler, float2(xf,yf));
	float4 basis2 = tex2D(gBasis2Sampler, float2(xf,yf));
	float4 basis3 = tex2D(gBasis3Sampler, float2(xf,yf));
		
	dc += dot(basis1, t1);
	dc += dot(basis2, t2);
	dc += dot(basis3, t3);
	dc += .5f;
		  
//   const int y = luma.g;
//   const int cg = chroma.r;
//   const int co = chroma.g;
//   int r = y - cg*.5 + co*.5;
//   int g = y + cg*.5;
//   int b = y - cg*.5 - co*.5;
  
	float4 chroma = tex2D(gChromaSampler, uv0); 
	float y = dc * gChromaRange.x + gChromaOfs.x;
	float co = chroma.g * gChromaRange.z + gChromaOfs.z;
	float cg = chroma.r * gChromaRange.y + gChromaOfs.y;
	float3 outColor = float3(y, y, y) + cg * float3(-.5, .5, -.5) + co * float3(.5, 0, -.5);
	    
	return float4(outColor.r, outColor.g, outColor.b, 1);
}


