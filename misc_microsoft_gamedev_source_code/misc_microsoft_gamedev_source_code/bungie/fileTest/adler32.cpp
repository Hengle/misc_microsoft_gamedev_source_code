// File: adler32.cpp

#include "ens.h"
#include "adler32.h"

namespace ens
{

// reference: zlib standard
#define BASE 65521L
#define NMAX 5552
#define DO1(buf,i)  {s1 += buf[i]; s2 += s1;}
#define DO2(buf,i)  DO1(buf,i); DO1(buf,i+1);
#define DO4(buf,i)  DO2(buf,i); DO2(buf,i+2);
#define DO8(buf,i)  DO4(buf,i); DO4(buf,i+4);
#define DO16(buf)   DO8(buf,0); DO8(buf,8);

uint calcAdler32(const void* p, uint len, uint adler)
{
  const uchar* Pbuf = reinterpret_cast<const uchar*>(p);
  uint s1 = adler & 0xffff;
  uint s2 = (adler >> 16) & 0xffff;
  int k;

  if (Pbuf == NULL) return 1L;

  while (len > 0)
  {
    k = (len < NMAX) ? len : NMAX;

    len -= k;

    while (k >= 16)
    {
      DO16(Pbuf);
      Pbuf += 16;
      k -= 16;
    }

    if (k != 0)
    {
      do
      {
        s1 += *Pbuf++;
        s2 += s1;
      } while (--k);
    }

    s1 %= BASE;
    s2 %= BASE;
  }

  return (s2 << 16) | s1;
}

} // namespace ens
