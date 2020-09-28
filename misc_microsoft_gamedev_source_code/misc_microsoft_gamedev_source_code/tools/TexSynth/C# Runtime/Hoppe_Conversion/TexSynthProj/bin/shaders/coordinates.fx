/** 

  Tools to compute coordinates of pixels within texture space

  Sylvain Lefebvre 2004-07-21

*/

//-----------------------------------------------------------------------------

#ifndef __FX_COORDINATES__
#define __FX_COORDINATES__

#define fmodp(x,n) ((n)*frac((x)/(n)))

float2 relative2absolute(float2 rij,float4 winpos)
{
  return (rij + winpos.xy);
}

float2 absolute2relative(float2 aij,float4 winpos)
{
  return (aij - winpos.xy);
}

float2 pix_unpack2pack(float2 rij,float4 quad)
{
  float2 int_rij = (rij);
  float2 part    = fmodp(int_rij,2.0);
  float2 pij     = floor(int_rij*0.5);
  
  return ( (0.5+pij)*quad.zw + part*0.5 );   // 0.5+ required for np2 buffers
}


#endif

//-----------------------------------------------------------------------------
