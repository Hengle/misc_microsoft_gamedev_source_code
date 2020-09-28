/** 

  Random number generator (hash function)

  Sylvain Lefebvre - 2004-07-03 - (c) Microsoft Corp.

*/


//-----------------------------------------------------------------------------

#define M_PI 3.14159265358979323846

#define fmodp(x,n) ((n)*frac((x)/(n)))

//-----------------------------------------------------------------------------


float2 old_rand(float2 ij,float2 n)
{
  float2 xy0 = floor((ij+n)*2000.0)/256.0;
  float2 xym = frac(xy0/257.0)*257.0 + 1.0;
  float2 xym2= frac(xym*xym);
  float2 pxy = xym2.xy * xym.yx;
  float2 xy1 = xy0 + pxy.xy + pxy.yx;
  return (xy1);
}

  
//-----------------------------------------------------------------------------


float2 our_rand(float2 ij)
{
  const float4 a=float4(M_PI * M_PI * M_PI * M_PI, exp(4.0),  1.0, 0.0);
  const float4 b=float4(pow(13.0, M_PI / 2.0), sqrt(1997.0),  0.0, 1.0);

  float2 xy0    = ij/M_PI;
  float2 xym    = fmodp(xy0.xy,257.0)+1.0;
  float2 xym2   = frac(xym*xym);
  float4 pxy    = float4(xym.yx * xym2 , frac(xy0));
  float2 xy1    = float2(dot(pxy,a) , dot(pxy,b));
  float2 result = frac(xy1);
  
  return (result*256.0);
}


//-----------------------------------------------------------------------------


float2 mccool_rand(float2 ij)
{
  static const float4 a=float4(M_PI * M_PI * M_PI * M_PI, exp(4.0), pow(13.0, M_PI / 2.0), sqrt(1997.0));
  float4 result =float4(ij,ij);

  
	result.x = frac(dot(result, a));
	result.y = frac(dot(result, a));
	result.z = frac(dot(result, a));
	result.w = frac(dot(result, a));
	result.x = frac(dot(result, a));
	result.y = frac(dot(result, a));
	result.z = frac(dot(result, a));
	result.w = frac(dot(result, a));
	result.x = frac(dot(result, a));
	result.y = frac(dot(result, a));
	result.z = frac(dot(result, a));
	result.w = frac(dot(result, a));

  return (result.xy*256.0);
}

//-----------------------------------------------------------------------------

// select a rand function

float2 rand(float2 ij,float2 n)
{
//return mccool_rand(ij);
//return 0;//old_rand(ij,n);
  return (our_rand(ij));
//  return (permut_rand(ij));
}


//-----------------------------------------------------------------------------
