// File: bink.fx

//
// simple pass through vertex shader
//

struct VS_DATA
{
  float4 Pos : POSITION;
  float2 T0: TEXCOORD0;
};

VS_DATA VS_PositionAndTexCoordPassThrough( VS_DATA In )
{
  return In;
}

//
// pixel shader common variables and structures
//

sampler tex0   : register( s0 );
sampler tex1   : register( s1 );
sampler tex2   : register( s2 );
sampler tex3   : register( s3 );
sampler tex4   : register( s4 );
bool    bUseMaskTexture : register( b0 );

struct VS_OUT
{
  float2 T0: TEXCOORD0;
};

//
// simple pixel shader to apply the yuvtorgb matrix
//

float4 PS_YCrCbToRGBNoPixelAlpha( VS_OUT In ) : COLOR
{
  const float4 crc = { 1.595794678f, -0.813476563f, 0, 0 };
  const float4 crb = { 0, -0.391448975f, 2.017822266f, 0 };
  const float4 adj = { -0.87065506f, 0.529705048f, -1.081668854f, 0 };
  float4 p;

  float y = tex2D( tex0, In.T0 ).a;
  float cr = tex2D( tex1, In.T0 ).a;
  float cb = tex2D( tex2,In.T0 ).a;

  p = y * 1.164123535f;

  p += crc * cr;
  p += crb * cb;
  p += adj;

  p.w = 1;
  if (bUseMaskTexture)
    p.w *= tex2D(tex4, In.T0).a;

  return p;
}

//
// simple pixel shader to apply the yuvtorgb matrix with alpha
//

float4 PS_YCrCbAToRGBA( VS_OUT In ) : COLOR
{
  const float4 crc = { 1.595794678f, -0.813476563f, 0, 0 };
  const float4 crb = { 0, -0.391448975f, 2.017822266f, 0 };
  const float4 adj = { -0.87065506f, 0.529705048f, -1.081668854f, 0 };
  float4 p;

  float y = tex2D( tex0, In.T0 ).a;
  float cr = tex2D( tex1, In.T0 ).a;
  float cb = tex2D( tex2,In.T0 ).a;
  float a = tex2D( tex3,In.T0 ).a;

  p = y * 1.164123535f;

  p += crc * cr;
  p += crb * cb;
  p += adj;

  p.w = a;
  if (bUseMaskTexture)
    p.w *= tex2D(tex4, In.T0).a;

  return p;
}
