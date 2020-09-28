
//-----------------------------------------------------------------------------
inline unsigned short endSwapW(unsigned short i)
{
   return (i<<8) | (i>>8);
}
//-----------------------------------------------------------------------------
inline int endSwapI(int i)
{
   return endSwapW(i&0x0000FFFF)<<16 | endSwapW(i>>16);
}
//-----------------------------------------------------------------------------
inline float endSwapF( float f )
{
   union
   {
      float f;
      unsigned char b[4];
   } dat1, dat2;

   dat1.f = f;
   dat2.b[0] = dat1.b[3];
   dat2.b[1] = dat1.b[2];
   dat2.b[2] = dat1.b[1];
   dat2.b[3] = dat1.b[0];
   return dat2.f;
}
//-------------------------------------------------------------------------------------------
inline D3DXFLOAT16 endSwapW16( unsigned short *f )
{
   union
   {
      unsigned short f;
      unsigned char b[2];
   } dat1, dat2;

   dat1.f = (unsigned short)*f;
   dat2.b[0] = dat1.b[1];
   dat2.b[1] = dat1.b[0];
   return *((D3DXFLOAT16*)&dat2.f);
}
//-------------------------------------------------------------------------------------------
inline float endSwapF32( float f )
{
   union
   {
      float f;
      unsigned char b[4];
   } dat1, dat2;

   dat1.f = f;
   dat2.b[0] = dat1.b[3];
   dat2.b[1] = dat1.b[2];
   dat2.b[2] = dat1.b[1];
   dat2.b[3] = dat1.b[0];
   return dat2.f;
}
