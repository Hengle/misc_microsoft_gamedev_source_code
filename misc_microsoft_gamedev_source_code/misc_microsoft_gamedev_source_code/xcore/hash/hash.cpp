//==============================================================================
//
// hash.cpp
//
// Copyright (c) 2001, Ensemble Studios
//
//==============================================================================
#include "xcore.h"
#include "hash.h"

//=============================================================================
// mix macro used in hash function below.
// Adapted from http://burtleburtle.net/bob/hash/doobs.html
//=============================================================================
//--------------------------------------------------------------------
//mix -- mix 3 32-bit values reversibly.
//For every delta with one or two bits set, and the deltas of all three
//  high bits or all three low bits, whether the original value of a,b,c
//  is almost all zero or is uniformly distributed,
//* If mix() is run forward or backward, at least 32 bits in a,b,c
//  have at least 1/4 probability of changing.
//* If mix() is run forward, every bit of c will change between 1/3 and
//  2/3 of the time.  (Well, 22/100 and 78/100 for some 2-bit deltas.)
//mix() takes 36 machine instructions, but only 18 cycles on a superscalar
//  machine (like a Pentium or a Sparc).  No faster mixer seems to work,
//  that's the result of my brute-force search.  There were about 2^^68
//  hashes to choose from.  I only tested about a billion of those.
//--------------------------------------------------------------------
#define mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

//=============================================================================
// hash function that returns 32 bit value.  Adapted from http://burtleburtle.net/bob/hash/doobs.html
// Use power of two hash table size and mask off upper bits of has
//=============================================================================
//--------------------------------------------------------------------
//hash() -- hash a variable-length key into a 32-bit value
//  k       : the key (the unaligned variable-length array of bytes)
//  len     : the length of the key, counting by bytes
//  initval : can be any 4-byte value
//Returns a 32-bit value.  Every bit of the key affects every bit of
//the return value.  Every 1-bit and 2-bit delta achieves avalanche.
//About 6*len+35 instructions.
//
//The best hash table sizes are powers of 2.  There is no need to do
//mod a prime (mod is sooo slow!).  If you need less than 32 bits,
//use a bitmask.  For example, if you need only 10 bits, do
//  h = (h & hashmask(10));
//In which case, the hash table should have hashsize(10) elements.
//
//If you are hashing n strings (ub1 **)k, do it like this:
//  for (i=0, h=0; i<n; ++i) h = hash( k[i], len[i], h);
//
//By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.  You may use this
//code any way you wish, private, educational, or commercial.  It's free.
//
//See http://burtleburtle.net/bob/hash/evahash.html
//Use for hash table lookup, or anything where one collision in 2^^32 is
//acceptable.  Do NOT use for cryptographic purposes.
//--------------------------------------------------------------------
DWORD hash(const unsigned char *key, DWORD length, DWORD initialValue)
{
   DWORD a, b, c, len;

   // Set up the internal state
   len = length;
   a = b = 0x9e3779b9;  // the golden ratio; an arbitrary value 
   c = initialValue;         // the previous hash value

   // handle most of the key
   while (len >= 12)
   {
      a += (key[0] +((DWORD)key[1]<<8) +((DWORD)key[2]<<16) +((DWORD)key[3]<<24));
      b += (key[4] +((DWORD)key[5]<<8) +((DWORD)key[6]<<16) +((DWORD)key[7]<<24));
      c += (key[8] +((DWORD)key[9]<<8) +((DWORD)key[10]<<16)+((DWORD)key[11]<<24));
      mix(a,b,c);
      key += 12; 
      len -= 12;
   }

   // Handle the last 11 bytes 
   c += length;
   switch(len)              // all the case statements fall through intentionally
   {
      case 11: c+=((DWORD)key[10]<<24);
      case 10: c+=((DWORD)key[9]<<16);
      case 9 : c+=((DWORD)key[8]<<8);
         // the first byte of c is reserved for the length
      case 8 : b+=((DWORD)key[7]<<24);
      case 7 : b+=((DWORD)key[6]<<16);
      case 6 : b+=((DWORD)key[5]<<8);
      case 5 : b+=key[4];
      case 4 : a+=((DWORD)key[3]<<24);
      case 3 : a+=((DWORD)key[2]<<16);
      case 2 : a+=((DWORD)key[1]<<8);
      case 1 : a+=key[0];
      //case 0: nothing left to add
   }
   mix(a,b,c);

   // Done -- c holds the result.
   return(c);
}

// See Jenkins, "The Hash", Dr. Dobb's Sept. 1997
void BHash::bitMix(void)
{ 
   v[0] -= v[1]; 
   v[0] -= v[2]; 
   v[0] ^= (v[2] >> 13); 
   v[1] -= v[2]; 
   v[1] -= v[0]; 
   v[1] ^= (v[0] << 8); 
   v[2] -= v[0]; 
   v[2] -= v[1]; 
   v[2] ^= (v[1] >> 13); 
   v[0] -= v[1]; 
   v[0] -= v[2]; 
   v[0] ^= (v[2] >> 12);  
   v[1] -= v[2]; 
   v[1] -= v[0]; 
   v[1] ^= (v[0] << 16); 
   v[2] -= v[0]; 
   v[2] -= v[1]; 
   v[2] ^= (v[1] >> 5); 
   v[0] -= v[1]; 
   v[0] -= v[2]; 
   v[0] ^= (v[2] >> 3);  
   v[1] -= v[2]; 
   v[1] -= v[0]; 
   v[1] ^= (v[0] << 10); 
   v[2] -= v[0]; 
   v[2] -= v[1]; 
   v[2] ^= (v[1] >> 15);
}

BHash& BHash::update(const void* pData, uint dataLen)
{
   const uint* pData32 = reinterpret_cast<const uint*>(pData);
   uint len = dataLen;

   while (len >= 12)
   {
      v[0] += pData32[0];
      v[1] += pData32[1];
      v[2] += pData32[2];
      bitMix();

      pData32 += 3;
      len -= 12;
   }

   v[2] += dataLen;

   const uchar* pSrc = reinterpret_cast<const uchar*>(pData32);
   uchar* pDst = reinterpret_cast<uchar*>(v);

   while (len)
   {
      *pDst = static_cast<uchar>(*pDst + *pSrc);

      pSrc++;
      pDst++;

      if (pDst == reinterpret_cast<uchar*>(&v[2]))
         pDst++;

      len--;
   }

   bitMix();

   return *this;
}

// Adapted from code by Paul Hsieh
// http://www.azillionmonkeys.com/qed/hash.html
// [3/21/07] - Paul has updated this, but I haven't merged the changes.
DWORD hashFast(const unsigned char *key, DWORD length, DWORD initialValue)
{
   DWORD hash = initialValue;
   if (NULL == key) 
      return hash;

   const DWORD rem = length & 3;
   length >>= 2;

   // -- Main loop
   for (DWORD i = 0; i < length; i++) 
   {
      hash += *((const unsigned short *) key);
      key += sizeof (short);
      hash ^= hash << 16;
      hash ^= (*((const unsigned short *) key)) << 11;
      key += sizeof (short);
      hash += hash >> 11;
   }

   // -- Handle end cases
   switch (rem) 
   {
     case 3: hash += *((const unsigned short *) key);
        hash ^= hash << 16;
        hash ^= key[sizeof (short)] << 18;
        hash += hash >> 11;
        break;
     case 2: hash += *((const unsigned short *) key);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
     case 1: hash += *key;
        hash ^= hash << 10;
        hash += hash >> 1;
   }

   //-- Force final avalanching 
   hash ^= hash << 3;
   hash += hash >> 5;
   hash ^= hash << 2;
   hash += hash >> 15;
   hash ^= hash << 10;

   return hash;
}
